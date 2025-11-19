import numpy as np
import matplotlib.pyplot as plt

import particula as par
import pyrcel as pm

# --- 1) Construire un système Particula --- #

# Exemple simple : une espèce de vapeur organique (comme dans l'exemple Particula)
organic = (
    par.gas.GasSpeciesBuilder()
    .set_name("organic")
    .set_molar_mass(180e-3, "kg/mol")
    .set_vapor_pressure_strategy(par.gas.ConstantVaporPressureStrategy(1e2))  # Pa
    .set_partitioning(True)
    .set_concentration(np.array([1e2]), "kg/m^3")
    .build()
)

atmosphere = (
    par.gas.AtmosphereBuilder()
    .set_temperature(298.15, "K")
    .set_pressure(101325, "Pa")
    .set_more_partitioning_species(organic)
    .build()
)

particle_dist = (
    par.particles.PresetParticleRadiusBuilder()
    .set_mode(np.array([100e-9]), "m")             # rayon modal
    .set_geometric_standard_deviation(np.array([1.2]))
    .set_number_concentration(np.array([1e8]), "1/m^3")
    .set_density(1e3, "kg/m^3")
    .build()
)

aerosol = (
    par.AerosolBuilder()
    .set_atmosphere(atmosphere)
    .set_particles(particle_dist)
    .build()
)

# --- 2) Extraire les propriétés particulaires pour Pyrcel --- #

# On va récupérer la “distribution” du rayon (ou des rayons) de Particula.
# Particula ne donne pas directement une “distribution lognormale classique” à Pyrcel,
# donc il faut approximer. Ici, on fait une approximation simple : on prend le rayon modal
# et la sigma géométrique qu'on a défini plus haut.

# Rayon modal en m
r_mode = 100e-9  
# GSD (geometric standard deviation)
gsd = 1.2  

# Concentration de particules
n_conc = 1e8  # #/m3 dans Particula

# On choisit aussi un κ (kappa) pour la théorie de Köhler : ici, à titre d'exemple
kappa = 0.3  

# --- 3) Créer une espèce d’aérosol Pyrcel --- #

# Pyrcel définit une AerosolSpecies : nom, distribution (lognormale), nombre de bins, kappa
aero_species = pm.AerosolSpecies(
    species="organique_particula",
    distribution=pm.Lognorm(mu=r_mode, sigma=gsd, N=n_conc),
    kappa=kappa,
    bins=200,
    r_min=50e-9,
    r_max=1e-6
)

# --- 4) Instancier le modèle de parcel Pyrcel --- #

# Conditions initiales du parcel
P0 = 101325.0      # pression initiale [Pa]
T0 = 298.15        # température initiale [K]
S0 = 0.0           # supersaturation initiale (0 = équilibre, pas de sursaturation)
V = 1.0            # vitesse d'ascension (par exemple 1 m/s, mais adapter selon ton cas)

# Créer le modèle
parcel = pm.ParcelModel(
    aerosols=[aero_species],
    V=V,
    T0=T0,
    S0=S0,
    P0=P0,
    console=True,            # affiche des infos pendant la simulation
    accom=1.0,                # coefficient d’accommodation
    truncate_aerosols=False
)

# --- 5) Lancer la simulation --- #

# Temps d’intégration (par exemple 300 s = 5 minutes)
t_final = 300.0  
dt = 1.0  # pas de temps [s]

# Run
res = parcel.run(z_top=None, time=t_final, dt=dt)

# `res` est typiquement un objet netCDF ou une structure avec les résultats du parcel
# On peut extraire par exemple la supersaturation, la température, la pression, etc.

# Voici comment extraire quelques variables :
time = res["time"]           # tableau des temps
S = res["S"]                 # supersaturation au cours du temps
T = res["T"]                 # température
P = res["P"]                 # pression

# --- 6) Tracer des résultats --- #

plt.figure()
plt.plot(time, S * 100)  # convertir fraction en pourcentage
plt.xlabel("Temps (s)")
plt.ylabel("Supersaturation (%)")
plt.title("Évolution de la sursaturation dans le parcel (Pyrcel)")
plt.grid(True)

plt.figure()
plt.plot(time, T)
plt.xlabel("Temps (s)")
plt.ylabel("Température (K)")
plt.title("Évolution de la température dans le parcel")
plt.grid(True)

plt.figure()
plt.plot(time, P / 100)  # par exemple en hPa
plt.xlabel("Temps (s)")
plt.ylabel("Pression (hPa)")
plt.title("Évolution de la pression dans le parcel")
plt.grid(True)

plt.show()
