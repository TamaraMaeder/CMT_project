
// albedo_fn.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

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

/* --------------------------- Helper utilities --------------------------- */

static void trim_crlf(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\r' || s[n-1] == '\n')) {
        s[n-1] = '\0';
        --n;
    }
}

// Return pointer to basename within 'path' (no allocation)
static const char* path_basename(const char *path) {
    if (!path) return "";
    const char *slash1 = strrchr(path, '/');
#ifdef _WIN32
    const char *slash2 = strrchr(path, '\\');
    const char *slash = (slash1 && slash2) ? (slash1 > slash2 ? slash1 : slash2)
                                           : (slash1 ? slash1 : slash2);
#else
    const char *slash = slash1;
#endif
    return slash ? (slash + 1) : path;
}

// Copy directory portion of 'path' into 'out_dir' (size out_n). If no dir, "."
static void path_dirname(const char *path, char *out_dir, size_t out_n) {
    if (!path || !out_dir || out_n == 0) return;
    const char *base = path_basename(path);
    size_t dir_len = (size_t)(base - path);
    if (dir_len == 0) {
        snprintf(out_dir, out_n, ".");
        return;
    }
    if (dir_len >= out_n) dir_len = out_n - 1;
    memcpy(out_dir, path, dir_len);
    out_dir[dir_len] = '\0';
}

// Remove trailing ".csv" (case-insensitive) from basename string in-place
static void strip_csv_ext(char *name) {
    if (!name) return;
    size_t n = strlen(name);
    if (n >= 4) {
        char *ext = name + (n - 4);
        if ((ext[0] == '.' || ext[0] == '.') &&
            (ext[1] == 'c' || ext[1] == 'C') &&
            (ext[2] == 's' || ext[2] == 'S') &&
            (ext[3] == 'v' || ext[3] == 'V')) {
            ext[0] = '\0';
        }
    }
}

// Parse aerosol and scenario from basename "<aerosol>_profile_<scenario>[.csv]"
static void parse_aerosol_and_scenario_from_name(const char *basename,
                                                 char *aerosol_out, size_t a_n,
                                                 char *scenario_out, size_t s_n) {
    if (!basename || !aerosol_out || !scenario_out) return;

    // Work on local mutable copy
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", basename);
    strip_csv_ext(tmp);

    const char *marker = "_profile_";
    char *pos = strstr(tmp, marker);
    if (!pos) {
        // No marker found: put entire name as aerosol, scenario empty
        snprintf(aerosol_out, a_n, "%s", tmp);
        snprintf(scenario_out, s_n, "");
        return;
    }

    // aerosol = [start .. pos-1]
    size_t aerosol_len = (size_t)(pos - tmp);
    if (aerosol_len >= a_n) aerosol_len = a_n - 1;
    memcpy(aerosol_out, tmp, aerosol_len);
    aerosol_out[aerosol_len] = '\0';

    // scenario = [pos + strlen(marker) .. end]
    const char *sc_start = pos + strlen(marker);
    snprintf(scenario_out, s_n, "%s", sc_start);
}

/* ------------------- CSV table read/update/write ------------------- */

typedef struct {
    char *scenario;
    char *aerosol;
    double albedo;
} Row;

static void free_rows(Row *rows, size_t n) {
    if (!rows) return;
    for (size_t i = 0; i < n; ++i) {
        free(rows[i].scenario);
        free(rows[i].aerosol);
    }
    free(rows);
}

static int read_albedo_table(const char *path, Row **out_rows, size_t *out_n) {
    *out_rows = NULL;
    *out_n = 0;

    FILE *fp = fopen(path, "r");
    if (!fp) {
        // If file doesn't exist, that's fine.
        return (errno == ENOENT) ? 0 : -1;
    }

    char line[2048];
    size_t cap = 0;
    Row *rows = NULL;

    int is_first_line = 1;
    while (fgets(line, sizeof(line), fp)) {
        trim_crlf(line);
        char *p = line;
        // skip empty
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0') continue;

        // Skip header if present
        if (is_first_line) {
            is_first_line = 0;
            if (strncmp(p, "scenario,", 9) == 0) {
                // header line
                continue;
            }
        }

        // Split by commas: scenario,aerosol_name,albedo
        char *c1 = strchr(p, ',');
        if (!c1) continue;
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;

        *c1 = '\0';
        *c2 = '\0';
        char *tok_scenario = p;
        char *tok_aerosol  = c1 + 1;
        char *tok_albedo   = c2 + 1;

        // Parse albedo
        char *endA = NULL;
        double A = strtod(tok_albedo, &endA);
        if (endA == tok_albedo) {
            // not a number; skip
            continue;
        }

        // Grow array
        if (*out_n == cap) {
            size_t new_cap = cap ? cap * 2 : 16;
            Row *tmp = (Row*)realloc(rows, new_cap * sizeof(Row));
            if (!tmp) { fclose(fp); free_rows(rows, *out_n); return -1; }
            rows = tmp;
            cap = new_cap;
        }

        // Copy strings
        size_t ls = strlen(tok_scenario);
        size_t la = strlen(tok_aerosol);
        rows[*out_n].scenario = (char*)malloc(ls + 1);
        rows[*out_n].aerosol  = (char*)malloc(la + 1);
        if (!rows[*out_n].scenario || !rows[*out_n].aerosol) {
            fclose(fp);
            free_rows(rows, *out_n);
            return -1;
        }
        memcpy(rows[*out_n].scenario, tok_scenario, ls + 1);
        memcpy(rows[*out_n].aerosol,  tok_aerosol,  la + 1);
        rows[*out_n].albedo = A;

        (*out_n)++;
    }

    fclose(fp);
    *out_rows = rows;
    return 0;
}

static int write_albedo_table_atomic(const char *path, const Row *rows, size_t n) {
    // Write to a temp file in the same directory, then rename.
    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) return -1;

    // Header
    fprintf(fp, "scenario,aerosol_name,albedo\n");
    for (size_t i = 0; i < n; ++i) {
        // Print with good precision
        fprintf(fp, "%s,%s,%.17g\n", rows[i].scenario, rows[i].aerosol, rows[i].albedo);
    }

    if (fclose(fp) != 0) {
        remove(tmp_path);
        return -1;
    }

    // Replace original
    if (rename(tmp_path, path) != 0) {
        // On Windows, rename over existing may fail; attempt fallback:
        remove(path);
        if (rename(tmp_path, path) != 0) {
            remove(tmp_path);
            return -1;
        }
    }
    return 0;
}

/*
 * Upsert (scenario,aerosol_name,albedo) into data/all_albedo_values.csv
 * where 'data' is the directory of the input 'profile_csv_path'.
 *
 * If row exists (matching both scenario and aerosol), updates albedo.
 * Otherwise, appends a new row.
 *
 * Returns 0 on success, non-zero on error.
 */
static int upsert_albedo_value(const char *profile_csv_path,
                               const char *scenario,
                               const char *aerosol,
                               double albedo) {
    // Determine output table path: <dir_of_input>/all_albedo_values.csv
    char dir[1024];
    path_dirname(profile_csv_path, dir, sizeof(dir));

    char out_path[1200];
#ifdef _WIN32
    snprintf(out_path, sizeof(out_path), "%s\\all_albedo_values.csv", dir);
#else
    snprintf(out_path, sizeof(out_path), "%s/all_albedo_values.csv", dir);
#endif

    Row *rows = NULL;
    size_t n = 0;
    if (read_albedo_table(out_path, &rows, &n) < 0) {
        // Could not read existing; try to proceed with empty set
        rows = NULL;
        n = 0;
    }

    // Search for existing row
    size_t idx = n; // default "not found"
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(rows[i].scenario, scenario) == 0 &&
            strcmp(rows[i].aerosol,  aerosol)  == 0) {
            idx = i;
            break;
        }
    }

    if (idx < n) {
        // Update existing
        rows[idx].albedo = albedo;
    } else {
        // Append new
        Row *tmp = (Row*)realloc(rows, (n + 1) * sizeof(Row));
        if (!tmp) { free_rows(rows, n); return -1; }
        rows = tmp;

        size_t ls = strlen(scenario);
        size_t la = strlen(aerosol);
        rows[n].scenario = (char*)malloc(ls + 1);
        rows[n].aerosol  = (char*)malloc(la + 1);
        if (!rows[n].scenario || !rows[n].aerosol) {
            free_rows(rows, n);
            return -1;
        }
        memcpy(rows[n].scenario, scenario, ls + 1);
        memcpy(rows[n].aerosol,  aerosol,  la + 1);
        rows[n].albedo = albedo;
        n++;
    }

    // Write back atomically
    int rc = write_albedo_table_atomic(out_path, rows, n);
    free_rows(rows, n);
    return rc;
}

/* ------------------- Public API: compute & upsert ------------------- */

/*
 * Computes albedo from a profile CSV and upserts the result into
 * "<same directory>/all_albedo_values.csv" with columns:
 *   scenario,aerosol_name,albedo
 *
 * Parsing rules:
 * - aerosol_name: part before "_profile_"
 * - scenario:     part after  "_profile_"
 *
 * Returns albedo on success.
 * Returns NaN if albedo couldn't be computed (file missing or no valid rows).
 * If albedo is NaN, the CSV is NOT updated.
 */
double compute_albedo_and_upsert(const char *profile_csv_path) {
    // 1) Compute albedo
    double A = albedo_from_profile_csv(profile_csv_path);
    if (!(A >= 0.0) || !(A < 1.0)) {
        // invalid; do not touch output table
        return NAN;
    }

    // 2) Parse aerosol & scenario from filename
    const char *base = path_basename(profile_csv_path);
    char aerosol[512], scenario[512];
    parse_aerosol_and_scenario_from_name(base, aerosol, sizeof(aerosol),
                                         scenario, sizeof(scenario));

    // 3) Upsert into all_albedo_values.csv in the same directory
    if (upsert_albedo_value(profile_csv_path, scenario, aerosol, A) != 0) {
        // could not update table; still return albedo
        // (you can decide to return NAN if you prefer)
    }

    return A;
}
