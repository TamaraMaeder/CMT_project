# #!/bin/bash

# echo "=== Step 0: Generate aerosol parameters CSV (C program) ==="
# gcc src/c/aerosols_param.c -o aerosols_param
# ./aerosols_param
# echo "CSV generated."

# echo "=== Step 1: Generating lognormal distribution and plot then generate aerosol profiles and plot (Python) ==="
# python3 src/python/height_vs_radius.py
# if [ $? -ne 0 ]; then
#     echo "Error during Step 1."
#     exit 1
# fi

# echo ""
# echo "=== Step 2: Computing cloud albedo (C program) ==="
# gcc src/c/process_all.c -o process_all
# ./albedo_fct
# if [ $? -ne 0 ]; then
#     echo "Error during Step 2."
#     exit 1
# fi

# echo ""
# echo "=== Step 3: Final plots and analysis (Python) ==="
# python3 src/python/plot_albedo_by_scenario.py
# if [ $? -ne 0 ]; then
#     echo "Error during Step 3."
#     exit 1
# fi

# echo ""
# echo "All simulations successfully completed!"


#!/bin/bash
set -e  # stoppe le script à la première erreur

echo "=== Step 0: Generate aerosol parameters CSV (C program) ==="

# Root of the project 
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

SRC_C="$PROJECT_ROOT/src/c"
SRC_PY="$PROJECT_ROOT/src/python"
DATA_DIR="$PROJECT_ROOT/data"
RESULTS_DIR="$PROJECT_ROOT/results"

# Créer les dossiers nécessaires
mkdir -p "$DATA_DIR"
mkdir -p "$RESULTS_DIR"

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
echo "=== Step 2: Computing cloud albedo (C program) ==="

# Compile and link both source files
gcc -arch arm64 -std=c11 -O2 -Wall -Wextra \
    "$SRC_C/process_all.c" \
    "$SRC_C/albedo_fn.c" \
    -o "$SRC_C/process_all"


"$SRC_C/process_all"
if [ $? -ne 0 ]; then
    echo "Error during Step 2."
    exit 1
fi

echo ""
echo "=== Step 3: Final plots and analysis (Python) ==="

python3 "$SRC_PY/plot_albedo_by_scenario.py"
if [ $? -ne 0 ]; then
    echo "Error during Step 3."
    exit 1
fi

echo ""
echo "All simulations successfully completed!"
