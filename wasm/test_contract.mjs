// Node test for the built WASM module. Mirrors wasm/test_contract.cpp, which is
// the same contract exercised natively.
//
//   node wasm/test_contract.mjs ../build-wasm/wasm/plusweb.js
import { loadPlusWeb } from './plusweb.mjs';

const modulePath = process.argv[2] || '../build-wasm/wasm/plusweb.js';
const pw = await loadPlusWeb(modulePath);

let failures = 0;
const check = (cond, what, detail = '') => {
  console.log(`  [${cond ? 'PASS' : 'FAIL'}] ${what}${detail ? '  -> ' + detail : ''}`);
  if (!cond) failures++;
};
const eq = (a, b, what) => check(JSON.stringify(a) === JSON.stringify(b), what,
                                 JSON.stringify(a));

console.log('== version() ==');
const v = pw.version();
check(v.version && v.commit && v.builtAt, 'returns { version, commit, builtAt }',
      JSON.stringify(v));

console.log('\n== reset() + registerRoute() ==');
pw.reset();
eq(pw.registerRoute('GET', '/users/:id'), 0, 'first route gets id 0');
eq(pw.registerRoute('GET', '/users/new'), 1, 'ids increment');
eq(pw.registerRoute('POST', '/users'), 2, 'ids span methods');
let threw = false;
try { pw.registerRoute('GET', '/users/:id'); } catch (e) { threw = true; }
check(threw, 'duplicate route throws');
threw = false;
try { pw.registerRoute('GET', 'no-slash'); } catch (e) { threw = true; }
check(threw, "pattern without leading '/' throws");

console.log('\n== dispatch() ==');
const d = pw.dispatch('GET', '/users/42');
check(d.matched && d.routeId === 0, 'parameter route matches', JSON.stringify(d));
eq(d.params, { id: '42' }, 'binds :id');
check(typeof d.ms === 'number' && d.ms >= 0, 'reports ms');
eq(pw.dispatch('GET', '/users/new').routeId, 1, 'literal beats parameter');
check(!pw.dispatch('GET', '/nope').matched, 'miss reports matched=false');
check(!pw.dispatch('DELETE', '/users/42').matched, 'method is part of the key');

console.log('\n== parseRequest() ==');
const p = pw.parseRequest(
  'GET /search?q=cats HTTP/1.1\r\nHost: localhost:8084\r\n\r\n');
check(p.ok, 'valid request parses', JSON.stringify(p).slice(0, 110));
eq(p.method, 'GET', 'method');
eq(p.path, '/search', 'path excludes the query');
eq(p.version, '1.1', 'version');
eq(p.headers.Host, 'localhost:8084', 'header value keeps its colon');
eq(p.bodyLength, 0, 'bodyLength 0 for a bodyless GET');
const pb = pw.parseRequest(
  'POST /echo HTTP/1.1\r\nHost: h\r\nContent-Length: 11\r\n\r\nhello world');
eq(pb.bodyLength, 11, 'body length is reported');
check(!pw.parseRequest('GARBAGE\r\n\r\n').ok, 'garbage reports an error, no crash');
check(!pw.parseRequest('GET /x HTTP/1.1\r\nHost: h\r\n').ok, 'incomplete is not ok');

console.log('\n== benchDispatch() ==');
const ns = pw.benchDispatch('GET', '/users/42', 20000);
check(ns > 0, 'returns nanos/op', `${ns.toFixed(1)} ns`);
check(pw.benchDispatch('GET', '/x', 0) < 0, 'iters<=0 returns -1');

console.log('\n== dumpTrie() ==');
const t = pw.dumpTrie();
eq(t.routeCount, 3, 'routeCount matches registrations');
const walk = (n, f) => { f(n); n.children.forEach((c) => walk(c, f)); };
let sawGet = false, sawParam = false;
walk(t.root, (n) => {
  if (n.value === 'GET:') sawGet = true;
  if (n.isParameter && n.parameterName === 'id') sawParam = true;
});
check(sawGet, "depth 1 holds the method key 'GET:'");
check(sawParam, "parameter node ':id' present with its name");

console.log('\n== reset() clears everything ==');
pw.reset();
eq(pw.dumpTrie().routeCount, 0, 'routeCount back to 0');
check(!pw.dispatch('GET', '/users/42').matched, 'old route no longer matches');
eq(pw.registerRoute('GET', '/users/:id'), 0, 'ids restart after reset');

console.log(`\n${failures ? 'FAILED' : 'ALL CHECKS PASSED'} (${failures} failures)`);
process.exit(failures ? 1 : 0);
