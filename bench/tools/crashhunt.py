#!/usr/bin/env python3
"""Find the smallest request that kills the server, restarting it each time.

Sends one request per server instance so a crash is unambiguously attributable
to that request.
"""
import socket
import subprocess
import sys
import time

BIN = sys.argv[1]
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 18095


def start():
    p = subprocess.Popen([BIN, str(PORT), "20", "0"],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for _ in range(100):
        line = p.stdout.readline()
        if b"READY" in line:
            return p
    return p


def try_request(payload, label):
    """-> (alive_after, response_bytes)"""
    p = start()
    time.sleep(0.15)
    resp = b""
    try:
        s = socket.create_connection(("127.0.0.1", PORT), timeout=2.0)
        s.settimeout(2.0)
        s.sendall(payload)
        try:
            while True:
                c = s.recv(65536)
                if not c:
                    break
                resp += c
        except socket.timeout:
            pass
        s.close()
    except Exception as e:
        resp = b"<conn error: %s>" % str(e).encode()
    time.sleep(0.2)
    rc = p.poll()
    crashed = rc is not None
    if not crashed:
        p.kill()
    p.wait()
    status = "CRASH rc=%s" % rc if crashed else "survived"
    print("%-56s -> %-14s resp=%d bytes" % (label, status, len(resp)))
    return (not crashed), resp


print("=== does the padded request crash the server? ===")
for pad in [0, 100, 500, 900, 950, 960, 970, 980, 990, 1000, 1100, 2000]:
    req = ("GET /echo HTTP/1.1\r\nHost: bench\r\nX-Pad: %s\r\n"
           "Connection: close\r\n\r\n" % ("A" * pad)).encode()
    try_request(req, "GET /echo with %d-byte X-Pad (total %d)" % (pad, len(req)))

print()
print("=== isolate: header line with no colon ===")
try_request(b"GET /echo HTTP/1.1\r\nHost: bench\r\nBadHeaderNoColon\r\n"
            b"Connection: close\r\n\r\n", "header line containing no ':'")
try_request(b"GET /echo HTTP/1.1\r\nHost: bench\r\nConnection: close\r\n\r\n",
            "control: same request, all headers well-formed")

print()
print("=== isolate: truncation producing a colon-less final line ===")
# 1024-byte recv buffer, 1023 usable. Build a request whose 1023-byte prefix
# ends inside a header name, so the last parsed line has no colon at all.
base = "GET /echo HTTP/1.1\r\nHost: bench\r\n"
for name_off in range(0, 6):
    filler = "X-F: %s\r\n" % ("A" * (1023 - len(base) - len("X-F: \r\n")
                                     - len("X-Second-Header-Name") + name_off))
    req = (base + filler + "X-Second-Header-Name: v\r\nConnection: close\r\n\r\n")
    try_request(req.encode(),
                "cut lands %d bytes into a header NAME (total %d)"
                % (name_off, len(req)))

print()
print("=== isolate: request line itself truncated ===")
long_path = "/echo?" + "k=v&" * 300
req = ("GET %s HTTP/1.1\r\nHost: bench\r\nConnection: close\r\n\r\n" % long_path)
try_request(req.encode(), "request line longer than the 1023-byte buffer")

print()
print("=== isolate: no CRLFCRLF within the first 1023 bytes ===")
req = ("GET /echo HTTP/1.1\r\nX-Pad: %s" % ("A" * 2000)).encode()
try_request(req, "headers never terminate inside the buffer")

print()
print("=== isolate: empty / garbage first line ===")
try_request(b"\r\n\r\n", "bare CRLFCRLF")
try_request(b"GARBAGE\r\n\r\n", "single-token request line")
try_request(b"GET\r\n\r\n", "request line with no path or protocol")
try_request(b"GET /echo\r\n\r\n", "request line with no protocol")
