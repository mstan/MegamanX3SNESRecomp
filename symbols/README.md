# DiztinGUIsh annotations

Discovery metadata from [bogaa/dizProjects](https://github.com/bogaa/dizProjects),
pinned at `b60813ef4785a2f87b1f601e4f8a2be0ee38d9a4` by the
`third_party/dizProjects` submodule. X1 credits GoldS and bogaa; X2/X3 credit
kandowontu. Upstream describes the projects as work in progress.

This follows the Mario Kart: Super Circuit symbol overlay pattern: pin the
dependency, verify the ROM, export names and structural facts, and preserve
reviewed project names. No ROM bytes, assembly source or upstream comments
are exported. `tools/diz_annotations.py` is shared verbatim across X1-X3;
changes to this standalone importer should be applied to all three copies.

## Coverage at the pinned revision

| Game | Nonempty labels | Identified instructions | Classified ROM bytes | Boundary disagreements |
|---|---:|---:|---:|---:|
| X1 | 9,374 | 85,569 | 95.14% | 1 |
| X2 | 1 | 32,462 | 4.62% | 0 |
| X3 | 1 | 70,916 | 7.60% | 0 |

Classified ROM bytes include graphics, text, pointers and data; this is not
the percentage of game logic understood. X1 includes 744 code labels and
3,984 RAM labels. One empty X1 label record is omitted. X2 and X3 each supply
only `Emulation_RESET`; their instruction metadata supports further analysis
but provides little semantic naming today.

X1's instruction-boundary disagreement at `$02:A981` is recorded in the JSON
and TSV. It remains a discovery finding. Matching ROM hashes and instruction
lengths does not prove correct classifications, names or function boundaries.
This import has not exported and reassembled the upstream projects.

## Files and precedence

| File | Purpose |
|---|---|
| `diz_annotations.json` | Provenance, ROM hashes, counts, mismatches, labels and reviewed aliases |
| `imported_symbols.tsv` | Searchable ROM, RAM and external address names |
| `classification.tsv` | Bank-bounded code, data and unknown regions |
| `instruction_metadata.tsv` | Recorded M/X, DB, DP, architecture and instruction lengths |
| `../recomp/bank00.cfg` | Marker-delimited code `symbol` overlay, including LoROM execution mirrors |

The symbol TSV uses `-` when no reviewed project alias exists.

The overlay adds only non-promoting `symbol` directives. It does not create
functions, change bounds, exclude data, classify unknown bytes as data or
impose recorded M/X states. Reviewed `func`, `name` and project `symbol`
declarations keep precedence. Synthetic names referenced by handwritten
host code or reviewed generation hooks are preserved too. Imported C names use `diz_`; mirror aliases
and collisions receive deterministic suffixes. RAM/data labels remain in
the discovery map and generated comments, outside the callable function map.

`tools/regen.sh` adds `/* DIZ: ... */` comments at named basic blocks and
constant-address memory accesses. The comment stage asserts that removing
its comments reproduces the original C exactly. The USA annotations do not
apply to the Japanese Rockman X variant.

## Import and search

From the project root, with the matching ROM present:

```sh
git submodule update --init third_party/dizProjects
python tools/diz_annotations.py --game x3
python tools/diz_annotations.py --game x3 --check
python tools/diz_annotations.py --query charge
python tools/diz_annotations.py --query 7E0BCF
bash tools/regen.sh --no-tests
```

Use `--game x2` or `--game x3` in the corresponding project. `--report` prints
counts without writing. Import and generated annotations verify the ROM
SHA-1 after stripping a copier header. Upstream `export/info.txt` must match
the supported ROM revision too.

To update, advance the submodule deliberately, re-import, inspect the diff
and run `--check`. Normal regeneration uses tracked metadata without fetching
upstream or opening a GUI.

Prepare an independent comparison without the overlay:

```sh
python tools/diz_annotations.py --cfg-copy-without-overlay _diz_baseline_cfg
```

Run the same `v2_emit.py` invocation using that CFG directory, an independent
output directory and `--source-root src`. Keep ROM, engine, profiles and host
roots identical. Execution manifests and executable C must agree after
normalizing symbol names, comments, build provenance and forward-declaration order.

The initial import passed nine importer contract tests in X1 and A/B checks
across 114 X1, 128 X2 and 126 X3 C translation units. Execution manifests and
normalized C matched in all three games. X1 used its recorded engine pin
`88a9f7f`; X2/X3 used `b39da563`. All three Windows executables built
successfully; X1 used isolated checkouts at its recorded engine and UI pins.
