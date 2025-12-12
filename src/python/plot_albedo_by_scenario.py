
# plot_albedo_by_scenario.py
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

# --- Config (per your setup) ---
CSV_PATH = 'all_albedo_values.csv'       # input CSV in workflow root
OUT_DIR  = 'results'                      # output folder
OUT_PATH = os.path.join(OUT_DIR, 'albedo_by_scenario.png')

# Ensure output directory exists
os.makedirs(OUT_DIR, exist_ok=True)

# --- Load data ---
if not os.path.exists(CSV_PATH):
    raise FileNotFoundError(f"CSV not found at {CSV_PATH}. Run this script from the workflow root.")

df = pd.read_csv(CSV_PATH)

# Normalize column names
df.columns = [c.strip().lower() for c in df.columns]
required_cols = {'scenario', 'aerosol_name', 'albedo'}
if not required_cols.issubset(df.columns):
    raise ValueError("CSV must contain columns: scenario, aerosol_name, albedo")

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
df['aerosol_label'] = df['aerosol_name'].apply(lambda s: s.replace('_', ' ').title())

# Sort aerosols within each scenario by albedo (descending for clarity)
df_sorted = df.sort_values(['scenario_label', 'albedo'], ascending=[True, False])

# Prepare scenarios
scenarios = df_sorted['scenario_label'].unique().tolist()
num_scenarios = len(scenarios)

# Layout: grid with up to 2 columns when >2 scenarios
cols = 2 if num_scenarios > 2 else 1
rows = int(np.ceil(num_scenarios / cols))

# Dynamic figure size
fig_w = 6 * cols
fig_h = 4.5 * rows
fig, axes = plt.subplots(rows, cols, figsize=(fig_w, fig_h), squeeze=False)

# Color mapping: keep aerosol colors consistent across scenarios
base_palette = ['#1f77b4','#ff7f0e','#2ca02c','#d62728','#9467bd',
                '#8c564b','#e377c2','#7f7f7f','#bcbd22','#17becf']
aerosols = df_sorted['aerosol_label'].unique().tolist()
color_map = {a: base_palette[i % len(base_palette)] for i, a in enumerate(aerosols)}

# (Optional) Use a common y-limit across all subplots for stricter comparison
# global_max = df_sorted['albedo'].max()

for idx, scen in enumerate(scenarios):
    r = idx // cols
    c = idx % cols
    ax = axes[r][c]
    d = df_sorted[df_sorted['scenario_label'] == scen]

    x = np.arange(len(d))
    colors = [color_map[a] for a in d['aerosol_label']]
    ax.bar(x, d['albedo'].values, color=colors)

    ax.set_title(scen)
    ax.set_xticks(x)
    ax.set_xticklabels(d['aerosol_label'].tolist(), rotation=0)
    ax.set_ylabel('Albedo (unitless)')

    ymax = d['albedo'].max()
    ax.set_ylim(0, max(ymax * 1.25, 0.3))  # change to: ax.set_ylim(0, global_max * 1.25) if using global_max
    ax.grid(axis='y', linestyle=':', alpha=0.35)

    # Annotate bars
    for i, v in enumerate(d['albedo'].values):
        ax.text(i, v + (0.02 * ymax), f"{v:.3f}", ha='center', va='bottom', fontsize=9)

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
