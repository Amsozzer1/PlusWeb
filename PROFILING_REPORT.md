# PlusWeb vs Express — single-threaded, head to head

**Date:** 2026-09-20 (re-profiled after the six correctness/performance fixes)
**PlusWeb:** current tree (libuv event loop + llhttp parser + routing trie)
**Express:** 5.2.1 on Node v25.6.1 (libuv event loop + llhttp parser + linear router)
**Host:** Intel i5-7600, 4 cores, `powersave` governor, Linux 7.0.11, GCC 16.1.1

Both servers run as **one process on one thread, pinned to CPU 0**. The load
generator is pinned to CPUs 1-3. Routes, handlers and payloads are identical on
both sides. Same event loop, same HTTP parser, same I/O model — what differs is
the language above them and the router design.

**Control:** a bare epoll server doing no parsing and no routing, pinned
identically, reaches **164,736 rps** at 32 connections (55,033 at 1, 154,191 at
512). Every number below reads against that ceiling.

---

## What changed since the last profile

Six fixes landed: header size limit, HEAD handling, parameter-child pointer in
the trie, middleware by reference, writev, absolute-form targets. Measured
back-to-back against the pre-fix binary on this machine:

| Dimension | Before | After | Change |
|---|---|---|---|
| 404 miss, 1,000 routes | 35,608 | 74,977 | **2.1x** |
| 404 miss, 10,000 routes | 5,847 | 76,742 | **13.1x** |
| 16 middleware | 69,859 | 75,291 | +7.8% |
| 100 KB body | 11,002 | 11,897 | +7.5% |
| 10 KB body | 53,656 | 57,402 | +6% |
| 5 B body | 88,545 | 90,820 | +2% |
| `/files/archive` with `/files/:name` | 404 | matches param | fixed |
| `HEAD /plain` | 404, 53-byte body | 200, 0-byte body | fixed |
| 1 MB of headers | accepted, 200 | 431 / connection closed | fixed |
| absolute-form URI | 404 | 200 | fixed |
| **RSS at 10,000 routes** | **13.9 MB** | **24.2 MB** | **+74% (cost)** |

The memory growth is the price of `GET` also registering `HEAD`: the trie now
holds two entries per route. It is the only regression, and PlusWeb is still
5.6x smaller than Express at that route count.

---

## The headline depends on your route table

Express matches routes by walking its layer stack in registration order, so its
throughput depends on **where the matching route sits**. PlusWeb walks a trie and
does not care — now for misses as well as hits.

| Scenario | PlusWeb | Express | Ratio |
|---|---|---|---|
| Small app (5 routes) | 91,950 | 19,236 | **4.8x** |
| 1,000 routes, early route | 89,293 | 16,885 | **5.3x** |
| 1,000 routes, last route | 90,085 | 5,639 | **16x** |
| 10,000 routes, middle route | 89,885 | 678 | **133x** |
| 10,000 routes, last route | 89,636 | 355 | **253x** |

*(rps, 32 connections, keep-alive)*

**If you quote one number, quote 4.8x.** That is a small app, which is what most
apps are, and the case Express looks best under. The larger multiples measure
Express's router degrading, not PlusWeb speeding up — PlusWeb is flat at ~90k
across every row.

---

## 1. Throughput and latency vs concurrency

**Small route table (5 routes) — the fair baseline:**

| Conns | PlusWeb | Express | Ratio | PW p99 | EX p99 | PW max | EX max |
|---|---|---|---|---|---|---|---|
| 1 | 38,140 | 17,095 | 2.2x | 0.075 ms | 0.143 ms | 3.17 ms | 12.37 ms |
| 4 | 82,514 | 19,073 | 4.3x | 0.103 | 0.466 | 2.93 | 2.80 |
| 32 | 91,950 | 19,236 | 4.8x | 0.545 | 2.329 | 3.38 | 9.18 |
| 128 | 90,072 | 18,862 | 4.8x | 1.953 | 7.766 | 4.78 | **397.2** |
| 512 | 82,231 | 18,472 | 4.5x | 7.013 | 29.318 | 20.67 | **4,804.2** |

**1,000 routes, hitting `/plain` (registered last):**

| Conns | PlusWeb | Express | Ratio | Express starved |
|---|---|---|---|---|
| 1 | 40,005 | 5,253 | 7.6x | 0 |
| 4 | 84,558 | 6,339 | 13.3x | 0 |
| 16 | 90,928 | 6,363 | 14.3x | 0 |
| 64 | 89,288 | 6,332 | 14.1x | 0 |
| 128 | 88,649 | 6,273 | 14.1x | 0 |
| 256 | 86,831 | 6,089 | 14.3x | **10** |
| 512 | 83,849 | 6,195 | 13.5x | **264** |

At 32 connections PlusWeb is at **55% of the bare-epoll ceiling** while doing
full HTTP parsing, a 1,000-route lookup and response serialization. Express is at
3.9%.

Beyond throughput: at 512 connections Express's worst request took **4,988 ms**
against PlusWeb's 20 ms, and it left **264 of 512 connections with no response at
all**. PlusWeb starved zero connections in every run at every level.

---

## 2. Where the gap narrows

### Large response bodies

Measured as two passes on fresh processes each, since this is the noisiest row.

| Payload | PlusWeb | Express | Ratio |
|---|---|---|---|
| 5 B | 94,270 | 5,841 | 16.1x |
| 1 KB | 90,385 | 6,098 | 14.8x |
| 10 KB | 59,014 | 5,596 | 10.5x |
| **100 KB** | **11,941** | **2,679** | **4.5x** |

Still the weakest dimension. writev helped (+7.5% at 100 KB) but once the body
dominates, both servers are mostly moving bytes and the language stops mattering.

### Connection churn (no keep-alive)

| Mode | PlusWeb | Express | Ratio |
|---|---|---|---|
| keep-alive | 90,972 | 6,383 | 14.3x |
| `Connection: close` | 18,063 | 4,391 | **4.1x** |

PlusWeb falls **5.0x** without keep-alive; Express only **1.5x**. A fresh `Conn`,
a fresh `HttpParser` and an `llhttp_init` per connection cost PlusWeb
proportionally much more. Unchanged by this round of fixes, and the clearest
remaining optimization target — pooling parsers across connections would attack
it directly.

### Middleware chain

| Middleware | PlusWeb | Express |
|---|---|---|
| 0 | 92,634 | 15,549 |
| 1 | 89,669 | 16,015 |
| 4 | 87,453 | 15,404 |
| 16 | 76,840 | 14,912 |

Returning `const&` improved this (69,859 to 76,840 at 16 middleware) but did not
flatten it: **-17% at 16 middleware against Express's -4%**. The remaining cost is
`executeMiddlewareChain` heap-allocating a `std::function<void()>` for `next` at
every level, which a reference return cannot fix.

---

## 3. Routing — now flat in every direction

Throughput by where the matching route sits (32 connections):

| Routes | PlusWeb first / middle / last | Express first / middle / last |
|---|---|---|
| 100 | 90,195 / 91,663 / 91,195 | 18,544 / 17,306 / 16,136 |
| 1,000 | 89,293 / 90,666 / 90,085 | 16,885 / 8,579 / 5,639 |
| 10,000 | 90,680 / 89,885 / 89,636 | 17,665 / **678** / **355** |

Express spreads **50x** between its first and last route at 10,000 routes.
PlusWeb varies by under 2% across a 100x range of table sizes and every position.

### The 404 path is fixed

| Routes | PlusWeb miss | Express miss |
|---|---|---|
| 10 | 74,797 | 22,214 |
| 100 | 75,277 | 18,376 |
| 1,000 | 70,036 | 6,044 |
| 10,000 | **76,568** | **393** |

Previously this degraded 12x from 10 to 10,000 routes. It is now flat, because a
parameter child has its own pointer instead of being found by scanning every
child of the failing node. **195x Express at 10,000 routes.**

---

## 4. Memory and startup

| | PlusWeb idle | PlusWeb loaded | Express idle | Express loaded |
|---|---|---|---|---|
| 10 routes | **4.6 MB** | 4.9 MB | 92.1 MB | 115.7 MB |
| 1,000 routes | 6.5 MB | 6.8 MB | 97.7 MB | 122.7 MB |
| 10,000 routes | 24.2 MB | 24.4 MB | 134.9 MB | 168.4 MB |

**20x smaller idle, 24x under load** at small route counts, narrowing to 5.6x at
10,000 routes now that every `GET` also occupies a `HEAD` slot in the trie.
PlusWeb's footprint barely moves under traffic (+0.3 MB) where Express grows
~24 MB.

Boot to first request: **~0.00 s vs 0.11 s** (0.02 s vs 0.25 s at 10,000 routes).

---

## 5. HTTP correctness — now identical

Every check, one request against a freshly started server:

| Check | PlusWeb | Express |
|---|---|---|
| valid GET | 200 | 200 |
| garbage request line | 400 | 400 |
| request line with no path/protocol | 400 | 400 |
| bare CRLFCRLF | no response | no response |
| header with no colon | 400 | 400 |
| 2 KB of headers | 200 | 200 |
| **64 KB of headers** | **connection dropped** | connection dropped |
| bad / negative / duplicate `Content-Length` | 400 | 400 |
| unknown method | 400 | 400 |
| HTTP/1.0 | 200 | 200 |
| chunked request body | 200, body intact | *(harness artifact)* |
| 3 pipelined requests | 3 responses | 3 responses |
| request split mid-header | 200 | 200 |
| **`HEAD /plain`** | **200, 0-byte body** | 200, 0-byte body |
| **absolute-form URI** | **200** | 200 |
| literal route / param route | correct | correct |
| **`/files/archive` under `/files/:name`** | **matches param** | matches param |

**PlusWeb now matches Express on every row.** The four gaps from the last profile
— no header limit, broken HEAD, unhandled absolute-form targets, parameter routes
shadowed by literal siblings — are all closed. A 17 KB header gets a clean `431`;
64 KB and above are dropped mid-stream, which is what Node does too.

Express's `raw_len: 0` on the chunked body is this benchmark app's
`express.text()` configuration, not a Node defect.

### Head-of-line blocking: still a tie

| | PlusWeb | Express |
|---|---|---|
| trivial request behind one 500 ms blocking handler | 0.35 s | 0.36 s |

Both are single-threaded event loops. Neither has an answer beyond "do not block
the loop".

---

## 6. Where PlusWeb's time goes

32 connections, 12 s, `/plain`, 1,000 routes, 8,996 samples at 1 kHz:

| flat % | symbol |
|---|---|
| ~71% | libc syscall stubs (`send`/`recv`) |
| 3.0% | `llhttp__internal__run` |
| 2.3% | `syscall` |
| 1.8% | `HttpParser::Impl::onMessageComplete` |
| 1.2% | `std::string::_M_append` |
| 0.8% | `malloc` |
| 0.6% | `HttpServer::handleRequest` |
| 0.5% | `HttpResponse::serialize` |
| 0.5% | `Node::find` (the routing trie) |

libc has no DWARF here, so libc frame names come from nearest-symbol lookup and
are not reliable; the ~71% is attributed to the socket syscalls by elimination,
since every framework symbol is individually accounted for below it.

Routing and response serialization are now **half a percent each**. The framework
is thoroughly syscall-bound, so further gains must come from doing fewer syscalls
(batched writes across connections, `io_uring`) rather than faster userspace code.

---

## 7. Honest summary

**PlusWeb wins on:** throughput (4.8x small app, 253x with a large route table),
tail latency (248x lower worst case at 512 connections), memory (20x), startup
(~10x), route-table scaling (flat against a 50x spread), the 404 path (195x), and
never starving a connection at any concurrency tested.

**PlusWeb loses on:** connection churn (advantage drops to 4.1x), large bodies
(4.5x at 100 KB), and middleware scaling (-17% at 16 middleware against Express's
-4%). It also now costs 74% more memory at 10,000 routes than before the HEAD fix.

**Dead even on:** every HTTP correctness check, and head-of-line blocking.

**And the part no benchmark shows:** Express has sessions, cookies, static file
serving, body parsers, template engines, TLS and a decade of found edge cases.
PlusWeb has a fast, now-correct core. The performance argument is settled; that
was never why anyone picks Express.

### What to fix next, in order

1. **Connection churn** — pool parsers and connection objects instead of
   allocating per connection. The 4.1x row is the biggest remaining gap.
2. **Middleware `next` closures** — pass an index and a chain pointer instead of
   heap-allocating a `std::function` per level.
3. **Body size limits and idle timeouts** — headers are capped now, bodies are not.
4. **Deduplicate the HEAD registration** — store one handler with a method mask
   rather than two trie entries, recovering the 74% memory growth.
5. **TLS**, if this is ever meant to face a network directly.

---

## Methodology

`bench/` holds the harness; `bench/README.md` has the commands.

**Load generation.** `pw_loadgen`, a closed-loop epoll client, one request in
flight per connection, pinned to CPUs 1-3. It reports `starved_conns` — the
number of connections that completed *zero* requests — alongside percentiles,
because throughput and p99 alone cannot distinguish "fast" from "fast for the few
connections being served".

**Parity.** Identical routes, handlers and payloads on both sides, built from the
same fixed strings. One process, one thread, pinned to CPU 0. Each gets a
2-second warm-up before every measurement so Node's JIT is hot.

**Two traps worth recording:**

- *Route position, not route count.* Hitting `/r1/...` — Express's second
  registered route — makes Express look flat in route count. It is flat in route
  *position*. The position sweep and the small-table sweep exist to correct for
  that.
- *Cross-session variance.* The 10 KB payload row read 76,458 rps in one sweep and
  ~59,000 in eight subsequent measurements across fresh processes. Within a single
  server process it is stable to ±0.5%; the outlier was not reproducible. Payload
  figures here are two passes on fresh processes, and the before/after comparisons
  were run back-to-back against both binaries in one sitting. Treat single
  measurements from different sessions as unreliable at the ±5% level, and the
  10 KB row more loosely than that.

**Caveats.** Single host, `powersave` governor, loopback only — no real network,
no TLS, no competing services. Absolute rps is conservative; ratios are the
durable figures. Express results depend on route count and position, so every
Express row states both.
