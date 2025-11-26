#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.141592653589793
#define MAX_SPECIES 50
#define MAX_LINE 256

typedef struct {
    char species[64];
    double sum_r3n;    // pour r_v
    double sum_n;
    double sum_r3n_eff; // pour r_eff
    double sum_r2n_eff;
    double prev_r;
    int first_line;
} Aerosol;

// fonction pour retrouver l'espèce dans le tableau ou l'ajouter si nouvelle
int find_or_add_species(Aerosol *arr, int *n_species, const char *name) {
    for (int i = 0; i < *n_species; i++) {
        if (strcmp(arr[i].species, name) == 0)
            return i;
    }
    // nouvelle espèce
    strcpy(arr[*n_species].species, name);
    arr[*n_species].sum_r3n = 0.0;
    arr[*n_species].sum_n = 0.0;
    arr[*n_species].sum_r3n_eff = 0.0;
    arr[*n_species].sum_r2n_eff = 0.0;
    arr[*n_species].prev_r = 0.0;
    arr[*n_species].first_line = 1;
    return (*n_species)++;
}

// fonction pour lire bins.csv et calculer r_v et r_eff pour chaque espèce
int compute_rv_re(const char *filename, Aerosol *aerosols) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error opening bins file");
        exit(1);
    }
    char line[MAX_LINE];
    fgets(line, sizeof(line), f); // skip header
    int n_species = 0;

    while (fgets(line, sizeof(line), f)) {
        char species[64];
        double r_um, n;
        if (sscanf(line, "%63[^,],%lf,%lf", species, &r_um, &n) != 3)
            continue;

        int idx = find_or_add_species(aerosols, &n_species, species);
        double r_m = r_um * 1e-6; // um -> m
        double r2 = r_m * r_m;
        double r3 = r2 * r_m;

        // r_v
        aerosols[idx].sum_r3n += n * r3;
        aerosols[idx].sum_n += n;

        // r_eff
        if (!aerosols[idx].first_line) {
            double r_prev = aerosols[idx].prev_r;
            double r_prev2 = r_prev * r_prev;
            double r_prev3 = r_prev2 * r_prev;
            aerosols[idx].sum_r3n_eff += n * r_prev3;
            aerosols[idx].sum_r2n_eff += n * r_prev2;
        } else {
            aerosols[idx].first_line = 0;
        }
        aerosols[idx].prev_r = r_m;
    }

    fclose(f);
    return n_species;
}

// fonctions optiques existantes
double compute_LWP(double r_v, double N, double h) {
    double rho = 1000000.0; // g/m^3
    return (4.0/3.0) * PI * r_v*r_v*r_v * rho * N * h;
}

double compute_tau(double LWP, double re) {
    double Qext = 2.0;
    return (3.0/4.0) * Qext * (LWP / re);
}

double compute_albedo(double tau) {
    double g = 0.85;
    double a = 2.0;
    return tau / (a/(1 - g) + tau);
}

int main() {
    Aerosol aerosols[MAX_SPECIES];
    int n_species = compute_rv_re("bins.csv", aerosols);

    FILE *fin = fopen("aerosol_summary.csv", "r");
    FILE *fout = fopen("optical_properties_bis.csv", "w");
    if (!fin || !fout) {
        printf("Error opening input/output files.\n");
        return 1;
    }

    fprintf(fout, "Aerosol,LWP,r_v,re,tau_c,Albedo\n");
    char line[MAX_LINE];
    fgets(line, sizeof(line), fin); // skip header

    while (fgets(line, sizeof(line), fin)) {
        char aerosol[64];
        double N_cm3, dummy_r;
        if (sscanf(line, "%63[^,],%lf,%lf", aerosol, &N_cm3, &dummy_r) != 3)
            continue;

        // chercher l'espèce dans le tableau d'aérosols
        int idx = -1;
        for (int i = 0; i < n_species; i++) {
            if (strcmp(aerosols[i].species, aerosol) == 0) {
                idx = i;
                break;
            }
        }
        if (idx == -1) continue; // espèce non trouvée

        double rv = pow(aerosols[idx].sum_r3n / aerosols[idx].sum_n, 1.0/3.0);
        double re = aerosols[idx].sum_r2n_eff > 0 ? aerosols[idx].sum_r3n_eff / aerosols[idx].sum_r2n_eff : 0.0;
        double N = N_cm3 * 1e6; // cm^-3 -> m^-3
        double h = 250.0;      // hauteur (m)
        double LWP = compute_LWP(rv, N, h);
        double tau_c = compute_tau(LWP, re);
        double albedo = compute_albedo(tau_c);

        fprintf(fout, "%s,%e,%e,%e,%e,%e\n",
                aerosol, LWP, rv, re, tau_c, albedo);
    }

    fclose(fin);
    fclose(fout);
    printf("File optical_properties_bis.csv created with r_v and r_eff for each aerosol.\n");
    return 0;
}
