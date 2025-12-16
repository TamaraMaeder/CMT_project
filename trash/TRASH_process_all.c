
// process_all.c
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

double compute_albedo_and_upsert(const char *profile_csv_path); // from albedo_fn.c

static int ends_with(const char *s, const char *suffix) {
    size_t ls = strlen(s), lf = strlen(suffix);
    return ls >= lf && strcmp(s + ls - lf, suffix) == 0;
}

static int is_regular_file(const char *dir, const char *name) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

int main(int argc, char **argv) {
    // If you want: allow passing a custom data directory via argv[1]
    const char *data_dir = (argc >= 2) ? argv[1] : "data";

    DIR *d = opendir(data_dir);
    if (!d) {
        fprintf(stderr, "Cannot open directory: %s\n", data_dir);
        return 1;
    }

    struct dirent *de;
    size_t processed = 0, failed = 0;

    while ((de = readdir(d)) != NULL) {
        const char *name = de->d_name;
        // Skip . and ..
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        // Skip the consolidated table itself
        if (strcmp(name, "all_albedo_values.csv") == 0) continue;

        // Only CSV files that contain "_profile_"
        if (!ends_with(name, ".csv")) continue;
        if (strstr(name, "_profile_") == NULL) continue;

        // Must be a regular file
        if (!is_regular_file(data_dir, name)) continue;

        // Build full path
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", data_dir, name);

        double A = compute_albedo_and_upsert(path);
        if (!(A >= 0.0) || !(A < 1.0)) {
            fprintf(stderr, "Failed to compute albedo for: %s\n", path);
            failed++;
        } else {
            printf("Updated: %-40s  albedo=%.6f\n", name, A);
            processed++;
        }
    }

    closedir(d);
    printf("\nProcessed: %zu, Failed: %zu\n", processed, failed);
    return (failed == 0) ? 0 : 2;
}


