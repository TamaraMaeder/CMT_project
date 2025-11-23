#include <stdio.h>
#include <stdlib.h>

/* Origine des aérosols : naturel ou anthropique */
typedef enum {
    AEROSOL_NATURAL = 0,
    AEROSOL_ANTHROPOGENIC = 1
} AerosolOrigin;

/* Structure de base pour un mode lognormal d'aérosols */
typedef struct {
    const char *name;       // Nom du mode (ex: "Sulfate_accumulation")
    AerosolOrigin origin;   // Naturel / Anthropique
    double N0_cm3;          // Concentration totale (nombre) [#/cm^3]
    double Dg_um;           // Diamètre géométrique moyen sec [µm]
    double sigma_g;         // Ecart-type géométrique (sans dimension)
    double kappa;           // Paramètre d'hygroscopicité κ
} AerosolMode;

/* Conversion enum -> string pour écrire dans le CSV */
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
       1. Définition des différents modes d'aérosols
          (valeurs EXEMPLES, à ajuster selon ta littérature / besoins)
       ------------------------------------------------------------------ */

    AerosolMode modes[] = {
        /*  name               origin               N0     Dg      sigma_g  kappa */
        { "Sea_salt_coarse",   AEROSOL_NATURAL,     50.0,  0.80,   2.0,     1.2  },
        { "Biogenic_OC",       AEROSOL_NATURAL,    300.0,  0.15,   1.6,     0.15 },
        { "Sulfate_accum",     AEROSOL_ANTHROPOGENIC, 800.0, 0.10, 1.7,     0.60 },
        { "Anthropic_OC",      AEROSOL_ANTHROPOGENIC, 400.0, 0.12, 1.6,     0.20 },
        { "Black_carbon_fine", AEROSOL_ANTHROPOGENIC, 200.0, 0.05, 1.5,     0.01 }
    };

    size_t n_modes = sizeof(modes) / sizeof(modes[0]);

    /* ------------------------------------------------------------------
       2. Ouverture du fichier CSV en écriture
       ------------------------------------------------------------------ */

    const char *filename = "aerosol_modes.csv";
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        perror("Erreur lors de l'ouverture du fichier CSV");
        return EXIT_FAILURE;
    }

    /* ------------------------------------------------------------------
       3. Écriture de l'en-tête (header) du CSV
       ------------------------------------------------------------------ */

    fprintf(f, "name,origin,N0_cm3,Dg_um,sigma_g,kappa\n");

    /* ------------------------------------------------------------------
       4. Écriture des lignes (un mode par ligne)
       ------------------------------------------------------------------ */

    for (size_t i = 0; i < n_modes; ++i) {
        AerosolMode *m = &modes[i];
        fprintf(f,
                "%s,%s,%.6g,%.6g,%.6g,%.6g\n",
                m->name,
                origin_to_string(m->origin),
                m->N0_cm3,
                m->Dg_um,
                m->sigma_g,
                m->kappa);
    }

    /* ------------------------------------------------------------------
       5. Fermeture du fichier
       ------------------------------------------------------------------ */

    if (fclose(f) != 0) {
        perror("Erreur lors de la fermeture du fichier CSV");
        return EXIT_FAILURE;
    }

    printf("Fichier '%s' écrit avec succès (%zu modes).\n", filename, n_modes);

    return EXIT_SUCCESS;
}
