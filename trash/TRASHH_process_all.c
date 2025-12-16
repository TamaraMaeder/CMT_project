
/*
 * process_all.c
 *
 * Explicitly pairs CSVs by scenario and calls * Explicitly pairs CSVs by scenario and calls `albedo` for each pair.
 * Writes results to data/all_albedo_values.csv with columns: scenario,albedo
 *
 * Paired scenarios (from your list):
 *  - Biogenic_OC_and_Anthropogenic_OC
 *  - Biogenic_OC_and_Sea_salt_coarse
 *  - Sea_salt_coarse_and_Black_carbon_fine
 *  - Sulfate_accum_and_Sea_salt_coarse
 *
 * Compile (one-liner, your style):
 *   gcc ./src/c/process_all.c ./src/c/albedo_fn.c && ./a.out
 *
 * If your `albedo` has signature: int albedo(const char*, const char*, double*)
 *   gcc -DALBEDO_RETURNS_INT ./src/c/process_all.c ./src/c/albedo_fn.c && ./a.out
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* ====== Adjust this if needed: The albedo function signature ====== */
/* Option A (default): returns the albedo value directly */
#ifndef ALBEDO_RETURNS_INT
double albedo(const char* csv_path_a, const char* csv_path_b);
#else
/* Option B: returns 0 on success and writes to out_albedo */
int albedo(const char* csv_path_a, const char* csv_path_b, double* out_albedo);
#endif
/* ================================================================== */

/* Scenario specification: scenario name and the two CSV filenames */
typedef struct {
    const char* scenario;
    const char* fileA;  /* first CSV (order matters if albedo expects it) */
    const char* fileB;  /* second CSV */
} ScenarioSpec;

/* ---- Explicit list of scenarios and their CSVs (as provided) ---- */
static const ScenarioSpec SCENARIOS[] = {
    {
        "Biogenic_OC_and_Anthropogenic_OC",
        "Anthropogenic_OC_profile_for_Biogenic_OC_and_Anthropogenic_OC_simulation.csv",
        "Biogenic_OC_profile_for_Biogenic_OC_and_Anthropogenic_OC_simulation.csv"
    },
    {
        "Biogenic_OC_and_Sea_salt_coarse",
        "Biogenic_OC_profile_for_Biogenic_OC_and_Sea_salt_coarse_simulation.csv",
        "Sea_salt_coarse_profile_for_Biogenic_OC_and_Sea_salt_coarse_simulation.csv"
    },
    {
        "Sea_salt_coarse_and_Black_carbon_fine",
        "Black_carbon_fine_profile_for_Sea_salt_coarse_and_Black_carbon_fine_simulation.csv",
        "Sea_salt_coarse_profile_for_Sea_salt_coarse_and_Black_carbon_fine_simulation.csv"
    },
    {
        "Sulfate_accum_and_Sea_salt_coarse",
        "Sea_salt_coarse_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv",
        "Sulfate_accum_profile_for_Sulfate_accum_and_Sea_salt_coarse_simulation.csv"
    }
};
static const size_t N_SCENARIOS = sizeof(SCENARIOS)/sizeof(SCENARIOS[0]);

/* Try "./data" first (project root), then "/data" */
static const char* resolve_data_dir(void) {
    static char cached[PATH_MAX] = {0};
    if (cached[0] != '\0') return cached;

    DIR *d = opendir("./data");
    if (d) { closedir(d); strncpy(cached, "./data", sizeof(cached)-1); return cached; }
    d = opendir("/data");
    if (d) { closedir(d); strncpy(cached, "/data", sizeof(cached)-1); return cached; }

    /* Fallback to ./data */
    strncpy(cached, "./data", sizeof(cached)-1);
    return cached;
}

static int build_full_path(char *out, size_t outsz, const char *dir, const char *name) {
    int n = snprintf(out, outsz, "%s/%s", dir, name);
    return (n > 0 && (size_t)n < outsz) ? 0 : -1;
}

/* Quick existence check to warn if a file is missing */
static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int main(void) {
    const char *data_dir = resolve_data_dir();

    /* Prepare output path: data/all_albedo_values.csv */
    char outpath[PATH_MAX];
    if (build_full_path(outpath, sizeof(outpath), data_dir, "all_albedo_values.csv") != 0) {
        fprintf(stderr, "ERROR: output path too long.\n");
        return 1;
    }

    FILE *out = fopen(outpath, "w");
    if (!out) {
        fprintf(stderr, "ERROR: cannot open output file '%s' for writing: %s\n", outpath, strerror(errno));
        return 1;
    }
    /* Header */
    fprintf(out, "scenario,albedo\n");

    size_t processed = 0, skipped = 0;

    /* Loop over the explicit scenario list */
    for (size_t i = 0; i < N_SCENARIOS; ++i) {
        const ScenarioSpec *spec = &SCENARIOS[i];

        char pathA[PATH_MAX], pathB[PATH_MAX];
        if (build_full_path(pathA, sizeof(pathA), data_dir, spec->fileA) != 0 ||
            build_full_path(pathB, sizeof(pathB), data_dir, spec->fileB) != 0) {
            fprintf(stderr, "WARNING: path too long for scenario '%s'. Skipping.\n", spec->scenario);
            skipped++;
            continue;
        }

        /* Warn if files are missing, but continue to the next scenario */
        if (!file_exists(pathA)) {
            fprintf(stderr, "WARNING: missing CSV: %s\n", pathA);
            skipped++;
            continue;
        }
        if (!file_exists(pathB)) {
            fprintf(stderr, "WARNING: missing CSV: %s\n", pathB);
            skipped++;
            continue;
        }

        double value = 0.0;
#ifndef ALBEDO_RETURNS_INT
        value = albedo(pathA, pathB);
        /* If albedo can signal errors via NaN or special values, you can check here */
#else
        int rc = albedo(pathA, pathB, &value);
        if (rc != 0) {
            fprintf(stderr, "ERROR: albedo() failed for scenario '%s' (rc=%d). Skipping.\n", spec->scenario, rc);
            skipped++;
            continue;
        }
#endif
        fprintf(out, "%s,%.15g\n", spec->scenario, value);
        processed++;
    }

    fclose(out);

    fprintf(stdout, "Done. Wrote %zu scenario(s) to '%s'. Skipped %zu.\n", processed, outpath, skipped);
    return 0;
