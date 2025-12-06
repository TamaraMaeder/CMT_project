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
import csv

class AerosolMode:
    def __init__(self, name, origin, N0_cm3, Dg_um, sigma_g, kappa):
        self.name = str(name)
        self.origin = str(origin)
        self.N0_cm3 = float(N0_cm3)
        self.Dg_um = float(Dg_um)      # median diameter in µm (as in aerosol_modes.csv)
        self.sigma_g = float(sigma_g)
        self.kappa = float(kappa)

    def __repr__(self):
        return (f"AerosolMode(name={self.name!r}, N0_cm3={self.N0_cm3}, "
                f"Dg_um={self.Dg_um}, sigma_g={self.sigma_g}, kappa={self.kappa})")

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
            ))
    return modes

# chemin vers le CSV (même dossier que le script)
csv_path = os.path.join(os.path.dirname(__file__), 'aerosol_modes.csv')
modes = load_aerosol_modes_csv(csv_path)
print("Loaded aerosol modes:", modes)

def height_vs_radius(modes: list, first: str, second: str, P=77500., T=274., S=-0.02) -> None:
    """This fonction plots the height vs the supersaturation and the droplet radius of two aerosols. 
    - modes: list of AerosolMode instances (from load_aerosol_modes_csv)
    - first, second: names of the two modes to compare (case-insensitive)
    The plots of number concentration vs dry radius and number per bin vs dry radius are generated. 
    The dict aer2 contains the following keys: name,origin,N0_cm3,Dg_um,sigma_g,kappa. 
    The meteological conditions can be modified if necessary. By default, they are:  
    P = 77500. # Pressure, Pa
    T = 274.   # Temperature, K
    S = -0.02  # Supersaturation, 1-RH (98% here)
    It returns nothing but creates two csv files with the name of the aerosol and the values 
    of aerosol number concentration and the droplet raduis associated for each height. 
    """ 
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


def logn_size_dist_compare_plot(modes: list, first: str, second: str, P=77500., T=274., S=-0.02) -> None:
    """
    Compare two aerosol modes (by name) from 'modes' using Pyrcel and Particula.
    - modes: list of AerosolMode instances (from load_aerosol_modes_csv)
    - first, second: names of the two modes to compare (case-insensitive)
    The plots of number concentration vs dry radius and number per bin vs dry radius are generated. 
    The dict aer2 contains the following keys: name,origin,N0_cm3,Dg_um,sigma_g,kappa. 
    The meteological conditions can be modified if necessary. By default, they are:  
    P = 77500. # Pressure, Pa
    T = 274.   # Temperature, K
    S = -0.02  # Supersaturation, 1-RH (98% here)
    """

    print("Please notice that Particula and Pyrcel have two different y-axis values even because " \
    "Particula gives a probability mass function while Pyrcel gives a number concentration." \
    "It means that Particula is plotting normalized values (area under the curve equals number of particles N)," \
    " but not scaled to concentration units.")

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

    if mode1 is None:
        raise ValueError(f"Mode '{first}' not found in modes (available: {[m.name for m in modes]})")
    if mode2 is None:
        raise ValueError(f"Mode '{second}' not found in modes (available: {[m.name for m in modes]})")

    print(f"Plotting modes: {mode1.name} and {mode2.name}")

    # Create Pyrcel AerosolSpecies objects
    # Pyrcel often expects mu as radius (µm) -> use Dg_um/2
    pyrcel_mu1 = mode1.Dg_um / 2.0
    pyrcel_mu2 = mode2.Dg_um / 2.0

    species1 = pc.AerosolSpecies(mode1.name,
                                pc.Lognorm(mu=pyrcel_mu1, sigma=mode1.sigma_g, N=mode1.N0_cm3),
                                kappa=mode1.kappa, bins=200)
    species2 = pc.AerosolSpecies(mode2.name,
                                pc.Lognorm(mu=pyrcel_mu2, sigma=mode2.sigma_g, N=mode2.N0_cm3),
                                kappa=mode2.kappa, bins=200)

    # ---------- Figure 1 : Pyrcel (bars) ----------
    fig1 = plt.figure(figsize=(10, 5))
    ax1 = fig1.add_subplot(111)
    ax1.grid(False, "minor")

    col1 = "#CC0066"
    col2 = "#0099FF"

    ax1.bar(species1.rs[:-1], species1.Nis * 1e-6, np.diff(species1.rs),
            color=col1, label=f"{mode1.name} (Pyrcel)", edgecolor=col1, alpha=0.7)
    ax1.bar(species2.rs[:-1], species2.Nis * 1e-6, np.diff(species2.rs),
            color=col2, label=f"{mode2.name} (Pyrcel)", edgecolor=col2, alpha=0.7)

    ax1.set_xscale('log')
    ax1.set_xlabel("Aerosol dry radius, micron")
    ax1.set_ylabel("Pyrcel Aerosol number conc., cm$^{-3}$")
    ax1.set_title(f"Pyrcel: {mode1.name} vs {mode2.name}")
    ax1.legend(loc='upper right')

    # show first figure
    fig1.tight_layout()
    fig1_path = os.path.join(os.path.dirname(__file__), 'pyrcel_comparison.png')
    fig1.savefig(fig1_path, dpi=150, bbox_inches='tight')
    print(f"Saved Pyrcel figure -> {fig1_path}")
    fig1.show()

    # ---------- Figure 2 : Particula (PMF lines) ----------
    # Particula routines need diameters as input, but we will plot against radius
    x_diam = np.logspace(-3, 1, 2000)  # diameters [µm]
    r_vals = x_diam / 2.0              # convert diameters -> radii [µm] for plotting

    # compute PMFs with Particula (pass diameters as required)
    pmf1 = pm.particles.get_lognormal_pmf_distribution(
        x_diam, np.array([mode1.Dg_um]), np.array([mode1.sigma_g]), np.array([mode1.N0_cm3])
    )
    pmf2 = pm.particles.get_lognormal_pmf_distribution(
        x_diam, np.array([mode2.Dg_um]), np.array([mode2.sigma_g]), np.array([mode2.N0_cm3])
    )

    fig2 = plt.figure(figsize=(10, 6))
    ax2 = fig2.add_subplot(111)
    ax2.grid(True, "both")

    # colors for Particula lines (kept distinct / slightly darker for contrast)
    col1_part = "#8B004D"
    col2_part = "#0077CC"

    # Plot PMF vs RADIUS so it lines up with Pyrcel bars (which use rs in µm)
    ax2.plot(r_vals, pmf1, color=col1_part, linestyle='-', linewidth=2.5,
             label=f"{mode1.name} (Particula)")
    ax2.plot(r_vals, pmf2, color=col2_part, linestyle='--', linewidth=2.5,
             label=f"{mode2.name} (Particula)")

    ax2.set_xscale('log')
    ax2.set_xlabel("Particle radius (μm)")   # now radius, consistent with Pyrcel
    ax2.set_ylabel("Particula PMF (arbitrary units)")
    # force bottom of Particula axis to 0 (like Pyrcel)
    ymin, ymax = ax2.get_ylim()
    ax2.set_ylim(0, ymax)

    ax2.set_title(f"Particula: {mode1.name} vs {mode2.name} (plotted vs radius)")
    ax2.legend(loc='upper right')

    fig2.tight_layout()
    fig2_path = os.path.join(os.path.dirname(__file__), 'particula_comparison.png')
    fig2.savefig(fig2_path, dpi=150, bbox_inches='tight')
    print(f"Saved Particula figure -> {fig2_path}")
    fig2.show()

    return None