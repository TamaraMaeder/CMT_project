import os
import pandas as pd
import matplotlib.pyplot as plt

# --- Paths ---
csv_path = os.path.join("data", "albedo_values.csv")
output_dir = "results"
output_path = os.path.join(output_dir, "albedo_all_scenarios.png")

os.makedirs(output_dir, exist_ok=True)

# --- Load data ---
df = pd.read_csv(csv_path)
df['albedo'] = pd.to_numeric(df['albedo'], errors='coerce')
df = df.dropna(subset=['albedo'])

# --- Colors (one per scenario) ---
colors = [
    "#1f77b4",  # blue
    "#ff7f0e",  # orange
    "#2ca02c",  # green
    "#d62728"   # red
]

# --- Plot ---
plt.style.use('seaborn-v0_8')
fig, ax = plt.subplots(figsize=(8, 5))

bars = ax.bar(
    df['scenario'],
    df['albedo'],
    width=0.6,
    color=colors[:len(df)]
)

ax.set_ylabel("Albedo")
ax.set_ylim(0, 1.0)
ax.set_title("Albedo comparison between scenarios", pad=45)
ax.set_xlabel("Scenarios")
ax.legend(
    bars,
    df['scenario'],
    loc='upper center',
    bbox_to_anchor=(0.5, 1.12),  # ← SOUS le titre
    ncol=2,
    frameon=False
)

# Remove x tick labels (legend replaces them)
ax.set_xticks([])

# Annotate values on bars
ax.bar_label(bars, fmt="%.3f", padding=3)

plt.tight_layout()
fig.savefig(output_path, dpi=300, bbox_inches="tight")
plt.close(fig)

print(f"Saved figure to: {output_path}")
