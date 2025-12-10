
# plot_albedo_absolute_labels.py
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# --- 1) Load data ---
df = pd.read_csv('albedo_values.csv')  # expects columns: file, albedo
df = df.rename(columns={c: c.lower() for c in df.columns})
if not {'file', 'albedo'}.issubset(df.columns):
    raise ValueError("CSV must contain columns 'file' and 'albedo'.")

# --- 2) Map filenames to clean labels ---
label_map = {
    'Sea_salt_coarse_profile.csv': 'Sea salt coarse',
    'Sulfate_accum_profile.csv': 'Sulfate', 
}

def to_clean_label(fname):
    return label_map.get(fname, fname.replace('_', ' ').replace('.csv', '').strip())

df['label'] = df['file'].apply(to_clean_label)

df = df.sort_values('albedo').reset_index(drop=True)
ref_file = df.loc[0, 'file']
ref_label = df.loc[0, 'label']
ref_albedo = df.loc[0, 'albedo']

# --- 4) Plot (Absolute albedo) ---
plt.figure(figsize=(9, 5))

colors = ['#2ca02c' if f == ref_file else '#1f77b4' for f in df['file']]

pos = np.arange(len(df))
plt.bar(pos, df['albedo'], color=colors)

plt.xticks(pos, df['label'])

plt.ylabel('Albedo (unitless)')
plt.title(f'Cloud Albedo by Aerosol Type\n for simulation 1')

# Annotate each bar with its value
for i, v in enumerate(df['albedo']):
    plt.text(i, v + 0.01, f"{v:.3f}", ha='center', va='bottom', fontsize=9)

plt.ylim(0, df['albedo'].max() + 0.2)
plt.grid(axis='y', linestyle=':', alpha=0.35)
plt.tight_layout()

plt.show()
