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
    
    # Import AerosolMode class if not already in scope
    import pyrcel as pc
    from pyrcel import binned_activation
    
    def find_mode_by_name(name):
        key = name.strip().lower()
        for m in modes:
            n = m.name.replace('_', ' ').lower()
            if n == key or m.name.lower() == key or m.name.replace('_', '').lower() == key.replace(' ', ''):
                return m
        return None
    
    mode1 = find_mode_by_name(first)
    mode2 = find_mode_by_name(second)
    
    if mode1 is None:
        raise ValueError(f"Mode '{first}' not found")
    if mode2 is None:
        raise ValueError(f"Mode '{second}' not found")
    
    print(f"Running parcel model with {mode1.name} and {mode2.name}")
    print(f"mode1:", mode1)
    print(f"mode2:", mode2)
    print(f"  P={P} Pa, T0={T0} K, S0={S0}, V={V} m/s, t_end={t_end} s")
    
    # Create Pyrcel AerosolSpecies objects
    pyrcel_mu1 = mode1.Dg_um / 2.0
    pyrcel_mu2 = mode2.Dg_um / 2.0
    
    species1 = pc.AerosolSpecies(mode1.name,
                                pc.Lognorm(mu=pyrcel_mu1, sigma=mode1.sigma_g, N=mode1.N0_cm3),
                                kappa=mode1.kappa, bins=200)
    species2 = pc.AerosolSpecies(mode2.name,
                                pc.Lognorm(mu=pyrcel_mu2, sigma=mode2.sigma_g, N=mode2.N0_cm3),
                                kappa=mode2.kappa, bins=200)
    
    initial_aerosols = [species1, species2]
    print(f"initial_aerosols=", initial_aerosols)

    # Run parcel model
    model = pc.ParcelModel(initial_aerosols, V, T0, S0, P, console=False, accom=0.3)
    parcel_trace, aerosol_traces = model.run(t_end, dt, solver='cvode')
    
    # ---------- Figure 1 : Height vs Supersaturation + Droplet Radius ----------
    fig1, (ax_S, ax_r) = plt.subplots(1, 2, figsize=(12, 5), sharey=True)
    
    # Left plot: Supersaturation vs height
    ax_S.plot(parcel_trace['S']*100., parcel_trace['z'], color='k', lw=2)
    ax_S_T = ax_S.twiny()
    ax_S_T.plot(parcel_trace['T'], parcel_trace['z'], color='r', lw=1.5)
    
    Smax = parcel_trace['S'].max() * 100
    z_at_smax = parcel_trace['z'].iloc[parcel_trace['S'].argmax()]
    ax_S.annotate("max S = %0.2f%%" % Smax,
                  xy=(Smax, z_at_smax),
                  xytext=(Smax-0.3, z_at_smax+50.),
                  arrowprops=dict(arrowstyle="->", color='k',
                                  connectionstyle='angle3,angleA=0,angleB=90'),
                  zorder=10)
    
    ax_S.set_xlim(0, 0.7)
    # ax_S.set_ylim(0, t_end * V / 1.2)  # Scale to max height
    ax_S.set_ylim(0,250)
    ax_S_T.set_xticks([T0-2, T0-1, T0, T0+1])
    ax_S_T.xaxis.label.set_color('red')
    ax_S_T.tick_params(axis='x', colors='red')
    ax_S.set_xlabel("Supersaturation, %")
    ax_S_T.set_xlabel("Temperature, K")
    ax_S.set_ylabel("Height, m")
    ax_S.grid(False, 'both')
    
    # Right plot: Droplet radius vs height for both aerosols
    aerosol_1_array = aerosol_traces[mode1.name].values
    aerosol_2_array = aerosol_traces[mode2.name].values
    
    col1 = "#CC0066"
    col2 = "#0099FF"
    
    ss = ax_r.plot(aerosol_1_array[:, ::10]*1e6, parcel_trace['z'], color=col1)
    sa = ax_r.plot(aerosol_2_array[:, ::10]*1e6, parcel_trace['z'], color=col2)
    ax_r.plot([], [], color=col1, label=mode1.name)
    ax_r.plot([], [], color=col2, label=mode2.name)
    ax_r.set_xscale('log')
    ax_r.set_xlim(1e-2, 100.)
    ax_r.legend(loc='upper right')
    ax_r.set_xlabel("Droplet radius, µm")
    ax_r.grid(False, 'both')
    
    fig1.tight_layout()
    fname = f"height_vs_{mode1.name.replace(' ', '_')}_vs_{mode2.name.replace(' ', '_')}.png"
    fig1_path = os.path.join(os.path.dirname(__file__), fname)
    fig1.savefig(fig1_path, dpi=150, bbox_inches='tight')
    print(f"Saved figure -> {fig1_path}")
    fig1.show()
    
    # ---------- Export CSV data for each aerosol ----------
    ind_final = int(t_end/dt) - 1
    T_final = parcel_trace['T'].iloc[ind_final]
    
    for aerosol_obj, aerosol_name in [(species1, mode1.name), (species2, mode2.name)]:
        # Get trace for this aerosol
        aerosol_trace = aerosol_traces[aerosol_name]
        
        # Calculate representative radius per bin
        r_rep = 0.5 * (aerosol_obj.rs[:-1] + aerosol_obj.rs[1:])
        
        # Build CSV rows: height, concentration per bin, radius per bin
        csv_rows = []
        
        for time_idx, z_val in enumerate(parcel_trace['z']):
            # r_wet at a certain time
            r_wet_bins = aerosol_trace.iloc[time_idx].values  # in meters 
            
            # Nis doesn't depend on time
            conc_bins = aerosol_obj.Nis 

            for bin_idx, (r_wet, conc_val) in enumerate(zip(r_wet_bins, conc_bins)):
                csv_rows.append({
                    'height_m': z_val,
                    'bin_index': bin_idx,
                    'r_wet_m': r_wet,
                    'number_concentration_m3': conc_val,
                })
        
        # Save to CSV
        csv_df = pd.DataFrame(csv_rows)
        csv_path = os.path.join(os.path.dirname(__file__), f'{aerosol_name.replace(" ", "_")}_profile.csv')
        csv_df.to_csv(csv_path, index=False)
        print(f"Saved {aerosol_name} profile -> {csv_path}")
    
    return None

print(plot_height_vs_aerosol(modes,"Sulfate_accum","Sea_salt_coarse"))
