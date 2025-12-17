
// File: src/c/re_run_for_N.c// File: src/c/re_run
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Prototype for your function defined in re_fn.c
double compute_effective_radius_two_csv(const char *csv_file_1, const char *csv_file_2);

int main(void)
{
    // Relative paths from repo root
    const char *csv_file_1 = "data/Sea_salt_coarse_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv";
    const char *csv_file_2 = "data/profile_sulfate_seasalt_N_for_3.0e+02_cm3.csv";
    const char *output_csv = "data/re_values.csv";

    // Compute effective radius
    double re_m = compute_effective_radius_two_csv(csv_file_1, csv_file_2);

    // Open output CSV for writing
    FILE *out = fopen(output_csv, "w");
    if (!out) {
        fprintf(stderr, "ERROR: Could not open output file '%s' for writing.\n", output_csv);
        return EXIT_FAILURE;
    }

    // Write header
    fprintf(out, "effective_radius_m\n");

    // Write value (NaN case still writes a file)
    if (isnan(re_m)) {
        fprintf(out, "NaN\n");
        fclose(out);
        fprintf(stderr, "WARNING: Effective radius is NaN (check input files or data).\n");
        return EXIT_SUCCESS;  // file written successfully with NaN
    }

    // Normal case
    fprintf(out, "%.15g\n", re_m);
    fclose(out);

    printf("Effective radius written to '%s': %.15g m\n", output_csv, re_m);
    return EXIT_SUCCESS;
