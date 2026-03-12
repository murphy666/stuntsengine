#!/usr/bin/env python3
"""
Full migration: remove ALL extern declarations from data_game.h.

Classification:
  - nowhere: delete extern from header + definition from data_game.c
  - single-file, already in target (not in data_game.c): make definition static in target
  - single-file, in data_game.c: move definition to target as static, remove from data_game.c
  - single-file but cross-refs in data_game.c init: treat as multi-file
  - multi-file: keep in data_game.c, add per-file extern block in each consumer
"""
import re, os, sys
from collections import defaultdict

HEADER   = "include/data_game.h"
DATA_C   = "src/data_game.c"
SRC_DIR  = "src"

# ── Variables that have cross-refs in data_game.c initializers ────────────────
# These look like single-file but can't be moved without breaking data_game.c
CROSS_REF_FORCE_MULTI = {
    'material_pattern_list', 'material_pattern2_list',
    'material_patlist_ptr', 'material_patlist2_ptr',
    'material_clrlist_ptr', 'material_clrlist2_ptr',
}

# ── Parse all extern lines from header ────────────────────────────────────────
def parse_header_externs(path):
    results = []
    with open(path) as f:
        for line in f:
            s = line.rstrip('\n')
            if not s.startswith('extern'):
                continue
            m = re.search(r'\b(\w+)\s*(?:\[[^\]]*\])?\s*;', s)
            if m:
                results.append((m.group(1), s))
    return results  # [(varname, full_line), ...]

# ── Scan all src/*.c files for usage ──────────────────────────────────────────
def find_usages(varnames, src_dir):
    usage = defaultdict(set)
    c_files = sorted(f for f in os.listdir(src_dir) if f.endswith('.c'))
    for fname in c_files:
        fpath = os.path.join(src_dir, fname)
        text = open(fpath).read()
        for v in varnames:
            if re.search(r'\b' + re.escape(v) + r'\b', text):
                usage[v].add(fname)
    return usage

# ── Find definition line(s) in a file (returns indices) ───────────────────────
def find_def_lines(lines, varname):
    """Find lines that appear to define varname (not extern, not call, not comment)."""
    results = []
    pat = re.compile(r'\b' + re.escape(varname) + r'\b')
    for i, line in enumerate(lines):
        s = line.strip()
        if not pat.search(s):
            continue
        if s.startswith('extern') or s.startswith('//') or s.startswith('/*') or s.startswith('*'):
            continue
        # Skip if it looks like a function call: varname(
        if re.search(r'\b' + re.escape(varname) + r'\s*\(', s):
            continue
        # Must contain = (assignment/init) or be a bare declaration like 'type varname;'
        # For arrays: type varname[N] = {...}; or type varname[N];
        # For simple: type varname = val;  or type varname;
        if '=' in s or re.match(
            r'^(?:static\s+)?(?:(?:unsigned|signed|const|volatile|struct|enum|union)\s+)*\w[\w\s\*]*\b' + re.escape(varname) + r'\b\s*(?:\[[^\]]*\])?\s*;',
            s
        ):
            results.append(i)
    return results

# ── Find last #include line index ─────────────────────────────────────────────
def last_include_idx(lines):
    idx = -1
    for i, line in enumerate(lines):
        if line.startswith('#include'):
            idx = i
    return idx

# ── Make a definition line static ─────────────────────────────────────────────
def make_static(line):
    """Prepend 'static ' if not already static."""
    s = line.rstrip('\n')
    if s.lstrip().startswith('static '):
        return line
    # Preserve leading whitespace? These are top-level, so no indent.
    return 'static ' + s.lstrip() + '\n'

# ──────────────────────────────────────────────────────────────────────────────
def main():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    print("=== Parsing externs from header ===")
    externs = parse_header_externs(HEADER)
    varname_to_decl = {v: d for v, d in externs}
    all_varnames = [v for v, _ in externs]
    print(f"  {len(externs)} externs found")

    print("=== Scanning src/*.c for usage ===")
    usage = find_usages(all_varnames, SRC_DIR)

    # Classify
    nowhere   = []
    single    = defaultdict(list)  # fname -> [vnames]
    multi     = defaultdict(list)  # vname -> [fnames]

    for v in all_varnames:
        consumers = {f for f in usage.get(v, set()) if f != 'data_game.c'}
        if v in CROSS_REF_FORCE_MULTI and consumers:
            multi[v] = sorted(consumers)
        elif not consumers:
            nowhere.append(v)
        elif len(consumers) == 1:
            single[list(consumers)[0]].append(v)
        else:
            multi[v] = sorted(consumers)

    print(f"  nowhere:     {len(nowhere)}")
    print(f"  single-file: {sum(len(v) for v in single.values())}")
    print(f"  multi-file:  {len(multi)}")

    # ── Load all key files ────────────────────────────────────────────────────
    with open(HEADER)  as f: header_lines  = f.readlines()
    with open(DATA_C)  as f: data_c_lines  = f.readlines()

    # ── Phase A: Strip all extern lines from header ───────────────────────────
    removed_from_hdr = set()
    new_header = []
    for line in header_lines:
        if line.startswith('extern'):
            m = re.search(r'\b(\w+)\s*(?:\[[^\]]*\])?\s*;', line)
            if m:
                removed_from_hdr.add(m.group(1))
                continue
        new_header.append(line)
    print(f"\n=== Phase A: Removed {len(removed_from_hdr)} extern lines from header ===")

    # ── Phase B: Delete nowhere-used definitions from data_game.c ─────────────
    print(f"\n=== Phase B: Deleting {len(nowhere)} nowhere-used vars from data_game.c ===")
    lines_to_del = set()
    for v in nowhere:
        idxs = find_def_lines(data_c_lines, v)
        if idxs:
            lines_to_del.update(idxs)
            print(f"  DELETE {v} at lines {[i+1 for i in idxs]}")
        else:
            print(f"  SKIP   {v} (no def in data_game.c)")
    new_data_c = [l for i, l in enumerate(data_c_lines) if i not in lines_to_del]
    data_c_lines = new_data_c

    # ── Phase C: Handle single-file variables ─────────────────────────────────
    print(f"\n=== Phase C: Moving single-file vars to consumers ===")
    for fname, vnames in sorted(single.items()):
        fpath = os.path.join(SRC_DIR, fname)
        consumer_lines = open(fpath).readlines()
        li = last_include_idx(consumer_lines)
        if li == -1:
            print(f"  SKIP {fname}: no #include found")
            continue

        print(f"  {fname}: {vnames}")
        lines_to_del = set()
        static_block = []  # lines to insert

        for v in sorted(vnames):
            # Check if var is already defined in consumer file (not in data_game.c)
            data_idxs = find_def_lines(data_c_lines, v)
            consumer_def_idxs = find_def_lines(consumer_lines, v)

            if data_idxs:
                # Extract the definition from data_game.c
                for di in data_idxs:
                    def_line = data_c_lines[di]
                    # Make it static
                    static_line = make_static(def_line)
                    static_block.append(static_line)
                    lines_to_del.add(di)
            elif consumer_def_idxs:
                # Already in consumer — make it static there
                for ci in consumer_def_idxs:
                    if not consumer_lines[ci].lstrip().startswith('static '):
                        consumer_lines[ci] = make_static(consumer_lines[ci])
                print(f"    {v}: already in {fname}, made static")
            else:
                print(f"    WARNING: {v} not found in data_game.c or {fname}")

        # Remove from data_game.c
        new_data_c2 = [l for i, l in enumerate(data_c_lines) if i not in lines_to_del]
        data_c_lines = new_data_c2

        if static_block:
            # Check for existing "Variables moved" block
            insertion_point = li + 1
            existing_idx = None
            for i, line in enumerate(consumer_lines):
                if '/* Variables moved from data_game.c */' in line:
                    existing_idx = i
                    break

            if existing_idx is not None:
                consumer_lines = consumer_lines[:existing_idx+1] + static_block + consumer_lines[existing_idx+1:]
            else:
                insert_block = ['\n', '/* Variables moved from data_game.c */\n'] + static_block
                consumer_lines = consumer_lines[:insertion_point] + insert_block + consumer_lines[insertion_point:]

        with open(fpath, 'w') as f:
            f.writelines(consumer_lines)

    # ── Phase D: Add per-file extern blocks for multi-file variables ──────────
    print(f"\n=== Phase D: Adding per-file externs for {len(multi)} multi-file vars ===")
    # Build: file -> [(varname, decl_line)]
    file_multi = defaultdict(list)
    for v, files in multi.items():
        decl = varname_to_decl[v]
        for fname in files:
            file_multi[fname].append((v, decl))

    for fname, var_decls in sorted(file_multi.items()):
        fpath = os.path.join(SRC_DIR, fname)
        consumer_lines = open(fpath).readlines()
        li = last_include_idx(consumer_lines)
        if li == -1:
            print(f"  SKIP {fname}: no #include found")
            continue

        # Sort by varname
        var_decls_sorted = sorted(var_decls, key=lambda x: x[0])
        print(f"  {fname}: {len(var_decls_sorted)} vars")

        # Find existing "Shared globals" block
        existing_idx = None
        for i, line in enumerate(consumer_lines):
            if '/* Shared globals from data_game.c */' in line:
                existing_idx = i
                break

        extern_lines = [v_decl[1].strip() + '\n' for v_decl in var_decls_sorted]

        if existing_idx is not None:
            # Find end of existing block
            end_idx = existing_idx + 1
            while end_idx < len(consumer_lines):
                l = consumer_lines[end_idx]
                if l.startswith('extern ') or l.strip() == '' or l.startswith('/*'):
                    end_idx += 1
                else:
                    break
            new_block = ['/* Shared globals from data_game.c */\n'] + extern_lines
            consumer_lines = consumer_lines[:existing_idx] + new_block + consumer_lines[end_idx:]
        else:
            insert_block = ['\n', '/* Shared globals from data_game.c */\n'] + extern_lines
            consumer_lines = consumer_lines[:li+1] + insert_block + consumer_lines[li+1:]

        with open(fpath, 'w') as f:
            f.writelines(consumer_lines)

    # ── Write modified files ──────────────────────────────────────────────────
    print("\n=== Writing modified files ===")
    with open(HEADER, 'w') as f:
        f.writelines(new_header)

    with open(DATA_C, 'w') as f:
        f.writelines(data_c_lines)

    extern_remaining = sum(1 for l in new_header if l.startswith('extern'))
    print(f"  {HEADER}: {extern_remaining} extern lines remaining")
    print(f"  {DATA_C}: written")
    print("\nDone!")

if __name__ == '__main__':
    main()
