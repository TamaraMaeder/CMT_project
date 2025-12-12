
// albedo_fn.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Compute albedo from a CSV with columns:
 *   height_m,bin_index,r_wet_m,number_concentration_m3
 *
 * Assumptions:
 * - r_wet_m is already in meters
 * - number_concentration_m3 is already in m^-3
 * - Each CSV row represents a set of particle properties at a level (the code
 *   does NOT integrate over height; it just sums r^2 * N across rows).
 *
 * Formula:
 *   tau = 2*pi * sum(r^2 * N)           // r in meters, N in m^-3
 *   A   = tau / (a/(1-g) + tau), with g=0.85, a=2.0
 *
 * Returns:
 *   - albedo of the cloud in [0,1) on success
 *   - NaN if the file cannot be opened or no valid rows are found
 */
double albedo_from_profile_csv(const char *csv_file_name) {
    const double g = 0.85;
    const double a = 2.0;
    const double TWO_PI = 2.0 * 3.14159265358979323846;

    FILE *fp = fopen(csv_file_name, "r");
    if (!fp) {
        return NAN;
    }

    char line[2048];
    double sum_r2N = 0.0;
    int valid_rows = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Skip leading whitespace
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (*p == '\0') continue;

        // Skip comment lines
        if (*p == '#') continue;

        // We expect 4 comma-separated fields:
        //   height_m,bin_index,r_wet_m,number_concentration_m3
        char *c1 = strchr(p, ',');
        if (!c1) continue;
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        char *c3 = strchr(c2 + 1, ',');
        if (!c3) continue;

        // Split tokens in place
        *c1 = '\0';
        *c2 = '\0';
        *c3 = '\0';

        char *tok_height_m = p;           // currently unused
        char *tok_bin_index = c1 + 1;     // unused, but parsed
        char *tok_r_m = c2 + 1;           // r_wet_m (meters)
        char *tok_N_m3 = c3 + 1;          // number_concentration_m3 (m^-3)

        // Trim leading spaces on r and N
        while (*tok_r_m == ' ' || *tok_r_m == '\t') ++tok_r_m;
        while (*tok_N_m3 == ' ' || *tok_N_m3 == '\t') ++tok_N_m3;

        // Parse r (meters)
        char *end_r = NULL;
        double r_m = strtod(tok_r_m, &end_r);
        if (end_r == tok_r_m) {
            // Could be header "r_wet_m"; skip
            continue;
        }

        // Parse N (m^-3)
        char *end_N = NULL;
        double N_m3 = strtod(tok_N_m3, &end_N);
        if (end_N == tok_N_m3) {
            // Could be header "number_concentration_m3"; skip
            continue;
        }

        // Ignore negative or NaN values
        if (!(r_m >= 0.0) || !(N_m3 >= 0.0)) {
            continue;
        }

        // Accumulate r^2 * N
        sum_r2N += r_m * r_m * N_m3;
        ++valid_rows;
    }

    fclose(fp);

    if (valid_rows == 0) {
        return NAN;
    }

    double tau = TWO_PI * sum_r2N;
    double denom = a / (1.0 - g) + tau;
    if (denom <= 0.0) {
        return NAN;
    }

    double A = tau / denom;

    // Clamp to [0,1)
    if (A < 0.0) A = 0.0;
    if (A >= 1.0) A = nextafter(1.0, 0.0);

    return A;
}
