"""Collect local material dependencies for a private Source 1 authoring stage."""
import argparse, hashlib, json, re, struct, zlib
from pathlib import Path


class Resources:
    def __init__(self, roots):
        self.files={}
        for root in roots:
            for path in sorted(root.rglob('*'),key=lambda p:len(p.parts)):
                if not path.is_file():continue
                parts=path.parts
                if path.suffix.lower() in ('.vmt','.vtf') and 'materials' in [p.lower() for p in parts]:
                    start=[p.lower() for p in parts].index('materials')
                    key='/'.join(parts[start:]).lower();self.files.setdefault(key,path)
            for archive in root.rglob('*.vpk'):
                if '_dir' not in archive.stem and re.search(r'_\d{3}$',archive.stem):continue
                self.add_vpk(archive)

    def add_vpk(self,archive):
        with archive.open('rb') as stream:
            header=stream.read(28);sig,version,length=struct.unpack_from('<III',header)
            if sig != 0x55aa1234 or version not in (1,2):return
            base=(12 if version==1 else 28)+length
            stream.seek(base-length);tree=stream.read(length)
        pos=0
        def token():
            nonlocal pos
            end=tree.index(0,pos);value=tree[pos:end].decode();pos=end+1;return value
        while ext:=token():
            while folder:=token():
                while stem:=token():
                    crc,pre,part,offset,size,term=struct.unpack_from('<IHHIIH',tree,pos);pos+=18
                    payload=tree[pos:pos+pre];pos+=pre
                    name=((folder+'/' if folder!=' ' else '')+stem+'.'+ext).replace('\\','/').lower()
                    if ext not in ('vmt','vtf') or not name.startswith('materials/'):continue
                    assert term==65535
                    chunk=archive if part==32767 else archive.with_name(archive.name.replace('_dir.vpk',f'_{part:03d}.vpk'))
                    self.files.setdefault(name,(chunk,base+offset if part==32767 else offset,size,payload,crc))

    def get(self,name):
        value=self.files.get(name.replace('\\','/').lower())
        if value is None:return None
        if isinstance(value,Path):return value.read_bytes()
        chunk,offset,size,pre,crc=value
        with chunk.open('rb') as stream:stream.seek(offset);payload=pre+stream.read(size)
        if zlib.crc32(payload)!=crc:raise ValueError('VPK CRC mismatch: '+name)
        return payload


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--roots',type=Path,nargs='+',required=True)
    parser.add_argument('--catalog',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();resources=Resources(args.roots);missing=[];copied={}
    def collect(name):
        name=name.replace('\\','/').lower()
        if name in copied:return True
        payload=resources.get(name)
        if payload is None:missing.append(name);return False
        destination=args.output/name
        if not destination.resolve().is_relative_to(args.output.resolve()):raise ValueError(name)
        destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(payload)
        copied[name]=hashlib.sha256(payload).hexdigest()
        if name.endswith('.vmt'):
            text=payload.decode('utf-8-sig',errors='replace')
            for field,value in re.findall(r'"?(\$\w+|include)"?\s+"([^"\r\n]+)"',text):
                if field.lower() in ('$basetexture','$bumpmap','$phongexponenttexture','$envmapmask','$detail','$lightwarptexture'):
                    collect('materials/'+value.replace('\\','/').removesuffix('.vtf')+'.vtf')
                elif field.lower()=='include':collect(value if value.lower().startswith('materials/') else 'materials/'+value)
        return True
    for entry in json.loads(args.catalog.read_text()):
        folder=args.catalog.parent/entry['folder'];qc=(folder/'view.qc').read_text()
        directories=re.findall(r'\$cdmaterials\s+"([^"]+)"',qc)
        for mesh in entry['meshes']:
            lines=(folder/mesh).read_text().split('triangles\n',1)[1].splitlines()
            materials=set(lines[:-1:4])
            for material in materials:
                found=False
                for directory in directories:
                    key='materials/'+directory.replace('\\','/').strip('/')+'/'+material+'.vmt'
                    if key.lower() in resources.files:
                        collect(key);found=True;break
                if not found:missing.append(entry['folder']+': '+material+'.vmt')
    report={'copied':copied,'missing':sorted(set(missing))}
    (args.output/'material_audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Dependencies copied:',len(copied),'Missing:',len(report['missing']))
    for name in report['missing']:print(name)


if __name__=='__main__':main()
