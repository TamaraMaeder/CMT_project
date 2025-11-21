#We will compare the different lognormal aerosol size distribution generated with pyrcel and particula.

import particula as pm 
import pyrcel as pc
import matplotlib.pyplot as plt
import numpy as np

# First let's do it with pyrcel: 

P0 = 77500. # Pressure, Pa
T0 = 274.   # Temperature, K
S0 = -0.02  # Supersaturation, 1-RH (98% here)

sulfate =  pc.AerosolSpecies('sulfate',
                             pc.Lognorm(mu=0.015, sigma=1.6, N=850.),
                             kappa=0.54, bins=200)
sea_salt = pc.AerosolSpecies('sea salt',
                             pc.Lognorm(mu=0.85, sigma=1.2, N=10.),
                             kappa=1.2, bins=40)


fig = plt.figure(figsize=(10,5))
ax = fig.add_subplot(111)
ax.grid(False, "minor")

sul_c = "#CC0066"
ax.bar(sulfate.rs[:-1], sulfate.Nis*1e-6, np.diff(sulfate.rs),
        color=sul_c, label="sulfate", edgecolor="#CC0066")
sea_c = "#0099FF"
ax.bar(sea_salt.rs[:-1], sea_salt.Nis*1e-6, np.diff(sea_salt.rs),
        color=sea_c, label="sea salt", edgecolor="#0099FF")
ax.set_xscale('log')

ax.set_xlabel("Aerosol dry radius, micron")
ax.set_ylabel("Aerosl number conc., cm$^{-3}$")
ax.legend(loc='upper right')

#plt.show()

#Then let's do it with Particula
# Define the x_values as a range of particle diameters
x_values = np.logspace(-3, 1, 2000)  # From 0.001 to 10 microns

# Single mode distribution
single_mode_gsd = np.array([1.4])
single_mode = np.array([0.02])
single_mode_nparticles = np.array([1e3])

single_mode_distribution = pm.particles.get_lognormal_pdf_distribution(
    x_values, single_mode, single_mode_gsd, single_mode_nparticles
)

# Multi-mode distribution
multi_mode_gsd = np.array([1.4, 1.8])
multi_mode = np.array([0.02, 1.0])
multi_mode_nparticles = np.array([1e3, 1e3])

multi_mode_distribution = pm.particles.get_lognormal_pdf_distribution(
    x_values, multi_mode, multi_mode_gsd, multi_mode_nparticles
)

#Probability Mass Function (PMF) for Aerosol Distributions
single_pmf_distribution = pm.particles.get_lognormal_pmf_distribution(
    x_values, single_mode, single_mode_gsd, single_mode_nparticles
)
multi_pmf_distribution = pm.particles.get_lognormal_pmf_distribution(
    x_values, multi_mode, multi_mode_gsd, multi_mode_nparticles
)

plt.figure(figsize=(10, 6))
plt.plot(x_values, single_pmf_distribution, label="Single Mode", linewidth=4)
plt.plot(x_values, multi_pmf_distribution, label="Multi Mode")
plt.title("Lognormal PMF Particle Size Distribution")
plt.xlabel("Particle Diameter (μm)")
plt.ylabel("Number of Particles")
plt.xscale("log")
plt.legend()
plt.grid(True)
plt.show()