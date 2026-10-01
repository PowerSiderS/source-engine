import argparse
import hashlib
import json
import struct
from pathlib import Path


HEADER = {
    "id": 0,
    "version": 4,
    "checksum": 8,
    "name": 12,
    "length": 76,
    "flags": 152,
    "numbones": 156,
    "boneindex": 160,
    "numlocalanim": 180,
    "localanimindex": 184,
    "numlocalseq": 188,
    "localseqindex": 192,
    "numtextures": 204,
    "textureindex": 208,
    "numcdtextures": 212,
    "cdtextureindex": 216,
    "numskinref": 220,
    "numskinfamilies": 224,
    "skinindex": 228,
    "numbodyparts": 232,
    "bodypartindex": 236,
}

BONE_SIZE = 216
SEQDESC_SIZE = 212


def i32(data, offset):
    return struct.unpack_from("<i", data, offset)[0]


def cstr(data, offset, limit=None):
    if offset < 0 or offset >= len(data):
        return "<invalid>"
    end = data.find(b"\0", offset, len(data) if limit is None else min(len(data), offset + limit))
    if end < 0:
        end = len(data) if limit is None else min(len(data), offset + limit)
    return data[offset:end].decode("utf-8", "replace")


def inspect(path):
    data = path.read_bytes()
    result = {
        "file": str(path),
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": len(data),
        "id": data[:4].decode("ascii", "replace"),
        "version": i32(data, HEADER["version"]),
        "checksum": i32(data, HEADER["checksum"]),
        "internal_name": cstr(data, HEADER["name"], 64),
        "declared_length": i32(data, HEADER["length"]),
    }
    for key in (
        "flags", "numbones", "boneindex", "numlocalanim", "localanimindex",
        "numlocalseq", "localseqindex", "numtextures", "textureindex",
        "numcdtextures", "cdtextureindex", "numskinref", "numskinfamilies",
        "numbodyparts", "bodypartindex",
    ):
        result[key] = i32(data, HEADER[key])

    bones = []
    if 0 <= result["numbones"] <= 512 and 0 <= result["boneindex"] < len(data):
        for idx in range(result["numbones"]):
            base = result["boneindex"] + idx * BONE_SIZE
            if base + BONE_SIZE > len(data):
                bones.append({"index": idx, "error": "outside file"})
                break
            name_rel = i32(data, base)
            bones.append({
                "index": idx,
                "name": cstr(data, base + name_rel, 128),
                "parent": i32(data, base + 4),
                "flags": i32(data, base + 160),
            })
    result["bones"] = bones

    sequences = []
    if 0 <= result["numlocalseq"] <= 512 and 0 <= result["localseqindex"] < len(data):
        for idx in range(result["numlocalseq"]):
            base = result["localseqindex"] + idx * SEQDESC_SIZE
            if base + 12 > len(data):
                sequences.append({"index": idx, "error": "outside file"})
                break
            label_rel = i32(data, base + 4)
            activity_rel = i32(data, base + 8)
            sequences.append({
                "index": idx,
                "label": cstr(data, base + label_rel, 128),
                "activity": cstr(data, base + activity_rel, 128),
            })
    result["sequences"] = sequences

    cdtextures = []
    table = result["cdtextureindex"]
    if 0 <= result["numcdtextures"] <= 128 and 0 <= table < len(data):
        for idx in range(result["numcdtextures"]):
            absolute = i32(data, table + idx * 4)
            cdtextures.append(cstr(data, absolute, 256))
    result["cdtextures"] = cdtextures
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--compact", action="store_true")
    args = parser.parse_args()
    results = [inspect(Path(value)) for value in args.paths]
    if args.compact:
        compact = []
        for item in results:
            compact.append({
                key: item[key]
                for key in (
                    "file", "sha256", "size", "id", "version", "checksum",
                    "internal_name", "declared_length", "numbones", "numlocalanim",
                    "numlocalseq", "numtextures", "numcdtextures", "numskinref",
                    "numskinfamilies", "numbodyparts", "cdtextures",
                )
            } | {
                "bones": [bone.get("name", bone.get("error")) for bone in item["bones"]],
                "sequences": [seq.get("label", seq.get("error")) for seq in item["sequences"]],
            })
        results = compact
    print(json.dumps(results, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
