// attack_demo.cpp - Runs six attack scenarios against a conventional (legacy)
// PKES car and against the proposed secure car, and prints the results.
//
// Build:  g++ -std=c++17 -O2 -Iinclude src/attack_demo.cpp -o attack_demo
// Run:    ./attack_demo        (Windows: attack_demo.exe)

#include <cstdio>
#include <string>
#include <vector>

#include "pkes.hpp"

using namespace pkes;

struct Scenario {
    std::string name;
    KeyFob fobLegacy;            // legacy fobs never sleep
    KeyFob fobSecure;            // proposed fob with Layer 3
    RadioPath path;
    std::optional<Reply> replayed;
};

static const char* verdict(const Decision& d) { return d.unlocked ? "UNLOCKED" : "REFUSED"; }

int main() {
    const Key16 SECRET = randomBytes<16>();          // 128-bit key shared by car and fob
    const Key16 OTHER_SECRET = randomBytes<16>();    // attacker's fake / cloned fob

    KeyFob ownerLegacy{SECRET};  ownerLegacy.motionAware = false;
    KeyFob ownerNew{SECRET};     ownerNew.motionAware = true;
    KeyFob idleLegacy = ownerLegacy; idleLegacy.secondsSinceMotion = 3600;  // still for 1 hour
    KeyFob idleNew = ownerNew;       idleNew.secondsSinceMotion = 3600;
    KeyFob fake{OTHER_SECRET};   fake.motionAware = false;

    // A response the attacker recorded during an earlier unlock
    std::optional<Reply> recorded = ownerLegacy.respond(randomBytes<8>());

    std::vector<Scenario> scenarios = {
        {"1. Owner at the door (1 m)",              ownerLegacy, ownerNew, RadioPath::direct(1.0), std::nullopt},
        {"2. Relay attack, key 20 m away in house", ownerLegacy, ownerNew, RadioPath::relayed(0.5, 20, 1.0, 1000), std::nullopt},
        {"3. 'Perfect' zero-delay relay, 5 m link", ownerLegacy, ownerNew, RadioPath::relayed(0.5, 5, 1.0, 0), std::nullopt},
        {"4. Replay of a recorded response",        ownerLegacy, ownerNew, RadioPath::direct(1.0), recorded},
        {"5. Fake / cloned fob with wrong key",     fake,        fake,     RadioPath::direct(1.0), std::nullopt},
        {"6. Relay at night, key idle on table",    idleLegacy,  idleNew,  RadioPath::relayed(0.5, 15, 1.0, 1000), std::nullopt},
    };

    LegacyCar legacy(SECRET);
    SecureCar secure(SECRET, /*seed=*/42);

    std::printf("\nPKES relay-attack demonstration\n");
    std::printf("%s\n", std::string(96, '=').c_str());
    std::printf("%-42s%-14s%-17s%s\n", "Scenario", "Legacy PKES", "Proposed design", "Reason (proposed)");
    std::printf("%s\n", std::string(96, '-').c_str());
    for (const auto& s : scenarios) {
        Decision oldCar = legacy.tryUnlock(s.fobLegacy, s.path, s.replayed);
        Decision newCar = secure.tryUnlock(s.fobSecure, s.path, s.replayed);
        std::printf("%-42s%-14s%-17s%s\n", s.name.c_str(), verdict(oldCar), verdict(newCar), newCar.reason.c_str());
    }
    std::printf("%s\n", std::string(96, '-').c_str());

    // Monitoring: repeated relay attempts trigger an alert
    for (int i = 0; i < 3; ++i) secure.tryUnlock(ownerNew, RadioPath::relayed(0.5, 20, 1.0, 1000));
    std::printf("Gateway ECU relay alert raised after repeated attempts: %s\n", secure.relayAlert() ? "YES" : "NO");

    // Layer 4: PIN-to-drive
    SecureCar pinCar(SECRET, 1, 2.0, 5, /*requirePin=*/true);
    std::printf("PIN-to-drive: wrong PIN -> %s, correct PIN -> %s\n\n",
                pinCar.tryStart("0000") ? "starts" : "blocked",
                pinCar.tryStart("2580") ? "starts" : "blocked");
    return 0;
}
