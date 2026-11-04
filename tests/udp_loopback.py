"""Repeatable local IPv4 loopback integration smoke test; no SDR hardware required."""
import socket
import subprocess
import sys
import time

if len(sys.argv) != 2:
    raise SystemExit("usage: udp_loopback.py <orcstream_udp_demo>")

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]

receiver = subprocess.Popen(
    [sys.argv[1], "receive", str(port), "48"],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
)
try:
    time.sleep(0.20)
    sender = subprocess.run(
        [sys.argv[1], "send", "127.0.0.1", str(port), "48"],
        check=True,
        capture_output=True,
        text=True,
        timeout=15,
    )
    out, err = receiver.communicate(timeout=7)
    print(sender.stdout.strip())
    print(out.strip())
    if err:
        print(err, file=sys.stderr)
    if receiver.returncode != 0 or "missing_packets=0, missing_samples=0" not in out:
        raise SystemExit("UDP loopback failed")
finally:
    if receiver.poll() is None:
        receiver.kill()
        receiver.communicate()
