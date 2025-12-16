
// albedo_fn.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Compute cloud albedo for a mixture of TWO aerosol populations,
 * each provided as a CSV with columns:
 *   height_m,bin_index,r_wet_m,number_concentration_m3
 *
 * Assumptions:
 * - r_wet_m is already in meters
 * - number_concentration_m3 is already in m^-3
 * - Each CSV row represents one (bin, level) sample; we sum r^2 * N across rows.
 * - We do NOT integrate over height (no dz weighting).
 *
 * Optical depth (mixture of two aerosols):
 *   tau = 2*pi * ( sum_1(r^2 * N) + sum_2(r^2 * N) )
 *
 * Albedo:
 *   A = tau / (a/(1-g) + tau), with g=0.85, a=2.0
 *
 * Returns:
 *   - albedo in [0,1) on success
 *   - NaN if a file cannot be opened or no valid rows are found in BOTH files
 */

static int accumulate_r2N_from_csv(const char *csv_file_name,
                                   double *out_sum_r2N,
                                   int *out_valid_rows)
{
    if (!csv_file_name || !out_sum_r2N || !out_valid_rows) {
        return -1;
    }

    *out_sum_r2N = 0.0;
    *out_valid_rows = 0;

    FILE *fp = fopen(csv_file_name, "r");
    if (!fp) {
        return -1; // caller can decide to fail if either file is missing
    }

    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        // Skip leading whitespace
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (*p == '\0') continue;

        // Skip comments
        if (*p == '#') continue;

        // Expect 4 CSV fields:
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

        // char *tok_height_m = p;        // unused
        // char *tok_bin_index = c1 + 1;  // unused
        char *tok_r_m = c2 + 1;           // r_wet_m (meters)
        char *tok_N_m3 = c3 + 1;          // number_concentration_m3 (m^-3)

        // Trim leading spaces on r and N
        while (*tok_r_m == ' ' || *tok_r_m == '\t') ++tok_r_m;
        while (*tok_N_m3 == ' ' || *tok_N_m3 == '\t') ++tok_N_m3;

        // Parse r (meters)
        char *end_r = NULL;
        double r_m = strtod(tok_r_m, &end_r);
        if (end_r == tok_r_m) {
            // Likely header "r_wet_m"; skip
            continue;
        }

        // Parse N (m^-3)
        char *end_N = NULL;
        double N_m3 = strtod(tok_N_m3, &end_N);
        if (end_N == tok_N_m3) {
            // Likely header "number_concentration_m3"; skip
            continue;
        }

        // Ignore negative or NaN
        if (!(r_m >= 0.0) || !(N_m3 >= 0.0)) {
            continue;
        }

        *out_sum_r2N += r_m * r_m * N_m3;
        (*out_valid_rows)++;
    }

    fclose(fp);
    return 0;
}

double albedo_from_two_aerosol_csvs(const char *csv_aerosol_one,
                                    const char *csv_aerosol_two)
{
    const double g = 0.85;
    const double a = 2.0;
    const double TWO_PI = 2.0 * 3.14159265358979323846;

    // Parse and sum r^2 N for each aerosol
    double sum_r2N_1 = 0.0, sum_r2N_2 = 0.0;
    int valid_rows_1 = 0, valid_rows_2 = 0;

    if (accumulate_r2N_from_csv(csv_aerosol_one, &sum_r2N_1, &valid_rows_1) != 0) {
        return NAN; // fail if first CSV cannot be opened
    }
    if (accumulate_r2N_from_csv(csv_aerosol_two, &sum_r2N_2, &valid_rows_2) != 0) {
        return NAN; // fail if second CSV cannot be opened
    }

    if ((valid_rows_1 + valid_rows_2) == 0) {
        return NAN; // no usable data
    }

    // Mixture optical depth
    double tau = TWO_PI * (sum_r2N_1 + sum_r2N_2);

    // Albedo
    double denom = a / (1.0 - g) + tau;
    if (!(denom > 0.0)) {
        return NAN;
    }

    double A = tau / denom;

    // Clamp to [0, 1)
    if (A < 0.0) A = 0.0;
    if (A >= 1.0) {
        // nextafter(1.0, 0.0) requires math.h
        A = nextafter(1.0, 0.0);
    }

    return A;
}

