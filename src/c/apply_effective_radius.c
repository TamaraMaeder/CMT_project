
// apply_effective_radius.c
// Computes effective radius for multiple scenarios and writes results to:
//   data/effective_radius_mix_scenarios.csv
//
// Each scenario pairs a sulfate+seasalt N profile CSV with the common
// sea-salt coarse profile CSV.
//
// Output CSV columns: scenario,effective_radius_m

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Prototype for the function implemented in effective_radius.c */
double effective_radius_mix(const char *csv_path_aerosol1,
                            const char *csv_path_aerosol2,
                            double layer_thickness_m);

typedef struct {
    const char *label;   /* e.g., "3.0e+02_cm3" */
    const char *csvN;    /* e.g., "data/profile_sulfate_seasalt_N_for_3.0e+02_cm3.csv" */
    const char *csvR;    /* e.g., "data/Sea_salt_coarse_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv" */
} Scenario;

int main(void) {
    const double h = 250.0; /* layer thickness in meters */

    /* Common sea-salt coarse profile (same for all scenarios) */
    const char *sea_salt_coarse_profile = "data/Sea_salt_coarse_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv";

    /* Define scenarios */
    Scenario scenarios[] = {
        { "3.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_3.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "4.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_4.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "5.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_5.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "6.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_6.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "7.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_7.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "8.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_8.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "9.0e+02_cm3",  "data/profile_sulfate_seasalt_N_for_9.0e+02_cm3.csv",  sea_salt_coarse_profile },
        { "1.0e+03_cm3",  "data/profile_sulfate_seasalt_N_for_1.0e+03_cm3.csv",  sea_salt_coarse_profile },
    };
    const size_t n_scenarios = sizeof(scenarios) / sizeof(scenarios[0]);

    /* Open output CSV */
    const char *out_path = "data/effective_radius_mix_scenarios.csv";
    FILE *out = fopen(out_path, "w");
    if (!out) {
        perror("fopen (output)");
        return 2;
    }

    /* Header */
    fprintf(out, "scenario,effective_radius_m\n");

    /* Compute and write each scenario */
    for (size_t i = 0; i < n_scenarios; ++i) {
        const Scenario *sc = &scenarios[i];

        double re = effective_radius_mix(sc->csvN, sc->csvR, h);

        /* If computation failed, write nan but continue with other scenarios */
        if (!isfinite(re)) {
            fprintf(stderr,
                "Warning: scenario %s failed to compute (check input files and format).\n",
                sc->label);
            fprintf(out, "%s,nan\n", sc->label);
        } else {
            fprintf(out, "%s,%.15g\n", sc->label, re);
        }
    }

    fclose(out);
    printf("Saved scenario results to %s\n", out_path);
    return 0;
}
