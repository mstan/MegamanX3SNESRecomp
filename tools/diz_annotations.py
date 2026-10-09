#!/usr/bin/env python3
"""Import ROM-bound discovery metadata from a pinned DiztinGUIsh project.

Only names, addresses and classification facts are exported. Unknown bytes,
recorded M/X states and data classifications never change execution policy.
The CFG overlay uses non-promoting symbols; reviewed project names win.
This standalone tool is shared verbatim by the X1, X2 and X3 projects.
"""
from __future__ import annotations

import argparse
from collections import Counter
import csv
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
GAMES = {
    "x1": ("Megaman X1", "megamanX.diz", "mmx.sfc",
           "C65216760BA99178100A10D98457CF11496C2097"),
    "x2": ("Megaman X2", "MMX2.diz", "mmx2.sfc",
           "637079014421563283CDED6AEAA0604597B2E33C"),
    "x3": ("Megaman X3", "mmx3.diz", "mmx3.sfc",
           "B226F7EC59283B05C1E276E2F433893F45027CAC"),
}
FLAGS = dict(zip("U+.GMXTABCDEFH", (
    "unknown", "opcode", "operand", "graphics", "music", "empty", "text",
    "data8", "data16", "data24", "data32", "pointer16", "pointer24", "pointer32")))
SUBSTITUTIONS = [
    ("0001E", "ZQ"), ("B0001", "Zq"), ("C0001", "ZX"),
    ("B7E", "Zx"), ("07F01", "ZY"), ("0001D", "Zy"),
    ("C7E", "ZZ"), ("07E", "Zz"), ("00001", "ZS"), ("0001", "Zs"),
]
FAKE64 = {c: n * 4 for n, c in enumerate(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")}
FAKE64.update({"A": 208, "0": 0})
BEGIN = "# >>> BEGIN DiztinGUIsh symbols (generated)"
END = "# <<< END DiztinGUIsh symbols"
C_ANNOTATION = re.compile(
    r"(?m)^[ \t]*/\* DIZ: [^\r\n]* \*/(?:\r?\n|$)| /\* DIZ: [^\r\n]* \*/")
TRACE_PC = re.compile(r"cpu_trace_block\(cpu,\s*0x([0-9a-fA-F]{6})[uU]?\)")
MEMORY_PC = re.compile(
    r"cpu_(?:read|write)(?:8|16)\(cpu,\s*0x([0-9A-Fa-f]{1,2}),\s*"
    r"(?:\(uint16\))?\s*0x([0-9A-Fa-f]{1,4})\b")


def tag(element):
    return element.tag.rsplit("}", 1)[-1]


def clean_rom(path):
    rom = Path(path).read_bytes()
    return rom[512:] if len(rom) % 0x8000 == 512 else rom


def verify_rom(rom, expected):
    actual = hashlib.sha1(rom).hexdigest().upper()
    if actual != expected.upper():
        raise ValueError(f"ROM SHA-1 mismatch: expected {expected}, got {actual}")
    return actual


def rows(text):
    lines = text.strip().splitlines()
    if not lines or lines.pop(0) != "version:201,compress_groupblocks,compress_table_1":
        raise ValueError("Unsupported Diz ROM metadata encoding")
    for line in lines:
        for long, short in reversed(SUBSTITUTIONS):
            line = line.replace(short, long)
        count = 1
        if line.startswith("r "):
            _, count, line = line.split(" ")
            count = int(count)
        line = line.split(";", 1)[0].strip()
        if not line:
            continue
        if count <= 0 or len(line) > 9 or line[0] not in FLAGS:
            raise ValueError(f"Invalid Diz ROM metadata row: {line!r}")
        yield line.ljust(9, "0"), count


def offset_for(pc, rom_size):
    bank, address = pc >> 16, pc & 0xFFFF
    if bank in (0x7E, 0x7F) or address < 0x8000:
        return None
    offset = (bank & 0x7F) * 0x8000 + address - 0x8000
    return offset if offset < rom_size else None


def pc_for(offset):
    return (offset // 0x8000 << 16) | 0x8000 | (offset % 0x8000)


def strip_overlay(text):
    if BEGIN not in text:
        return text
    if text.count(BEGIN) != 1 or text.count(END) != 1:
        raise ValueError("Malformed Diz CFG overlay markers")
    before, rest = text.split(BEGIN)
    _, after = rest.split(END)
    return before.rstrip() + "\n" + after.lstrip("\r\n")


def project_names(cfg_dir):
    names = {}
    for path in sorted(Path(cfg_dir).glob("bank*.cfg")):
        bank = int(path.stem[4:], 16)
        for line in strip_overlay(path.read_text(encoding="utf-8")).splitlines():
            parts = line.split("#", 1)[0].split()
            if len(parts) < 3:
                continue
            if parts[0] == "func":
                names[(bank << 16) | (int(parts[2], 16) & 0xFFFF)] = parts[1]
            elif parts[0] in ("name", "symbol"):
                names[int(parts[1], 16)] = parts[2]
    # Host code and reviewed post-generation hooks also depend on synthetic
    # function names. Preserve those addresses even without a CFG declaration.
    # Discovery labels still appear in the index and generated comments.
    reference = re.compile(r"\bbank_([0-9A-Fa-f]{2})_([0-9A-Fa-f]{4})(?:_M[01]X[01])?\b")
    literal = re.compile(r"\b0x([0-9A-Fa-f]{1,6})\b")
    for directory in (ROOT / "src", ROOT / "tools"):
        for path in sorted(directory.rglob("*")):
            if not path.is_file() or path.suffix not in (".c", ".h", ".inc", ".py", ".sh"):
                continue
            if (ROOT / "src/gen") in path.parents or "build" in path.parts:
                continue
            text = path.read_text(encoding="utf-8")
            addresses = {(int(match[1], 16) << 16) | int(match[2], 16)
                         for match in reference.finditer(text)}
            # Address-driven hook tables can derive synthetic names rather
            # than spelling them literally. Preserve both execution mirrors.
            addresses.update(int(match[1], 16) for match in literal.finditer(text))
            for pc in addresses:
                if pc & 0xFFFF < 0x8000 or (pc >> 16) & 0x7F >= 0x40:
                    continue
                for alias in (pc, pc ^ 0x800000):
                    names.setdefault(alias, f"bank_{alias >> 16:02X}_{alias & 0xFFFF:04X}")
    return names


def read_project(path, rom):
    document = ET.fromstring(gzip.decompress(Path(path).read_bytes()))
    if document.attrib.get("Watermark") != "DiztinGUIsh" or document.attrib.get("SaveVersion") not in ("102", "104"):
        raise ValueError("Unsupported Diz project version")
    data = next(element for element in document.iter() if tag(element) == "Data")
    if data.attrib.get("RomMapMode") != "LoRom":
        raise ValueError("This importer currently supports standard LoROM projects")
    records = []
    byte_text = next(element.text for element in data if tag(element) == "RomBytes")
    for row, count in rows(byte_text):
        if len(records) + count > len(rom):
            raise ValueError("Diz metadata is larger than the ROM")
        records.extend([row] * count)
    if len(records) != len(rom):
        raise ValueError("Diz metadata size does not match the ROM")
    labels = []
    for section in data:
        if tag(section) != "Labels":
            continue
        for item in section:
            value = list(item)[0]
            pc = int(item.attrib["Key"])
            name = value.attrib.get("Name", "")
            if not name:
                continue
            if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name):
                raise ValueError(f"Unsupported label identifier: {name!r}")
            offset = offset_for(pc, len(rom))
            kind = ("ram" if pc >> 16 in (0x7E, 0x7F) else "external") if offset is None else FLAGS[records[offset][0]]
            labels.append({"address": f"{pc:06X}", "name": name, "kind": kind})
    return records, sorted(labels, key=lambda label: (label["address"], label["name"]))


def instruction_rows(records, rom, opcode_table):
    result, mismatches = [], []
    for offset, row in enumerate(records):
        if row[0] != "+":
            continue
        bits = FAKE64[row[1]]
        m, x = (bits >> 3) & 1, (bits >> 2) & 1
        architecture = int(row[8], 16) & 3
        length = 1
        while offset + length < len(records) and records[offset + length][0] == ".":
            length += 1
        consistent = True
        if architecture == 0:
            spec = opcode_table[rom[offset]][2]
            expected = spec(m, x) if callable(spec) else spec
            consistent = expected == length
            if not consistent:
                mismatches.append(f"{pc_for(offset):06X}")
        result.append((f"{pc_for(offset):06X}", m, x, row[2:4], row[4:8],
                       architecture, length, int(consistent)))
    return result, mismatches


def tsv(header, rows_):
    output = io.StringIO(newline="")
    writer = csv.writer(output, delimiter="\t", lineterminator="\n")
    writer.writerow(header)
    writer.writerows(rows_)
    return output.getvalue()


def symbol_overlay(labels, existing):
    output, emitted, claimed = [], set(existing), set(existing.values())
    for label in labels:
        if label["kind"] != "opcode":
            continue
        original = int(label["address"], 16)
        for pc in (original, original ^ 0x800000):
            if pc in emitted:
                continue
            name = "diz_" + label["name"]
            if pc != original:
                name += f"_mirror{pc >> 16:02X}"
            if name in claimed:
                name += f"_{pc:06X}"
            output.append(f"symbol {pc:06X} {name}")
            emitted.add(pc)
            claimed.add(name)
    return sorted(output)


def build_artifacts(game, dependency, rom_path, cfg_dir):
    folder, filename, _, expected = GAMES[game]
    rom = clean_rom(rom_path)
    verify_rom(rom, expected)
    source = Path(dependency) / "snes" / folder / filename
    info = (source.parent / "export/info.txt").read_text(encoding="utf-8")
    upstream_sha1 = re.search(r"SHA-1:\s*([0-9A-Fa-f]{40})", info)
    if not upstream_sha1 or upstream_sha1[1].upper() != expected:
        raise ValueError("Upstream ROM identity differs from the supported game revision")
    revision = subprocess.check_output(
        ["git", "-C", str(dependency), "rev-parse", "HEAD"], text=True).strip()
    records, labels = read_project(source, rom)
    existing = project_names(cfg_dir)
    sys.path.insert(0, str(ROOT / "snesrecomp/recompiler"))
    from snes65816 import opcode_table
    instructions, mismatches = instruction_rows(records, rom, opcode_table())
    counts = Counter(FLAGS[row[0]] for row in records)
    regions = []
    start = 0
    for offset in range(1, len(records) + 1):
        kind = "code" if records[start][0] in "+." else FLAGS[records[start][0]]
        next_kind = None if offset == len(records) else (
            "code" if records[offset][0] in "+." else FLAGS[records[offset][0]])
        if next_kind != kind or offset % 0x8000 == 0:
            regions.append((f"{pc_for(start):06X}", offset - start, kind))
            start = offset
    for label in labels:
        pc = int(label["address"], 16)
        label["project_name"] = existing.get(pc, existing.get(pc ^ 0x800000, "")) if label["kind"] not in ("ram", "external") else ""
    overlay = symbol_overlay(labels, existing)
    summary = {
        "format_version": 1, "game": game,
        "upstream_url": "https://github.com/bogaa/dizProjects",
        "upstream_commit": revision,
        "project_path": f"snes/{folder}/{filename}",
        "project_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "rom_sha1": expected, "rom_sha256": hashlib.sha256(rom).hexdigest(),
        "rom_size": len(rom), "byte_counts": dict(sorted(counts.items())),
        "classified_percent": round(100 * (len(rom) - counts["unknown"]) / len(rom), 2),
        "labels_by_kind": dict(sorted(Counter(label["kind"] for label in labels).items())),
        "overlay_symbols": len(overlay), "instruction_count": len(instructions),
        "instruction_boundary_mismatches": mismatches,
        "policy": "Discovery metadata only. Unknown is not data. Recorded modes and classifications are not execution contracts.",
        "labels": labels,
    }
    artifacts = {
        "diz_annotations.json": json.dumps(summary, indent=2) + "\n",
        "imported_symbols.tsv": tsv(("address", "kind", "name", "project_name"),
            ((label["address"], label["kind"], label["name"], label["project_name"] or "-") for label in labels)),
        "classification.tsv": tsv(("start", "length", "kind"), regions),
        "instruction_metadata.tsv": tsv(("address", "m", "x", "db", "dp", "architecture", "length", "boundary_consistent"), instructions),
    }
    cfg = Path(cfg_dir) / "bank00.cfg"
    base = strip_overlay(cfg.read_text(encoding="utf-8")).rstrip()
    artifacts[cfg] = base + "\n\n" + BEGIN + "\n" + (
        "# Pinned upstream labels only; no new roots, bounds, data exclusions or width overrides.\n"
        + "\n".join(overlay) + "\n" + END + "\n")
    return artifacts, summary


def annotate_text(text, labels):
    names = {}
    for label in labels:
        pc = int(label["address"], 16)
        names.setdefault(pc, []).append(label["name"])
        if label["kind"] not in ("ram", "external"):
            names.setdefault(pc ^ 0x800000, []).append(label["name"])
    lines = []
    hits = 0
    for line in C_ANNOTATION.sub("", text).splitlines(keepends=True):
        pcs = [int(match[1], 16) for match in TRACE_PC.finditer(line)]
        pcs.extend((int(match[1], 16) << 16) | int(match[2], 16)
                   for match in MEMORY_PC.finditer(line))
        annotations = [f"${pc:06X}={','.join(sorted(set(names[pc])))}"
                       for pc in sorted(set(pcs)) if pc in names]
        if annotations:
            ending = "\r\n" if line.endswith("\r\n") else "\n" if line.endswith("\n") else ""
            indent = re.match(r"[ \t]*", line)[0]
            # Keep the original statement line intact: reviewed generation
            # hooks often match cpu_trace_block lines with an end anchor.
            lines.append(indent + "/* DIZ: " + "; ".join(annotations) + " */" + (ending or "\n"))
            hits += 1
        lines.append(line)
    return "".join(lines), hits


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", choices=GAMES)
    parser.add_argument("--dependency", type=Path, default=ROOT / "third_party/dizProjects")
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--cfg-dir", type=Path, default=ROOT / "recomp")
    parser.add_argument("--out-dir", type=Path, default=ROOT / "symbols")
    parser.add_argument("--check", action="store_true", help="Verify tracked imports are current without writing")
    parser.add_argument("--report", action="store_true", help="Show counts without writing")
    parser.add_argument("--query", help="Find labels by name or exact 24-bit address")
    parser.add_argument("--limit", type=int, default=30)
    parser.add_argument("--annotate-generated", type=Path, help="Add label comments to generated C without changing C tokens")
    parser.add_argument("--cfg-copy-without-overlay", type=Path, help="Prepare an independent CFG directory for A/B validation")
    args = parser.parse_args()
    if args.cfg_copy_without_overlay:
        destination = args.cfg_copy_without_overlay.resolve()
        if destination == args.cfg_dir.resolve():
            parser.error("The A/B CFG copy must have a different destination")
        destination.mkdir(parents=True, exist_ok=True)
        for source in args.cfg_dir.iterdir():
            if source.suffix not in (".cfg", ".h") or not source.is_file():
                continue
            text = source.read_text(encoding="utf-8")
            (destination / source.name).write_text(strip_overlay(text), encoding="utf-8", newline="\n")
        return 0
    if args.query or args.annotate_generated:
        metadata = json.loads((args.out_dir / "diz_annotations.json").read_text(encoding="utf-8"))
        if args.game and metadata["game"] != args.game:
            parser.error("Annotation metadata belongs to a different game")
        if args.query:
            query = args.query.lower().removeprefix("$").removeprefix("0x").replace(":", "")
            matches = [label for label in metadata["labels"] if query in label["name"].lower()
                       or query == label["address"].lower() or query in label["project_name"].lower()]
            for label in matches[:args.limit]:
                print(f"${label['address']}  {label['kind']:10} {label['name']}" +
                      (f"  (project: {label['project_name']})" if label['project_name'] else ""))
            print(f"{len(matches)} match(es)")
            return 0
        if not args.rom:
            parser.error("--annotate-generated requires --rom to verify identity")
        verify_rom(clean_rom(args.rom), metadata["rom_sha1"])
        hits = 0
        for path in sorted(args.annotate_generated.glob("*.c")):
            original = path.read_text(encoding="utf-8")
            annotated, count = annotate_text(original, metadata["labels"])
            if C_ANNOTATION.sub("", original) != C_ANNOTATION.sub("", annotated):
                raise ValueError(f"Annotation unexpectedly changed C tokens in {path}")
            if original != annotated:
                path.write_text(annotated, encoding="utf-8", newline="\n")
            hits += count
        print(f"Diz annotations: {hits} named code/RAM locations in {args.annotate_generated}")
        return 0
    if not args.game:
        parser.error("Import requires --game")
    rom = args.rom or ROOT / GAMES[args.game][2]
    artifacts, metadata = build_artifacts(args.game, args.dependency, rom, args.cfg_dir)
    print(f"{args.game}: {len(metadata['labels'])} labels, {metadata['instruction_count']} instructions, "
          f"{metadata['classified_percent']}% classified ROM bytes, "
          f"{len(metadata['instruction_boundary_mismatches'])} boundary mismatches")
    if args.report:
        return 0
    stale = []
    for name, text in artifacts.items():
        path = name if isinstance(name, Path) else args.out_dir / name
        if path.exists() and path.read_text(encoding="utf-8") == text:
            continue
        stale.append(str(path))
        if not args.check:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8", newline="\n")
    if args.check and stale:
        print("Stale Diz imports:\n" + "\n".join(stale), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, ET.ParseError, subprocess.CalledProcessError) as error:
        print(f"diz_annotations: {error}", file=sys.stderr)
        sys.exit(1)
