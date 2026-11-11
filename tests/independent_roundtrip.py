"""Independent native byte-fixture generator, file transport and SHA256 oracle.
Does not import or call the OrcStream C library. Python 3 stdlib only.
"""
import hashlib
import math
import pathlib
import random
import struct
import subprocess
import sys
import zlib


def fixture(fmt, samples):
    rng = random.Random(0x5EED + fmt)
    out = bytearray()
    byteorder = "<" if fmt in (3, 5) else ">"
    signed_extremes = [-32768, 32767, -1, 0, 1, -2, 2, -127, 127]
    special_float_bits = [0, 0x80000000, 1, 0x80000001, 0x7FC12345, 0x7FA00001,
                          0x7F800000, 0xFF800000, 0x3F800000, 0xBF800000]
    for index in range(samples):
        for channel in range(2):
            key = index * 2 + channel
            if key < 20:
                v = signed_extremes[key % len(signed_extremes)]
            elif key % 11 == 0:
                v = int(23000 * math.sin(2 * math.pi * index / 37 + channel))
            else:
                v = rng.randint(-32768, 32767)
            if fmt == 1:
                out.append((v // 256 + 128) & 255)
            elif fmt == 2:
                out.append((v // 256) & 255)
            elif fmt in (3, 4):
                out.extend(struct.pack(byteorder + "h", v))
            else:
                if key < len(special_float_bits):
                    bits = special_float_bits[key]
                else:
                    bits = struct.unpack("=I", struct.pack("=f", v / 32768.0))[0]
                out.extend(struct.pack(byteorder + "I", bits))
    return bytes(out)


def frames(data):
    result = []
    offset = 0
    while offset < len(data):
        if offset + 64 > len(data):
            raise ValueError("trailing frame header truncated")
        length = struct.unpack_from(">I", data, offset + 56)[0]
        end = offset + 64 + length
        if end > len(data):
            raise ValueError("trailing payload truncated")
        result.append(data[offset:end])
        offset = end
    return result


def patched(frame, pos, val):
    data = bytearray(frame)
    data[pos] = val
    crc = zlib.crc32(data[:60] + data[64:]) & 0xFFFFFFFF
    data[60:64] = struct.pack(">I", crc)
    return bytes(data)


def execute(program, *args, success=True):
    proc = subprocess.run([str(program), *map(str, args)], capture_output=True, text=True)
    if (proc.returncode == 0) != success:
        raise AssertionError(f"{program.name}: expected success={success} got {proc.returncode}: {proc.stderr}\n{proc.stdout}")
    return proc.stdout.strip()


def run(producer, consumer, directory):
    directory.mkdir(parents=True, exist_ok=True)
    total_cases = 0
    for fmt in range(1, 7):
        count = 20017
        data = fixture(fmt, count)
        src = directory / f"p1_{fmt}.raw"
        framed = directory / f"p1_{fmt}.orcs"
        reconstructed = directory / f"p1_{fmt}.rebuilt"
        src.write_bytes(data)
        for chunk in (1, 37, 255, 4096):
            rate = 960000 if chunk % 2 else 2400000
            if chunk * (2 if fmt < 3 else 4 if fmt < 5 else 8) > 32768:
                raise AssertionError("bad fixture chunk")
            execute(producer, src, framed, fmt, chunk, rate)
            out = execute(consumer, framed, reconstructed, fmt)
            rebuilt = reconstructed.read_bytes()
            assert rebuilt == data, f"format={fmt} chunk={chunk} byte difference"
            assert hashlib.sha256(rebuilt).digest() == hashlib.sha256(data).digest()
            assert f"samples={count}" in out
            total_cases += 1
        print(f"format={fmt} samples={count} sha256={hashlib.sha256(data).hexdigest()}")
    data = fixture(3, 19)
    src = directory / "negative.raw"
    encoded = directory / "negative.orcs"
    output = directory / "negative.rebuilt"
    src.write_bytes(data)
    execute(producer, src, encoded, 3, 5, 2400000)
    good = frames(encoded.read_bytes())
    assert len(good) >= 5
    cases = {
        "invalid_magic": [patched(good[0], 0, 0xEE)] + good[1:],
        "invalid_version": [patched(good[0], 5, 99)] + good[1:],
        "wrong_header_size": [patched(good[0], 7, 63)] + good[1:],
        "bad_type": [good[0], patched(good[1], 8, 99)] + good[2:],
        "bad_format": [good[0], patched(good[1], 9, 88)] + good[2:],
        "bad_clock_flags": [good[0], patched(good[1], 10, 0x00), patched(good[2], 10, 0x01)] + good[3:],
        "bad_length": [good[0], patched(good[1], 59, 255)] + good[2:],
        "bad_crc": [good[0], good[1][:67] + bytes([good[1][67] ^ 0x80]) + good[1][68:]] + good[2:],
        "truncated": good[:-1] + [good[-1][:-2]],
        "missing_eos": good[:-1],
        "dropped_frame": [good[0]] + good[2:],
        "reordered_frames": [good[0], good[2], good[1]] + good[3:],
        "duplicated_frame": good[:2] + [good[1]] + good[2:],
        "metadata_shift": [good[0], good[1], patched(good[2], 43, 0xDD)] + good[3:],
        "multistream": [good[0], good[1], patched(good[2], 15, 0x25)] + good[3:],
        "trailing_garbage": good + [b"invalid"],
    }
    for label, contents in cases.items():
        corrupt = directory / "corrupt.orcs"
        corrupt.write_bytes(b"".join(contents))
        execute(consumer, corrupt, output, 3, success=False)
    print(f"independent roundtrip: {total_cases} fixture cases passed; {len(cases)} corrupted streams rejected")
    for file in directory.glob("p1_*.*"):
        file.unlink()
    for file in (src, encoded, output, directory / "corrupt.orcs"):
        if file.exists():
            file.unlink()


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: independent_roundtrip.py PRODUCER CONSUMER BUILD_DIR")
    run(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3]))
