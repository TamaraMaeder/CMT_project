import numpy as np
import matplotlib.pyplot as plt
import pyrcel as pm

# Supersaturation grid (0.01–2 %)
S_perc = np.linspace(0.01, 2.0, 200)
S = S_perc / 100.0

def compute_ccn(mode, S):
    """Compute CCN activation using the pyrcel CDF."""
    return np.array([mode.cdf(s) for s in S])

# --- Define aerosol modes ---

sulfate =  pm.AerosolSpecies('sulfate',
                             pm.Lognorm(mu=0.015, sigma=1.6, N=850.),
                             kappa=0.54, bins=200)
mode_sulfate = pm.Lognorm(mu=0.015, sigma=1.6, N=850.)
sea_salt = pm.AerosolSpecies('sea salt',
                             pm.Lognorm(mu=0.85, sigma=1.2, N=10.),
                             kappa=1.2, bins=40)
mode_sea_salt = pm.Lognorm(mu=0.85, sigma=1.2, N=10.)

# Compute CCN spectra
ccn_seasalt = compute_ccn(mode_sea_salt, S)
ccn_sulfate = compute_ccn(mode_sulfate, S)

# --- Plot ---
plt.figure(figsize=(7,5))
plt.plot(S_perc, ccn_seasalt, label="Sea salt", linewidth=2)
plt.plot(S_perc, ccn_sulfate, label="Sulfate", linewidth=2)
plt.xlabel("Supersaturation (%)")
plt.ylabel("CCN concentration (cm⁻³)")
plt.title("CCN Spectrum vs Supersaturation (Pyrcel)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
