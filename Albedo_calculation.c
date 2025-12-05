
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Compute albedo from a CSV file that contains the wet radius and the number concentration of each bin for an aerosol :
 *   Header: r[m],N[m^-3]
 *   Rows:   r,N  (r in meters, N in m^-3)
 *
 * tau  = 2*pi * sum(r^2 * N)
 * A    = tau / (a/(1-g) + tau), with g=0.85, a=2.0
 *
 * Returns:
 *   - albedo in [0,1) on success
 *   - NaN if the file cannot be opened or no valid rows are found
 */

double albedo(const char *csv_file_name) {
    const double g = 0.85;
    const double a = 2.0;
    const double TWO_PI = 2.0 * 3.14159265358979323846;

    FILE *fp = fopen(csv_file_name, "r");
    if (!fp) {
        return NAN;
    }

    char line[1024];
    double sum_r2N = 0.0;
    int valid_rows = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Skip the empty space
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
        if (*p == '\0') continue;  
        // Find comma separating r and N
        char *comma = strchr(p, ',');
        if (!comma) {
            continue;
        }
        // Split r & N tokens and trim spaces around N token
        *comma = '\0';
        char *tok_r = p;
        char *tok_N = comma + 1;
        // Trim spaces at start of tok_N
        while (*tok_N == ' ' || *tok_N == '\t') ++tok_N;
        // Convert r into a double
        char *end_r = NULL;
        double r = strtod(tok_r, &end_r);
        if (end_r == tok_r) {
            /* Could not parse r (e.g., header "r[m]"); skip */
            continue;
        }
        // Convert N into a double
        char *end_N = NULL;
        double N = strtod(tok_N, &end_N);
        if (end_N == tok_N) {
            continue;
        }
        // Ignore negative values of r and N
        if (r < 0.0 || N < 0.0) {
            continue;
        }
        sum_r2N += r * r * N;
        valid_rows++;
    }

    fclose(fp);

    if (valid_rows == 0) {
        return NAN;
    }

    double tau = TWO_PI * sum_r2N;
    double denom = a / (1.0 - g) + tau;
    if (denom <= 0.0) {
        return NAN;
    }

    double A = tau / denom;

    // Control that that the albedo is in [0,1]
    if (A < 0.0) A = 0.0;
    if (A >= 1.0) A = nextafter(1.0, 0.0);

    return A;
}
