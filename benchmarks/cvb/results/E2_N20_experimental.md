# E2_N20 both-arm timing summary

This summary is based on the expected E2_N20 both-arm campaign for 20 modules per arm.
The board capture has not yet completed successfully, so the values below remain placeholders until the run is measured.

| Target | Samples | Minimum (us) | Average (us) | Observed maximum (us) | Maximum (cycles) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Both arms | 1,890,000 | TBD | TBD | TBD | TBD |

Notes:
- Samples = 630 cases × 3 batches × 1000 executions per batch.
- Cycle counts would be converted using 170 MHz: `cycles / 170e6 * 1e6`.
- This is the both-arms result only; separate upper-arm and lower-arm captures are required for the full E2 per-arm table.
- Final E2 values will replace the `TBD` entries after a successful capture run.

## Configuration

- `CVB_MODULES_PER_ARM`: 20
- `CVB_SCOPE`: 0 for both arms, 1 for upper arm, 2 for lower arm
- `BATCH_SAMPLES`: 1000
- `CVB_CASES_PER_PATTERN`: 5 * (N + 1) = 105
- `CVB_CASE_COUNT`: 6 * 105 = 630 total cases
- Full campaign per target: 630 cases × 3 batches × 1000 samples = 1,890,000 samples

## Notes for capture

- The `N=20` campaign is intentionally experimental and should be captured with the same process as E1.
- The board must be flashed with the E2 build and then run the `capture.py` + `analyze_campaign.py` flow for `--modules-per-arm 20`.
- The current workspace contains no measured E2 data yet.
