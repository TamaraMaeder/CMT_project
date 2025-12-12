
// albedo_fn.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DEFAULT_ALBEDO_OUT_CSV "all_albedo_values.csv"

/*
 * Compute albedo from a CSV with columns:
 *   height_m,bin_index,r_wet_m,number_concentration_m3
 *
 * Formula:
 *   tau = 2*pi * sum(r^2 * N)           // r in meters, N in m^-3
 *   A   = tau / (a/(1-g) + tau), with g=0.85, a=2.0
 *
 * Returns:
 *   - albedo of the cloud in [0,1) on success
 *   - NaN if the file cannot be opened or no valid rows are found
 */
static double albedo_from_profile_csv(const char *csv_file_name) {
    const double g = 0.85;
    const double a = 2.0;
    const double TWO_PI = 2.0 * 3.14159265358979323846;

    FILE *fp = fopen(csv_file_name, "r");
    if (!fp) return NAN;

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

        // Expect 4 comma-separated fields:
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

        char *tok_r_m = c2 + 1;           // r_wet_m (meters)
        char *tok_N_m3 = c3 + 1;          // number_concentration_m3 (m^-3)

        // Trim leading spaces on r and N
        while (*tok_r_m == ' ' || *tok_r_m == '\t') ++tok_r_m;
        while (*tok_N_m3 == ' ' || *tok_N_m3 == '\t') ++tok_N_m3;

        // Parse r (meters)
        char *end_r = NULL;
        double r_m = strtod(tok_r_m, &end_r);
        if (end_r == tok_r_m) continue; // header or malformed

        // Parse N (m^-3)
        char *end_N = NULL;
        double N_m3 = strtod(tok_N_m3, &end_N);
        if (end_N == tok_N_m3) continue; // header or malformed

        // Ignore negative or NaN values
        if (!(r_m >= 0.0) || !(N_m3 >= 0.0)) continue;

        // Accumulate r^2 * N
        sum_r2N += r_m * r_m * N_m3;
        ++valid_rows;
    }

    fclose(fp);

    if (valid_rows == 0) return NAN;

    double tau = TWO_PI * sum_r2N;
    double denom = a / (1.0 - g) + tau;
    if (denom <= 0.0) return NAN;

    double A = tau / denom;

    // Clamp to [0,1)
    if (A < 0.0) A = 0.0;
    if (A >= 1.0) A = nextafter(1.0, 0.0);

    return A;
}

/* ------------------------ Helpers for parsing & CSV upsert ------------------------ */

static void trim_inplace(char *s) {
    if (!s) return;
    // Trim leading
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    if (p != s) memmove(s, p, strlen(p) + 1);

    // Trim trailing
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) {
        s[n-1] = '\0';
        --n;
    }
}

static void basename_no_ext(const char *path, char *out, size_t outsz) {
    // Get base name (strip directory separators)
    const char *base = path;
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *bslash = strrchr(path, '\\');
    if (!slash || (bslash && bslash > slash)) slash = bslash;
#endif
    if (slash) base = slash + 1;

    // Copy into out
    strncpy(out, base, outsz - 1);
    out[outsz - 1] = '\0';

    // Strip extension if any (last dot after last slash)
    char *dot = strrchr(out, '.');
    if (dot && dot != out) *dot = '\0';
}

/*
 * From a file name like:
 *   "Sea_salt_coarse_profile_for_Biogenic_OC_and_Sea_salt_coarse_simulation.csv"
 * produce:
 *   aerosol_name = "Sea_salt_coarse"
 *   scenario     = "for_Biogenic_OC_and_Sea_salt_coarse_simulation"
 *
 * If "_profile_" is not found:
 *   aerosol_name = "" (empty), scenario = whole base name.
 */
static void parse_aerosol_and_scenario_from_filename(const char *csv_file_name,
                                                     char *aerosol_name, size_t a_sz,
                                                     char *scenario, size_t s_sz) {
    char base[1024];
    basename_no_ext(csv_file_name, base, sizeof(base));

    const char *key = "_profile_";
    char *pos = strstr(base, key);

    if (pos) {
        // aerosol is everything before "_profile_"
        size_t len_a = (size_t)(pos - base);
        if (len_a >= a_sz) len_a = a_sz - 1;
        memcpy(aerosol_name, base, len_a);
        aerosol_name[len_a] = '\0';

        // scenario is everything after "profile_"
        const char *after_profile = pos + strlen("_profile_");
        strncpy(scenario, after_profile, s_sz - 1);
        scenario[s_sz - 1] = '\0';
    } else {
        aerosol_name[0] = '\0';
        strncpy(scenario, base, s_sz - 1);
        scenario[s_sz - 1] = '\0';
    }

    trim_inplace(aerosol_name);
    trim_inplace(scenario);
}

static int file_exists(const char *path) {
    FILE *fp = fopen(path, "r");
    if (fp) { fclose(fp); return 1; }
    return 0;
}

/*
 * Upsert a row into CSV:
 *   columns: scenario,aerosol_name,albedo
 *
 * Behavior:
 *  - If file doesn't exist, create with header and the row.
 *  - If row with same (scenario,aerosol_name) exists, update albedo.
 *  - Else append new row.
 *
 * Returns 0 on success; nonzero on error.
 */
static int upsert_albedo_row(const char *out_csv_path,
                             const char *scenario,
                             const char *aerosol_name,
                             double albedo) {
    const char *HEADER = "scenario,aerosol_name,albedo\n";

    // If no file yet -> create and write header + row
    if (!file_exists(out_csv_path)) {
        FILE *wf = fopen(out_csv_path, "w");
        if (!wf) return -1;
        fputs(HEADER, wf);
        fprintf(wf, "%s,%s,%.15g\n", scenario, aerosol_name, albedo);
        fclose(wf);
        return 0;
    }

    // Read existing file
    FILE *rf = fopen(out_csv_path, "r");
    if (!rf) return -2;

    typedef struct {
        char *scenario;
        char *aerosol;
        double albedo;
    } Row;

    Row *rows = NULL;
    size_t nrows = 0, cap = 0;
    char line[2048];
    int has_header = 0;

    while (fgets(line, sizeof(line), rf)) {
        char buf[2048];
        strncpy(buf, line, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        trim_inplace(buf);
        if (buf[0] == '\0') continue;

        if (!has_header && strncmp(buf, "scenario,", 9) == 0) {
            has_header = 1;
            continue; // skip header
        }

        // Parse CSV fields: scenario,aerosol_name,albedo
        char *p = buf;
        char *c1 = strchr(p, ',');
        if (!c1) continue;
        *c1 = '\0';
        char *c2 = strchr(c1 + 1, ',');
        if (!c2) continue;
        *c2 = '\0';

        char *tok_scn = p;
        char *tok_aer = c1 + 1;
        char *tok_alb = c2 + 1;

        trim_inplace(tok_scn);
        trim_inplace(tok_aer);
        trim_inplace(tok_alb);

        char *endptr = NULL;
        double alb_val = strtod(tok_alb, &endptr);
        if (endptr == tok_alb) continue; // malformed numeric

        // Grow array
        if (nrows == cap) {
            size_t newcap = cap ? cap * 2 : 16;
            Row *tmp = (Row *)realloc(rows, newcap * sizeof(Row));
            if (!tmp) { fclose(rf); free(rows); return -3; }
            rows = tmp; cap = newcap;
        }

        rows[nrows].scenario = strdup(tok_scn);
        rows[nrows].aerosol  = strdup(tok_aer);
        rows[nrows].albedo   = alb_val;
        if (!rows[nrows].scenario || !rows[nrows].aerosol) {
            fclose(rf);
            for (size_t i = 0; i < nrows; ++i) {
                free(rows[i].scenario); free(rows[i].aerosol);
            }
            free(rows);
            return -4;
        }
        ++nrows;
    }
    fclose(rf);

    // Upsert logic
    size_t idx = (size_t)-1;
    for (size_t i = 0; i < nrows; ++i) {
        if (strcmp(rows[i].scenario, scenario) == 0 &&
            strcmp(rows[i].aerosol,  aerosol_name) == 0) {
            idx = i; break;
        }
    }

    if (idx == (size_t)-1) {
        // Append new
        if (nrows == cap) {
            size_t newcap = cap ? cap * 2 : 16;
            Row *tmp = (Row *)realloc(rows, newcap * sizeof(Row));
            if (!tmp) {
                for (size_t i = 0; i < nrows; ++i) { free(rows[i].scenario); free(rows[i].aerosol); }
                free(rows);
                return -5;
            }
            rows = tmp; cap = newcap;
        }
        rows[nrows].scenario = strdup(scenario);
        rows[nrows].aerosol  = strdup(aerosol_name);
        rows[nrows].albedo   = albedo;
        if (!rows[nrows].scenario || !rows[nrows].aerosol) {
            for (size_t i = 0; i < nrows; ++i) { free(rows[i].scenario); free(rows[i].aerosol); }
            free(rows);
            return -6;
        }
        ++nrows;
    } else {
        // Update existing
        rows[idx].albedo = albedo;
    }

    // Write back atomically: to temp file, then rename
    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", out_csv_path);

    FILE *wf = fopen(tmp_path, "w");
    if (!wf) {
        for (size_t i = 0; i < nrows; ++i) {
            free(rows[i].scenario); free(rows[i].aerosol);
        }
        free(rows);
        return -7;
    }

    fputs("scenario,aerosol_name,albedo\n", wf);
    for (size_t i = 0; i < nrows; ++i) {
        fprintf(wf, "%s,%s,%.15g\n", rows[i].scenario, rows[i].aerosol, rows[i].albedo);
    }
    fclose(wf);

    // Replace original
    remove(out_csv_path);
    if (rename(tmp_path, out_csv_path) != 0) {
        remove(tmp_path);
        for (size_t i = 0; i < nrows; ++i) {
            free(rows[i].scenario); free(rows[i].aerosol);
        }
        free(rows);
        return -8;
    }

    for (size_t i = 0; i < nrows; ++i) {
        free(rows[i].scenario); free(rows[i].aerosol);
    }
    free(rows);
    return 0;
}

/*
 * Public function:
 *  - Computes albedo using the CSV content
 *  - Extracts aerosol_name and scenario from the file name (based on "_profile_")
 *  - Upserts the result into DEFAULT_ALBEDO_OUT_CSV (same directory as executable)
 *
 * Returns:
 *  - The albedo computed (NaN on failure) — same as the pure compute function.
 */
double albedo_from_profile_csv_and_store(const char *csv_file_name) {
    double A = albedo_from_profile_csv(csv_file_name);
    if (!(A >= 0.0) && !(A < 1.0)) {
        // NaN or invalid -> do not write
        return A;
    }

    char aerosol[256];
    char scenario[512];
    parse_aerosol_and_scenario_from_filename(csv_file_name,
                                             aerosol, sizeof(aerosol),
                                             scenario, sizeof(scenario));

    // Upsert into output CSV next to executable
    (void) upsert_albedo_row(DEFAULT_ALBEDO_OUT_CSV, scenario, aerosol, A);
    return A;
}
