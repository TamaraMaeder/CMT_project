
// albedo_fn.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Compute albedo from a CSV with columns:
 *   height_m,radius_micron,concentration_m3
 *
 * Assumptions/Conversions:
 * - radius_micron is converted to meters (1 micron = 1e-6 m)
 * - concentration_m3 column is actually in cm^-3 -> convert to m^-3 by multiplying by 1e6
 *
 * Formula:
 *   tau = 2*pi * sum(r^2 * N)           // r in meters, N in m^-3
 *   A   = tau / (a/(1-g) + tau), with g=0.85, a=2.0
 *
 * Returns:
 *   - albedo in [0,1) on success
 *   - NaN if the file cannot be opened or no valid rows are found
 */
double albedo_from_profile_csv(const char *csv_file_name) {
    const double g = 0.85;
    const double a = 2.0;
    const double TWO_PI = 2.0 * 3.14159265358979323846;
    const double MICRON_TO_M = 1e-6;
    const double CM3_TO_M3 = 1e12;  // multiply cm^-3 by 1e6 to get m^-3

    FILE *fp = fopen(csv_file_name, "r");
    if (!fp) {
        return NAN;
    }

    char line[1024];
    double sum_r2N = 0.0;
    int valid_rows = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Skip leading whitespace
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (*p == '\0') continue;
        // Optional: skip comment lines
        if (*p == '#') continue;

        // Expect 3 comma-separated fields: height,radius_micron,concentration_m3 (actually cm^-3)
        char *c1 = strchr(p, ',');
        if (!c1) continue;
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;

        // Split tokens
        *c1 = '\0';
        *c2 = '\0';
        char *tok_height = p;                // unused, but parsed
        char *tok_radius_micron = c1 + 1;
        char *tok_N_cm3 = c2 + 1;

        // Trim leading spaces on radius and N
        while (*tok_radius_micron == ' ' || *tok_radius_micron == '\t') ++tok_radius_micron;
        while (*tok_N_cm3 == ' ' || *tok_N_cm3 == '\t') ++tok_N_cm3;

        // Parse radius (micron) -> meters
        char *end_r = NULL;
        double r_micron = strtod(tok_radius_micron, &end_r);
        if (end_r == tok_radius_micron) {
            // Could be header "radius_micron"; skip
            continue;
        }

        // Parse number concentration (assumed cm^-3) -> convert to m^-3
        char *end_N = NULL;
        double N_cm3 = strtod(tok_N_cm3, &end_N);
        if (end_N == tok_N_cm3) {
            // Could be header "concentration_m3"; skip
            continue;
        }

        // Ignore negative or NaN values
        if (!(r_micron >= 0.0) || !(N_cm3 >= 0.0)) {
            continue;
        }

        // Convert micron -> meter, cm^-3 -> m^-3, and accumulate r^2 * N
        double r_m = r_micron * MICRON_TO_M;
        double N_m3 = N_cm3 * CM3_TO_M3;

        sum_r2N += r_m * r_m * N_m3;
        ++valid_rows;
    }

    fclose(fp);

    if (valid_rows == 0) {
        return NAN;
    }

    double tau = 250.0 * TWO_PI * sum_r2N; // multiply by 250 m to get the optical depth of the whole cloud 
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
