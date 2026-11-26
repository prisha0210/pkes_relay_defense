// monte_carlo.cpp - Monte Carlo simulation behind the article's results table.
//
// Model:   d_measured = d_path + c * dt_relay / 2 + noise,   noise ~ N(0, sigma)
// Unlock:  d_measured <= 2 m
//
//  * Owner: stands 0.5-1.5 m from the door. We count how often the owner is
//    wrongly refused (False Rejection Rate, FRR), with 1 ranging round and
//    with the median of 5 rounds.
//  * Attacker: relay A 0.5 m from car, relay B 1 m from key, relay link
//    L = 5, 20, 50 m, extra delay 0, 10, 100, 1000 ns. We count successes.
//
// 100,000 trials per configuration, fixed seed 2026 -> same results every run,
// on every compiler.
//
// Build:  g++ -std=c++17 -O2 -Iinclude src/monte_carlo.cpp -o monte_carlo
// Run:    ./monte_carlo        (Windows: monte_carlo.exe)
// Output: results/simulation_results.csv, results/distance_histogram.csv,
//         results/sensitivity_path_sweep.csv
//
// NOTE: relay success is evaluated with a SINGLE ranging round (the attacker's
// best case in this model). Every relay path is 0.5 + L + 1.0 >= 6.5 m, so the
// 0% result follows from the chosen geometry; step 5 shows what happens when
// the effective path is close to the threshold.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "pkes.hpp"

using namespace pkes;

constexpr int N = 100000;
constexpr double D_MAX = 2.0;
const double SIGMAS[3] = {0.10, 0.30, 0.50};
const double RELAY_LINKS_M[3] = {5, 20, 50};
const double RELAY_DELAYS_NS[4] = {0, 10, 100, 1000};

// One UWB distance estimate
static double measure(Rng& rng, double truePathM, double delayNs, double sigma) {
    return truePathM + C_M_PER_NS * delayNs / 2.0 + rng.normal(0.0, sigma);
}

static double median5(double v[5]) {
    std::sort(v, v + 5);
    return v[2];
}

int main() {
    Rng rng(2026);
    FILE* csv = std::fopen("results/simulation_results.csv", "w");
    if (!csv) { std::printf("Please create a 'results' folder first (mkdir results).\n"); return 1; }
    std::fprintf(csv, "metric,sigma_m,relay_link_m,relay_delay_ns,percent\n");

    double frrSingle[3], frrMedian[3];

    // 1) Owner false rejection, single ranging round
    for (int s = 0; s < 3; ++s) {
        int refused = 0;
        for (int i = 0; i < N; ++i) {
            double d = rng.uniform(0.5, 1.5);
            if (measure(rng, d, 0, SIGMAS[s]) > D_MAX) ++refused;
        }
        frrSingle[s] = 100.0 * refused / N;
        std::fprintf(csv, "owner_frr_single_round,%.2f,,,%.2f\n", SIGMAS[s], frrSingle[s]);
    }

    // 2) Owner false rejection, median of 5 rounds
    for (int s = 0; s < 3; ++s) {
        int refused = 0;
        for (int i = 0; i < N; ++i) {
            double d = rng.uniform(0.5, 1.5), v[5];
            for (double& x : v) x = measure(rng, d, 0, SIGMAS[s]);
            if (median5(v) > D_MAX) ++refused;
        }
        frrMedian[s] = 100.0 * refused / N;
        std::fprintf(csv, "owner_frr_median_of_5,%.2f,,,%.2f\n", SIGMAS[s], frrMedian[s]);
    }

    // 3) Relay attack success, single round (easiest case for the attacker)
    double worstRelay = 0.0;
    for (double L : RELAY_LINKS_M)
        for (double dt : RELAY_DELAYS_NS)
            for (int s = 0; s < 3; ++s) {
                int success = 0;
                double path = 0.5 + L + 1.0;
                for (int i = 0; i < N; ++i)
                    if (measure(rng, path, dt, SIGMAS[s]) <= D_MAX) ++success;
                double pct = 100.0 * success / N;
                worstRelay = std::max(worstRelay, pct);
                std::fprintf(csv, "relay_success,%.2f,%.0f,%.0f,%.3f\n", SIGMAS[s], L, dt, pct);
            }
    std::fclose(csv);

    // 4) Histogram data: owner vs zero-delay relay (5 m link), sigma = 0.3 m
    //    0.1 m bins from -0.5 m to 8.5 m (90 bins) saved to CSV
    const int BINS = 90;
    const double LO = -0.5, W = 0.1;
    std::vector<int> owner(BINS, 0), relay(BINS, 0);
    for (int i = 0; i < N; ++i) {
        double o = measure(rng, rng.uniform(0.5, 1.5), 0, 0.30);
        double r = measure(rng, 6.5, 0, 0.30);
        int bo = static_cast<int>(std::floor((o - LO) / W));
        int br = static_cast<int>(std::floor((r - LO) / W));
        if (bo >= 0 && bo < BINS) ++owner[bo];
        if (br >= 0 && br < BINS) ++relay[br];
    }
    FILE* h = std::fopen("results/distance_histogram.csv", "w");
    std::fprintf(h, "bin_start_m,bin_end_m,owner_trials,relay_trials\n");
    for (int b = 0; b < BINS; ++b)
        std::fprintf(h, "%.1f,%.1f,%d,%d\n", LO + b * W, LO + (b + 1) * W, owner[b], relay[b]);
    std::fclose(h);

    // 5) Sensitivity analysis (added in revision 2; runs AFTER steps 1-4 so their
    //    random draws, and therefore their results, are unchanged).
    //    Question: how close to the 2 m threshold must the effective one-way
    //    path be before a relayed (or distance-reduced) measurement passes?
    //    Zero relay delay; effective path d_eff = 2.25, 2.5, 3.0, 3.5 m.
    const double PATHS[4] = {2.25, 2.5, 3.0, 3.5};
    double sweepSingle[4][3], sweepMedian[4][3];
    FILE* sw = std::fopen("results/sensitivity_path_sweep.csv", "w");
    std::fprintf(sw, "effective_path_m,sigma_m,success_single_round_percent,success_median_of_5_percent\n");
    for (int p = 0; p < 4; ++p)
        for (int s = 0; s < 3; ++s) {
            int okSingle = 0, okMedian = 0;
            for (int i = 0; i < N; ++i) {
                if (measure(rng, PATHS[p], 0, SIGMAS[s]) <= D_MAX) ++okSingle;
                double v[5];
                for (double& x : v) x = measure(rng, PATHS[p], 0, SIGMAS[s]);
                if (median5(v) <= D_MAX) ++okMedian;
            }
            sweepSingle[p][s] = 100.0 * okSingle / N;
            sweepMedian[p][s] = 100.0 * okMedian / N;
            std::fprintf(sw, "%.2f,%.2f,%.3f,%.3f\n", PATHS[p], SIGMAS[s], sweepSingle[p][s], sweepMedian[p][s]);
        }
    std::fclose(sw);

    // ---- Print results ----
    std::printf("\nSimulation results (100,000 trials each, seed 2026)\n");
    std::printf("%s\n", std::string(66, '-').c_str());
    std::printf("%-36s%10s%10s%10s\n", "Metric", "s=0.1 m", "s=0.3 m", "s=0.5 m");
    std::printf("%-36s%9.2f%%%9.2f%%%9.2f%%\n", "Owner FRR, single round", frrSingle[0], frrSingle[1], frrSingle[2]);
    std::printf("%-36s%9.2f%%%9.2f%%%9.2f%%\n", "Owner FRR, median of 5 rounds", frrMedian[0], frrMedian[1], frrMedian[2]);
    std::printf("%-36s%s%.3f%%\n", "Relay success (36 combinations)", "max = ", worstRelay);
    std::printf("%s\n", std::string(66, '-').c_str());
    std::printf("Note: every simulated relay path is >= 6.5 m (0.5 + L + 1.0 with L >= 5 m),\n"
                "so 0%% relay success is expected from the geometry alone.\n");

    std::printf("\nSensitivity: success rate when the effective path is close to the threshold\n");
    std::printf("(zero relay delay; 'single / median of 5' ranging rounds)\n");
    std::printf("%-12s%18s%18s%18s\n", "d_eff", "s=0.1 m", "s=0.3 m", "s=0.5 m");
    for (int p = 0; p < 4; ++p) {
        std::printf("%-12.2f", PATHS[p]);
        for (int s = 0; s < 3; ++s) std::printf("%9.2f /%6.2f%%", sweepSingle[p][s], sweepMedian[p][s]);
        std::printf("\n");
    }

    // ASCII histogram (0.5 m rows) so the result is visible without any plotting tool
    std::printf("\nMeasured distance, owner (O) vs zero-delay relay (R), sigma = 0.3 m\n");
    std::printf("Each symbol = 1,000 trials.\n\n");
    for (int row = 0; row < BINS / 5; ++row) {
        int o = 0, r = 0;
        for (int k = 0; k < 5; ++k) { o += owner[row * 5 + k]; r += relay[row * 5 + k]; }
        double start = LO + row * 0.5;
        std::printf("%5.1f-%4.1f m  ", start, start + 0.5);
        for (int k = 0; k < o / 1000; ++k) std::printf("O");
        for (int k = 0; k < r / 1000; ++k) std::printf("R");
        std::printf("\n");
        if (std::fabs(start + 0.5 - D_MAX) < 1e-9)
            std::printf("            ---------- 2 m threshold: rows above = UNLOCK, rows below = REFUSE ----------\n");
    }
    std::printf("\nSaved results/simulation_results.csv, results/distance_histogram.csv and\n"
                "results/sensitivity_path_sweep.csv\n\n");
    return 0;
}
