# Acceptance Criteria

Use the same configuration throughout the campaign and record it in `config/campaign_record.csv`.

## Deadline

A batch passes the firmware deadline check when:

```text
overruns == 0
and
L_max + C_max < Ts
```

## Slack

Slack must be positive. For the real MMC task, define and document the engineering margin you require before declaring the implementation comfortably schedulable.

## Repeatability

With a fixed input pattern, the measured target maximum should not drift substantially between batches. Investigate configuration changes, interrupts, compiler changes, or background activity if it does.

## DWT vs oscilloscope

After E0 offset correction, the guide uses a cross-validation target of approximately 1% for target durations of at least 5 us. For shorter windows, report absolute difference because oscilloscope resolution and edge overhead dominate the relative percentage.

## Clock validation

Use E0b to check the assumed 170 MHz DWT conversion against the oscilloscope. Investigate and rescale if the independent result does not support the assumed CPU frequency.
