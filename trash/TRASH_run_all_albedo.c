
// run_all_albedo.c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Declare the function implemented in albedo_fn.c
// Ensure this matches the signature in albedo_fn.c
double albedo_from_two_aerosol_csvs(const char *csv_aerosol_one,
                                    const char *csv_aerosol_two);

int main(void)
{
    // Paths relative to the project root (adjust if needed)
    const char *csv_one = "data/Anthropogenic_OC_profile_for_Biogenic_OC_and_Anthropogenic_OC_simulation.csv";
    const char *csv_two = "data/Biogenic_OC_profile_for_Biogenic_OC_and_Anthropogenic_OC_simulation.csv";
    const char *out_csv = "data/albedo_values.csv";

    // Compute albedo
    double A = albedo_from_two_aerosol_csvs(csv_one, csv_two);

    // Check for failure (NaN indicates error in file open or no valid rows)
    if (isnan(A)) {
        fprintf(stderr,
                "Error: Failed to compute albedo. "
                "Possible causes: missing files, unreadable CSV, or no valid data rows.\n"
                "Checked files:\n  - %s\n  - %s\n",
                csv_one, csv_two);
        return EXIT_FAILURE;
    }

    // Write result to CSV
    FILE *fp = fopen(out_csv, "w");
    if (!fp) {
        fprintf(stderr, "Error: Could not open output file for writing: %s\n", out_csv);
        return EXIT_FAILURE;
    }

    // Header and single-row output
    // You can add timestamp or additional metadata if needed
    fprintf(fp, "aerosol_one,aerosol_two,albedo\n");
    fprintf(fp, "\"%s\",\"%s\",%.10f\n", csv_one, csv_two, A);

    fclose(fp);

    printf("Albedo computation succeeded.\n");
    printf("Result written to: %s\n", out_csv);
    printf("Albedo value: %.10f\n", A);

    return EXIT_SUCCESS;
}
