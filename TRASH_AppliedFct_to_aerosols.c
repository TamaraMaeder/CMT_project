
// main.c
#include <stdio.h>
#include <math.h>

// Declaration of the function implemented in albedo_fn.c
double albedo_from_profile_csv(const char *csv_file_name);

int main(void) {
    const char *files[] = {
        "Sea_salt_coarse_profile.csv",
        "Sulfate_accum_profile.csv"
    };
    const int nfiles = (int)(sizeof(files) / sizeof(files[0]));

    FILE *out = fopen("albedo_values.csv", "w");
    if (!out) {
        fprintf(stderr, "Error: cannot open output file albedo_values.csv\n");
        return 1;
    }

    // Header
    fprintf(out, "file,albedo\n");

    for (int i = 0; i < nfiles; ++i) {
        const char *fn = files[i];
        double A = albedo_from_profile_csv(fn);

        if (isnan(A)) {
            fprintf(stderr, "Warning: could not compute albedo for %s (NaN)\n", fn);
            fprintf(out, "%s,NaN\n", fn);
        } else {
            // Write with good precision
            fprintf(out, "%s,%.15g\n", fn, A);
        }
    }

    fclose(out);

    printf("Wrote albedo values to albedo_values.csv\n");
    return 0;
}
