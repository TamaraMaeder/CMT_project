#!/bin/bash
set -e  # stoppe le script à la première erreur

echo "=== Step 0: Generate aerosol parameters CSV (C program) ==="

# Root of the project 
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

SRC_C="$PROJECT_ROOT/src/c"
SRC_PY="$PROJECT_ROOT/src/python"
DATA_DIR="$PROJECT_ROOT/data"
RESULTS_DIR="$PROJECT_ROOT/results"
BIN_DIR="$PROJECT_ROOT/bin"

# Créer les dossiers nécessaires
mkdir -p "$DATA_DIR" "$RESULTS_DIR" "$BIN_DIR"

# Compilation and execution of the scripts 

gcc "$SRC_C/aerosols_param.c" -o "$SRC_C/aerosols_param"

"$SRC_C/aerosols_param" "$DATA_DIR/aerosol_modes.csv"

echo "CSV generated at $DATA_DIR/aerosol_modes.csv"

echo ""
echo "=== Step 1: Generating lognormal distributions and aerosol profiles (Python) ==="

python3 "$SRC_PY/height_vs_radius.py"
if [ $? -ne 0 ]; then
    echo "Error during Step 1."
    exit 1
fi

echo ""

echo "=== Step 2: Computing cloud albedo (C programs) ==="

# Ensure we run from project root so ./a.out is created and found here
cd "$PROJECT_ROOT"

# Build & run albedo_run (paired with albedo_fn.c)
echo "[build] albedo_fn.c + albedo_run.c -> a.out"
gcc -std=c11 -O2 -Wall -Wextra \
    "$SRC_C/albedo_fn.c" \
    "$SRC_C/albedo_run.c" \
    -o bin/albedo \
    -lm

echo "[run] a.out (albedo_run)"
./bin/albedo
echo

# Build & run albedo_vs_N (paired with albedo_fn.c)
echo "[build] albedo_fn.c + albedo_vs_N_run.c -> a.out"
gcc -std=c11 -O2 -Wall -Wextra \
    "$SRC_C/albedo_fn.c" \
    "$SRC_C/albedo_vs_N_run.c" \
    -o bin/albedo_N \
    -lm

echo "[run] a.out (albedo_vs_N_run)"
./bin/albedo_N
echo

# Build & run effradius (effective_radius.c + apply_effective_radius.c)
echo "[build] effective_radius.c + apply_effective_radius.c -> a.out"
gcc -std=c11 -O2 -Wall -Wextra \
    "$SRC_C/effective_radius.c" \
    "$SRC_C/apply_effective_radius.c" \
    -o bin/albedo_r \
    -lm

echo "[run] a.out (effradius)"
./bin/albedo_r

echo "=== Step 3: Final plots and analysis (Python) ==="

python3 "$SRC_PY/plot_albedo_by_scenario.py"
python3 "$SRC_PY/plot_albedo_effradius_vs_N.py"
if [ $? -ne 0 ]; then
    echo "Error during Step 3."
    exit 1
fi

echo ""
echo "All simulations successfully completed!"

