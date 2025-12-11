
#!/bin/bash
# Robust pipeline to generate aerosol parameters, profiles, albedo, and plots.

set -euo pipefail

# Optional: Set base directory (project root). If not set, default to current directory.
BASE_DIR="${BASE_DIR:-$(pwd)}"
echo "BASE_DIR -> $BASE_DIR"

# Ensure data directory exists (matches your Python convention)
DATA_DIR="$BASE_DIR/data"
mkdir -p "$DATA_DIR"

echo "=== Step 0: Generate aerosol parameters CSV (C program) ==="
if gcc src/c/aerosols_param.c -O2 -o aerosols_param -lm; then
    echo "Compiled aerosols_param."
else
    echo "Compilation failed: src/c/aerosols_param.c"
    exit 1
fi

if ./aerosols_param; then
    echo "CSV generated (aerosol parameters)."
else
    echo "Execution failed: ./aerosols_param"
    exit 1
fi

echo
echo "=== Step 1: Generating lognormal distribution & profiles (Python) ==="
# This script should produce the per-aerosol profile CSVs:
# e.g., <DATA_DIR>/<AEROSOL>_profile_for_<first>_and_<second>_simulation.csv
if python3 src/python/height_vs_radius.py; then
    echo "Step 1 completed: profile CSVs and plots created."
else
    echo "Error during Step 1 (height_vs_radius.py)."
    exit 1
fi

echo
echo "=== Step 2: Computing cloud albedo (C program) ==="
# Compile your updated albedo program that reads profile CSVs and writes albedo CSVs.
# Name: Albedo_fct_2.c
if gcc src/c/Albedo_fct_2.c -O2 -o albedo_fct_2 -lm; then
    echo "Compiled albedo_fct_2."
else
    echo "Compilation failed: src/c/Albedo_fct_2.c"
    exit 1
fi

# Run the program; if it needs arguments (BASE_DIR, aerosol name, first, second, inputs),
# pass them here. If it auto-discovers files, just run it.
# Example if your program expects no args:
if ./albedo_fct_2; then
    echo "Albedo CSV(s) created."
else
    echo "Execution failed: ./albedo_fct_2"
    exit 1
fi

# If your C program expects arguments (uncomment and adapt as needed):
# ./albedo_fct_2 "$BASE_DIR" "Sea Salt" day1 day2 "$DATA_DIR/Sea_Salt_profile_for_day1_and_day2_simulation.csv"

echo
echo "=== Step 3: Final albedo plots and analysis (Python) ==="
# This script should read the albedo CSV(s):
# e.g., <DATA_DIR>/<AEROSOL>_albedo_for_<first>_and_<second>_simulation.csv
# and save the PNG plots next to them.
if python3 src/python/Albedo_simulations_plots.py; then
    echo "Step 3 completed: albedo plots saved as PNG."
else
    echo "Error during Step 3 (Albedo_simulations_plots.py)."
    exit 1
fi

echo
echo "All simulations successfully completed!"
