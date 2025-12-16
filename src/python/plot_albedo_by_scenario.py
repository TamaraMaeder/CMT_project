
import os
import pandas as pd
import matplotlib.pyplot as plt

# --- Paths (adjust if needed) ---
csv_path = os.path.join("data", "albedo_values.csv")  # update name if different
output_dir = "results"
output_path = os.path.join(output_dir, "albedo_scenarios.png")

# --- Ensure output directory exists ---
os.makedirs(output_dir, exist_ok=True)

# --- Load data ---
# CSV should have columns: scenario, albedo
df = pd.read_csv(csv_path)

# Optional: clean / sort if needed
df['albedo'] = pd.to_numeric(df['albedo'], errors='coerce')
df = df.dropna(subset=['albedo']).reset_index(drop=True)

# --- Plot settings ---
plt.style.use('seaborn-v0_8')
fig, axes = plt.subplots(2, 2, figsize=(12, 8), constrained_layout=True)
axes = axes.flatten()

# --- Create 4 subplots, one per scenario ---
for i, (idx, row) in enumerate(df.iterrows()):
    ax = axes[i]
    scenario = row['scenario']
    albedo = row['albedo']

    # Single bar for the scenario
    ax.bar([scenario], [albedo], color="#2E86C1")
    ax.set_title(scenario, fontsize=11, pad=10)
    ax.set_ylim(0, max(1.0, albedo * 1.2))  # albedo typically in [0,1]; adjust safely
    ax.set_ylabel("Albedo")
    ax.set_xticklabels([])  # hide x tick label (title already shows scenario)

    # Annotate the bar with the numeric value
    ax.bar_label(ax.containers[0], fmt="%.3f", padding=3)

# If fewer than 4 scenarios, hide the unused axes
for j in range(len(df), 4):
    fig.delaxes(axes[j])

# --- Overall title ---
fig.suptitle("Albedo per Scenario (2×2 subplots)", fontsize=14)

# --- Save PNG ---
fig.savefig(output_path, dpi=300)
plt.close(fig)

print(f"Saved figure to: {output_path}")
