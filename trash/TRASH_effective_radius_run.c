
// effective_radius_run.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>


// Or declare the prototype if the function is compiled separately:
double effective_radius_from_two_aerosol_csvs(const char *csv_aerosol_one,
                                              const char *csv_aerosol_two);

/* Return 1 if file exists and is non-empty, 0 if not, -1 on error */
static int file_nonempty(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;            // doesn't exist
    return (st.st_size > 0) ? 1 : 0;               // non-empty?
}

/* Append (or create) the CSV with header if needed */
static int append_result_csv(const char *out_csv_path,
                             const char *scenario_label,
                             double re_m)
{
    int has_content = file_nonempty(out_csv_path);

    FILE *fp = fopen(out_csv_path, "a");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open output CSV for writing: %s\n", out_csv_path);
        return -1;
    }

    // Write header if file is new/empty
    if (!has_content) {
        fprintf(fp, "scenario,effective_radius_m,effective_radius_um,timestamp\n");
    }

    // Convert to micrometers
    double re_um = re_m * 1.0e6;

    // Timestamp in UTC (ISO-like)
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    char ts_buf[64];
    if (utc) {
        snprintf(ts_buf, sizeof(ts_buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday,
                 utc->tm_hour, utc->tm_min, utc->tm_sec);
    } else {
        snprintf(ts_buf, sizeof(ts_buf), "NA");
    }

    // Write one row
    // Note: scenario label is printed as-is; if it may contain commas, you can wrap in quotes.
    fprintf(fp, "%s,%.15e,%.6f,%s\n", scenario_label, re_m, re_um, ts_buf);

    fclose(fp);
    return 0;
}

int main(void)
{
    // --- Input CSV paths (exactly as you specified) ---
    const char *csv1 = "data/Sea_salt_coarse_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv";
    const char *csv2 = "data/profile_sulfate_seasalt_N_for_3.0e+02_cm3.csv";

    // --- Scenario label for the output row ---
    const char *scenario_label = "for N 3.0e+02 cm3";

    // --- Output CSV path (in data/ folder) ---
    const char *out_csv = "data/effective_radius_results.csv";

    // Compute effective radius (meters)
    double re_m = effective_radius_from_two_aerosol_csvs(csv1, csv2);

    if (isnan(re_m)) {
        fprintf(stderr, "ERROR: Failed to compute effective radius (NaN returned).\n");
        fprintf(stderr, "Check that input CSVs exist and contain valid rows.\n");
        return 1;
    }

    // Append result to output CSV (creates it with header if needed)
    if (append_result_csv(out_csv, scenario_label, re_m) != 0) {
        fprintf(stderr, "ERROR: Writing output CSV failed.\n");
        return 1;
    }

    printf("OK: Effective radius written to %s\n", out_csv);
    printf("Scenario: %s\n", scenario_label);
    printf("Effective radius: %.6e m (%.3f µm)\n", re_m, re_m * 1e6);

    return 0;
