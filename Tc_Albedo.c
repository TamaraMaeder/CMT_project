
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ALPHA 0.19
#define A 2.0
#define G 0.85

double compute_Tc(double LWP, double N) {
    return ALPHA * pow(LWP, 5.0/6.0) * pow(N, 1.0/3.0);
}

double compute_albedo(double Tc) {
    return Tc / ((A / (1.0 - G)) + Tc);
}

int main() {
    FILE *fp = fopen("aerosol_summary.csv", "r");
    if (!fp) {
        perror("Error opening input file");
        return 1;
    }

    // Read all lines into memory first
    char lines[100][1024];
    int count = 0;
    while (fgets(lines[count], sizeof(lines[count]), fp)) {
        count++;
    }
    fclose(fp);

    fp = fopen("aerosol_summary.csv", "w");
    if (!fp) {
        perror("Error overwriting file");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        char *aerosol, *cdnc_str, *radius_str;
        aerosol = strtok(lines[i], ",");
        cdnc_str = strtok(NULL, ",");
        radius_str = strtok(NULL, ",");

        if (i == 0) {
            fprintf(fp, "%s,%s,%s,Tc,Albedo\n", aerosol, cdnc_str, radius_str);
            continue;
        }

        double cdnc = atof(cdnc_str);
        double LWP = 0.0;

        // a changer quand les LWP seront dans le csv----
        if (strcmp(aerosol, "sulfate") == 0) {
            LWP = 2.254828;
        } else if (strcmp(aerosol, "sea salt") == 0) {
            LWP = 7.95587;
        } else {
            LWP = 1.0; // default if unknown
        }
        // ----

        double Tc = compute_Tc(LWP, cdnc);
        double albedo = compute_albedo(Tc);

        fprintf(fp, "%s,%.6f,%.6f,%.6f,%.6f\n", aerosol, cdnc, atof(radius_str), Tc, albedo);
    }

    fclose(fp);
    printf("File updated successfully: aerosol_summary.csv\n");
    return 0;
}

