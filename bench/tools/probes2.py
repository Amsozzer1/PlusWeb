#!/usr/bin/env python3
"""Behavioural probes, each against a freshly started server.

The first probe run died partway because a malformed request killed the process,
so every probe here gets its own server instance and reports whether the server
was still alive afterwards.
"""
import json
import socket
import subprocess
import sys
import time

BIN = sys.argv[1]
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 18150
results = []


class Server:
    def __init__(self, nroutes=100):
        self.p = subprocess.Popen([BIN, str(PORT), str(nroutes), "0"],
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for _ in range(200):
            line = self.p.stdout.readline()
            if b"READY" in line:
                break
        time.sleep(0.15)

    def alive(self):
        return self.p.poll() is None

    def kill(self):
        if self.p.poll() is None:
            self.p.kill()
        self.p.wait()


def rec(name, claim, verdict, detail):
    results.append({"probe": name, "claim": claim, "verdict": verdict, "detail": detail})
    print("[%-4s] %-36s %s" % (verdict, name, detail))


def req(payload, timeout=3.0, port=None):
    p = port or PORT
    s = socket.create_connection(("127.0.0.1", p), timeout=timeout)
    s.settimeout(timeout)
    s.sendall(payload if isinstance(payload, bytes) else payload.encode())
    data = b""
    try:
        while True:
            c = s.recv(65536)
            if not c:
                break
            data += c
            head, sep, body = data.partition(b"\r\n\r\n")
            if sep:
                cl = None
                for line in head.split(b"\r\n"):
                    if line.lower().startswith(b"content-length:"):
                        cl = int(line.split(b":", 1)[1])
                if cl is not None and len(body) >= cl:
                    break
    except socket.timeout:
        pass
    s.close()
    return data


def status(raw):
    try:
        return int(raw.split(b" ")[1])
    except Exception:
        return None


def bodyof(raw):
    return raw.partition(b"\r\n\r\n")[2]


GET = "GET {} HTTP/1.1\r\nHost: bench\r\nConnection: close\r\n\r\n"

# ---------------------------------------------- trie backtracking (routing bug)
sv = Server()
try:
    a = req(GET.format("/files/archive/list"))
    b = req(GET.format("/files/photo.png"))
    c = req(GET.format("/files/archive"))
    lit = status(a) == 200 and b'"literal"' in bodyof(a)
    par = status(b) == 200 and b'"param"' in bodyof(b)
    detail = ("/files/archive/list -> %s | /files/photo.png -> %s | "
              "/files/archive -> %s" %
              ("literal handler" if lit else "MISS(%s)" % status(a),
               "param handler" if par else "MISS(%s)" % status(b),
               status(c)))
    if lit and par and status(c) == 200:
        rec("trie backtracking", "literal beats param, both reachable", "PASS", detail)
    else:
        rec("trie backtracking", "literal beats param, both reachable", "FAIL",
            detail + "  <- /files/:name is shadowed: find() commits to the "
                     "literal child and never backtracks")
finally:
    sv.kill()

# ------------------------------------------------------ header value with colon
sv = Server()
try:
    r = req("GET /echo HTTP/1.1\r\nHost: localhost:8084\r\nConnection: close\r\n\r\n")
    j = json.loads(bodyof(r))
    host = j["headers"].get("Host", "<missing>")
    rec("header value containing ':'", "req.headers parsed correctly",
        "PASS" if host == "localhost:8084" else "FAIL",
        "sent 'Host: localhost:8084' -> parser stored %r" % host)
except Exception as e:
    rec("header value containing ':'", "req.headers parsed correctly", "FAIL", repr(e))
finally:
    sv.kill()

# ------------------------------------------------ request split across packets
sv = Server()
try:
    s = socket.create_connection(("127.0.0.1", PORT), timeout=3)
    s.settimeout(3)
    s.sendall(b"GET /plain HTTP/1.1\r\nHost: bench\r\n")
    time.sleep(0.3)
    s.sendall(b"Connection: close\r\n\r\n")
    data = b""
    try:
        while True:
            ch = s.recv(65536)
            if not ch:
                break
            data += ch
    except socket.timeout:
        pass
    s.close()
    ok = status(data) == 200 and b"hello" in data
    rec("request split across 2 packets", "HTTP framing", "PASS" if ok else "FAIL",
        ("handled" if ok else
         "got %d bytes, status=%s - one recv() per request assumes the whole "
         "request arrives in a single segment; server alive=%s"
         % (len(data), status(data), sv.alive())))
finally:
    sv.kill()

# ------------------------------------------------------------ pipelined requests
sv = Server()
try:
    s = socket.create_connection(("127.0.0.1", PORT), timeout=3)
    s.settimeout(2)
    one = "GET /plain HTTP/1.1\r\nHost: bench\r\nConnection: keep-alive\r\n\r\n"
    s.sendall((one * 2).encode())
    time.sleep(0.5)
    data = b""
    try:
        while True:
            ch = s.recv(65536)
            if not ch:
                break
            data += ch
    except socket.timeout:
        pass
    s.close()
    n = data.count(b"HTTP/1.1 200")
    rec("2 pipelined requests", "HTTP/1.1 pipelining", "PASS" if n == 2 else "FAIL",
        "sent 2 requests in one write -> %d responses (the second is discarded "
        "with the recv buffer)" % n)
finally:
    sv.kill()

# ------------------------------------------------------------------ NUL in body
sv = Server()
try:
    payload = b"AB\x00CD"
    r = req(b"POST /echobody HTTP/1.1\r\nHost: bench\r\nContent-Type: text/plain\r\n"
            b"Content-Length: %d\r\nConnection: close\r\n\r\n" % len(payload) + payload)
    j = json.loads(bodyof(r))
    ok = j["raw_len"] == len(payload)
    rec("body containing a NUL byte", "binary bodies survive", "PASS" if ok else "FAIL",
        "sent %d bytes -> handler saw %d (buffer is turned into a std::string "
        "with the char* constructor, which stops at the first NUL)"
        % (len(payload), j["raw_len"]))
except Exception as e:
    rec("body containing a NUL byte", "binary bodies survive", "FAIL", repr(e))
finally:
    sv.kill()

# ------------------------------------------------------------ body with CRLFCRLF
sv = Server()
try:
    payload = "line1\r\n\r\nline2"
    r = req("POST /echobody HTTP/1.1\r\nHost: bench\r\nContent-Type: text/plain\r\n"
            "Content-Length: %d\r\nConnection: close\r\n\r\n%s" % (len(payload), payload))
    j = json.loads(bodyof(r))
    ok = j["raw_len"] == len(payload)
    rec("body containing a blank line", "body delivered intact", "PASS" if ok else "FAIL",
        "sent %d bytes -> handler saw %d (%r): the buffer is split on every "
        "CRLFCRLF, not just the first"
        % (len(payload), j["raw_len"], j["raw_prefix"]))
except Exception as e:
    rec("body containing a blank line", "body delivered intact", "FAIL", repr(e))
finally:
    sv.kill()

# ---------------------------------------------------- head-of-line blocking
# One slow handler occupies a worker; with N workers, N slow requests stall
# everyone else. Demonstrates the cost of thread-per-connection with a small pool.
sv = Server()
try:
    import threading
    workers = 4
    done = []

    def slow():
        try:
            req(GET.format("/slow"), timeout=10)
        except Exception:
            pass
        done.append(1)

    ts = [threading.Thread(target=slow) for _ in range(workers)]
    for t in ts:
        t.start()
    time.sleep(0.2)
    t0 = time.time()
    try:
        r = req(GET.format("/plain"), timeout=8)
        dt = time.time() - t0
        rec("head-of-line blocking", "pool serves other clients meanwhile",
            "PASS" if dt < 0.2 else "FAIL",
            "%d concurrent 500ms handlers -> a trivial /plain request waited "
            "%.2fs (all %d workers were occupied)" % (workers, dt, workers))
    except Exception as e:
        rec("head-of-line blocking", "pool serves other clients meanwhile", "FAIL",
            "%d concurrent 500ms handlers -> /plain never answered (%s)"
            % (workers, type(e).__name__))
    for t in ts:
        t.join()
finally:
    sv.kill()

# ------------------------------------------------------------ HEAD returns a body
sv = Server()
try:
    r = req("HEAD /plain HTTP/1.1\r\nHost: bench\r\nConnection: close\r\n\r\n")
    n = len(bodyof(r))
    rec("HEAD response body", "RFC: HEAD must not return a body",
        "PASS" if n == 0 else "FAIL",
        "status=%s, %d body bytes returned" % (status(r), n))
finally:
    sv.kill()

# ---------------------------------------------------------------- 404 shape
sv = Server()
try:
    r = req(GET.format("/definitely/not/a/route"))
    rec("JSON 404", "unmatched routes get a JSON 404",
        "PASS" if status(r) == 404 else "FAIL",
        "status=%s body=%r" % (status(r), bodyof(r)[:60]))
finally:
    sv.kill()

print()
print(json.dumps(results, indent=1))
