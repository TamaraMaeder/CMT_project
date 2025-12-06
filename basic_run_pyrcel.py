# Suppress warnings
import warnings
warnings.simplefilter('ignore')
import os
os.environ['OMP_NESTED'] = 'FALSE'  # Désactive OpenMP warning
import pyrcel as pc
import numpy as np
import matplotlib.pyplot as plt
from pyrcel import binned_activation
import pandas as pd
import csv

# sulfate =  pm.AerosolSpecies('sulfate',
#                              pm.Lognorm(mu=0.015, sigma=1.6, N=850.),
#                              kappa=0.54, bins=200)
# sea_salt = pm.AerosolSpecies('sea salt',
#                              pm.Lognorm(mu=0.85, sigma=1.2, N=10.),
#                              kappa=1.2, bins=40)

# P0 = 77500. # Pressure, Pa
# T0 = 274.   # Temperature, K
# S0 = -0.02  # Supersaturation, 1-RH (98% here)
# sul_c = "#CC0066"
# sea_c = "#0099FF"

# initial_aerosols = [sulfate, sea_salt]
# V = 1.0 # updraft speed, m/s

# dt = 1.0 # timestep, seconds
# t_end = 250./V # end time, seconds... 250 meter simulation

# model = pm.ParcelModel(initial_aerosols, V, T0, S0, P0, console=False, accom=0.3)
# parcel_trace, aerosol_traces = model.run(t_end, dt, solver='cvode')

# fig, [axS, axA] = plt.subplots(1, 2, figsize=(10, 4), sharey=True)

# axS.plot(parcel_trace['S']*100., parcel_trace['z'], color='k', lw=2)
# axT = axS.twiny()
# axT.plot(parcel_trace['T'], parcel_trace['z'], color='r', lw=1.5)

# Smax = parcel_trace['S'].max()*100
# z_at_smax = parcel_trace['z'].iloc[parcel_trace['S'].argmax()]
# axS.annotate("max S = %0.2f%%" % Smax,
#              xy=(Smax, z_at_smax),
#              xytext=(Smax-0.3, z_at_smax+50.),
#              arrowprops=dict(arrowstyle="->", color='k',
#                              connectionstyle='angle3,angleA=0,angleB=90'),
#              zorder=10)

# axS.set_xlim(0, 0.7)
# axS.set_ylim(0, 250)

# axT.set_xticks([270, 271, 272, 273, 274])
# axT.xaxis.label.set_color('red')
# axT.tick_params(axis='x', colors='red')

# axS.set_xlabel("Supersaturation, %")
# axT.set_xlabel("Temperature, K")
# axS.set_ylabel("Height, m")

# sulf_array = aerosol_traces['sulfate'].values
# sea_array = aerosol_traces['sea salt'].values

# ss = axA.plot(sulf_array[:, ::10]*1e6, parcel_trace['z'], color=sul_c,
#          label="sulfate")
# sa = axA.plot(sea_array*1e6, parcel_trace['z'], color=sea_c, label="sea salt")
# axA.set_xscale('log')
# axA.set_xlim(1e-2, 10.)
# axA.set_xticks([1e-2, 1e-1, 1e0, 1e1], [0.01, 0.1, 1.0, 10.0])
# axA.legend([ss[0], sa[0]], ['sulfate', 'sea salt'], loc='upper right')
# axA.set_xlabel("Droplet radius, micron")

# for ax in [axS, axA, axT]:
#     ax.grid(False, 'both', 'both')

# #CDND 
# sulf_trace = aerosol_traces['sulfate']
# sea_trace = aerosol_traces['sea salt']

# ind_final = int(t_end/dt) - 1

# T = parcel_trace['T'].iloc[ind_final]
# eq_sulf, kn_sulf, alpha_sulf, phi_sulf = \
#     binned_activation(Smax/100, T, sulf_trace.iloc[ind_final],  sulfate)
# eq_sulf *= sulfate.total_N

# eq_sea, kn_sea, alpha_sea, phi_sea = \
#     binned_activation(Smax/100, T, sea_trace.iloc[ind_final], sea_salt)
# eq_sea *= sea_salt.total_N

# print("  CDNC(sulfate) = {:3.1f}".format(eq_sulf))
# print(" CDNC(sea salt) = {:3.1f}".format(eq_sea))
# print("------------------------")
# print("          total = {:3.1f} / {:3.0f} ~ act frac = {:1.2f}".format(
#       eq_sulf+eq_sea,
#       sea_salt.total_N+sulfate.total_N,
#       (eq_sulf+eq_sea)/(sea_salt.total_N+sulfate.total_N)
# ))


# ## Create a csv that contains the bins radius 

# bin_rows = []

# for aerosol in [sulfate, sea_salt]:

#     # representative radius (microns)
#     r_rep = 0.5 * (aerosol.rs[:-1] + aerosol.rs[1:])
#     r_rep_micron = r_rep  # µm

#     # Concentration per bin (converted in cm^-3)
#     N_bin_cm3 = aerosol.Nis * 1e-6

#     for r, N in zip(r_rep_micron, N_bin_cm3):
#         bin_rows.append({
#             "species": aerosol.species,            #  <<<<< FIX !
#             "radius_micron": r,
#             "concentration_cm^-3": N    
#         })

# # DataFrame
# bins_df = pd.DataFrame(bin_rows)

# # Export CSV in the fill of the script 
# bin_csv = os.path.join(os.path.dirname(__file__), 'bins.csv')
# bins_df.to_csv(bin_csv, index=False)

# print(f"\nSaved bin data -> {bin_csv}")
# print(bins_df.head())

# plt.tight_layout()
# plt.show()

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


def plot_height_vs_aerosol(modes, first: str, second: str, P=77500., T0=274., S0=-0.02, V=1.0, t_end=250., dt=1.0):
    """
    Plot height vs supersaturation and droplet radius for two aerosol modes from a parcel model run.
    
    - modes: list of AerosolMode instances (from load_aerosol_modes_csv)
    - first, second: names of the two modes to compare (case-insensitive)
    - P: Pressure (Pa), default 77500
    - T0: Initial Temperature (K), default 274
    - S0: Initial Supersaturation (1-RH), default -0.02
    - V: Updraft speed (m/s), default 1.0
    - t_end: End time (seconds), default 250 (or 250/V meters)
    - dt: Timestep (seconds), default 1.0
    
    Returns: None
    Creates two PNG figures and two CSV files (one per aerosol) with height, concentration, and radius data.
    """

    # Helper : find aerosol mode

    def find_mode_by_name(name):
        key = name.strip().lower()
        for m in modes:
            if m.name.lower() == key or m.name.replace("_", " ").lower() == key:
                return m
        return None

    mode1 = find_mode_by_name(first)
    mode2 = find_mode_by_name(second)

    if mode1 is None or mode2 is None:
        raise ValueError("Aerosol mode not found.")

    print(f"\nRunning parcel simulation with {mode1.name} and {mode2.name}")

    # Pyrcel parameters : conversion median diameter -> mu parameter
    mu1 = mode1.Dg_um / 2.0
    mu2 = mode2.Dg_um / 2.0

    # Species: depedning on the specie, the number of bin can very 
    # if you have a high number concentration 
    species1 = pc.AerosolSpecies(
        mode1.name,
        pc.Lognorm(mu=mu1, sigma=mode1.sigma_g, N=mode1.N0_cm3),
        kappa=mode1.kappa, bins=int(mode1.nb_bins)
    )
    species2 = pc.AerosolSpecies(
        mode2.name,
        pc.Lognorm(mu=mu2, sigma=mode2.sigma_g, N=mode2.N0_cm3),
        kappa=mode2.kappa, bins=int(mode2.nb_bins)
    )


    # Run parcel model
    model = pc.ParcelModel([species1, species2], V, T0, S0, P, console=False, accom=0.3)
    parcel_trace, aerosol_traces = model.run(t_end, dt, solver="cvode")

    z = parcel_trace["z"]
    S = parcel_trace["S"] * 100.0  # %
    T = parcel_trace["T"]

    # # Number of bins
    # n1 = wet_r1.shape[1]
    # n2 = wet_r2.shape[1]

    # ----------------------------------
    # FIGURE : Height vs S + wet radii
    # ----------------------------------
    fig, (axS, axR) = plt.subplots(1, 2, figsize=(12, 5), sharey=True)

    # --- Left panel : Supersaturation ---
    axS.plot(S, z, color='k', lw=2)
    axS_T = axS.twiny()
    axS_T.plot(T, z, color='r', lw=1.5)

    Smax = parcel_trace['S'].max()*100
    z_at_smax = parcel_trace['z'].iloc[parcel_trace['S'].argmax()]
    axS.annotate("max S = %0.2f%%" % Smax,
             xy=(Smax, z_at_smax),
             xytext=(Smax-0.3, z_at_smax+50.),
             arrowprops=dict(arrowstyle="->", color='k',
                             connectionstyle='angle3,angleA=0,angleB=90'),
             zorder=10)

    axS.set_xlim(0, 0.7)
    axS.set_ylim(0, 250)
    axS.set_xlabel("Supersaturation (%)")
    axS.set_ylabel("Height (m)")
    #axS.set_xlim(0, 0.7)
    axS_T.set_xticks([270, 271, 272, 273, 274])
    axS_T.xaxis.label.set_color('red')
    axS_T.tick_params(axis='x', colors='red')
    axS_T.set_xlabel("Temperature (K)", color="r")

    # --- Right panel : Droplet radii ---
    col1 = "#CC0066"
    col2 = "#0099FF"

   # Right panel: radius vs height
    # Retrieve wet radii (in meters), convert to µm
    aerosol1_array = aerosol_traces[mode1.name].values
    aerosol2_array = aerosol_traces[mode2.name].values

    # for b in range(0, n1, 10):
    #     axR.plot(wet_r1[:, b], z, color=col1, alpha=0.7)

    # for b in range(0, n2, 10):
    #     axR.plot(wet_r2[:, b], z, color=col2, alpha=0.7)

    # depending on the number of bin for each aerosol, it is possible to take less lines 
    #for example for the sulfate, there are 200 bins so it is possible
    ss = axR.plot(aerosol1_array[:, ::10]*1e6, parcel_trace['z'], color=col1,
         label=mode1.name)
    sa = axR.plot(aerosol2_array*1e6, parcel_trace['z'], color=col2, label=mode2.name)
    axR.semilogx()
    axR.set_xlim(1e-2, 10.)
    axR.set_xticks([1e-2, 1e-1, 1e0, 1e1], [0.01, 0.1, 1.0, 10.0])
    axR.legend([ss[0], sa[0]], [mode1.name, mode2.name], loc='upper right')
    axR.set_xlabel("Droplet radius, micron")

# for ax in [axS, axA, axT]:
#     ax.grid(False, 'both', 'both')
#     axR.set_xscale("log")
#     axR.set_xlim(1e-2, 20)  # correct pyrcel wet radius scale
#     axR.set_xlabel("Droplet radius (µm)")
#     axR.legend()

    fig.tight_layout()

    fig_path = os.path.join(os.path.dirname(__file__), "height_vs_aerosol.png")
    fig.savefig(fig_path, dpi=150, bbox_inches="tight")
    print(f"Saved figure → {fig_path}")

    # ---------------------------
    # CSV export with wet radii
    # ---------------------------
    for sp, wet_r, name in [(species1, aerosol1_array*1e6, mode1.name),
                            (species2,aerosol2_array*1e6, mode2.name)]:

        r_rep = 0.5 * (sp.rs[:-1] + sp.rs[1:]) * 1e6  # µm

        rows = []
        for ti, height in enumerate(z):
            conc = aerosol_traces[name].iloc[ti].values
            for ri, rv in enumerate(r_rep):
                rows.append({
                    "height_m": height,
                    "wet_radius_micron": rv,
                    "concentration_m3": conc[ri]
                })

        df = pd.DataFrame(rows)
        out = os.path.join(os.path.dirname(__file__), f"{name}_profile.csv")
        df.to_csv(out, index=False)
        print(f"Saved CSV → {out}")

    return None

print(plot_height_vs_aerosol(modes,'Sulfate_accum','Sea_salt_coarse'))
