
# plot_albedo_by_scenario.py
import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

# -------------------- Paths resolved from project root --------------------
SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
# src/python -> project root (go up two levels)
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))

DATA_DIR = os.path.join(PROJECT_ROOT, 'data')
CSV_PATH = os.path.join(DATA_DIR, 'all_albedo_values.csv')

OUT_DIR = os.path.join(PROJECT_ROOT, 'results')
OUT_PATH = os.path.join(OUT_DIR, 'albedo_by_scenario.png')

# -------------------- Config toggles --------------------
USE_GLOBAL_YMAX = False   # Set True to use common y-limit across subplots

# Ensure output directory exists
os.makedirs(OUT_DIR, exist_ok=True)

# -------------------- Load data --------------------
if not os.path.exists(CSV_PATH):
    raise FileNotFoundError(
        f"CSV not found at {CSV_PATH}. "
        "Make sure you ran the C routine to generate data/all_albedo_values.csv."
    )

df = pd.read_csv(CSV_PATH)

if df.empty:
    raise ValueError(f"CSV at {CSV_PATH} is empty. Nothing to plot.")

# Normalize column names
df.columns = [c.strip().lower() for c in df.columns]
required_cols = {'scenario', 'aerosol_name', 'albedo'}
missing = required_cols - set(df.columns)
if missing:
    raise ValueError(
        f"CSV must contain columns: scenario, aerosol_name, albedo. Missing: {sorted(missing)}"
    )

# Deduplicate: keep last entry for (scenario, aerosol_name)
# This ensures reruns update values without producing multiple bars
df = df.sort_index()  # keep original order
df = df.drop_duplicates(subset=['scenario', 'aerosol_name'], keep='last')

# Clean/pretty labels for readability
def prettify(text: str) -> str:
    return (
        str(text)
        .replace('_', ' ')
        .replace('simulation', '')
        .replace('for ', '')
        .strip()
        .title()
    )

df['scenario_label'] = df['scenario'].apply(prettify)
df['aerosol_label'] = df['aerosol_name'].apply(lambda s: str(s).replace('_', ' ').title())

# Sort aerosols within each scenario by albedo (descending for clarity)
df_sorted = df.sort_values(['scenario_label', 'albedo'], ascending=[True, False])

# Prepare scenarios
scenarios = df_sorted['scenario_label'].unique().tolist()
num_scenarios = len(scenarios)
if num_scenarios == 0:
    raise ValueError("No scenarios found in the CSV after preprocessing.")

# Layout: grid with up to 2 columns when >2 scenarios
cols = 2 if num_scenarios > 2 else 1
rows = int(np.ceil(num_scenarios / cols))

# Dynamic figure size (tune as needed)
fig_w = 6 * cols
fig_h = 4.5 * rows
fig, axes = plt.subplots(rows, cols, figsize=(fig_w, fig_h), squeeze=False)

# Color mapping: keep aerosol colors consistent across scenarios
base_palette = ['#1f77b4','#ff7f0e','#2ca02c','#d62728','#9467bd',
                '#8c564b','#e377c2','#7f7f7f','#bcbd22','#17becf']
aerosols = df_sorted['aerosol_label'].unique().tolist()
color_map = {a: base_palette[i % len(base_palette)] for i, a in enumerate(aerosols)}

# Optional global Y limit for strict comparison
global_max = df_sorted['albedo'].max() if USE_GLOBAL_YMAX else None

for idx, scen in enumerate(scenarios):
    r = idx // cols
    c = idx % cols
    ax = axes[r][c]
    d = df_sorted[df_sorted['scenario_label'] == scen]

    x = np.arange(len(d))
    colors = [color_map[a] for a in d['aerosol_label']]
    ax.bar(x, d['albedo'].values, color=colors)

    ax.set_title(scen, fontsize=12)
    ax.set_xticks(x)
    ax.set_xticklabels(d['aerosol_label'].tolist(), rotation=0, ha='center')
    ax.set_ylabel('Albedo (unitless)')

    ymax = d['albedo'].max()
    if USE_GLOBAL_YMAX and global_max is not None:
        ylim_top = max(global_max * 1.25, 0.3)
    else:
        ylim_top = max(ymax * 1.25, 0.3)
    ax.set_ylim(0, ylim_top)
    ax.grid(axis='y', linestyle=':', alpha=0.35)

    # Annotate bars
    # Place labels slightly above each bar; cap to axes top to avoid overflow
    for i, v in enumerate(d['albedo'].values):
        label_y = min(v + (0.02 * ylim_top), ylim_top * 0.98)
        ax.text(i, label_y, f"{v:.3f}", ha='center', va='bottom', fontsize=9)

# Remove empty subplots if the grid has extra cells
for idx in range(num_scenarios, rows * cols):
    r = idx // cols
    c = idx % cols
    fig.delaxes(axes[r][c])

# Global legend for aerosol colors
legend_handles = [Patch(color=color_map[a], label=a) for a in aerosols]
fig.legend(handles=legend_handles, loc='upper right', bbox_to_anchor=(0.98, 0.98), title='Aerosols')

fig.suptitle('Cloud Albedo by Aerosol Type per Scenario', fontsize=14, y=0.995)
fig.tight_layout(rect=[0, 0, 1, 0.97])

# Save single PNG containing all scenarios
plt.savefig(OUT_PATH, dpi=180)
print(f"Saved figure to: {OUT_PATH}")
