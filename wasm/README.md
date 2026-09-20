# PlusWeb WebAssembly contract

Exposes PlusWeb's **routing trie** and **HTTP parser** to WebAssembly. There are
no sockets in WASM, so the event loop is not part of this — libuv is not linked
and `HttpServer` is not compiled in. What you get is the pure request-handling
core: register routes, dispatch paths, parse raw requests, inspect the trie.

## The contract

| Function | Returns |
|---|---|
| `version()` | `{ version, commit, builtAt }` |
| `reset()` | — drops every route and the node graph |
| `registerRoute(method, pattern)` | `routeId` (number), or throws on error |
| `dispatch(method, path)` | `{ matched, routeId, params, ms }` (plus `pattern` when matched) |
| `parseRequest(rawText)` | `{ ok, method, path, version, headers, bodyLength, error }` |
| `benchDispatch(method, path, iters)` | `nanosPerOp` (number) |
| `dumpTrie()` | `{ root, routeCount, routes, note }` |

## Use

```js
import { loadPlusWeb } from './plusweb.mjs';

const pw = await loadPlusWeb('./plusweb.js');

pw.registerRoute('GET', '/users/:id');   // -> 0
pw.dispatch('GET', '/users/42');
// { matched: true, routeId: 0, params: { id: '42' }, ms: 0.0052,
//   pattern: 'GET /users/:id' }

pw.parseRequest('GET /search?q=cats HTTP/1.1\r\nHost: h\r\n\r\n');
// { ok: true, method: 'GET', path: '/search', version: '1.1',
//   headers: { Host: 'h' }, bodyLength: 0, error: null, query: { q: 'cats' } }

pw.benchDispatch('GET', '/users/42', 100000);   // -> 458.3
```

## Build

**WebAssembly** (needs [emsdk](https://emscripten.org/docs/getting_started/downloads.html)):

```bash
emcmake cmake -B build-wasm -DPLUSWEB_BUILD_WASM=ON -DPLUSWEB_BUILD_TESTS=OFF \
                            -DPLUSWEB_BUILD_EXAMPLES=OFF
cmake --build build-wasm
node wasm/test_contract.mjs ../build-wasm/wasm/plusweb.js
```

Produces `plusweb.js` + `plusweb.wasm`, built as an ES6 module
(`-sMODULARIZE -sEXPORT_ES6`) with growable memory.

**Native** — the same translation units, so the contract is testable without
Emscripten:

```bash
cmake -B build -DPLUSWEB_BUILD_WASM=ON
cmake --build build
./build/wasm/pw_wasm_contract_test
```

This also registers as a CTest case (`wasm_contract`).

## Notes on behaviour

- **Route ids** are assigned in registration order from 0, and restart after
  `reset()`. Registering the same `method + pattern` twice is an error; the error
  carries the existing `routeId`.
- **`ms` in `dispatch`** times the trie lookup only, not the JSON marshalling.
  It is a single sample — use `benchDispatch` for anything you intend to quote.
- **`benchDispatch`** warms up, then reports the best of 3 runs, so one scheduler
  hiccup does not become the number. Returns `-1` for `iters <= 0`.
- **`version`** in `parseRequest` is the bare number (`"1.1"`), not `"HTTP/1.1"`.
- **`version()` fields are stamped at CMake configure time**, not at build time.
  Re-run `cmake` to refresh the commit and timestamp. A dirty tree is reported as
  `<sha>-dirty`.
- **Returned pointers** are owned by the module and valid until the next call to
  *that same function*. The JS wrapper copies into JS strings immediately, so
  this only matters if you call the C ABI directly.
- **Single-threaded.** State is a process-wide singleton, which is what a WASM
  instance gives you. One instance, one route table.

## `dumpTrie()` shape

Trie keys are `METHOD:/path` split on `/`, so **depth 1 holds the methods**
(`"GET:"`, `"POST:"`). After registering `GET /users/:id` and `POST /users`:

```json
{
  "root": {
    "value": "", "isLeaf": false, "isParameter": false, "hasHandler": false,
    "children": [
      { "value": "GET:", "isLeaf": false, "isParameter": false, "hasHandler": false,
        "children": [
          { "value": "users", "isLeaf": false, "isParameter": false, "hasHandler": false,
            "children": [
              { "value": ":id", "isLeaf": true, "isParameter": true,
                "parameterName": "id", "hasHandler": true, "children": [] }
            ] }
        ] },
      { "value": "POST:", "...": "..." }
    ]
  },
  "routeCount": 2,
  "routes": [
    { "routeId": 0, "route": "GET /users/:id" },
    { "routeId": 1, "route": "POST /users" }
  ]
}
```

Children are sorted by `value` so the dump is stable — the underlying container
is an `unordered_map`, whose iteration order is not.

## Router behaviour worth checking here

Both router bugs this contract was written to surface are now fixed, and
`dispatch()` is the cheapest way to confirm it:

- A literal route no longer **shadows its parameter sibling**. Register
  `GET /files/:name` and `GET /files/archive/list`, then dispatch
  `/files/archive`: it matches the parameter route and binds `name=archive`.
- A **miss no longer scans every child** of the failing node, now that a
  parameter child has its own pointer. `benchDispatch` on an unmatched path is
  flat against table size — about 150 ns/op at 10, 1,000 and 10,000 siblings,
  where it used to degrade with the table.

For reference, the module built by CI runs a hit at roughly 170 ns/op against
147 ns/op for the same translation units compiled natively.
