"""Capture one complete CVB campaign (or calibration batches) over USB CDC."""
import argparse
import json
from pathlib import Path
import time

import serial
from serial.tools import list_ports


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--serial-number", default="423250070032003B")
    parser.add_argument("--mode", type=int, choices=(0, 2, 3), required=True)
    parser.add_argument("--scope", type=int, choices=(0, 1, 2), default=0)
    parser.add_argument("--timeout", type=float, default=300)
    parser.add_argument("--modules-per-arm", type=int, choices=(5, 10), default=5)
    parser.add_argument("--batches", type=int, default=20, help="Calibration batches")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        raise SystemExit(f"Refusing to overwrite {args.output}")
    deadline = time.monotonic() + args.timeout
    connection = None
    while time.monotonic() < deadline:
        ports = [p for p in list_ports.comports() if p.serial_number == args.serial_number]
        if len(ports) == 1:
            try:
                connection = serial.Serial(ports[0].device, 115200, timeout=1)
                break
            except serial.SerialException:
                pass
        time.sleep(0.5)
    if connection is None:
        raise SystemExit("Board did not enumerate or serial port is busy")
    # USB CDC retains old/partial output while no host is reading. Drain it
    # before accepting records; keep strict validation once capture begins.
    drain_end = time.monotonic() + 1.5
    while time.monotonic() < drain_end:
        connection.read(connection.in_waiting or 1)
    connection.reset_input_buffer()
    campaign_batches = 6 * 5 * (args.modules_per_arm + 1) * 3
    expected_checks = 369 * 9 * (args.modules_per_arm + 1)**2
    required = campaign_batches if args.mode == 3 else args.batches
    seen = set()
    metadata = {}
    started = time.monotonic()
    last_progress = started
    print(f"Capturing {connection.port}: need {required} distinct batches", flush=True)
    try:
        with connection, args.output.open("x", encoding="utf-8") as log:
            while time.monotonic() < deadline and len(seen) < required:
                line = connection.readline().decode("utf-8", errors="replace")
                if not line:
                    continue
                log.write(line)
                log.flush()
                if "messages dropped" in line or line.startswith("BATCH_GAP,"):
                    raise RuntimeError("Reporting lost data; raw diagnostic log retained")
                if line.startswith("CVB_META,"):
                    fields = line.strip().split(",")
                    if len(fields) == 12:
                        metadata[int(fields[1])] = fields
                if line.startswith("CSV,"):
                    fields = line.strip().split(",")
                    if len(fields) != 19:
                        raise RuntimeError(f"Malformed CSV: {line}")
                    batch, mode, n, samples = map(int, fields[1:5])
                    if mode != args.mode or n != args.modules_per_arm or samples != 1000:
                        raise RuntimeError(f"Unexpected firmware configuration: {line}")
                    meta = metadata.pop(batch, None)
                    if meta is None:
                        continue  # First line can start partway through a report.
                    if int(meta[3]) != args.scope:
                        raise RuntimeError("Wrong CVB scope on board")
                    if int(meta[9]) != expected_checks or int(meta[10]) != 0:
                        raise RuntimeError("On-board CVB self-test did not pass")
                    if args.mode == 3:
                        case = int(meta[2])
                        phase = (batch - 1) % campaign_batches
                        if case != phase // 3:
                            raise RuntimeError("Input case does not match batch sequence")
                        seen.add(phase)
                    else:
                        seen.add(batch)
                now = time.monotonic()
                if now - last_progress >= 10:
                    print(f"Captured {len(seen)}/{required} batches ({now-started:.0f}s)", flush=True)
                    last_progress = now
    finally:
        summary = {
            "serial_number": args.serial_number, "port": connection.port,
            "mode": args.mode, "scope": args.scope, "modules_per_arm": args.modules_per_arm,
            "distinct_batches": len(seen), "required_batches": required,
            "complete": len(seen) == required,
            "duration_s": round(time.monotonic() - started, 3),
        }
        args.output.with_suffix(".capture.json").write_text(json.dumps(summary, indent=2)+"\n")
    print(json.dumps(summary), flush=True)
    if not summary["complete"]:
        raise SystemExit("Capture incomplete; raw log retained for diagnosis")


if __name__ == "__main__":
    main()
