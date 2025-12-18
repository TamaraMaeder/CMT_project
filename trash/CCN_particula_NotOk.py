"""
CCN Activation Curve using Particula

This script:
1. Creates a realistic aerosol population (sulfate + organics)
2. Computes Köhler activation properties (critical radius, supersaturation)
3. Calculates the number of activated CCN for a range of supersaturations
4. Plots the CCN spectrum

AI help from: Particula Assistant
"""
kohler.critical_radius()

import numpy as np
import matplotlib.pyplot as plt

from particula import distribution, environment, particle
from particula.util import kohler

# -------------------------------------------------------
# 1. Atmospheric and aerosol setup
# -------------------------------------------------------
atm = environment.Atmosphere(temperature=298.15, pressure=101325)  # 25°C, 1 atm

# Define two aerosol modes: sulfate (hygroscopic) and organics (less hygroscopic)
aerosols = [
    {
        "name": "Sulfate",
        "count": 500e6,               # particles per m³ (~500 cm⁻³)
        "mode_diameter": 0.08e-6,     # 80 nm mode
        "gstd": 1.5,
        "kappa": 0.6                  # hygroscopic parameter (highly soluble)
    },
    {
        "name": "Organics",
        "count": 300e6,               # ~300 cm⁻³
        "mode_diameter": 0.12e-6,     # 120 nm mode
        "gstd": 1.6,
        "kappa": 0.1                  # less hygroscopic
    }
]

# -------------------------------------------------------
# 2. Function to compute CCN activation
# -------------------------------------------------------
def compute_ccn_spectrum(aerosol_list, supersaturation_range):
    """Compute total CCN concentration as a function of supersaturation."""
    total_ccn = np.zeros_like(supersaturation_range)

    for aero in aerosol_list:
        # Lognormal number distribution
        dist = distribution.LogNormal(
            count=aero["count"],
            mode_diameter=aero["mode_diameter"],
            geometric_std_dev=aero["gstd"]
        )
        
        # Hygroscopicity
        kappa_val = aero["kappa"]

        # Get representative dry radii (log-spaced)
        dry_radii = np.logspace(-8, -6, 200)  # 10 nm to 1 µm
        
        # Köhler critical supersaturation for each radius
        _, s_crit = kohler.critical_radius(
            dry_radius=dry_radii,
            kappa=kappa_val,
            temperature=atm.temperature
        )

        # Convert to supersaturation (%)
        s_crit_percent = (s_crit - 1) * 100

        # For each supersaturation level, count activated particles
        for i, S in enumerate(supersaturation_range):
            activated = dry_radii[s_crit_percent <= S]
            n_ccn = dist.integral_above(activated.min()) if activated.size > 0 else 0
            total_ccn[i] += n_ccn

    return total_ccn

# -------------------------------------------------------
# 3. Define supersaturation range (0.05% to 1%)
# -------------------------------------------------------
S_range = np.linspace(0.05, 1.0, 50)  # [%]

# Compute CCN number as function of supersaturation
ccn_counts = compute_ccn_spectrum(aerosols, S_range)

# Convert to cm⁻³ (Particula uses m⁻³)
ccn_counts_cm3 = ccn_counts / 1e6

# -------------------------------------------------------
# 4. Plot results
# -------------------------------------------------------
plt.figure(figsize=(8, 5))
plt.plot(S_range, ccn_counts_cm3, 'b-', lw=2)
plt.xlabel('Supersaturation [%]')
plt.ylabel('CCN concentration [cm$^{-3}$]')
plt.title('CCN Activation Spectrum (Sulfate + Organics)')
plt.grid(True)
plt.tight_layout()
plt.show()


