#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define G 0.85
#define PI 3.141592653589793

int main() {
    FILE *infile, *outfile;
    char line[256];

    // open the file generated with pyrcel on Py
    infile = fopen("aerosols_pyrcel.csv", "r");
    if (!infile) {
        printf("Error : not possible to open it aerosols_pyrcel.csv\n");
        return 1;
    }

    outfile = fopen("results_albedo.csv", "w");
    if (!outfile) {
        printf("Error : not possible to open results_albedo.csv\n");
        fclose(infile);
        return 1;
    }

    // Header output file 
    fprintf(outfile, "aerosol,tau_c,R\n");

    // Read and ignore the header line
    fgets(line, sizeof(line), infile);

    char aerosol[64];
    double re_um, N_cm3, h_m; // ATTENTION LES UTNITES !!!!

    // Lecture line by line 
    while (fgets(line, sizeof(line), infile)) {

        // name, re_um, N_cm3, h_m
        if (sscanf(line, "%63[^,],%lf,%lf,%lf", aerosol, &re_um, &N_cm3, &h_m) != 4) {
            continue; // ignore incorrect lines
        }

        // units convertion
        double re_m = re_um * 1e-6;     // µm → m
        double N_m3 = N_cm3 * 1e6;      // cm^-3 → m^-3

        // tau_c
        double tau_c = 2.0 * PI * re_m * N_m3 * h_m;

        // albedo 
        double numerator = (1.0 - G) * tau_c;
        double denominator = numerator + 2.0;
        double R = numerator / denominator;

        // output CSV file
        fprintf(outfile, "%s,%.6e,%.6f\n", aerosol, tau_c, R);
    }

    fclose(infile);
    fclose(outfile);

    printf("results_albedo.csv created.\n");
    return 0;
}
