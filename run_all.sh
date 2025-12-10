#!/bin/bash

echo "=== Step 1: Generating aerosol profiles (Python) ==="
python3 scripts/step1_generate_profiles.py
if [ $? -ne 0 ]; then
    echo "Error during Step 1."
    exit 1
fi

echo ""
echo "=== Step 2: Computing cloud albedo (C program) ==="
gcc src/c/albedo_fct.c -o albedo_fct
./albedo_fct
if [ $? -ne 0 ]; then
    echo "Error during Step 2."
    exit 1
fi

echo ""
echo "=== Step 3: Final plots and analysis (Python) ==="
python3 scripts/step3_final_plots.py
if [ $? -ne 0 ]; then
    echo "Error during Step 3."
    exit 1
fi

echo ""
echo "All simulations successfully completed!"
