#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_SPECIES 50
#define MAX_BINS    2000
#define MAX_NAME    64

typedef struct {
    char name[MAX_NAME];
    double radius[MAX_BINS];  // en mètres
    double number[MAX_BINS];  // en m^-3
    int nBins;
} SpeciesData;

typedef struct {
    double rv;  // rayon volumique moyen
    double re;  // rayon effectif
} AerosolResults;

// Fonction pour calculer rv et re
AerosolResults computeAerosolRadius(const double *radius,
                                    const double *number,
                                    int nBins,
                                    double k) {
    double sum_r3N = 0.0;
    double sum_N = 0.0;

    for (int i = 0; i < nBins; i++) {
        sum_r3N += number[i] * pow(radius[i], 3);
        sum_N += number[i];
    }

    AerosolResults res;
    if (sum_N > 0) {
        res.rv = pow(sum_r3N / sum_N, 1.0/3.0);
        res.re = (1.0 / k) * pow(res.rv, 3);
    } else {
        res.rv = 0.0;
        res.re = 0.0;
    }
    return res;
}

int main() {
    FILE *file = fopen("bins.csv", "r");
    if (!file) {
        printf("Erreur : impossible d'ouvrir bins.csv\n");
        return 1;
    }

    SpeciesData species[MAX_SPECIES];
    int speciesCount = 0;

    char sp[MAX_NAME];
    double r_um, conc_cm3;
    char buffer[256];

    // Lire et ignorer l'entête
    fgets(buffer, sizeof(buffer), file);

    // Lecture ligne par ligne
    while (fscanf(file, "%63[^,],%lf,%lf", sp, &r_um, &conc_cm3) == 3) {
        // Sauter le '\n' restant
        fgetc(file);

        int found = -1;
        for (int i = 0; i < speciesCount; i++) {
            if (strcmp(species[i].name, sp) == 0) {
                found = i;
                break;
            }
        }

        if (found == -1) {
            found = speciesCount;
            strcpy(species[speciesCount].name, sp);
            species[speciesCount].nBins = 0;
            speciesCount++;
        }

        int idx = species[found].nBins;
        species[found].radius[idx] = r_um * 1e-6;    // µm → m
        species[found].number[idx] = conc_cm3 * 1e6; // cm^-3 → m^-3
        species[found].nBins++;
    }

    fclose(file);

    double k = 0.8; // constante pour le rayon effectif

    printf("\n=== Résultats des rayons par espèce ===\n\n");

    for (int i = 0; i < speciesCount; i++) {
        AerosolResults res = computeAerosolRadius(
            species[i].radius,
            species[i].number,
            species[i].nBins,
            k
        );

        printf("Espèce : %s\n", species[i].name);
        printf("  - Nombre de bins : %d\n", species[i].nBins);
        printf("  - rv = %.6e m\n", res.rv);
        printf("  - re = %.6e m\n\n", res.re);
    }

    return 0;
}
