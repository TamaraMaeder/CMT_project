
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256

double compute_albedo(double tau) {
    double g = 0.85;
    double a = 2.0;
    return tau / (a / (1 - g) + tau);
}

int main() {
    FILE *input = fopen("aerosol_modes.csv", "r");
    FILE *output = fopen("aerosol_modes_with_albedo.csv", "w");

    if (!input || !output) {
        perror("Error opening file");
        return 1;
    }

    char line[MAX_LINE];
    int is_header = 1;

    while (fgets(line, sizeof(line), input)) {
        if (is_header) {
            // Add new column name
            fprintf(output, "%s,albedo\n", strtok(line, "\n"));
            is_header = 0;
            continue;
        }

        char *token;
        char *columns[7];
        int col = 0;

        token = strtok(line, ",");
        while (token && col < 6) {
            columns[col++] = token;
            token = strtok(NULL, ",");
        }

        double tau = atof(columns[5]);
        double albedo = compute_albedo(tau);

        // Write original columns + albedo
        fprintf(output, "%s,%s,%s,%s,%s,%s,%.6f\n",
                columns[0], columns[1], columns[2], columns[3], columns[4], columns[5], albedo);
    }

    fclose(input);
    fclose(output);
    printf("Done! Check aerosol_modes_with_albedo.csv\n");
    return 0;
}
