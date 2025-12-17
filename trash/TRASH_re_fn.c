
// File: src/c/re_fn.c// File:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 * Effective Radius Computation (fixed h = 250 m)
 *
 * CSV format for each input file:
 *   height_m,bin_index,r_wet_m,number_concentration_m3
 *
 * Assumptions:
 * - r_wet_m is in meters
 * - number_concentration_m3 is in m^-3
 * - We sum across all rows (no height weighting/integration).
 * - h is fixed to 250 meters here, per user specification.
 *
 * Formula:
 *   r_e = sqrt( ( sum_1(r^2 * N) + sum_2(r^2 * N) ) /
 *               ( (sum_1(N) + sum_2(N)) * h ) )
 *
 * Returns:
 *   - r_e in meters on success
 *   - NaN on failure (cannot open files, no valid rows, bad denominator)
 */

/* Accumulate sum(r^2 * N) and sum(N) from a CSV file. */
static int accumulate_r2N_and_N_from_csv(const char *csv_file_name,
                                         double *out_sum_r2N,
                                         double *out_sumN,
                                         int *out_valid_rows)
{
    if (!csv_file_name || !out_sum_r2N || !out_sumN || !out_valid_rows) {
        return -1;
    }

    *out_sum_r2N = 0.0;
    *out_sumN = 0.0;
    *out_valid_rows = 0;

    FILE *fp = fopen(csv_file_name, "r");
    if (!fp) {
        return -1; // caller will decide whether to fail if missing
    }

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        // Trim leading whitespace
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (*p == '\0') continue;

        // Skip comments
        if (*p == '#') continue;

        // Expect 4 fields: height_m,bin_index,r_wet_m,number_concentration_m3
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
        char *tok_r_m  = c2 + 1;          // r_wet_m (meters)
        char *tok_N_m3 = c3 + 1;          // number_concentration_m3 (m^-3)

        // Trim leading spaces for r and N tokens
        while (*tok_r_m  == ' ' || *tok_r_m  == '\t') ++tok_r_m;
        while (*tok_N_m3 == ' ' || *tok_N_m3 == '\t') ++tok_N_m3;

        // Parse r (meters)
        char *end_r = NULL;
        double r_m = strtod(tok_r_m, &end_r);
        if (end_r == tok_r_m) {
            // Likely header line; skip
            continue;
        }

        // Parse N (m^-3)
        char *end_N = NULL;
        double N_m3 = strtod(tok_N_m3, &end_N);
        if (end_N == tok_N_m3) {
            // Likely header line; skip
            continue;
        }

        // Ignore negative or NaN
        if (!(r_m >= 0.0) || !(N_m3 >= 0.0)) {
            continue;
        }

        *out_sum_r2N += r_m * r_m * N_m3;
        *out_sumN    += N_m3;
        (*out_valid_rows)++;
    }

    fclose(fp);
    return 0;
}

/* Internal worker: compute effective radius given two CSVs and h (meters). */
static double effective_radius_from_two_aerosol_csvs(const char *csv_aerosol_one,
                                                     const char *csv_aerosol_two,
                                                     double h_m)
{
    if (!(h_m > 0.0)) {
        return NAN;
    }

    // Accumulate sums for each file
    double sum_r2N_1 = 0.0, sum_r2N_2 = 0.0;
    double sum_N_1   = 0.0, sum_N_2   = 0.0;
    int valid_rows_1 = 0,   valid_rows_2 = 0;

    if (accumulate_r2N_and_N_from_csv(csv_aerosol_one, &sum_r2N_1, &sum_N_1, &valid_rows_1) != 0) {
        return NAN; // fail if first CSV cannot be opened
    }
    if (accumulate_r2N_and_N_from_csv(csv_aerosol_two, &sum_r2N_2, &sum_N_2, &valid_rows_2) != 0) {
        return NAN; // fail if second CSV cannot be opened
    }

    if ((valid_rows_1 + valid_rows_2) == 0) {
        return NAN; // no usable data
    }

    const double numerator   = (sum_r2N_1 + sum_r2N_2);
    const double denom_inner = (sum_N_1   + sum_N_2);
    const double denom       = denom_inner * h_m;

    if (!(denom > 0.0)) {
        return NAN; // avoid division by zero or negative
    }
    if (!(numerator >= 0.0)) {
        return NAN;
    }

    return sqrt(numerator / denom); // meters
}

/*
 * Public API expected by your runner:
 *   double compute_effective_radius_two_csv(const char *csv_file_1, const char *csv_file_2);
 *
 * This wrapper fixes h = 250.0 meters, per your assumption.
 */
double compute_effective_radius_two_csv(const char *csv_file_1, const char *csv_file_2)
{
    const double h_m = 250.0; // fixed thickness/path length in meters
    return effective_radius_from_two_aerosol_csvs(csv_file_1, csv_file_2, h_m);

