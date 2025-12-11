#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_AEROSOLS  64
#define MAX_NAME_LEN 128
#define CMD_LEN       512

typedef enum {
    AEROSOL_NATURAL = 0,
    AEROSOL_ANTHROPOGENIC = 1,
    AEROSOL_UNKNOWN = 2
} AerosolOrigin;

typedef struct {
    char name[MAX_NAME_LEN];
    AerosolOrigin origin;
    double N0_cm3;
    double Dg_um;
    double sigma_g;
    double kappa;
} AerosolMode;

/* ---------- petites fonctions utilitaires ---------- */

static void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) {
        s[len-1] = '\0';
        len--;
    }
}

static AerosolOrigin parse_origin(const char *field) {
    if (!field) return AEROSOL_UNKNOWN;

    if (strcmp(field, "natural") == 0 ||
        strcmp(field, "NATURAL") == 0 ||
        strcmp(field, "0") == 0) {
        return AEROSOL_NATURAL;
    }

    if (strcmp(field, "anthropogenic") == 0 ||
        strcmp(field, "ANTHROPOGENIC") == 0 ||
        strcmp(field, "1") == 0) {
        return AEROSOL_ANTHROPOGENIC;
    }

    return AEROSOL_UNKNOWN;
}

/* Lecture du fichier CSV dans un tableau d'AerosolMode */
int load_aerosols_from_csv(const char *filepath, AerosolMode *modes, int max_modes) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        perror("Erreur d'ouverture du fichier CSV");
        return -1;
    }

    char line[1024];
    int count = 0;

    /* On saute l'en-tête */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) && count < max_modes) {
        trim_newline(line);
        if (line[0] == '\0') continue;  // ligne vide

        // On suppose : name,origin,N0_cm3,Dg_um,sigma_g,kappa
        char *token = strtok(line, ",");
        int col = 0;

        AerosolMode mode;
        memset(&mode, 0, sizeof(AerosolMode));
        mode.origin = AEROSOL_UNKNOWN;

        while (token) {
            switch (col) {
                case 0: // name
                    strncpy(mode.name, token, MAX_NAME_LEN - 1);
                    mode.name[MAX_NAME_LEN - 1] = '\0';
                    break;
                case 1: // origin
                    mode.origin = parse_origin(token);
                    break;
                case 2:
                    mode.N0_cm3 = atof(token);
                    break;
                case 3:
                    mode.Dg_um = atof(token);
                    break;
                case 4:
                    mode.sigma_g = atof(token);
                    break;
                case 5:
                    mode.kappa = atof(token);
                    break;
                default:
                    break;
            }
            token = strtok(NULL, ",");
            col++;
        }

        if (mode.name[0] != '\0') {
            modes[count++] = mode;
        }
    }

    fclose(fp);
    return count;
}

/* Affichage de debug */
void print_modes(const AerosolMode *modes, int n) {
    printf("Loaded %d aerosol modes:\n", n);
    for (int i = 0; i < n; ++i) {
        printf("  [%d] %s  (origin=%s)\n",
               i,
               modes[i].name,
               modes[i].origin == AEROSOL_NATURAL ? "natural" :
               modes[i].origin == AEROSOL_ANTHROPOGENIC ? "anthropogenic" :
               "unknown");
    }
}

/* Lancement d'une commande système et check de base */
int run_cmd(const char *cmd) {
    printf(">> %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "  ! Command failed with code %d\n", ret);
    }
    return ret;
}

int main(void) {
    AerosolMode modes[MAX_AEROSOLS];

    /* 1) Lire le CSV */
    const char *csv_path = "data/aerosols_modes.csv";
    int n_modes = load_aerosols_from_csv(csv_path, modes, MAX_AEROSOLS);
    if (n_modes <= 0) {
        fprintf(stderr, "Erreur: aucun aérosol chargé depuis %s\n", csv_path);
        return 1;
    }

    print_modes(modes, n_modes);

    /* 2) Séparer naturels et anthropogéniques */
    int nat_indices[MAX_AEROSOLS];
    int anth_indices[MAX_AEROSOLS];
    int n_nat = 0, n_anth = 0;

    for (int i = 0; i < n_modes; ++i) {
        if (modes[i].origin == AEROSOL_NATURAL) {
            nat_indices[n_nat++] = i;
        } else if (modes[i].origin == AEROSOL_ANTHROPOGENIC) {
            anth_indices[n_anth++] = i;
        }
    }

    if (n_nat == 0 || n_anth == 0) {
        fprintf(stderr,
                "Erreur: il faut au moins un aérosol naturel et un anthropogénique.\n");
        return 1;
    }

    printf("\nFound %d natural and %d anthropogenic aerosols.\n", n_nat, n_anth);

    /* 3) (optionnel) une fois pour toutes : comparaison lognormale */
    {
        char cmd[CMD_LEN];
        snprintf(cmd, sizeof(cmd),
                 "python3 src/python/logn_size_dist_compare.py");
        run_cmd(cmd);
    }

    /* 4) Boucle sur tous les couples naturel × anthropogénique */
    for (int i = 0; i < n_nat; ++i) {
        for (int j = 0; j < n_anth; ++j) {

            AerosolMode nat  = modes[nat_indices[i]];
            AerosolMode anth = modes[anth_indices[j]];

            printf("\n=== Simulation for pair: %s (natural) + %s (anthropogenic) ===\n",
                   nat.name, anth.name);

            char cmd[CMD_LEN];

            /* 4.a) height_vs_radius */
            snprintf(cmd, sizeof(cmd),
                     "python3 src/python/height_vs_radius.py \"%s\" \"%s\"",
                     nat.name, anth.name);
            run_cmd(cmd);

            /* 4.b) CCN_pyrcel */
            snprintf(cmd, sizeof(cmd),
                     "python3 src/python/CCN_pyrcel.py \"%s\" \"%s\"",
                     nat.name, anth.name);
            run_cmd(cmd);

            /* 4.c) CDNC_calc */
            snprintf(cmd, sizeof(cmd),
                     "python3 src/python/CDNC_calc.py \"%s\" \"%s\"",
                     nat.name, anth.name);
            run_cmd(cmd);

            /* Si tu veux lancer albedo_fct pour chaque couple :
             * (à adapter selon la logique de ton code albédo)
             */
            /*
            snprintf(cmd, sizeof(cmd),
                     "./albedo_fct \"%s\" \"%s\"",
                     nat.name, anth.name);
            run_cmd(cmd);
            */
        }
    }

    /* 5) Ou bien : calcul global des albédo après toutes les simulations */
    /*
    run_cmd("./albedo_fct");
    */

    printf("\nAll simulations done.\n");
    return 0;
}
