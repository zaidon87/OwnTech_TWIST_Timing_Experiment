"""Read RIGOL MHO984 CH1 positive width over a raw SCPI TCP socket.

Python standard library only; no VISA, drivers or firmware changes required.
Port 5555 is a candidate default, not yet verified on the user's instrument.
Use --port if the instrument's SOCKET address specifies a different port.
SCPI reference: RIGOL MHO900 Programming Guide, sections 3.14.11 and 3.17.
"""
import argparse
from datetime import datetime, timezone
import ipaddress
import json
import math
from pathlib import Path
import socket
import sys
import time


QUERIES = {
    "identity": "*IDN?",
    "visa_lxi": ":LAN:VISA? LXI",
    "visa_socket": ":LAN:VISA? SOCKet",
    "positive_width": ":MEASure:ITEM? PWIDth,CHANnel1",
    "statistics_enabled": ":MEASure:STATistic:DISPlay?",
    "current": ":MEASure:STATistic:ITEM? CURRent,PWIDth,CHANnel1",
    "average": ":MEASure:STATistic:ITEM? AVERages,PWIDth,CHANnel1",
    "minimum": ":MEASure:STATistic:ITEM? MINimum,PWIDth,CHANnel1",
    "maximum": ":MEASure:STATistic:ITEM? MAXimum,PWIDth,CHANnel1",
    "standard_deviation": ":MEASure:STATistic:ITEM? DEViation,PWIDth,CHANnel1",
    "count": ":MEASure:STATistic:ITEM? CNT,PWIDth,CHANnel1",
}
TIME_FIELDS = {"positive_width", "current", "average", "minimum", "maximum",
               "standard_deviation"}


def query(connection, command, timeout):
    """Only send whitelisted queries; abort on timeout to avoid stale replies."""
    if command not in QUERIES.values():
        raise ValueError("Command is not in the read-only allowlist")
    connection.sendall((command + "\n").encode("ascii"))
    deadline = time.monotonic() + timeout
    response = bytearray()
    while len(response) < 16384:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError(f"No complete response to {command}")
        connection.settimeout(remaining)
        chunk = connection.recv(1)
        if not chunk:
            raise ConnectionError(f"Connection closed during {command}")
        response.extend(chunk)
        if chunk == b"\n":
            # Preserve exact reply bytes separately; remove only CR/LF in text.
            return bytes(response)
    raise ValueError("SCPI response exceeded 16 KiB")


def read_scope(ip, port, timeout, result):
    with socket.create_connection((ip, port), timeout=timeout) as connection:
        result["tcp_connected"] = True
        for name, command in QUERIES.items():
            result["last_query"] = command
            reply = query(connection, command, timeout)
            text = reply.decode("ascii").rstrip("\r\n")
            item = {"query": command, "response": text, "response_hex": reply.hex()}
            result["readings"][name] = item
            print(f"{name}: {text}", flush=True)
            if name == "identity":
                parts = [part.strip().upper() for part in text.split(",")]
                if len(parts) < 2 or "RIGOL" not in parts[0] or parts[1] != "MHO984":
                    raise ValueError("Identity is not RIGOL MHO984; no measurement queries sent")
                result["identified"] = True
            if name in TIME_FIELDS or name == "count":
                try:
                    value = float(text)
                    # Reject common SCPI unavailable sentinels as measurements.
                    valid = math.isfinite(value) and 0 <= value < 1e30
                    item["status"] = "numeric" if valid else "unavailable"
                    if valid:
                        item["value"] = value
                        item["unit"] = "count" if name == "count" else "s"
                        if name in TIME_FIELDS:
                            item["microseconds"] = value * 1e6
                except ValueError:
                    item["status"] = "unavailable_or_error"
        result["queries_completed"] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ip", type=ipaddress.IPv4Address,
                        help="Actual numeric IP shown on the scope; not 192.168.x.x")
    parser.add_argument("--port", type=int, default=5555,
                        help="Raw SCPI TCP port (candidate default: 5555)")
    parser.add_argument("--timeout", type=float, default=5)
    parser.add_argument("--output", type=Path,
                        help="New JSON output file (existing files are never overwritten)")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("Port must be 1..65535 and timeout must be positive and finite")
    stamp = datetime.now(timezone.utc)
    output = args.output or Path(__file__).resolve().parent / "results" / (
        "rigol_lan_" + stamp.strftime("%Y%m%dT%H%M%S_%fZ") + ".json")
    output.parent.mkdir(parents=True, exist_ok=True)
    # Reserve the output before contacting the scope.
    with output.open("x", encoding="utf-8") as report:
        result = {
            "timestamp_utc": stamp.isoformat(), "ip": str(args.ip), "port": args.port,
            "transport": "SCPI over raw TCP, Python socket",
            "socket_resource_candidate": f"TCPIP0::{args.ip}::{args.port}::SOCKET",
            "tcp_connected": False, "identified": False, "queries_completed": False,
            "readings": {},
            "note": "Sequential readings are not an atomic snapshot. Statistics are read as-is; nothing is enabled or reset.",
        }
        try:
            read_scope(str(args.ip), args.port, args.timeout, result)
        except (OSError, ValueError) as exc:
            result["error"] = str(exc)
            print(f"Stopped: {exc}", file=sys.stderr)
        finally:
            json.dump(result, report, indent=2, allow_nan=False)
            report.write("\n")
    print(f"Saved: {output}")
    return 0 if result["queries_completed"] else 1


if __name__ == "__main__":
    sys.exit(main())
