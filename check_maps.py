import os, struct

base_dirs = [
    r"C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)\cstrike\maps",
    r"C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)\cstrike\download\maps"
]

results = []
for bdir in base_dirs:
    if not os.path.exists(bdir):
        continue
    for f in os.listdir(bdir):
        if not f.endswith(".bsp"):
            continue
        fpath = os.path.join(bdir, f)
        size_mb = os.path.getsize(fpath) / (1024*1024)
        try:
            with open(fpath, "rb") as fp:
                ident = fp.read(4)
                version = struct.unpack("<I", fp.read(4))[0]
                ident_str = ident.decode("latin1", errors="replace")
                status = "OK (VBSP v" + str(version) + ")" if ident == b"VBSP" else "BAD IDENT"
                results.append({
                    "map": f,
                    "ident": ident_str,
                    "version": version,
                    "size": f"{size_mb:.2f} MB",
                    "status": status,
                    "dir": os.path.basename(bdir)
                })
        except Exception as e:
            results.append({
                "map": f,
                "ident": "ERR",
                "version": 0,
                "size": f"{size_mb:.2f} MB",
                "status": str(e),
                "dir": os.path.basename(bdir)
            })

print(f"Total maps scanned: {len(results)}\n")
header = f"{'Map Name':<35} | {'Ident':<5} | {'Ver':<4} | {'Size':<10} | {'Folder':<10} | {'Status'}"
print(header)
print("-" * len(header))
for r in sorted(results, key=lambda x: x["map"].lower()):
    print(f"{r['map']:<35} | {r['ident']:<5} | {r['version']:<4} | {r['size']:<10} | {r['dir']:<10} | {r['status']}")
