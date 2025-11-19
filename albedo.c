#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.141592653589793

double compute_LWP(double r_m, double N, double h) {
    double rho = 1000.0;  // water density (kg/m3)
    return (3.0/4.0) * PI * pow(r_m, 3.0) * rho * N * h;
}
// voir si cest le bon r car jsp si r moyen est correct pour mettre dans les formules

double effective_radius(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error with bins file opening");
        exit(1);
    }
    char line[256];
    double r_um, n;
    double sum_r3n = 0.0;
    double sum_r2n = 0.0;
    fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%*[^,],%lf,%lf", &r_um, &n) == 2) {
            double r_m = r_um * 1e-6;  // µm to m
            sum_r3n += n * pow(r_m, 3.0);
            sum_r2n += n * pow(r_m, 2.0);
        }
    }
    fclose(f);
    if (sum_r2n == 0.0) {
        fprintf(stderr, "Erreur: somme r^2*n = 0\n");
        exit(1);
    }
    return sum_r3n / sum_r2n;  // r_eff in m
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
    FILE *fin = fopen("aerosol_summary.csv", "r");
    FILE *fout = fopen("optical_properties.csv", "w");
    if (!fin || !fout) {
        printf("Error: cannot open the file.\n");
        return 1;
    }
    char line[256];
    fgets(line, sizeof(line), fin);
    fprintf(fout, "Aerosol,LWP,re,tau_c,Albedo\n");
    while (fgets(line, sizeof(line), fin)) {
        char aerosol[64];
        double N, r_micron;
        if (sscanf(line, "%63[^,],%lf,%lf", aerosol, &N, &r_micron) != 3)
            continue;
        // Conversion radius microns to meter
        double r_m = r_micron * 1e-6;
        // Assumption h = 250 m 
        double h = 250.0;
        double LWP = compute_LWP(r_m, N, h);
        double re = effective_radius("bins.csv");
        double tau_c = compute_tau(LWP, re);
        double albedo = compute_albedo(tau_c);
        fprintf(fout, "%s,%e,%e,%e,%e\n",
                aerosol, LWP, re, tau_c, albedo);
    }
    fclose(fin);
    fclose(fout);
    printf("File optical_properties.csv created.\n");
    return 0;
}


// rechercher la formule exacte du rayon effectif
// rechercher le h (j'ai assumé à 250 m mais jsp)
// vérifier si cest les bonne formules de manière générale (suivant les sources ca diffère)
