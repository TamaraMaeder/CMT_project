#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define G 0.85
#define PI 3.141592653589793

// Fixed cloud thickness in meters
#define H_M 250.0

int main() {
    FILE *infile, *outfile;
    char line[256];

    // Open the input file generated with PyRCEL
    infile = fopen("aerosol_summary.csv", "r");
    if (!infile) {
        printf("Error: unable to open aerosol_summary.csv\n");
        return 1;
    }

    // Open the output file
    outfile = fopen("results_albedo.csv", "w");
    if (!outfile) {
        printf("Error: unable to open results_albedo.csv\n");
        fclose(infile);
        return 1;
    }

    // Write header for the output file
    fprintf(outfile, "Aerosol,tau_c,R\n");

    // Read and ignore the header line of the input file
    fgets(line, sizeof(line), infile);

    char aerosol[64];
    double CDNC_cm3, re_um;

    // Read input file line by line
    while (fgets(line, sizeof(line), infile)) {

        // Expected format: Aerosol,CDNC,Mean_Radius_micron
        if (sscanf(line, "%63[^,],%lf,%lf", aerosol, &CDNC_cm3, &re_um) != 3) {
            // Skip malformed or incomplete lines
            continue;
        }

        // Unit conversions
        double re_m = re_um * 1e-6;    // microns → meters
        double N_m3 = CDNC_cm3 * 1e6;  // cm^-3 → m^-3

        // Optical thickness tau_c = 2π r_e N h
        double tau_c = 2.0 * PI * re_m * N_m3 * H_M;

        // Cloud albedo approximation
        double numerator = (1.0 - G) * tau_c;
        double denominator = numerator + 2.0;
        double R = numerator / denominator;

        // Write computed values into the output CSV
        fprintf(outfile, "%s,%.6e,%.6f\n", aerosol, tau_c, R);
    }

    // Close files
    fclose(infile);
    fclose(outfile);

    printf("results_albedo.csv created.\n");
    return 0;
}
