# Securing Passive Keyless Entry Against Relay Attacks (C++)

C++ implementation for the article **"Securing Passive Keyless Entry Systems Against Relay Attacks: Limitations of Existing Market Countermeasures and a UWB Distance-Bounding Approach"**.

Prisha Gupta, Department of Electronics and Communication Engineering, Thapar Institute of Engineering and Technology
Course: UCS762 – Cybersecurity for Mobility Systems

---

## The problem

Cars with a "smart key" unlock when the key is *near*. The car decides this only by checking that it received a correct radio reply. In a **relay attack**, two thieves forward the signal between a key inside a house and the car outside. The car gets a correct reply, so it unlocks and starts. Stronger encryption doesn't help, because the relay forwards the encrypted messages without reading them.

## The solution

The car **measures distance using time**. Radio travels at the speed of light, about 30 cm per nanosecond, so the car times the key's Ultra-Wideband (UWB) reply:

```
d = c × (t_round − t_reply) / 2
```

Here `t_reply` is the key fob's fixed turnaround time. In the model, the estimate is `d̂ = d_path + c·Δt_relay/2 + ε`. `d_path` is the total one-way radio path; for a relay it is `d_CA + L + d_BK`, so the relay-link length `L` is counted once. `Δt_relay` is the extra relay processing delay over the round trip, and `ε` is the ranging noise.

A conventional relay can only lengthen the path and add delay, so it raises `d̂`. **This assumes the receiver's time-of-arrival estimate can be trusted.** Physical-layer attacks such as *Ghost Peak* (USENIX Security 2022) made commercial UWB chips report 0 m for devices 12 m apart, with up to 4% success per attempt. UWB is therefore **not** inherently immune: it needs authenticated ranging (unpredictable STS from a fresh session key), secure timestamps, hardened receivers and limits on repeated attempts.

| Layer | Control | Stops | Where in the code |
|---|---|---|---|
| 1 | UWB time-of-flight distance bounding (median of 5 rounds, unlock only if ≤ 2 m) | Relay and amplification attacks | `SecureCar::measureDistance()` |
| 2 | Simplified AES-128 challenge–response with fresh random nonces (a real design would use AES-CMAC, a KDF and STS; these are conceptual here) | Replay attacks, fake or cloned keys | `aes128.hpp`, `KeyFob::respond()` |
| 3 | Motion-aware key fob (radio sleeps after 60 s still) | Relay at night while the key is idle | `KeyFob::radioOn()` |
| 4 | **Optional** PIN-to-drive | A stolen key being used to drive | `SecureCar::tryStart()` |
| Monitor | Gateway ECU logs "valid but too far" replies and raises an alert | Repeated relay attempts go unnoticed | `SecureCar::relayAlert()` |

## Repository structure

```
pkes-relay-defense/
├── include/
│   ├── aes128.hpp         # AES-128 written from the FIPS-197 standard (no libraries)
│   └── pkes.hpp           # KeyFob, RadioPath, LegacyCar, SecureCar
├── src/
│   ├── attack_demo.cpp    # 6 attack scenarios: legacy car vs proposed car
│   └── monte_carlo.cpp    # Simulation behind Tables 5 and 6 of the article
├── tests/
│   └── test_pkes.cpp      # 13 tests, incl. the official AES-128 test vector
├── results/               # Program outputs (text, CSV incl. sensitivity sweep) and charts
├── paper/                 # Article source (Springer LNCS, Overleaf)
├── tools/plot_results.py  # OPTIONAL: redraws the charts from the CSV files
├── Makefile
└── LICENSE
```

Only the standard C++17 library is used. No external libraries are needed.

## How to build and run

You need a C++17 compiler: g++ (MinGW on Windows), clang++ or MSVC.

**Option A: one command per program (works everywhere)**

```bash
g++ -std=c++17 -O2 -Iinclude src/attack_demo.cpp -o attack_demo
g++ -std=c++17 -O2 -Iinclude src/monte_carlo.cpp -o monte_carlo
g++ -std=c++17 -O2 -Iinclude tests/test_pkes.cpp -o test_pkes

./test_pkes
./attack_demo
./monte_carlo        # needs a 'results' folder (already in the repo)
```

On Windows, run `test_pkes.exe`, `attack_demo.exe` and `monte_carlo.exe` instead.

**Option B: Makefile (Linux / macOS / MinGW)**

```bash
make        # build everything into bin/
make run    # run tests, demo and simulation
```

## Results

### 1. Attack demonstration (`attack_demo`)

```
Scenario                                  Legacy PKES   Proposed design  Reason (proposed)
1. Owner at the door (1 m)                UNLOCKED      UNLOCKED         valid response, key at 1.04 m
2. Relay attack, key 20 m away in house   UNLOCKED      REFUSED          key too far (171.4 m > 2.0 m)
3. 'Perfect' zero-delay relay, 5 m link   UNLOCKED      REFUSED          key too far (6.4 m > 2.0 m)
4. Replay of a recorded response          REFUSED       REFUSED          wrong cryptographic response
5. Fake / cloned fob with wrong key       REFUSED       REFUSED          wrong cryptographic response
6. Relay at night, key idle on table      UNLOCKED      REFUSED          no reply (fob asleep)
Gateway ECU relay alert raised after repeated attempts: YES
```

The legacy car is stolen in 3 of the 6 scenarios. The proposed car refuses every attack and still opens for the owner.

### 2. Monte Carlo simulation (`monte_carlo`)

100,000 trials per configuration, fixed seed 2026. The random generator is written so that every compiler gives identical results. Unlock threshold 2 m.

**Relay set-up:** relay A is 0.5 m from the car and relay B is 1.0 m from the key fob. The relay link L is 5, 20 or 50 m, and the extra delay Δt is 0, 10, 100 or 1000 ns. Relays are evaluated with a single ranging round, the attacker's best case. That gives 12 L/Δt combinations per noise level, or **36 configurations in total**.

| Metric | σ = 0.1 m | σ = 0.3 m | σ = 0.5 m |
|---|---|---|---|
| Owner wrongly refused, single ranging round | 0.00% | 0.57% | 4.04% |
| Owner wrongly refused, median of 5 rounds | 0.00% | 0.01% | 0.32% |
| Relay success, single round (12 configurations each) | 0% | 0% | 0% |

**Why 0%?** The shortest simulated relay path is 0.5 + 5 + 1.0 = 6.5 m, which is already 4.5 m beyond the threshold, so 0% is expected from the geometry alone. It does **not** show that the design defeats every practical relay.

**Sensitivity analysis** (`results/sensitivity_path_sweep.csv`): acceptance rate when the effective path is close to the threshold (Δt = 0; single round / median of 5):

| d_eff | σ = 0.1 m | σ = 0.3 m | σ = 0.5 m |
|---|---|---|---|
| 2.25 m | 0.64% / 0.00% | 20.43% / 5.90% | 30.99% / 17.52% |
| 2.50 m | 0.00% / 0.00% | 4.78% / 0.09% | 15.92% / 3.12% |
| 3.00 m | 0.00% / 0.00% | 0.04% / 0.00% | 2.28% / 0.01% |
| 3.50 m | 0.00% / 0.00% | 0.00% / 0.00% | 0.13% / 0.00% |

Near the threshold, noise alone lets some attempts through. The median of 5 rounds lowers both the owner's false rejections and these false acceptances.

σ is the UWB measurement noise: 0.1 m for a clear line of sight, 0.3 m for a key in a pocket, 0.5 m when the body blocks the signal.

![False rejection rate](results/frr_by_noise.png)
![Measured distance: owner vs relay](results/distance_distribution.png)

The program also prints this as a text histogram (see `results/monte_carlo_output.txt`). The sensitivity sweep runs after all other steps, so it does not change their random draws or results.

## Limitations

This is an educational **software simulation**, not production automotive code, and no radio hardware is involved:

- UWB ranging is simulated, with no STS and no radio. Noise is Gaussian and independent between rounds, with no non-line-of-sight bias or multipath. The noise levels and owner positions are assumptions.
- Physical-layer distance-reduction attacks such as Ghost Peak are **not modelled**.
- A single AES-128 block stands in for AES-CMAC. The AES code is for learning and is not a certified module. Nonces come from `std::random_device`, not a certified generator. Key storage, provisioning, the KDF and STS binding are conceptual only.
- The relay link is modelled as propagating at the speed of light, which favours the attacker.
- Attacks that bypass the key entirely (CAN injection, OBD key programming) are out of scope.

## Key references

1. Francillon, Danev, Čapkun. *Relay Attacks on Passive Keyless Entry and Start Systems in Modern Cars.* NDSS 2011.
2. Brands, Chaum. *Distance-Bounding Protocols.* EUROCRYPT 1993.
3. NIST FIPS-197. *Advanced Encryption Standard (AES).*
4. IEEE Std 802.15.4z-2020. Enhanced UWB PHYs and Associated Ranging Techniques.
5. ISO/SAE 21434:2021. Road vehicles – Cybersecurity engineering.
6. ARAI AIS-189. Cyber Security and Cyber Security Management System. (MoRTH draft G.S.R. 503(E), 22 June 2026, proposes making AIS-189/190 mandatory. No final notification had been found as of November 2026.)
7. Leu et al. *Ghost Peak: Practical Distance Reduction Attacks Against HRP UWB Ranging.* USENIX Security 2022.
8. NIST SP 800-38B. *The CMAC Mode for Authentication.*

## License

MIT. See [LICENSE](LICENSE).
