import numpy as np
import matplotlib.pyplot as plt
import pyrcel as pc
from pyrcel import binned_activation
import os
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


# # Correct aerosol species definitions
# sulfate = pc.AerosolSpecies(
#     'sulfate',
#     pc.Lognorm(mu=0.015, sigma=1.6, N=850.),
#     kappa=0.54, bins=200
# )

# OC = pc.AerosolSpecies(
#     'OC',
#     pc.Lognorm(mu=0.6, sigma=1.6, N=400.),
#     kappa=0.2, bins=100
# )

# sea_salt = pc.AerosolSpecies(
#     'sea salt',
#     pc.Lognorm(mu=0.85, sigma=1.2, N=10.),
#     kappa=1.2, bins=200
# )

# rs_sulf = sulfate.rs[:-1]
# rs_ss = sea_salt.rs[:-1]

# ---- CCN Spectrum computation using full binned_activation API ----
def ccn_spectrum(modes,first, second, S_perc=2.0, T=274.0):
    """
    ccn_spectrum aims to calculate the Cloud Condensation Nuclei for two types of aerosols
    and plot the graph of the CCN vs supersaturation for both aerosols
    
    :param modes: list of AerosolMode instances (from load_aerosol_modes_csv)
    :param first and second: names of the two modes to compare
    :optional param S_perc: is the maximum supersaturation that we want, then the the supersaturation grid will be (0.01–s_perc %)
    :optionam param T: temperature in K 
    """

    S_linspace = np.linspace(0.01, S_perc, 200)
    S = S_linspace / 100.0

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
    
    # Create Pyrcel AerosolSpecies objects
    pyrcel_mu1 = mode1.Dg_um / 2.0
    pyrcel_mu2 = mode2.Dg_um / 2.0
    
    species1 = pc.AerosolSpecies(mode1.name,
                                pc.Lognorm(mu=pyrcel_mu1, sigma=mode1.sigma_g, N=mode1.N0_cm3),
                                kappa=mode1.kappa, bins=int(mode1.nb_bins))
    species2 = pc.AerosolSpecies(mode2.name,
                                pc.Lognorm(mu=pyrcel_mu2, sigma=mode2.sigma_g, N=mode2.N0_cm3),
                                kappa=mode2.kappa, bins=int(mode2.nb_bins))
    
    initial_aerosols = [species1, species2]
    CCN =[]
    for species in initial_aerosols:
        rs=species.rs[:-1]
        Nc_array = []
        for Smax in S:
            eq, kn, alpha, phi = binned_activation(
                Smax=Smax,
                T=T,
                rs=rs,
                aerosol=species,
                approx=False  # full Köhler calculation
            )
            Nc = eq * species.total_N
            Nc_array.append(Nc)
        CCN.append(np.array(Nc_array))
    
    fig1 = plt.figure(figsize=(10, 5))
    ax1 = fig1.add_subplot(111)
    ax1.grid(False, "minor")

    col1 = "#CC0066"
    col2 = "#0099FF"

    ax1.plot(S_linspace, CCN[0],
            color=col1, label=f"{mode1.name}")
    ax1.plot(S_linspace, CCN[1],
            color=col2, label=f"{mode2.name}")
    
    ax1.set_xlabel("Supersaturation (%)")
    ax1.set_ylabel("CCN concentration (cm⁻³)")
    ax1.set_title("CCN Spectrum vs Supersaturation")
    ax1.legend(loc='upper right')

    # show first figure
    fig1.tight_layout()
    fname = f"CCN_vs_supersat_for_{mode1.name.replace(' ', '_')}_and_{mode2.name.replace(' ', '_')}.png"
    fig1_path = os.path.join(os.path.dirname(__file__), fname)
    fig1.savefig(fig1_path, dpi=150, bbox_inches='tight')
    print(f"Saved Pyrcel figure -> {fig1_path}")
    fig1.show()
    return None 

print(ccn_spectrum(modes,"sulfate_accum","sea_salt_coarse"))
