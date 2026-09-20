// JS wrapper over the PlusWeb WASM module.
//
//   import { loadPlusWeb } from './plusweb.mjs';
//   const pw = await loadPlusWeb();          // or loadPlusWeb('./plusweb.js')
//   pw.registerRoute('GET', '/users/:id');
//   pw.dispatch('GET', '/users/42');
//
// The module exports a plain C ABI that passes JSON strings; everything below is
// argument marshalling and JSON.parse.

export async function loadPlusWeb(modulePath = './plusweb.js') {
  const { default: createPlusWeb } = await import(modulePath);
  const m = await createPlusWeb();

  const str = (fn, argTypes) => m.cwrap(fn, 'string', argTypes);
  const raw = {
    version: str('pw_version', []),
    reset: m.cwrap('pw_reset', null, []),
    registerRoute: str('pw_register_route', ['string', 'string']),
    dispatch: str('pw_dispatch', ['string', 'string']),
    parseRequest: str('pw_parse_request', ['string']),
    benchDispatch: m.cwrap('pw_bench_dispatch', 'number', ['string', 'string', 'number']),
    dumpTrie: str('pw_dump_trie', []),
  };

  return {
    /** -> { version, commit, builtAt } */
    version: () => JSON.parse(raw.version()),

    /** Drops every route and the whole node graph. */
    reset: () => raw.reset(),

    /** -> routeId (number). Throws an Error carrying the reason on failure. */
    registerRoute(method, pattern) {
      const r = JSON.parse(raw.registerRoute(method, pattern));
      if (r.error !== undefined) {
        const e = new Error(r.error);
        if (r.routeId !== undefined) e.routeId = r.routeId;
        throw e;
      }
      return r.routeId;
    },

    /** -> { matched, routeId, params, ms }  (plus `pattern` when matched) */
    dispatch: (method, path) => JSON.parse(raw.dispatch(method, path)),

    /** -> { ok, method, path, version, headers, bodyLength, error } */
    parseRequest: (rawText) => JSON.parse(raw.parseRequest(rawText)),

    /** -> nanosPerOp (number). Best of 3 runs; -1 if iters <= 0. */
    benchDispatch: (method, path, iters) => raw.benchDispatch(method, path, iters),

    /** -> { root, routeCount, routes, note } */
    dumpTrie: () => JSON.parse(raw.dumpTrie()),

    /** Escape hatch: the underlying Emscripten module. */
    module: m,
  };
}
