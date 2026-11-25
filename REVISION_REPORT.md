# Revision report: revision 3 (25 November 2026)

Article: *Securing Passive Keyless Entry Systems Against Relay Attacks: Limitations of Existing Market Countermeasures and a UWB Distance-Bounding Approach*

Page numbers refer to the revised preview PDF (14 pages; the previous version had 12). Overleaf with the official `llncs.cls` may shift pages slightly.

**Source files used:** the brief mentioned `CYBERSECURITY_ARTICLE (2).pdf` and `pkes-relay-defense-cpp.zip`, but neither arrived with it. The revision is based on the last delivered versions of `paper/main.tex` and the C++ project, which are what that PDF and zip were built from.

---

## 1. Change report

| # | Original issue | Correction made | Section (page) |
|---|---|---|---|
| 1 | Abstract claimed "0% relay success across all tested relay lengths and delays" without assumptions; cited ADAC 237/3 statistic | Rewritten: problem, why encryption alone fails, market limits, layered architecture, C++ simulation method, actual numbers with the geometric explanation, explicit "not a hardware validation" | Abstract (1) |
| 2 | ADAC "only 3 of 237 models resisted": an older test (≈2019) presented as current | Replaced with ADAC's August 2026 results (850+ vehicles, almost half still openable) plus the ADAC note on UWB; added a caveat that results apply to the tested models | §1 (1) |
| 3 | "VbV found keyless entry on 59% of passenger cars stolen in 2025": original VbV source not found | **Removed** (see verification report) | §1 (1) |
| 4 | Indian regulation described as "becoming mandatory" | Now described as draft G.S.R. 503(E) of 22 June 2026, with proposed dates 1 Oct 2026 – 1 Oct 2029, no final notification found as of Nov 2026; conclusion corrected the same way | §1 (1), §10 (12) |
| 5 | UN R155 said to make a CSMS "mandatory in many markets" | Reworded: required for type approval in jurisdictions that apply it | §1 (1) |
| 6 | Named Indian car models (Creta, Seltos, Nexon) as PKES examples without a source | Generalised to "many cars sold in India and worldwide" | §1 (1) |
| 7 | Contributions implied a new solution | Contributions now say the article integrates existing techniques and "does not propose a new distance-bounding primitive" | §1 (2) |
| 8 | TARA described as "the method ISO/SAE 21434 prescribes" | Described as a simplified qualitative TARA *inspired by* ISO/SAE 21434, not a complete or formal one; scoring rule stated (illustrative matrix; Severe × VL/L/M/H → 2/3/4/5) | §3 (3–4) |
| 9 | TARA: phone-key relay rated High feasibility although only researcher demonstrations were cited | Re-rated Medium (risk 4); threat IDs T1–T5 added | Table 1 (4) |
| 10 | Table 6 (residual risk) rows did not match Table 1; "after" scores of 1 had no defined method | Rebuilt with the same T1–T5 rows, impact held constant, feasibility re-rated, risk from the same matrix (all → 2); labelled illustrative qualitative estimates; non-hardened UWB case noted (risk 3) | §8.3, Table 7 (11–12) |
| 11 | Research gap did not separate this work from prior UWB work | Rewritten: contribution is integration plus a reproducible evaluation, not a new protocol | §4.2 (5) |
| 12 | Literature review repeated what Table 2 says; Hancke & Kuhn missing from the table | Prose condensed into one subsection; Hancke & Kuhn added to Table 2; Ghost Peak figures added (12 m → 0 m, up to 4%, USD 65) | §4 (4–5) |
| 13 | Eq. (1)–(2) did not define the terms; unclear whether the relay link is counted twice | Defined t_round, t_reply, d_path (= d_CA + L + d_BK), propagation delay, Δt_relay (whole round trip), ε; added Eq. (1) for t_round and derived Eq. (3); stated that L is counted once | §6.1 (6–7) |
| 14 | "Since the attacker can only add delay, never remove it, the measured distance can only grow" | Replaced with a qualified paragraph: true for conventional relays *if* time-of-arrival is trustworthy; Ghost Peak counter-example; need for authenticated ranging, unpredictable STS, secure timestamps, robust receivers, retry limits; UWB "not inherently immune" | §6.1 (7) |
| 15 | "UWB accuracy of about 10 cm" (no source); "attacker cannot send early" (too absolute) | Removed or folded into the qualified paragraph | §6.1 (7) |
| 16 | Protocol presented as complete; session-key/STS derivation described as if implemented (it is not in the C++ code) | Implemented model (Eq. 4, single AES block) separated from a *conceptual* design (Eq. 5: AES-CMAC, KDF, STS seeded from K_s); key management and freshness discussed; stated that AES alone does not give PKES security | §6.2 (7) |
| 17 | Fig. 2: steps not grouped, red was the only cue for the time-critical steps, conceptual steps not marked | Redrawn: phases A–D, dashed box with a text label for time-critical ranging (repeated 5×), asterisks on conceptual steps, explanation in the caption | Fig. 2 (8) |
| 18 | Second factor sometimes worded as if mandatory | Consistently called optional | §6, §6.3 (5, 8) |
| 19 | Fig. 1 used red fill and red text as the only marker for relays | Relays shown as dashed boxes; L labelled; no colour-only cues | Fig. 1 (3) |
| 20 | Implementation section did not separate simulated from real, or educational AES from certified | States: software simulation, no radio hardware, not an ECU; AES educational (FIPS-197 test vector), not certified; lists implemented vs conceptual parts; notes that monte_carlo.cpp samples Eq. (3), which is equivalent to Eq. (1)–(2) | §7 (8–9) |
| 21 | GitHub URL | Kept as a clearly marked placeholder (`\repourl`, TODO comment); no URL invented | §7 (8) |
| 22 | Table 4 (demo) did not explain why replay is refused by both cars | Text explains that the legacy model also uses challenge–response; scenario parameters (L, Δt) and d̂ values added | §7, Table 4 (9) |
| 23 | Simulation said "all 12 combinations" | Explains 12 L/Δt combinations per noise level and 36 configurations in total; relays use a single round | §8.2 (10) |
| 24 | 0% relay success presented as showing the design is "secure" | Explained as a consequence of the geometry (shortest relay path 6.5 m = 9σ above threshold at σ = 0.5 m); explicitly "does not show that the architecture defeats every practical relay" | §8.2 (10) |
| 25 | No analysis near the threshold | **New sensitivity analysis** (code + CSV + Table 6): acceptance rate for d_eff = 2.25–3.5 m; shows non-zero success near the threshold and that the median of 5 also lowers false acceptance | §8.2, Table 6 (10–11) |
| 26 | FRR improvement not explained | Explained: the median needs ≥3 of 5 rounds over the threshold, which is unlikely with independent errors | §8.2 (11) |
| 27 | Evaluation mixed the principle, the simulation, hardware and PHY attacks | Opening paragraph separates the four questions; "Assumptions and limits" paragraph added | §8 (9, 11) |
| 28 | "Real relays… cost far more than a few nanoseconds" (unsupported) | Replaced: analogue relays can keep delay small, so the defence must not rely on delay alone; relay link modelled at c, which favours the attacker | §8.1 (10) |
| 29 | Limitations incomplete | Added: no hardware validation; PHY attacks not modelled; simplified crypto (no CMAC, `std::random_device`, no key storage); independent Gaussian errors; existing items kept | §9 (12) |
| 30 | Conclusion claimed the design "turns a law of physics into a security control" and that AIS-189/190 are becoming mandatory | Rewritten to state what was shown and its limits; India described as draft rules | §10 (12) |
| 31 | Email placeholder looked like a real address | Changed to `[institutional e-mail address]` with a TODO comment | Title (1) |
| 32 | References: AutoExpress (old ADAC data) | Replaced in the same slot (ref 19) by the ADAC 2026 press release; refs 1–22 keep their numbering; added refs 23–26 (ADAC UWB page, G.S.R. 503(E), FIPS-197, NIST SP 800-38B); access dates updated | References (13–14) |
| 33 | Terminology mixed (key / fob / key fob; relay delay vs processing delay) | Standardised: key fob, relay link, relay delay, distance bounding, false rejection rate, UWB ranging | Throughout |
| 34 | Risk-matrix table (one used row) took a float slot | Matrix stated in text and in the Table 1 caption | §3.2 (3) |

### Code and output changes

| File | Change |
|---|---|
| `src/monte_carlo.cpp` | Added step 5 (sensitivity sweep), run **after** steps 1–4 so all earlier random draws and results are unchanged; prints a note on the 6.5 m geometry; header documents the single-round relay evaluation |
| `results/sensitivity_path_sweep.csv` | New |
| `results/monte_carlo_output.txt`, `attack_demo_output.txt` | Regenerated (Table 5 numbers identical to the previous version) |
| `results/*.png` | Regenerated from the CSVs (the data used is unchanged) |
| `README.md` | Synchronised: equation definitions, Ghost Peak caveat, 36 configurations, geometric explanation, sensitivity table, implemented vs conceptual parts, expanded limitations, draft-regulation note |
| `paper/main.tex` | Revised article |

All programs build with `g++ -std=c++17 -Wall -Wextra -pedantic` with no warnings, and all 13 unit tests pass.

---

## 2. Verification report

### Verified with a source (kept, with citation)

| Claim | Status | Source |
|---|---|---|
| ADAC has tested 850+ keyless vehicles over ten years; almost half can still be opened illegally | Verified (ADAC press release, 18 Aug 2026; headline and sub-headline) | presse.adac.de, ref [19] |
| ADAC: UWB-equipped models are better protected | Verified (ADAC article "Zehn Jahre Keyless-Systeme im Test") | adac.de, ref [23] |
| MoRTH draft G.S.R. 503(E), 22 June 2026; Rules 125-T/125-U; AIS-189/190; phased 1 Oct 2026 – 1 Oct 2029; 30-day comment period | Verified as a **draft** (Autocar India, Autocar Pro, Outlook Business, TeamLease RegTech) | refs [22], [24] |
| No final notification of those rules as of mid-November 2026 | Secondary sources only (two industry sites reported none found as of 16 Nov 2026); the article says "we could not find a final notification" | Not cited as a fact |
| Ghost Peak: 12 m → 0 m, success up to 4%, USD 65 device, Apple U1 / NXP / Qorvo chips | Verified (USENIX abstract and arXiv paper) | ref [9] |
| Keyless theft "can take under a minute" | Verified as reported by Help Net Security (journalistic claim) | ref [20] |

### Qualified

| Claim | How it was qualified |
|---|---|
| ADAC statistic | Replaced by current ADAC data, with "results apply to the models ADAC tested" |
| TARA "following ISO/SAE 21434" | Now "simplified qualitative TARA inspired by ISO/SAE 21434; not a complete or formal TARA"; the risk matrix is described as an illustrative choice for this article, not taken from the standard |
| Residual-risk values | Labelled illustrative qualitative estimates that assume correct implementation |
| UWB security | "Not inherently immune"; depends on receiver hardening |
| 0% relay success | Attributed to the modelled geometry |
| Noise levels σ = 0.1 / 0.3 / 0.5 m | Stated as assumptions, not measurements |

### Removed

| Claim | Reason |
|---|---|
| "Stichting VbV found keyless entry on 59% of passenger cars stolen in 2025" | It appears only in the Help Net Security article. The VbV 2025 annual figures found (7,497 passenger cars stolen, +12%) say nothing about the keyless share, and no VbV report or dataset with the 59% figure or its denominator could be located. |
| "ADAC: only 3 of 237 models resisted" | Older test data (Which?/ADAC, about 2019), superseded by ADAC's 2026 figures |
| Hyundai Creta / Kia Seltos / Tata Nexon as PKES examples | Not checked against manufacturer specifications |
| "UWB accuracy of about 10 cm" | No source given |
| "Real relays cost far more than a few nanoseconds" | Unsupported; contradicted by low-latency analogue relays |

### Not independently verified in this session (please check before submission)

- Exact page ranges and issue numbers for refs [3] Hancke & Kuhn (pp. 67–73), [5] Ranganathan & Čapkun (IEEE S&P 15(3), 52–58) and [7] Wouters et al. (TCHES 2019(3), 66–85). These are believed correct but were not checked against the publishers' pages.
- The exact title wording and URLs of the NCC Group advisory [14] and the IOActive white paper [15]. The URLs are domain-level only; replace them with the exact document links if available.
- The AIS-189 document [21]: no publication year is given because the current revision could not be confirmed.
- "LF signals fade within about 1–2 m" and the LF/UHF frequency bands: taken from Francillon et al. [1]; not re-checked.
- Rasmussen & Čapkun's "about 1 ns processing delay": from memory of [4]; worth checking in the paper.
- Whether MoRTH published a final notification after 16 November 2026. Check egazette.gov.in before submission and update §1 and §10 if a final rule has appeared.

### Reproducibility check

| Paper value | Program output | Match |
|---|---|---|
| FRR single round: 0.00 / 0.57 / 4.04% | `monte_carlo_output.txt` | ✓ |
| FRR median of 5: 0.00 / 0.01 / 0.32% | same | ✓ |
| Relay success over 36 configurations: 0% (max 0.000%) | same | ✓ |
| Sensitivity table (12 cells × 2) | `sensitivity_path_sweep.csv` | ✓ |
| Demo: d̂ ≈ 1.0 m, ≈ 171 m, ≈ 6.4 m; three legacy thefts | `attack_demo_output.txt` | ✓ |
| 13 unit tests, 6 scenarios, file names | source tree | ✓ |
