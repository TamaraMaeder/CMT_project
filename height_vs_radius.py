# Suppress warnings
import warnings
warnings.simplefilter('ignore')
import os
os.environ['OMP_NESTED'] = 'FALSE'  # Desactive OpenMP warning

import pyrcel as pm
import numpy as np

import matplotlib.pyplot as plt
from pyrcel import binned_activation

import pandas as pd

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


fig, [axS, axA] = plt.subplots(1, 2, figsize=(10, 4), sharey=True)
sul_c = "#CC0066"
sea_c = "#0099FF"

axS.plot(parcel_trace['S']*100., parcel_trace['z'], color='k', lw=2)
axT = axS.twiny()
axT.plot(parcel_trace['T'], parcel_trace['z'], color='r', lw=1.5)

Smax = parcel_trace['S'].max()*100
#z_at_smax = parcel_trace['z'].ix[parcel_trace['S'].argmax()]
z_at_smax = parcel_trace['z'].loc[parcel_trace['S'].idxmax()]

axS.annotate("max S = %0.2f%%" % Smax,
             xy=(Smax, z_at_smax),
             xytext=(Smax-0.3, z_at_smax+50.),
             arrowprops=dict(arrowstyle="->", color='k',
                             connectionstyle='angle3,angleA=0,angleB=90'),
             zorder=10)

axS.set_xlim(0, 0.7)
axS.set_ylim(0, 250)

axT.set_xticks([270, 271, 272, 273, 274])
axT.xaxis.label.set_color('red')
axT.tick_params(axis='x', colors='red')

axS.set_xlabel("Supersaturation, %")
axT.set_xlabel("Temperature, K")
axS.set_ylabel("Height, m")

sulf_array = aerosol_traces['sulfate'].values #wet radius in meters in aerosol_trace
sea_array = aerosol_traces['sea salt'].values

ss = axA.plot(sulf_array[:, ::10]*1e6, parcel_trace['z'], color=sul_c, # values of radius * 10^6 to use micrometers
         label="sulfate")
sa = axA.plot(sea_array*1e6, parcel_trace['z'], color=sea_c, label="sea salt")
axA.semilogx()
axA.set_xlim(1e-2, 10.)
axA.set_xticks([1e-2, 1e-1, 1e0, 1e1], [0.01, 0.1, 1.0, 10.0])
axA.legend([ss[0], sa[0]], ['sulfate', 'sea salt'], loc='upper right')
axA.set_xlabel("Droplet radius, micron")

for ax in [axS, axA, axT]:
    ax.grid(False, 'both', 'both')

plt.tight_layout()
plt.show()

