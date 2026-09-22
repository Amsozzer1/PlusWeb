#!/usr/bin/env python3
"""Minimal reader for the gperftools CPU-profile format (no pprof on this box).

Format (all little-endian uint64):
  header: 0, 3, 0, period_usec, 0
  samples: count, npcs, pc[0..npcs-1]   (repeated)
  trailer: 0, 1, 0
  then b"--- \n" followed by a text copy of /proc/self/maps
Symbolization is done with addr2line against the mapped objects.
"""
import collections
import struct
import subprocess
import sys


def read_profile(path):
    data = open(path, "rb").read()
    marker = data.find(b"--- ")
    binary, text = (data[:marker], data[marker:]) if marker != -1 else (data, b"")
    words = struct.unpack("<%dQ" % (len(binary) // 8), binary[: len(binary) // 8 * 8])
    assert words[0] == 0 and words[1] == 3, "not a gperftools profile"
    period_us = words[3]
    samples, i = [], 5
    while i + 1 < len(words):
        count, npcs = words[i], words[i + 1]
        i += 2
        if count == 0 and npcs == 1:
            break
        pcs = words[i:i + npcs]
        i += npcs
        samples.append((count, list(pcs)))
    return period_us, samples, text.decode("utf-8", "replace")


def parse_maps(text):
    """-> list of (start, end, offset, path) for file-backed executable maps."""
    out = []
    for line in text.splitlines():
        parts = line.split()
        if len(parts) < 6 or "-" not in parts[0]:
            continue
        try:
            lo, hi = (int(x, 16) for x in parts[0].split("-"))
        except ValueError:
            continue
        if "x" not in parts[1]:
            continue
        path = parts[-1]
        if not path.startswith("/"):
            continue
        out.append((lo, hi, int(parts[2], 16), path))
    return out


def symbolize(pcs, maps, main_binary):
    """-> {pc: (function, file:line)} using addr2line, batched per object."""
    by_obj = collections.defaultdict(list)
    unmapped = 0
    for pc in pcs:
        hit = None
        for lo, hi, off, path in maps:
            if lo <= pc < hi:
                # File offset of this pc inside the mapped object. For ELF objects
                # built the usual way (text p_vaddr == p_offset) this is the
                # address addr2line wants, for PIE executables and .so alike.
                hit = (path, pc - lo + off)
                break
        if hit is None:
            unmapped += 1
            continue
        by_obj[hit[0]].append((pc, hit[1]))

    result = {}
    for obj, entries in by_obj.items():
        # A PIE main binary is also file-backed; addr2line wants the file offset.
        addrs = ["0x%x" % a for _, a in entries]
        # No -i: exactly two output lines (function, file:line) per address.
        try:
            proc2 = subprocess.run(
                ["addr2line", "-f", "-C", "-e", obj] + addrs,
                capture_output=True, text=True, timeout=300)
        except Exception:
            continue
        lines = proc2.stdout.splitlines()
        table = _symtab(obj)
        for idx, (pc, fo) in enumerate(entries):
            fn = lines[idx * 2] if idx * 2 < len(lines) else "??"
            loc = lines[idx * 2 + 1] if idx * 2 + 1 < len(lines) else "??"
            if fn in ("??", ""):
                fn = _nearest(table, fo)
            result[pc] = (fn, loc, obj)
    return result


_symcache = {}


def _symtab(obj):
    """Sorted [(value, name)] from the object's symbol tables, for stripped libs."""
    if obj in _symcache:
        return _symcache[obj]
    syms = []
    for flag in ("-D", ""):
        try:
            args = ["nm", "-C", "--defined-only"] + ([flag] if flag else []) + [obj]
            out = subprocess.run(args, capture_output=True, text=True,
                                 timeout=120).stdout
        except Exception:
            continue
        for line in out.splitlines():
            parts = line.split(" ", 2)
            if len(parts) == 3 and parts[1].upper() in ("T", "W", "I"):
                try:
                    syms.append((int(parts[0], 16), parts[2]))
                except ValueError:
                    pass
        if syms:
            break
    syms.sort()
    _symcache[obj] = syms
    return syms


def _nearest(table, addr):
    if not table:
        return "??"
    import bisect
    i = bisect.bisect_right(table, (addr, "\xff")) - 1
    if i < 0:
        return "??"
    return table[i][1]


def main():
    if len(sys.argv) < 3:
        print("usage: readprof.py <profile> <main-binary> [top-n] [maps-file]")
        return 1
    prof_path, main_binary = sys.argv[1], sys.argv[2]
    topn = int(sys.argv[3]) if len(sys.argv) > 3 else 35
    maps_file = sys.argv[4] if len(sys.argv) > 4 else None

    period_us, samples, maps_text = read_profile(prof_path)
    if maps_file:
        maps_text = open(maps_file).read()
    maps = parse_maps(maps_text)
    if not maps:
        print("WARNING: no memory map available; addresses cannot be symbolized")
    total = sum(c for c, _ in samples)

    flat = collections.Counter()      # pc appears as leaf
    cumul = collections.Counter()     # pc appears anywhere in the stack
    for count, pcs in samples:
        if not pcs:
            continue
        flat[pcs[0]] += count
        for pc in set(pcs):
            cumul[pc] += count

    allpcs = set(flat) | set(cumul)
    syms = symbolize(sorted(allpcs), maps, main_binary.split("/")[-1])

    def name(pc):
        fn, loc, obj = syms.get(pc, ("??", "??", "?"))
        if fn in ("??", ""):
            return "0x%x [%s]" % (pc, obj.split("/")[-1])
        return fn

    flat_by_fn = collections.Counter()
    cum_by_fn = collections.Counter()
    loc_by_fn = {}
    for pc, c in flat.items():
        flat_by_fn[name(pc)] += c
    for pc, c in cumul.items():
        cum_by_fn[name(pc)] += c
        if name(pc) not in loc_by_fn:
            loc_by_fn[name(pc)] = syms.get(pc, ("", "", ""))[1]

    print("samples: %d   period: %d us   wall-ish cpu time: %.3f s"
          % (total, period_us, total * period_us / 1e6))
    print()
    print("%-7s %-7s %-7s %-7s  %s" % ("flat", "flat%", "cum", "cum%", "function"))
    print("-" * 100)
    for fn, c in flat_by_fn.most_common(topn):
        print("%-7d %-6.2f%% %-7d %-6.2f%%  %s"
              % (c, 100.0 * c / total, cum_by_fn[fn],
                 100.0 * cum_by_fn[fn] / total, fn[:150]))
    print()
    print("=== top by cumulative (includes callees) ===")
    print("%-7s %-7s  %s" % ("cum", "cum%", "function"))
    print("-" * 100)
    for fn, c in cum_by_fn.most_common(topn):
        print("%-7d %-6.2f%%  %s" % (c, 100.0 * c / total, fn[:150]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
