'use strict';
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync('Board/remote_suite.js','utf8');
const start=source.indexOf('const axisState='),end=source.indexOf('function plot(',start);
const context=vm.createContext({});vm.runInContext(source.slice(start,end),context);
const range=(id,values,min,now)=>context.axisRange(id,values,min,now);
let v=range('v',[12.001,12.002],.05,0),i=range('i',[.001,.002],.005,0);
assert(v.hi-v.lo<.2&&v.lo<12.001&&v.hi>12.002,'Voltage zooms around signal, not zero');
assert(i.hi-i.lo<.02,'Current has its own scale');
v=range('v',[5,18],.05,1);assert(v.lo<=5&&v.hi>=18,'Immediate expansion');
const wide=v.hi-v.lo;v=range('v',[12.001],.05,2);assert.equal(v.hi-v.lo,wide);
v=range('v',[12.001],.05,5001);assert.equal(v.hi-v.lo,wide);
v=range('v',[12.001],.05,5003);assert(v.hi-v.lo<.2,'Five-second contraction');
for(const value of [0,-.001,-5,27,100000]){const a=range('constant'+value,[value],.005,0);assert(Number.isFinite(a.lo)&&Number.isFinite(a.hi)&&a.hi>a.lo&&a.lo<=value&&a.hi>=value)}
assert.equal(context.niceStep(.004),.005);assert.equal(context.niceStep(.011),.02);
console.log('PASS browser chart axes: independent V/A, constant/zero/negative, zoom without zero anchor, immediate expansion, delayed shrink, nice 1/2/5 ticks');

vm.runInContext(source.slice(source.indexOf('function decodePdo('),source.indexOf('function renderPd(')),context);
let p=context.decodePdo((100<<10)|300);assert.equal(p.name,'FIXED');assert.equal(p.max,5);assert.equal(p.limit,3);
p=context.decodePdo((0xc0000000|(110<<17)|(33<<8)|60)>>>0);assert.equal(p.name,'PPS');assert(Math.abs(p.min-3.3)<1e-9);assert.equal(p.max,11);assert.equal(p.limit,3);
p=context.decodePdo((0x40000000|(240<<20)|(100<<10)|180)>>>0);assert.equal(p.unit,'W');assert.equal(p.limit,45);
console.log('PASS browser PDO decode: fixed, PPS, battery power units');
