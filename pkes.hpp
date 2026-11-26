// pkes.hpp - Model of a Passive Keyless Entry and Start (PKES) system.
//
//   KeyFob      the car key (or phone key)
//   RadioPath   radio path between car and key: direct, or through relay devices
//   LegacyCar   conventional PKES: "if I hear a correct reply, unlock"
//   SecureCar   proposed design: correct reply AND UWB distance <= 2 m
//
// Proposed design (from the article):
//   Layer 1  UWB time-of-flight distance bounding (median of 5 ranging rounds)
//   Layer 2  AES-128 challenge-response with fresh random nonces
//   Layer 3  Motion-aware key fob (radio sleeps after 60 s without movement)
//   Layer 4  Optional PIN-to-drive
//   Monitor  Gateway ECU logs "valid but too far" replies and raises an alert
//
// Educational simulation - not production automotive code.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "aes128.hpp"

namespace pkes {

// Speed of light in metres per nanosecond (about 30 cm every nanosecond)
constexpr double C_M_PER_NS = 0.299792458;
constexpr double PI = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// Random numbers
// ---------------------------------------------------------------------------
// std::mt19937_64 gives the same sequence on every compiler. We build uniform
// and Gaussian numbers ourselves so results are identical on Windows, Linux
// and macOS (std::normal_distribution differs between compilers).
class Rng {
public:
    explicit Rng(uint64_t seed) : gen_(seed) {}
    double uniform() { return (gen_() >> 11) * (1.0 / 9007199254740992.0); }   // [0, 1)
    double uniform(double a, double b) { return a + (b - a) * uniform(); }
    double normal(double mean, double sd) {                                     // Box-Muller
        if (hasSpare_) { hasSpare_ = false; return mean + sd * spare_; }
        double u1 = uniform(), u2 = uniform();
        while (u1 <= 0.0) u1 = uniform();
        double r = std::sqrt(-2.0 * std::log(u1)), theta = 2.0 * PI * u2;
        spare_ = r * std::sin(theta); hasSpare_ = true;
        return mean + sd * r * std::cos(theta);
    }
private:
    std::mt19937_64 gen_;
    bool hasSpare_ = false;
    double spare_ = 0.0;
};

// Fresh random bytes for nonces (a new value every session)
template <size_t N>
std::array<uint8_t, N> randomBytes() {
    static std::random_device rd;
    std::array<uint8_t, N> out{};
    for (auto& b : out) b = static_cast<uint8_t>(rd() & 0xFF);
    return out;
}

using Key16    = aes::Key;
using Nonce8   = std::array<uint8_t, 8>;    // car's challenge  Nc
using Nonce4   = std::array<uint8_t, 4>;    // key's nonce      Nk
using KeyId    = std::array<uint8_t, 4>;    // key identifier   ID

// Pack Nc (8) || Nk (4) || ID (4) into one 16-byte AES block
inline aes::Block makeBlock(const Nonce8& nc, const Nonce4& nk, const KeyId& id) {
    aes::Block b{};
    std::copy(nc.begin(), nc.end(), b.begin());
    std::copy(nk.begin(), nk.end(), b.begin() + 8);
    std::copy(id.begin(), id.end(), b.begin() + 12);
    return b;
}

// The key's answer to a challenge
struct Reply {
    Nonce4 keyNonce;
    aes::Block response;   // R = AES_K(Nc || Nk || ID)
};

// ---------------------------------------------------------------------------
// Key fob  (Layer 2 crypto + Layer 3 motion-aware sleep)
// ---------------------------------------------------------------------------
struct KeyFob {
    Key16 secret{};
    KeyId id{{'K', 'E', 'Y', '1'}};
    bool motionAware = true;           // Layer 3 present?
    double secondsSinceMotion = 0.0;   // how long the key has been still
    double sleepAfterSeconds = 60.0;

    bool radioOn() const { return !motionAware || secondsSinceMotion < sleepAfterSeconds; }

    std::optional<Reply> respond(const Nonce8& carNonce) const {
        if (!radioOn()) return std::nullopt;            // fob asleep: no reply at all
        Reply r;
        r.keyNonce = randomBytes<4>();
        r.response = aes::encrypt(secret, makeBlock(carNonce, r.keyNonce, id));
        return r;
    }
};

// ---------------------------------------------------------------------------
// Radio path between car and key
// ---------------------------------------------------------------------------
struct RadioPath {
    double distanceM = 1.0;       // total one-way length travelled by the signal
    double relayDelayNs = 0.0;    // extra processing delay added by relay devices
    double noiseSigmaM = 0.10;    // UWB measurement noise (standard deviation, metres)
    bool isRelay = false;

    static RadioPath direct(double distanceM, double noiseSigmaM = 0.10) {
        return {distanceM, 0.0, noiseSigmaM, false};
    }

    // Relay attack: car -> relay A -> (long link) -> relay B -> key
    static RadioPath relayed(double carToRelayA, double relayLink, double relayBToKey,
                             double processingDelayNs, double noiseSigmaM = 0.10) {
        return {carToRelayA + relayLink + relayBToKey, processingDelayNs, noiseSigmaM, true};
    }

    // The car's LF wake-up reaches the key directly only within ~2 m.
    // Through a relay it always does: relay B re-broadcasts it next to the key.
    bool lfReachesKey() const { return isRelay || distanceM <= 2.0; }

    // Round-trip time the car measures for one UWB poll/response
    double uwbRoundTripNs(double keyReplyNs, Rng& rng) const {
        double flightNs = 2.0 * distanceM / C_M_PER_NS;
        double noiseNs = 2.0 * rng.normal(0.0, noiseSigmaM) / C_M_PER_NS;
        return flightNs + relayDelayNs + keyReplyNs + noiseNs;
    }
};

// ---------------------------------------------------------------------------
// Cars
// ---------------------------------------------------------------------------
struct Decision {
    bool unlocked = false;
    std::string reason;
    double measuredDistanceM = -1.0;   // -1 = not measured
};

// Conventional PKES: unlock if the LF challenge got a correct reply
class LegacyCar {
public:
    explicit LegacyCar(const Key16& secret, KeyId id = {{'K', 'E', 'Y', '1'}})
        : secret_(secret), id_(id) {}
    virtual ~LegacyCar() = default;

    // 'replayed' lets an attacker inject a response recorded earlier
    virtual Decision tryUnlock(const KeyFob& fob, const RadioPath& path,
                               const std::optional<Reply>& replayed = std::nullopt) {
        if (!path.lfReachesKey()) return {false, "key out of LF range"};
        Nonce8 carNonce = randomBytes<8>();                      // fresh challenge
        std::optional<Reply> reply = replayed ? replayed : fob.respond(carNonce);
        if (!reply) return {false, "no reply (fob asleep)"};
        aes::Block expected = aes::encrypt(secret_, makeBlock(carNonce, reply->keyNonce, id_));
        if (reply->response != expected) return {false, "wrong cryptographic response"};
        return {true, "valid response received"};
    }

protected:
    Key16 secret_;
    KeyId id_;
};

// Proposed design: valid response AND UWB-measured distance <= dMax
class SecureCar : public LegacyCar {
public:
    static constexpr double KEY_REPLY_NS = 200000.0;   // fixed, known processing time in the key

    SecureCar(const Key16& secret, uint64_t seed = 1, double dMaxM = 2.0,
              int rangingRounds = 5, bool requirePin = false)
        : LegacyCar(secret), rng_(seed), dMaxM_(dMaxM), rounds_(rangingRounds), requirePin_(requirePin) {}

    // Layer 1: d = c * (t_round - t_reply) / 2, median of several rounds
    double measureDistance(const RadioPath& path) {
        std::vector<double> est;
        for (int i = 0; i < rounds_; ++i) {
            double tRound = path.uwbRoundTripNs(KEY_REPLY_NS, rng_);
            est.push_back(C_M_PER_NS * (tRound - KEY_REPLY_NS) / 2.0);
        }
        std::sort(est.begin(), est.end());
        return est[est.size() / 2];
    }

    Decision tryUnlock(const KeyFob& fob, const RadioPath& path,
                       const std::optional<Reply>& replayed = std::nullopt) override {
        Decision auth = LegacyCar::tryUnlock(fob, path, replayed);  // Layer 2 (+ Layer 3)
        if (!auth.unlocked) return auth;
        double d = measureDistance(path);                            // Layer 1
        char buf[96];
        if (d > dMaxM_) {
            suspiciousEvents_.push_back(d);                          // Monitoring
            std::snprintf(buf, sizeof buf, "key too far (%.1f m > %.1f m)", d, dMaxM_);
            return {false, buf, d};
        }
        std::snprintf(buf, sizeof buf, "valid response, key at %.2f m", d);
        return {true, buf, d};
    }

    // Layer 4: optional PIN-to-drive
    bool tryStart(const std::string& enteredPin) const { return !requirePin_ || enteredPin == pin_; }

    // Gateway ECU alert after repeated "valid but too far" replies
    bool relayAlert() const { return suspiciousEvents_.size() >= 3; }

private:
    Rng rng_;
    double dMaxM_;
    int rounds_;
    bool requirePin_;
    std::string pin_ = "2580";
    std::vector<double> suspiciousEvents_;
};

}  // namespace pkes
