#This scipt aims to create the class AerosolMode that is filled with the csv file aerosol_modes.csv
import csv 

class AerosolMode:
    def __init__(self, name, origin, N0_cm3, Dg_um, sigma_g, kappa,nb_bins):
        self.name = str(name)
        self.origin = str(origin)
        self.N0_cm3 = float(N0_cm3)
        self.Dg_um = float(Dg_um)      # median diameter in µm (as in aerosol_modes.csv)
        self.sigma_g = float(sigma_g)
        self.kappa = float(kappa)
        self.nb_bins =int(nb_bins)

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
