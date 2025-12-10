#!/bin/bash

echo "=== Step 0: Generate aerosol parameters CSV (C program) ==="
gcc src/c/aerosols_param.c -o aerosols_param
./aerosols_param
echo "CSV generated."

echo "=== Step 1: Generating lognormal distribution and plot then generate aerosol profiles and plot (Python) ==="
python3 src/python/height_vs_radius.py
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
python3 src/python/graph.py
if [ $? -ne 0 ]; then
    echo "Error during Step 3."
    exit 1
fi

echo ""
echo "All simulations successfully completed!"
