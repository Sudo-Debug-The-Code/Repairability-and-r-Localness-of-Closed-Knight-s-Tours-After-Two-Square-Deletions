# Experimental Data

This directory contains the generated tours, deletion pair classifications, experimental parameters, and summary statistics. The individual repair records are omitted because of their size.

The complete experimental dataset is archived on Zenodo:

https://doi.org/10.5281/zenodo.24621048

To verify the complete dataset, download `experimental_data.zip`, extract it into the repository root, and run:

```bash
python verifier/Verifier.py data/run_1795778531166_seed_3862263958
```

The verifier requires the complete dataset, including the individual repair records.

The experimental data is licensed under CC BY 4.0: https://creativecommons.org/licenses/by/4.0/
