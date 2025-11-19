import numpy as np
import matplotlib.pyplot as plt
import pyrcel as pm

# 1. Définir une distribution lognormale d'aérosols
# Exemple : N = 1000 cm-3, r_g = 0.05 µm, sigma = 1.5
aero = pm.Lognorm(N=1000, r_g=0.05e-6, sigma=1.5, kappa=0.3)

# 2. Créer une population (ensemble de particules)
pop = pm.AerosolSpecies('sulfate',
                             pm.Lognorm(mu=0.025, sigma=1.3, N=2000.),
                             bins=200, kappa=0.54)

# 3. Définir un range de supersaturations (de 0.01% à 1%)
S = np.logspace(-4, -0.0, 50)   # supersaturation en fraction (0.0001 = 0.01%)

# 4. Calculer le CCN activé à chaque supersaturation
CCN = [pop.CCN(s) for s in S]

# 5. Plot
plt.figure()
plt.semilogx(S * 100, CCN)  # en %, plus lisible
plt.xlabel("Supersaturation (%)")
plt.ylabel("CCN concentration (#/cm³)")
plt.title("CCN activation curve from Pyrcel")
plt.grid(True)
plt.show()