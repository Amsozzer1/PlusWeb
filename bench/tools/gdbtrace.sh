#!/usr/bin/env bash
# Run the server under gdb, fire one malformed request at it, print the faulting
# stack. One trigger per invocation so the trace is unambiguous.
S="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$1"; PORT="$2"; PY_PAYLOAD="$3"; LABEL="$4"

cat > "$S/prof/gdbcmd.txt" <<EOF
set pagination off
set confirm off
handle SIGPIPE nostop noprint pass
run $PORT 20 0
echo \n===== FAULT: $LABEL =====\n
bt 25
echo \n--- faulting frame locals ---\n
info locals
echo \n--- all threads ---\n
info threads
quit
EOF

( sleep 1.2
  python3 -c "
import socket,sys,time
p=$PORT
try:
    s=socket.create_connection(('127.0.0.1',p),timeout=3)
    s.sendall($PY_PAYLOAD)
    s.settimeout(2)
    try:
        while s.recv(65536): pass
    except Exception: pass
    s.close()
except Exception as e:
    print('client:',e,file=sys.stderr)
"
  sleep 1.5
) &

timeout 25 gdb -q -x "$S/prof/gdbcmd.txt" --args "$BIN" "$PORT" 20 0 2>&1 \
  | grep -vE "^\[Thread|^\[New Thread|Downloading|^Reading|warning: Error dis"
wait 2>/dev/null
