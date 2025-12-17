
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Albedo vs sulfate number concentration plot.

- Reads: <root>/data/albedo_vs_N.csv (columns: scenario, albedo)
- Extracts sulfate concentration from scenario text (e.g., '... 3.0e+02 cm3' -> 300)
- Filters range: 300–1000 cm^-3
- Plots albedo vs concentration
- Saves PNG: <root>/results/albedo_vs_Nconc.png

To run from workflow root:
    python src/python/albedo_vs_Nconc_plot.py
"""

import os
import re
import math
import pandas as pd
import matplotlib.pyplot as plt

# ---------- Paths (relative to this script -> workflow root) ----------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))

CSV_PATH = os.path.join(ROOT_DIR, "data", "albedo_vs_N.csv")
OUTPUT_DIR = os.path.join(ROOT_DIR, "results")
OUTPUT_PNG = os.path.join(OUTPUT_DIR, "albedo_vs_Nconc.png")

# ---------- Ensure output dir exists ----------
os.makedirs(OUTPUT_DIR, exist_ok=True)

# ---------- Load data ----------
df = pd.read_csv(CSV_PATH)

# Make sure albedo is numeric
df["albedo"] = pd.to_numeric(df["albedo"], errors="coerce")
df = df.dropna(subset=["albedo"]).reset_index(drop=True)

# ---------- Extract concentration (cm^-3) from 'scenario' ----------
def extract_conc_cm3(text: str) -> float:
    """Extract number before 'cm3' in scientific or decimal notation."""
    if not isinstance(text, str):
        return math.nan
    m = re.search(r'([+-]?\d*\.?\d+(?:e[+-]?\d+)?)\s*cm3', text, flags=re.IGNORECASE)
    if m:
        try:
            return float(m.group(1))
        except ValueError:
            return math.nan
    # Fallback: any number if 'cm3' missing
    m2 = re.search(r'([+-]?\d*\.?\d+(?:e[+-]?\d+)?)', text, flags=re.IGNORECASE)
    if m2:
        try:
            return float(m2.group(1))
        except ValueError:
            return math.nan
    return math.nan

df["N_cm3"] = df["scenario"].apply(extract_conc_cm3)
df = df.dropna(subset=["N_cm3"]).reset_index(drop=True)

# ---------- Filter concentration range (cm^-3) ----------
MIN_C, MAX_C = 300.0, 1000.0
df = df[(df["N_cm3"] >= MIN_C) & (df["N_cm3"] <= MAX_C)].copy()

if df.empty:
    raise ValueError(
        "No rows in the 300–1000 cm^-3 range. "
        "Check the scenario parsing or adjust MIN_C/MAX_C."
    )

# ---------- Optional: convert to m^-3 ----------
CONVERT_TO_M3 = False  # set True to plot in m^-3
if CONVERT_TO_M3:
    df["N_m3"] = df["N_cm3"] * 1_000_000.0  # 1 cm^-3 = 1e6 m^-3
    x_col = "N_m3"
    x_label = "Sulfate number concentration (m⁻³)"
else:
    x_col = "N_cm3"
    x_label = "Sulfate number concentration (cm⁻³)"

# ---------- Sort for a clean line plot ----------
df = df.sort_values(by=x_col)

# ---------- Plot ----------
plt.style.use("seaborn-v0_8")
fig, ax = plt.subplots(figsize=(8, 5), constrained_layout=True)

ax.plot(df[x_col], df["albedo"], color="#2E86C1", linestyle="-", marker="o", label="Albedo")

# Labels & aesthetics
ax.set_title("Albedo vs Sulfate Number Concentration\n(Sulfate accumulation scenarios)", fontsize=13, pad=8)
ax.set_xlabel(x_label)
ax.set_ylabel("Albedo (unitless)")
ax.grid(True, linestyle="--", alpha=0.4)
ax.legend()

# Annotate points with albedo values
for x, y in zip(df[x_col], df["albedo"]):
    ax.annotate(f"{y:.3f}", xy=(x, y), xytext=(0, 6),
                textcoords="offset points", ha="center", fontsize=9)

# ---------- Save & close ----------
fig.savefig(OUTPUT_PNG, dpi=300)
plt.close(fig)

