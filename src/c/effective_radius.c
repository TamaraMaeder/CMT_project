
// effective_radius.c
// Computes the effective radius of a mixing of two aerosols by reading two CSV files.
// CSV columns per row: height_m,bin_index,r_wet_m,number_concentration_m3
//
// Formula:
//   re = sqrt( ( sum_1(r^2 * N) + sum_2(r^2 * N) ) / ( (sum_1(N) + sum_2(N)) * h ) )
// where h is the layer thickness (meters), default 250 m in example.
//
// Notes:
// - Lines with non-positive r_wet_m or number_concentration_m3 are ignored.
// - Header line (with letters/underscores) is skipped.
// - Returns NaN on error (file open failure, h <= 0, or total N <= 0).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Public function prototype (no header file needed). 
double effective_radius_mix(const char *csv_path_aerosol1,
                            const char *csv_path_aerosol2,
                            double layer_thickness_m);


// Trim leading/trailing whitespace (in place). Returns pointer to first non-space char. 
static char *trim(char *s) {
    if (!s) return s;
    while (isspace((unsigned char)*s)) s++;  // leading
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;  // trailing
    end[1] = '\0';
    return s;
}

// Return 1 if line looks like a header (contains letters or underscores), else 0. 
static int looks_like_header(const char *line) {
    if (!line) return 0;
    for (const char *p = line; *p; ++p) {
        if (isalpha((unsigned char)*p) || *p == '_') return 1;
    }
    return 0;
}

/* Accumulate sums from a CSV file:
 *   height_m,bin_index,r_wet_m,number_concentration_m3
 * Only uses r_wet_m (col 3) and number_concentration_m3 (col 4).
 * Adds to *sum_r2n and *sum_n. Returns 0 on success; non-zero on failure.
 */
static int accumulate_file(const char *path, double *sum_r2n, double *sum_n) {
    if (!path || !sum_r2n || !sum_n) return -1;

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        return -2;
    }

    char line[4096];
    int line_no = 0;

    while (fgets(line, sizeof(line), f)) {
        line_no++;

        // Strip UTF-8 BOM if present on first line
        if (line_no == 1 && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            memmove(line, line + 3, strlen(line) - 2);
        }

        char *s = trim(line);
        if (*s == '\0' || *s == '#') continue;  // skip empty/comment

        // Skip a header line like "height_m,bin_index,..."
        if (line_no == 1 && looks_like_header(s)) continue;

        // Tokenize by comma
        int tok_count = 0;
        char *saveptr = NULL;
        char *token = strtok_r(s, ",", &saveptr);

        double r_wet = NAN;
        double N = NAN;

        // Expected columns:
        // 1: height_m
        // 2: bin_index
        // 3: r_wet_m
        // 4: number_concentration_m3
        while (token) {
            tok_count++;
            char *t = trim(token);

            if (tok_count == 3) {
                r_wet = strtod(t, NULL);
            } else if (tok_count == 4) {
                N = strtod(t, NULL);
            }

            token = strtok_r(NULL, ",", &saveptr);
        }

        // Malformed line or header-like—skip
        if (tok_count < 4) continue;

        // Only accumulate valid positives
        if (isfinite(r_wet) && isfinite(N) && r_wet > 0.0 && N > 0.0) {
            *sum_r2n += (r_wet * r_wet) * N;
            *sum_n   += N;
        }
    }

    fclose(f);
    return 0;
}

// Public function 

double effective_radius_mix(const char *csv_path_aerosol1,
                            const char *csv_path_aerosol2,
                            double layer_thickness_m) {
    if (!csv_path_aerosol1 || !csv_path_aerosol2) return NAN;
    if (!(layer_thickness_m > 0.0)) return NAN;

    double sum_r2n = 0.0;
    double sum_n   = 0.0;

    if (accumulate_file(csv_path_aerosol1, &sum_r2n, &sum_n) != 0) {
        return NAN;
    }
    if (accumulate_file(csv_path_aerosol2, &sum_r2n, &sum_n) != 0) {
        return NAN;
    }

    if (!(sum_n > 0.0)) {
        // No valid concentrations found
        return NAN;
    }

    double re = sqrt(sum_r2n / (sum_n * layer_thickness_m));
    return re;
}

