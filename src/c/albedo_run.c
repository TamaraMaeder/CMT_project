
/*
 * albedo_run.c
 *
 * Calls albedo_from_two_aerosol_csvs() for each scenario and writes the
 * resulting albedo values to data/albedo_values.csv with only two columns:
 *   scenario,albedo
 *
 * Build example (from project root):
 *   gcc -O2 -Wall -Wextra -o bin/albedo_run src/c/albedo_run.c src/c/albedo_fn.c -lm
 *
 * Run (from project root):
 *   ./bin/albedo_run
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

extern double albedo_from_two_aerosol_csvs(const char *csv_aerosol_one,
                                           const char *csv_aerosol_two);


#ifndef DATA_DIR
#define DATA_DIR "data"
#endif

typedef struct {
    const char *name;     // Scenario label to write to output CSV 
    const char *csv1;     // First aerosol CSV filename (in DATA_DIR) 
    const char *csv2;     // Second aerosol CSV filename (in DATA_DIR) 
} Scenario;

static int path_join(char *out, size_t out_sz, const char *dir, const char *file) {
    if (!out || !dir || !file) return -1;
    int n = snprintf(out, out_sz, "%s/%s", dir, file);
    return (n > 0 && (size_t)n < out_sz) ? 0 : -1;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int main(void) {
    // Define scenarios exactly as provided 
    const Scenario scenarios[] = {
        {
            "Scenario 1: Anthropogenic OC + Biogenic OC",
            "Anthropogenic_OC_profile_for_Biogenic_OC_and_Anthropogenic_OC_simulation.csv",
            "Biogenic_OC_profile_for_Biogenic_OC_and_Anthropogenic_OC_simulation.csv"
        },
        {
            "Scenario 2: Sea salt coarse + Biogenic OC",
            "Sea_salt_coarse_profile_for_Biogenic_OC_and_Sea_salt_coarse_simulation.csv",
            "Biogenic_OC_profile_for_Biogenic_OC_and_Sea_salt_coarse_simulation.csv"
        },
        {
            "Scenario 3: Black carbon fine + Sea salt coarse",
            "Black_carbon_fine_profile_for_Sea_salt_coarse_and_Black_carbon_fine_simulation.csv",
            "Sea_salt_coarse_profile_for_Sea_salt_coarse_and_Black_carbon_fine_simulation.csv"
        },
        {
            "Scenario 4: Sea salt coarse + Sulfate accum",
            "Sea_salt_coarse_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv",
            "Sulfate_accum_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv"
        }
    };
    const size_t N = sizeof(scenarios) / sizeof(scenarios[0]);

    // Prepare output CSV path 
    char out_path[512];
    if (path_join(out_path, sizeof(out_path), DATA_DIR, "albedo_values.csv") != 0) {
        fprintf(stderr, "Error: output path too long.\n");
        return EXIT_FAILURE;
    }

    // Open output CSV 
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Error: cannot open output file '%s': %s\n", out_path, strerror(errno));
        return EXIT_FAILURE;
    }

    // Write header: only two columns as requested 
    fprintf(out, "scenario,albedo\n");

    // Process each scenario 
    for (size_t i = 0; i < N; ++i) {
        char path1[512], path2[512];

        if (path_join(path1, sizeof(path1), DATA_DIR, scenarios[i].csv1) != 0 ||
            path_join(path2, sizeof(path2), DATA_DIR, scenarios[i].csv2) != 0) {
            fprintf(stderr, "Warning: path too long for %s. Skipping.\n", scenarios[i].name);
            continue;
        }

        // Pre-check for clearer messages 
        if (!file_exists(path1)) {
            fprintf(stderr, "Warning: missing input CSV '%s' for %s. Skipping.\n",
                    path1, scenarios[i].name);
            continue;
        }
        if (!file_exists(path2)) {
            fprintf(stderr, "Warning: missing input CSV '%s' for %s. Skipping.\n",
                    path2, scenarios[i].name);
            continue;
        }

        // Compute albedo 
        double albedo = albedo_from_two_aerosol_csvs(path1, path2);

        if (isnan(albedo)) {
            fprintf(stderr, "Warning: albedo computation returned NaN for %s.\n", scenarios[i].name);
        }

        // Write: scenario label and albedo 
        fprintf(out, "%s,%.10g\n", scenarios[i].name, albedo);
    }

    fclose(out);
    printf("Wrote albedo values to '%s'\n", out_path);
    return EXIT_SUCCESS;
}
