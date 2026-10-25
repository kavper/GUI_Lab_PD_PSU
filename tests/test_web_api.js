'use strict';
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync('Board/remote.js','utf8');
const api=source.slice(source.indexOf('async function api('),source.indexOf('function connected('));
async function main(){
 let cleared=0,delay=0,seen;
 const success=vm.createContext({AbortController,fetch:async(path,options)=>{seen=options;return{ok:true,json:async()=>({fresh:true})}},setTimeout:(fn,ms)=>{delay=ms;return setTimeout(fn,10000)},clearTimeout:id=>{cleared++;clearTimeout(id)}});
 vm.runInContext(api,success);assert((await success.api('/api/state')).fresh);assert.equal(delay,3000);assert.equal(cleared,1);assert(seen.signal);
 await success.api('/api/command',{output:0});assert.equal(seen.method,'POST');assert.equal(seen.body,'{"output":0}');
 const stalled=vm.createContext({AbortController,fetch:async(path,options)=>new Promise((resolve,reject)=>options.signal.addEventListener('abort',()=>{const e=new Error('Aborted');e.name='AbortError';reject(e)})),setTimeout:(fn,ms)=>{assert.equal(ms,3000);return setTimeout(fn,1)},clearTimeout});
 vm.runInContext(api,stalled);await assert.rejects(stalled.api('/api/state'),e=>e.name==='AbortError');
 const bodyStalled=vm.createContext({AbortController,fetch:async(path,options)=>({ok:true,json:()=>new Promise((resolve,reject)=>options.signal.addEventListener('abort',()=>{const e=new Error('Aborted body');e.name='AbortError';reject(e)}))}),setTimeout:fn=>setTimeout(fn,1),clearTimeout});
 vm.runInContext(api,bodyStalled);await assert.rejects(bodyStalled.api('/api/state'),e=>e.name==='AbortError');
 const status=vm.createContext({AbortController,fetch:async()=>({ok:false,status:503}),setTimeout,clearTimeout});vm.runInContext(api,status);await assert.rejects(status.api('/api/state'),/HTTP 503/);
 console.log('PASS browser API: 3-second abort, full-response timeout, timer cleanup, preserved JSON POST and HTTP error propagation');
}
main().catch(e=>{console.error(e);process.exitCode=1});
