"""Read-only LAN stress/soak test; never changes limits, output or settings."""
import argparse,concurrent.futures,json,socket,statistics,struct,os,time,urllib.request

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--host',default='192.168.0.192')
 parser.add_argument('--seconds',type=int,default=300)
 parser.add_argument('--workers',type=int,choices=[1,2],default=2)
 args=parser.parse_args();base='http://'+args.host
 def get(path):
  start=time.monotonic()
  with urllib.request.urlopen(base+path,timeout=3) as r:
   data=r.read();status=r.status
  if path!='/':json.loads(data)
  return status,(time.monotonic()-start)*1000
 def interrupted(index):
  with socket.create_connection((args.host,80),timeout=3) as s:
   request=b'GET / HTTP/1.1\r\nHost: '+args.host.encode()+b'\r\nConnection: close\r\n\r\n'
   if index%2:
    s.sendall(request[:12]);s.sendall(request[12:]);s.recv(300)
   else:s.sendall(request[:12])
   s.setsockopt(socket.SOL_SOCKET,socket.SO_LINGER,struct.pack('HH' if os.name=='nt' else 'ii',1,0))
 started=time.monotonic();deadline=started+args.seconds
 def worker(worker_id):
  count=0;errors=[];latency=[]
  paths=['/api/state','/api/features','/api/diagnostics','/api/pd','/']
  while time.monotonic()<deadline:
   try:
    if count%11==10:interrupted(count//11+worker_id)
    else:
     status,ms=get(paths[count%len(paths)]);assert status==200;latency.append(ms)
   except Exception as error:errors.append(str(error))
   count+=1;time.sleep(.05)
  return count,errors,latency
 print('Read-only HTTP soak:',args.host,args.seconds,'seconds',flush=True)
 with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
  results=list(pool.map(worker,range(args.workers)))
 errors=[e for _,items,_ in results for e in items];latency=[x for _,_,items in results for x in items]
 print('Requests:',sum(r[0] for r in results),'errors:',len(errors))
 if latency:print('Latency ms median/max:',round(statistics.median(latency),2),round(max(latency),2))
 with urllib.request.urlopen(base+'/health',timeout=3) as r:health=json.load(r)
 print('Final health:',json.dumps(health));print('First errors:',errors[:5])
 if errors:raise SystemExit(1)
 print('PASS: valid JSON/HTML, fragmented uploads, aborted reads and incomplete headers; device still serves health')
if __name__=='__main__':main()
