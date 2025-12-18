
#!/usr/bin/env python3
"""
Plot albedo and effective radius vs. sulfate number concentration (N).

- Left y-axis: Albedo (–)
- Right y-axis: Effective radius [m]
- X-axis: Sulfate concentration N [cm^-3]

Inputs (expected under <project_root>/data/):
  - albedo_vs_N.csv with columns: scenario,albedo
      e.g., "Scenario 1: Sulfate accum 3.0e+02 cm3,0.20158..."
  - effective_radius_mix_scenarios.csv with columns: scenario,effective_radius_m
      e.g., "3.0e+02_cm3,6.54e-07"

Output (PNG only) is saved under <project_root>/results/.
"""

from pathlib import Path
import re
import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe  # for subtle white halo under lines

# Resolve project paths
SCRIPT_PATH = Path(__file__).resolve()
PROJECT_ROOT = SCRIPT_PATH.parents[2]  # .../<root>/src/python/this_file.py -> parents[2] == <root>
DATA_DIR = PROJECT_ROOT / 'data'
RESULTS_DIR = PROJECT_ROOT / 'results'
ALBEDO_CSV = DATA_DIR / 'albedo_vs_N.csv'
REFF_CSV = DATA_DIR / 'effective_radius_mix_scenarios.csv'
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

# Matches scientific notation like "3.0e+02 cm3" inside the label
_sci_re = re.compile(r'([0-9]+\.?[0-9]*e[+\-]?\d+)\s*cm3', re.IGNORECASE)

def extract_N_from_albedo_label(label: str) -> float:
    """Extract sulfate number concentration [cm^-3] from albedo scenario label.
    Example: "Scenario 1: Sulfate accum 3.0e+02 cm3" -> 300.0
    """
    if not isinstance(label, str):
        return float('nan')
    m = _sci_re.search(label)
    if m:
        try:
            return float(m.group(1))
        except Exception:
            pass
    # Fallback: try to find the last numeric token
    nums = re.findall(r"[0-9]+\.?[0-9]*", label)
    if nums:
        try:
            return float(nums[-1])
        except Exception:
            return float('nan')
    return float('nan')

def extract_N_from_reff_label(label: str) -> float:
    """Extract sulfate number concentration [cm^-3] from effective radius scenario label.
    Example: "3.0e+02_cm3" -> 300.0
    """
    if not isinstance(label, str):
        return float('nan')
    cleaned = label.replace('_cm3', '').replace('cm3', '').strip()
    try:
        return float(cleaned)
    except Exception:
        m = re.search(r'([0-9]+\.?[0-9]*e[+\-]?\d+)', cleaned, re.IGNORECASE)
        if m:
            try:
                return float(m.group(1))
            except Exception:
                return float('nan')
        return float('nan')

# Load data 
if not ALBEDO_CSV.exists() or not REFF_CSV.exists():
    print(
        f"Expected data files not found.\n"
        f"  Albedo CSV: {ALBEDO_CSV}\n"
        f"  Effective radius CSV: {REFF_CSV}\n"
        f"Please ensure your project has a 'data' folder containing these CSVs.",
        file=sys.stderr,
    )
    sys.exit(1)

# Read data
df_alb = pd.read_csv(ALBEDO_CSV)
df_reff = pd.read_csv(REFF_CSV)

# Validate columns
if not {'scenario', 'albedo'}.issubset(df_alb.columns):
    raise ValueError("albedo_vs_N.csv must have columns: 'scenario', 'albedo'")
if not {'scenario', 'effective_radius_m'}.issubset(df_reff.columns):
    raise ValueError("effective_radius_mix_scenarios.csv must have columns: 'scenario', 'effective_radius_m'")

# Parse N from scenario labels
df_alb['N_cm3'] = df_alb['scenario'].apply(extract_N_from_albedo_label)
df_reff['N_cm3'] = df_reff['scenario'].apply(extract_N_from_reff_label)

# Merge datasets on N and keep only 300–1000 cm^-3
merged = (
    pd.merge(df_alb[['N_cm3', 'albedo']], df_reff[['N_cm3', 'effective_radius_m']], on='N_cm3', how='inner')
      .query('300 <= N_cm3 <= 1000')
      .sort_values('N_cm3')
      .reset_index(drop=True)
)

if merged.empty:
    raise RuntimeError(
        "Merged dataset is empty. Check that both CSVs cover the same N range (300–1000 cm^-3) and parsing worked."
    )

# Plot 
plt.rcParams['figure.dpi'] = 160
plt.rcParams['axes.formatter.useoffset'] = False

fig, ax1 = plt.subplots(figsize=(7.6, 4.4))

# Colors: blue for albedo, orange for effective radius
BLUE = '#1f77b4'    # Tableau blue
ORANGE = '#ff7f0e'  # Tableau orange

# Common path effect: white halo to improve legibility over overlaps and grid
halo = [pe.Stroke(linewidth=4.5, foreground='white', alpha=0.95), pe.Normal()]

# Left y-axis: Albedo — solid line, hollow circle markers, near-opaque, rounded caps
ln1 = ax1.plot(
    merged['N_cm3'], merged['albedo'],
    color=BLUE, linestyle='-', linewidth=2.4,
    marker='o', markersize=5.5,
    markerfacecolor='white', markeredgecolor=BLUE, markeredgewidth=1.2,
    alpha=0.98,
    solid_capstyle='round',
    label='Albedo (–)',
    zorder=2,
    path_effects=halo
)
ax1.set_xlabel('Sulfate concentration $N$ [cm$^{-3}$]')
ax1.set_ylabel('Albedo (–)', color=BLUE)
ax1.tick_params(axis='y', labelcolor=BLUE)
ax1.grid(True, which='both', linestyle='--', linewidth=0.6, alpha=0.25)

# Right y-axis: Effective radius — dashed line with higher-contrast pattern, filled squares
ax2 = ax1.twinx()
ln2 = ax2.plot(
    merged['N_cm3'], merged['effective_radius_m'],
    color=ORANGE, linestyle='--', linewidth=2.4,
    dashes=(7, 3),  # long dash, short gap for better separation
    marker='s', markersize=5.5,
    markerfacecolor=ORANGE, markeredgecolor=ORANGE, markeredgewidth=1.0,
    alpha=0.98,
    solid_capstyle='round',
    label='Effective radius [m]',
    zorder=3,
    path_effects=halo
)
ax2.set_ylabel('Effective radius $r_\\mathrm{eff}$ [m]', color=ORANGE)
ax2.tick_params(axis='y', labelcolor=ORANGE)

# Color spines to match axes for clarity
ax1.spines['left'].set_color(BLUE)
ax2.spines['right'].set_color(ORANGE)

# Title and limits
ax1.set_title('Albedo and Effective Radius vs. Sulfate Number Concentration')
ax1.set_xlim(merged['N_cm3'].min() - 5, merged['N_cm3'].max() + 5)

# Combine legends from both axes
lines = ln1 + ln2
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='best', frameon=True, framealpha=0.9)

fig.tight_layout()

#Save PNG only into 'results' 
png_path = RESULTS_DIR / 'albedo_re_vs_N.png'  
fig.savefig(png_path, bbox_inches='tight')
