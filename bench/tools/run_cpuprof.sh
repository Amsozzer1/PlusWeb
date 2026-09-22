#!/usr/bin/env bash
# Sampling-profiler run: gperftools' libprofiler is preloaded into the server and
# the load generator drives it. The process memory map is snapshotted while the
# server is still alive, because the copy gperftools appends to the profile is
# lost if the process has to be killed.
set -u
S="$(cd "$(dirname "$0")/.." && pwd)"
SERVER_BIN="$1"; PORT="$2"; LABEL="$3"
CONNS="${4:-4}"; DUR="${5:-12}"; ROUTE="${6:-/plain}"; NROUTES="${7:-1000}"

PROF="$S/prof/cpu-$LABEL.prof"; MAPS="$S/prof/maps-$LABEL.txt"
rm -f "$PROF" "$MAPS"

CPUPROFILE="$PROF" CPUPROFILE_FREQUENCY=1000 LD_PRELOAD=/usr/lib/libprofiler.so \
  "$SERVER_BIN" "$PORT" "$NROUTES" 0 > "$S/prof/server-prof-$LABEL.log" 2>&1 &
SPID=$!
for _ in $(seq 1 100); do
    grep -q READY "$S/prof/server-prof-$LABEL.log" 2>/dev/null && break
    sleep 0.1
done
cp "/proc/$SPID/maps" "$MAPS" 2>/dev/null

"$S/tools/loadgen" 127.0.0.1 "$PORT" "$CONNS" 2 "$ROUTE" > /dev/null 2>&1
echo "--- profiling $LABEL ($CONNS conns, ${DUR}s, $ROUTE, $NROUTES routes) ---"
"$S/tools/loadgen" 127.0.0.1 "$PORT" "$CONNS" "$DUR" "$ROUTE"

kill -INT "$SPID" 2>/dev/null
for _ in $(seq 1 60); do kill -0 "$SPID" 2>/dev/null || break; sleep 0.25; done
kill -KILL "$SPID" 2>/dev/null; wait "$SPID" 2>/dev/null

if [ -s "$PROF" ]; then
    echo; python3 "$S/tools/readprof.py" "$PROF" "$SERVER_BIN" 28 "$MAPS"
else
    echo "NO PROFILE WRITTEN ($PROF)"
fi
