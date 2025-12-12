
// process_all.c
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Declare the function from albedo_fn.c
double albedo_from_profile_csv_and_store(const char *csv_file_name);

static int has_csv_extension(const char *name) {
    const char *dot = strrchr(name, '.');
    return (dot && strcmp(dot, ".csv") == 0);
}

int main(void) {
    const char *dir = "data";
    DIR *d = opendir(dir);
    if (!d) {
        perror("opendir data");
        return 1;
    }

    struct dirent *ent;
    char path[1024];

    while ((ent = readdir(d)) != NULL) {
        // Skip . and .. entries
        if (ent->d_name[0] == '.') continue;

        if (!has_csv_extension(ent->d_name)) continue;

        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        double A = albedo_from_profile_csv_and_store(path);

        if (A == A) { // NaN check: NaN != NaN
            printf("Computed: %-60s  albedo=%.6f\n", ent->d_name, A);
        } else {
            fprintf(stderr, "WARN: could not compute albedo for %s\n", ent->d_name);
        }
    }

    closedir(d);
    return 0;
}


