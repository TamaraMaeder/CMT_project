# Suppress warnings
import warnings
warnings.simplefilter('ignore')
import os
os.environ['OMP_NESTED'] = 'FALSE'  # Desactive OpenMP warning
import pyrcel as pc
import numpy as np
import matplotlib.pyplot as plt
from pyrcel import binned_activation
import pandas as pd
import csv

class AerosolMode:
    def __init__(self, name, origin, N0_cm3, Dg_um, sigma_g, kappa,nb_bins):
        self.name = str(name)
        self.origin = str(origin)
        self.N0_cm3 = float(N0_cm3)
        self.Dg_um = float(Dg_um)      # median diameter in µm (as in aerosol_modes.csv)
        self.sigma_g = float(sigma_g)
        self.kappa = float(kappa)
        self.nb_bins =float(nb_bins)

    def __repr__(self):
        return (f"AerosolMode(name={self.name!r}, N0_cm3={self.N0_cm3}, "
                f"Dg_um={self.Dg_um}, sigma_g={self.sigma_g}, kappa={self.kappa}, nb_bins={self.nb_bins})")

def load_aerosol_modes_csv(path):
    modes = []
    with open(path, newline='', encoding='utf-8') as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            modes.append(AerosolMode(
                row.get('name', '').strip(),
                row.get('origin', '').strip(),
                row.get('N0_cm3', '0'),
                row.get('Dg_um', '0'),
                row.get('sigma_g', '1.0'),
                row.get('kappa', '0.0'),
                int(row.get('nb_bins', '0')),
            ))
    return modes

# chemin vers le CSV (même dossier que le script)
csv_path = os.path.join(os.path.dirname(__file__), 'aerosol_modes.csv')
modes = load_aerosol_modes_csv(csv_path)
print("Loaded aerosol modes:", modes)

def calcul_CDNC(modes: list, first: str, second: str, P=77500., T0=274., S0=-0.02, V=1.0, t_end=250., dt=1.0):
    """
    Calculates the Cloud Droplet Number Concentration for a certain type of aerosol.
    - modes: list of AerosolMode instances
    - first, second: names of the mode of the simulation for which we want the CDNC
    The meteological conditions can be modified if necessary. By default, they are:  
    P = 77500. # Pressure, Pa
    T0 = 274.   # Temperature, K
    S0 = -0.02  # Supersaturation, 1-RH (98% here)
    V: Updraft speed (m/s), default 1.0
    t_end: End time (seconds), default 250 (or 250/V meters)
    dt: Timestep (seconds), default 1.0
    """

    def find_mode_by_name(name):
        key = name.strip().lower()
        for m in modes:
            # match ignoring underscores/spaces and case
            n = m.name.replace('_', ' ').lower()
            if n == key or m.name.lower() == key or m.name.replace('_', '').lower() == key.replace(' ', ''):
                return m
        return None

    mode1 = find_mode_by_name(first)
    mode2 = find_mode_by_name(second)
    
    pyrcel_mu1 = mode1.Dg_um / 2.0
    pyrcel_mu2 = mode2.Dg_um / 2.0

    species_1 = pc.AerosolSpecies(mode1.name,pc.Lognorm(mu=pyrcel_mu1, sigma=mode1.sigma_g, N=mode1.N0_cm3),
                                kappa=mode1.kappa, bins=int(mode1.nb_bins))
    species_2 = pc.AerosolSpecies(mode2.name,pc.Lognorm(mu=pyrcel_mu2, sigma=mode2.sigma_g, N=mode2.N0_cm3),
                                kappa=mode2.kappa, bins=int(mode2.nb_bins))
    
    initial_species = [species_1,species_2]
    model = pc.ParcelModel(initial_species, V, T0, S0, P, console=False, accom=0.3)
    parcel_trace, aerosol_traces = model.run(t_end, dt, solver='cvode')
    specie1_trace= aerosol_traces[mode1.name]
    species2_trace=aerosol_traces[mode2.name]
    Smax = parcel_trace['S'].max()*100

    ind_final = int(t_end/dt) - 1
    T = parcel_trace['T'].iloc[ind_final]

    eq_1, kn_1, alpha_1, phi_1 = \
        binned_activation(Smax/100, T, specie1_trace.iloc[ind_final], species_1)
    eq_1 *= species_1.total_N

    eq_2, kn_2, alpha_2, phi_2 = \
        binned_activation(Smax/100, T, species2_trace.iloc[ind_final], species_2)
    eq_2 *= species_2.total_N

    print(f"CDNC {first}", " = {:3.1f}".format(eq_1))
    print(f"CDNC {second}", " = {:3.1f}".format(eq_2))
    print("          total = {:3.1f} / {:3.0f} ~ act frac = {:1.2f}".format(
        eq_1+eq_2,
        species_1.total_N+species_2.total_N,
        (eq_1+eq_2)/(species_1.total_N+species_2.total_N)
    ))
    return None 

#print (calcul_CDNC(modes,"sea_salt_coarse","Black_carbon_fine"))

