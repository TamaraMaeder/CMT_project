#include <stdio.h>
#include <stdlib.h>

/* Aerosol origin: natural or anthropogenic */
typedef enum {
    AEROSOL_NATURAL = 0,
    AEROSOL_ANTHROPOGENIC = 1
} AerosolOrigin;

/* Base structure for a lognormal aerosol mode */
typedef struct {
    const char *name;       // Name of the mode (e.g., "Sulfate_accum")
    AerosolOrigin origin;   // Natural / Anthropogenic
    double N0_cm3;          // Total number concentration [#/cm^3]
    double Dg_um;           // Geometric mean dry diameter [µm]
    double sigma_g;         // Geometric standard deviation
    double kappa;           // Hygroscopicity parameter κ
    double nb_bins;            // number of bins for the lognormal distribution
} AerosolMode;

/* Convert enum -> string for writing in CSV */
const char *origin_to_string(AerosolOrigin origin) {
    switch (origin) {
        case AEROSOL_NATURAL:
            return "natural";
        case AEROSOL_ANTHROPOGENIC:
            return "anthropogenic";
        default:
            return "unknown";
    }
}

int main(void) {
    /* ------------------------------------------------------------------
       1. Definition of aerosol modes
          (EXAMPLE values; adjust according to literature/data needs)
       ------------------------------------------------------------------ */

    AerosolMode modes[] = {
        /*  name               origin               N0     Dg      sigma_g  kappa  nb_bins*/
        { "Sea_salt_coarse",    AEROSOL_NATURAL,        50.0,  1.7, 2.0,  1.2 , 40 },
        { "Biogenic_OC",        AEROSOL_NATURAL,       300.0,  0.15, 1.6,  0.15, 100 },
        { "Sulfate_accum",      AEROSOL_ANTHROPOGENIC, 800.0,  0.03, 1.7,  0.54, 200 },
        { "Anthropogenic_OC",   AEROSOL_ANTHROPOGENIC, 400.0,  0.12, 1.6,  0.20, 100},
        { "Black_carbon_fine",  AEROSOL_ANTHROPOGENIC, 200.0,  0.05, 1.5,  0.01, 100 }
    };

    size_t n_modes = sizeof(modes) / sizeof(modes[0]);

    /* ------------------------------------------------------------------
       2. Open the CSV file for writing
       ------------------------------------------------------------------ */

    const char *filename = "aerosol_modes.csv";
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        perror("Error while opening the CSV file");
        return EXIT_FAILURE;
    }

    /* ------------------------------------------------------------------
       3. Write the CSV header
       ------------------------------------------------------------------ */

    fprintf(f, "name,origin,N0_cm3,Dg_um,sigma_g,kappa,nb_bins\n");

    /* ------------------------------------------------------------------
       4. Write all aerosol modes (one per line)
       ------------------------------------------------------------------ */

    for (size_t i = 0; i < n_modes; ++i) {
        AerosolMode *m = &modes[i];
        fprintf(f,
                "%s,%s,%.6g,%.6g,%.6g,%.6g,%.6g\n",
                m->name,
                origin_to_string(m->origin),
                m->N0_cm3,
                m->Dg_um,
                m->sigma_g,
                m->kappa,
                m->nb_bins);
    }

    /* ------------------------------------------------------------------
       5. Close the file
       ------------------------------------------------------------------ */

    if (fclose(f) != 0) {
        perror("Error while closing the CSV file");
        return EXIT_FAILURE;
    }

    printf("File '%s' successfully written (%zu modes).\n", filename, n_modes);

    return EXIT_SUCCESS;
}
