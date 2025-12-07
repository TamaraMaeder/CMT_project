import numpy as np
import matplotlib.pyplot as plt
import pyrcel as pc
from pyrcel import binned_activation

# # Supersaturation grid (0.01–2 %)
# S_perc = np.linspace(0.01, 100.0, 200)
# S = S_perc / 100.0

# def compute_ccn(mode, S):
#     """Compute CCN activation using the pyrcel CDF:
#      cdf(x)
#         Evaluate cumulative concentration up to a particular radius x.
#     """
#     return np.array([mode.cdf(s) for s in S])

# # --- Define aerosol modes ---

# sulfate =  pc.AerosolSpecies('sulfate',
#                              pc.Lognorm(mu=0.015, sigma=1.6, N=850.),
#                              kappa=0.54, bins=200)
# mode_sulfate = pc.Lognorm(mu=0.015, sigma=1.6, N=850.)
# sea_salt = pc.AerosolSpecies('sea salt',
#                              pc.Lognorm(mu=0.85, sigma=1.2, N=10.),
#                              kappa=1.2, bins=40)
# mode_sea_salt = pc.Lognorm(mu=0.85, sigma=1.2, N=10.)

# # Compute CCN spectra
# ccn_seasalt = compute_ccn(mode_sea_salt, S)
# ccn_sulfate = compute_ccn(mode_sulfate, S)

# # --- Plot ---
# plt.figure(figsize=(7,5))
# plt.plot(S_perc, ccn_seasalt, label="Sea salt", linewidth=2)
# plt.plot(S_perc, ccn_sulfate, label="Sulfate", linewidth=2)
# plt.xlabel("Supersaturation (%)")
# plt.ylabel("CCN concentration (cm⁻³)")
# plt.title("CCN Spectrum vs Supersaturation (Pyrcel)")
# plt.grid(True)
# plt.legend()
# plt.tight_layout()
# plt.show()

# Supersaturation grid (0.01–2 %)
S_perc = np.linspace(0.01, 2.0, 200)
S = S_perc / 100.0

# Correct aerosol species definitions
sulfate = pc.AerosolSpecies(
    'sulfate',
    pc.Lognorm(mu=0.015, sigma=1.6, N=850.),
    kappa=0.54, bins=200
)

sea_salt = pc.AerosolSpecies(
    'sea salt',
    pc.Lognorm(mu=0.85, sigma=1.2, N=10.),
    kappa=1.2, bins=200
)

def ccn_spectrum(species, S, rs):
    Nc = []
    
    for s in S:
        res = pc.binned_activation([species], Smax=s, T=274.0, rs=rs)
        Nc.append(res.Nc)   # total activated CCN
    return np.array(Nc)

# Compute CCN spectra
ccn_sulfate = ccn_spectrum(sulfate, S)
ccn_seasalt = ccn_spectrum(sea_salt, S)

# Plot
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
