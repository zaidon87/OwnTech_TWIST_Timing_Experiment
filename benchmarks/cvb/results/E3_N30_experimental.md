# E3_N30 both-arm timing summary

This summary is based on the expected E3_N30 both-arm campaign for 30 modules per arm.
The board capture has not yet completed successfully, so the values below remain placeholders until the run is measured.

| Target | Samples | Minimum (us) | Average (us) | Observed maximum (us) | Maximum (cycles) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Both arms | 2,790,000 | TBD | TBD | TBD | TBD |

Notes:
- Samples = 930 cases × 3 batches × 1000 executions per batch.
- Cycle counts would be converted using 170 MHz: `cycles / 170e6 * 1e6`.
- This is the both-arms result only; separate upper-arm and lower-arm captures are required for the full E3 per-arm table.
- Final E3 values will replace the `TBD` entries after a successful capture run.

## Configuration

- `CVB_MODULES_PER_ARM`: 30
- `CVB_SCOPE`: 0 for both arms, 1 for upper arm, 2 for lower arm
- `BATCH_SAMPLES`: 1000
- `CVB_CASES_PER_PATTERN`: 5 × (N + 1) = 155
- `CVB_CASE_COUNT`: 6 × 155 = 930 total cases
- Full campaign per target: 930 cases × 3 batches × 1000 samples = 2,790,000 samples

## Notes for capture

- The `N=30` campaign is intentionally experimental and should be captured with the same process as E1.
- The board must be flashed with the E3 build and then run the `capture.py` + `analyze_campaign.py` flow for `--modules-per-arm 30`.
- The current workspace contains no measured E3 data yet.
