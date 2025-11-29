import pyrcel as pm
import numpy as np
import matplotlib.pyplot as plt

# ----------1. BASIC RUN PYRCEL ----------
P0 = 77500. # Pressure, Pa
T0 = 274.   # Temperature, K
S0 = -0.02  # Supersaturation, 1-RH (98% here)

sulfate =  pm.AerosolSpecies('sulfate',
                             pm.Lognorm(mu=0.015, sigma=1.6, N=850.),
                             kappa=0.54, bins=200)
sea_salt = pm.AerosolSpecies('sea salt',
                             pm.Lognorm(mu=0.85, sigma=1.2, N=10.),
                             kappa=1.2, bins=40)

initial_aerosols = [sulfate, sea_salt]
V = 1.0 # updraft speed, m/s

dt = 1.0 # timestep, seconds
t_end = 250./V # end time, seconds... 250 meter simulation

model = pm.ParcelModel(initial_aerosols, V, T0, S0, P0, console=False, accom=0.3)
parcel_trace, aerosol_traces = model.run(t_end, dt, solver='cvode')

Smax = parcel_trace['S'].max()*100
z_at_smax = parcel_trace['z'].loc[parcel_trace['S'].idxmax()]

# ============================================================
# === CALCUL DU CONTENU EN EAU (LWC) ET LWP DU NUAGE =========
# ============================================================

rho_w = 1000.0  # kg/m3 densité de l'eau

# Données venant du modèle Pyrcel
z = parcel_trace['z'].values
r_wet_sea_salt = aerosol_traces['sea salt'].values     # rayon humide [time, bins] en m
r_wet_sulfate = aerosol_traces['sulfate'].values     # rayon humide [time, bins] en m
N_bin_cm3_sulfate = sulfate.Nis * 1e-6   # concentration #/cm3 [time, bins]
N_bin_cm3_sea_salt = sea_salt.Nis * 1e-6   # concentration #/cm3 [time, bins]

# Volume d'une goutte (pour chaque bin)
V_drop_sea_salt= (4.0/3.0) * np.pi * r_wet_sea_salt**3   # m3
V_drop_sulfate= (4.0/3.0) * np.pi * r_wet_sulfate**3   # m3

# === LWC pour chaque niveau (kg/m3)
LWC_sea_salt = rho_w * np.sum(N_bin_cm3_sea_salt * V_drop_sea_salt, axis=1)
LWC_sulfate = rho_w * np.sum(N_bin_cm3_sulfate * V_drop_sulfate, axis=1)

# === LWP : intégrale verticale de LWC (kg/m2)
LWP_sea_salt = np.trapz(LWC_sea_salt, z)
LWP_sulfate = np.trapz(LWC_sulfate, z)

print("LWP of sea salt =", LWP_sea_salt, "kg/m²")
print("LWP of sulfate =", LWP_sulfate, "kg/m²")

##graph 

plt.figure(figsize=(7,5))
plt.plot(LWC_sulfate, z, linewidth=2)

plt.xlabel("Liquid Water Content LWC (kg/m³)")
plt.ylabel("Height (m)")
plt.title("Vertical profile of LWC from Pyrcel parcel simulation")
plt.grid(True)

plt.show()