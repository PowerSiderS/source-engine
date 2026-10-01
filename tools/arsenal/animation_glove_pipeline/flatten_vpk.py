"""Stream Valve multi-chunk VPK payloads into a Source-compatible VPK v2."""
from pathlib import Path
import struct,zlib
def flatten(index,output):
 with index.open('rb') as stream:
  magic,version,size=struct.unpack('<III',stream.read(12))
  if magic!=0x55aa1234 or version not in (1,2):raise ValueError('Invalid VPK')
  header=12 if version==1 else 28;stream.seek(header);tree=stream.read(size)
 pos=0;entries=[];newtree=bytearray();offset=0
 def token():
  nonlocal pos
  end=tree.index(0,pos);value=tree[pos:end];pos=end+1;return value
 while ext:=token():
  newtree+=ext+b'\0'
  while folder:=token():
   newtree+=folder+b'\0'
   while name:=token():
    crc,pre,part,start,length,term=struct.unpack_from('<IHHIIH',tree,pos);pos+=18
    if term!=65535:raise ValueError('Invalid terminator')
    preload=tree[pos:pos+pre];pos+=pre;total=pre+length
    if offset+total>0xffffffff:raise ValueError('Single VPK exceeds 32-bit offset range')
    newtree+=name+b'\0'+struct.pack('<IHHIIH',crc,0,32767,offset,total,65535)
    chunk=index if part==32767 else index.with_name(index.name.replace('_dir.vpk',f'_{part:03d}.vpk'))
    entries.append((chunk,(header+size if part==32767 else 0)+start,length,preload,crc));offset+=total
   newtree+=b'\0'
  newtree+=b'\0'
 newtree+=b'\0'
 with output.open('wb') as target:
  target.write(struct.pack('<7I',magic,2,len(newtree),offset,0,0,0));target.write(newtree)
  for chunk,start,length,preload,crc in entries:
   actual=zlib.crc32(preload);target.write(preload)
   with chunk.open('rb') as source:
    source.seek(start);remaining=length
    while remaining:
     data=source.read(min(8*1048576,remaining))
     if not data:raise ValueError('Truncated VPK payload')
     target.write(data);actual=zlib.crc32(data,actual);remaining-=len(data)
   if actual!=crc:raise ValueError('Input VPK CRC mismatch')
