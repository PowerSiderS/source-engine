from pathlib import Path
import struct,zlib
class VPK:
 def __init__(self,path):
  self.path=Path(path);self.entries={}
  with self.path.open('rb') as f:
   sig,version,size=struct.unpack('<III',f.read(12));assert sig==0x55aa1234 and version in (1,2)
   header=12 if version==1 else 28;f.seek(header);tree=f.read(size)
  pos=0
  def token():
   nonlocal pos
   end=tree.index(0,pos);v=tree[pos:end].decode();pos=end+1;return v
  while ext:=token():
   while folder:=token():
    while name:=token():
     crc,pre,part,offset,length,term=struct.unpack_from('<IHHIIH',tree,pos);pos+=18;assert term==65535
     preload=tree[pos:pos+pre];pos+=pre
     key=((folder+'/' if folder!=' ' else '')+name+'.'+ext).lower()
     chunk=self.path if part==32767 else self.path.with_name(self.path.name.replace('_dir.vpk',f'_{part:03d}.vpk'))
     self.entries[key]=(chunk,offset+(header+size if part==32767 else 0),length,preload,crc)
 def get(self,name):
  chunk,offset,length,pre,crc=self.entries[name.lower()]
  with chunk.open('rb') as f:f.seek(offset);data=pre+f.read(length)
  assert zlib.crc32(data)==crc and len(data)==len(pre)+length
  return data
if __name__=='__main__':
 import sys,json
 v=VPK(sys.argv[1]);prefix=sys.argv[2] if len(sys.argv)>2 else 'scripts/items/'
 print(json.dumps([n for n in v.entries if n.startswith(prefix)],indent=2))
