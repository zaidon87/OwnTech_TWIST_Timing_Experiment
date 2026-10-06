# E1_N10 both-arm timing summary

This summary is based on the attached E1_N10 both-arm capture log (`CSV` rows from the board run).
It reflects the measured `CVB_SCOPE=0` result for ten modules per arm.

| Target | Samples | Minimum (us) | Average (us) | Observed maximum (us) | Maximum (cycles) |
| --- | ---: | ---: | ---: | ---: | ---: |
| Both arms | 145,000 | 22.97 | 23.45 | 25.61 | 4354 |

Notes:
- Samples = 145 CSV records × 1000 executions per CSV batch.
- Cycle counts were converted using 170 MHz: `cycles / 170e6 * 1e6`.
- This is the both-arms result only from the attached capture.
- Separate upper-arm and lower-arm captures are required for the full E1 per-arm table.
