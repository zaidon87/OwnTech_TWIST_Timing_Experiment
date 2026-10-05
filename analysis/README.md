# Timing Analysis

`timing_analysis.py` reads a Serial Monitor log and extracts lines produced by the firmware in this format:

```text
CSV,batch,mode,N,samples,t_min,t_avg,t_max,c_min,c_avg,c_max,l_min,l_avg,l_max,p_min,p_avg,p_max,overruns,budget_cycles
```

All timing fields in the firmware CSV are CPU cycles.

## Basic use

```bash
python timing_analysis.py ../results/raw/E3_N16.log \
  --output ../results/processed/E3_N16_summary.csv
```

## With plots

```bash
pip install -r requirements.txt
python timing_analysis.py ../results/raw/E3_N16.log \
  --output ../results/processed/E3_N16_summary.csv \
  --plots-dir ../results/reports/E3_N16
```

## CPU frequency

The default is 170 MHz. Override it only when the campaign record and E0b validation support a different value:

```bash
python timing_analysis.py run.log --cpu-hz 169950000
```

The script reports observed maxima. It does not claim formal WCET.
