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
from pathlib import Path

from def_class import AerosolMode, load_aerosol_modes_csv
from logn_size_dist_compare import logn_size_dist_compare_plot

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA_DIR = os.path.join(PROJECT_ROOT, "data")
RESULTS_DIR = os.path.join(PROJECT_ROOT, "results")

def run_height_vs_N_sensitivity(modes, first: str, second: str, N_list: list, P=77500., T0=274., S0=-0.02, V=1.0, t_end=250., dt=1.0):
    """
    Run parcel simulations for sulfate + sea salt while varying number concentration N.
    One CSV is written per N value.

    - modes: list of AerosolMode instances (from load_aerosol_modes_csv)
    - first, second: names of the two modes to compare (case-insensitive)
    - N_list: list of number concentration to compare
    - P: Pressure (Pa), default 77500
    - T0: Initial Temperature (K), default 274
    - S0: Initial Supersaturation (1-RH), default -0.02
    - V: Updraft speed (m/s), default 1.0
    - t_end: End time (seconds), default 250 (or 250/V meters)
    - dt: Timestep (seconds), default 1.0
    
    Returns: None
    Creates n CSV files (one per number concentration) with height, concentration, and radius data.
    """

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
    for N_n in (N_list):
        species1 = pc.AerosolSpecies(mode1.name,
                                    pc.Lognorm(mu=pyrcel_mu1, sigma=mode1.sigma_g, N=N_n),
                                    kappa=mode1.kappa, bins=200)
        species2 = pc.AerosolSpecies(mode2.name,
                                    pc.Lognorm(mu=pyrcel_mu2, sigma=mode2.sigma_g, N=mode2.N0_cm3),
                                    kappa=mode2.kappa, bins=200)
        
        initial_aerosols = [species1, species2]

        # Run parcel model
        model = pc.ParcelModel(initial_aerosols, V, T0, S0, P, console=False, accom=0.3)
        parcel_trace, aerosol_traces = model.run(t_end, dt, solver='cvode')
        
 
    
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
            os.makedirs(DATA_DIR, exist_ok=True)
            csv_df = pd.DataFrame(csv_rows)
            csv_path = os.path.join(DATA_DIR, f"profile_sulfate_seasalt_N_for_{N_n:.1e}_cm3.csv")
            csv_df.to_csv(csv_path, index=False)
            print(f"Saved {aerosol_name} profile -> {csv_path}")
    return None 

