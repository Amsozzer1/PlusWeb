# Benchmark and profiling harness

The harness behind [`PROFILING_REPORT.md`](../PROFILING_REPORT.md). It exists
because this box had no `perf`, `valgrind`, `wrk` or `pprof`, so everything here
is self-contained.

## Build

```bash
cmake -B build -DPLUSWEB_BUILD_BENCH=ON
cmake --build build -j
```

Binaries land in `build/bench/`. Never benchmark a `Debug` build — the harness
forces `-O2` on its own targets even under `Debug`, but the library itself will
still be unoptimized, which is a ~2.6x difference.

## Targets

| Binary | What it does |
|---|---|
| `pw_loadgen` | Closed-loop epoll HTTP client. One request in flight per connection. |
| `pw_refserver` | Bare epoll server returning a canned response — the harness ceiling. |
| `pw_microbench` | In-process pipeline benchmarks with **exact** allocation counts. |
| `pw_benchserver` | PlusWeb under test (`HttpServer`). |

## The one metric that matters

`pw_loadgen` reports **`starved_conns`**: connections that completed *zero*
requests for the whole run. Throughput and percentiles alone can be
misleading: a server that answers a few connections quickly and ignores the rest
still reports a healthy p99, because percentiles only measure what got served.

```bash
./build/bench/pw_loadgen <host> <port> <conns> <seconds> <path> [--close]
```

## Recipes

Establish the ceiling for this machine first, so "the framework is slow" stays
distinguishable from "the harness is slow":

```bash
./build/bench/pw_refserver 18081 &
./build/bench/pw_loadgen 127.0.0.1 18081 64 5 /plain
```

Pipeline costs and allocation counts (no sockets involved):

```bash
./build/bench/pw_microbench 200000
```

Throughput sweep, watching for starvation:

```bash
./build/bench/pw_benchserver 18080 1000 0 &
for c in 1 4 32 128 512; do
    ./build/bench/pw_loadgen 127.0.0.1 18080 $c 5 /plain
done
```

CPU profile (needs gperftools' `libprofiler.so`; there is no `pprof` dependency,
`tools/readprof.py` parses the profile format directly):

```bash
./tools/run_cpuprof.sh ./build/bench/pw_benchserver 18080 mylabel 4 12 /plain 1000
```

Malformed-input probes and crash isolation — each runs against a freshly started
server so a crash is attributable to exactly one request:

```bash
python3 tools/probes2.py ./build/bench/pw_benchserver 18150
python3 tools/crashhunt.py ./build/bench/pw_benchserver 18095
```

Backtrace one specific malformed request:

```bash
./tools/gdbtrace.sh ./build/bench/pw_benchserver 18101 "b'GARBAGE\r\n\r\n'" "garbage request line"
```

## Comparing against Express

Not checked in (it needs `npm install express`), but the report's numbers came
from an Express app registering the same routes as `benchserver.cpp`, driven by
the same `pw_loadgen`.

## Caveats

- Run the load generator on the same box and it competes for cores with the
  server; absolute rps is conservative. Ratios are the trustworthy figures.
- `pw_microbench` replaces global `operator new`/`delete` with counting versions,
  so allocation counts are deterministic, not sampled. Timing is best-of-5 and
  runs in a separate pass so the counters do not pollute it.
- libc here has no DWARF, so libc frame *names* from `readprof.py` come from
  nearest-symbol lookup and are unreliable. Trust framework symbols and
  before/after comparisons, not libc labels.
