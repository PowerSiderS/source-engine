"""Create original fallback materials, arm textures and inventory metadata.

Fallbacks are explicitly authored gunmetal finishes, not Valve default albedos.
"""
import argparse, json, math, random, re, struct
from pathlib import Path


def write_vtf(path, width, height, rgba):
    if len(rgba)!=width*height*4:raise ValueError('RGBA image size')
    header=bytearray(80);header[:4]=b'VTF\0'
    struct.pack_into('<IIIHHIHH',header,4,7,2,80,width,height,0x300,1,0)
    struct.pack_into('<fff',header,32,0.2,0.2,0.2)
    struct.pack_into('<fI',header,48,1.,0)
    header[56]=1;struct.pack_into('<I',header,57,0xffffffff)
    header[61]=header[62]=0;struct.pack_into('<H',header,63,1)
    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(header+bytes(rgba))


def material(stage, name, kind='steel'):
    rng=random.Random(119);size=128;pixels=bytearray();normal=bytearray()
    for y in range(size):
        for x in range(size):
            noise=rng.randint(-3,3)
            if kind=='sleeve':
                grain=((x%4==0)+(y%4==0))*5;rgb=(51+grain+noise,58+grain+noise,48+grain+noise);alpha=35
            elif kind=='glove':
                grain=2 if (x+y)%3==0 else 0;rgb=(26+grain+noise,29+grain+noise,32+grain+noise);alpha=45
            else:
                grain=(y%3)*2;rgb=(67+grain+noise,73+grain+noise,80+grain+noise);alpha=130
            pixels.extend((*rgb,255));normal.extend((128,128,255,alpha))
    base='models/sourceadvanced/authored/'+kind
    write_vtf(stage/'materials'/(base+'.vtf'),size,size,pixels)
    write_vtf(stage/'materials'/(base+'_normal.vtf'),size,size,normal)
    target=stage/'materials'/(name+'.vmt');target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text('"VertexLitGeneric"\n{\n'+f' "$basetexture" "{base}"\n "$bumpmap" "{base}_normal"\n'+
        ' "$phong" "1"\n "$phongexponent" "32"\n "$phongboost" "0.65"\n "$phongfresnelranges" "[0.15 0.45 1]"\n'+
        (' "$envmap" "env_cubemap"\n "$normalmapalphaenvmapmask" "1"\n "$envmaptint" "[0.12 0.12 0.12]"\n' if kind=='steel' else '')+'}\n',encoding='utf-8')


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--catalog',type=Path,required=True);parser.add_argument('--stage',type=Path,required=True)
    args=parser.parse_args();catalog=json.loads(args.catalog.read_text());audit=json.loads((args.stage/'material_audit.json').read_text())
    generated=[]
    for missing in audit['missing']:
        if ': ' not in missing:continue
        folder,material_name=missing.split(': ',1)
        qc=(args.catalog.parent/folder/'view.qc').read_text()
        directory=re.findall(r'\$cdmaterials\s+"([^"]+)"',qc)[-1].replace('\\','/').strip('/')
        path=directory+'/'+material_name.removesuffix('.vmt')
        material(args.stage,path);generated.append(path)
    material(args.stage,'models/sourceadvanced/arms/unified_sleeve','sleeve')
    material(args.stage,'models/sourceadvanced/arms/unified_glove','glove')
    # These are cosmetic host classes. A visual Negev does not turn an M249
    # into a Negev mechanically. The mapping records that distinction.
    host={'ak-47':'ak47','aug':'aug','awp':'awp','desert_eagle':'deagle','dual_elites':'elite','famas':'famas','five-seven':'fiveseven','g3sg-1':'g3sg1','galil':'galil','glock_18':'glock','m249':'m249','m3':'m3','m4a1':'m4a1','mac10':'mac10','mp5':'mp5navy','p228':'p228','p90':'p90','scout':'scout','sg-550':'sg550','sg-552':'sg552','tmp':'tmp','ump':'ump45','usp':'usp','xm1014':'xm1014','knife':'knife'}
    rows=[];manifest=['"SkinsManifest"\n{\n']
    classes={'desert_eagle_r8_revolver':'revolver','five-seven_cz75':'cz75','five-seven_tec-9':'tec9','m249_negev':'negev','m3_sawed-off':'sawedoff','m4a1_m4a4':'m4a4','mp5_mp7':'mp7','p90_pp-bizon':'bizon','usp_p2000':'p2000','xm1014_mag-7':'mag7'}
    labels={'Ak-47':'AK-47','Aug':'AUG','Awp':'AWP','G3Sg1':'G3SG1','Cz75':'CZ75-Auto','Galil Ar':'Galil AR','Mp5-Sd':'MP5-SD','Mp7':'MP7','Pp-Bizon':'PP-Bizon','Ssg 08':'SSG 08','Scar-20':'SCAR-20','Sg 553':'SG 553','Mp9':'MP9','Ump-45':'UMP-45','Usp-S':'USP-S','Xm1014':'XM1014','M4A1-S':'M4A1-S'}
    for index,item in enumerate(catalog,1000):
        folder=item['folder'];prefix=next(p for p in sorted(host,key=len,reverse=True) if folder.startswith(p+'_'))
        name=folder[len(prefix)+1:].replace('_',' ').title();name=labels.get(name,name);weapon='weapon_'+classes.get(folder,host[prefix])
        category='Facas' if prefix=='knife' else 'Pistolas' if prefix in ('desert_eagle','dual_elites','five-seven','glock_18','p228','usp') else 'Snipers' if prefix in ('awp','g3sg-1','scout','sg-550') else 'SMGs' if prefix in ('mac10','mp5','p90','tmp','ump') else 'Pesadas' if prefix in ('m249','m3','xm1014') else 'Rifles'
        icon='sourceadvanced/inventory/'+folder
        row=dict(item_id=index,name=name,category=category,weapon_class=weapon,script_source_class='weapon_'+host[prefix],view_model=item['view_model'],world_model=item['world_model'],script='scripts/'+weapon+'.txt',icon=icon,skin=0)
        rows.append(row);manifest.append(f' "{index}"\n {{\n')
        for key in ('name','category','weapon_class','view_model','world_model','icon','skin'):manifest.append(f'  "{key}" "{row[key]}"\n')
        manifest.append(' }\n')
    manifest.append('}\n');path=args.stage/'scripts/skins_manifest.txt';path.parent.mkdir(parents=True,exist_ok=True);path.write_text(''.join(manifest),encoding='utf-8')
    (args.stage/'arsenal_mapping.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
    (args.stage/'authored_materials.json').write_text(json.dumps(generated,indent=2),encoding='utf-8')
    print('Items:',len(rows),'Original fallback materials:',len(generated))


if __name__=='__main__':main()
