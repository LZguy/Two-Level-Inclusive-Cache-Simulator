# Two-Level Inclusive Cache Simulator

A configurable simulator of a **two-level (L1/L2) inclusive data cache**, supporting **LRU** replacement, **Write-Back** policy and a configurable **Write-Allocate / No-Write-Allocate** behavior.  
The simulator parses a memory access trace and reports **L1/L2 miss rates** and **average access time**.

> Implementation notes and CLI follow the assignment spec (Hebrew PDF). :contentReference[oaicite:0]{index=0}

---

## 📘 Overview

- **Data-only cache** with **two levels (L1 & L2)**; both caches use **Write-Back** and a selectable **Write-Allocate / No-Write-Allocate** policy. The hierarchy is **inclusive**. Accesses are **4-byte and aligned**. :contentReference[oaicite:1]{index=1}  
- **LRU** eviction policy; caches are empty at reset. :contentReference[oaicite:2]{index=2}  
- The simulator computes:
  - `L1miss` and `L2miss` as decimal fractions with **three digits after the decimal point**,
  - `AccTimeAvg` = average access time (cycles) with **three digits**, rounded normally. :contentReference[oaicite:3]{index=3}

Timing (per spec):  
- L1 miss & L2 hit: `t_access = tL1 + tL2`  
- L1 miss & L2 miss: `t_access = tL1 + tL2 + t_mem`  
Writeback background traffic **does not** change `t_access`. :contentReference[oaicite:4]{index=4}

---

## 🧩 CLI & Input Trace

Run format (log₂ sizes and associativity):  
```bash
./cacheSim <input file> --mem-cyc <cycles> --bsize <log2(block_bytes)> \
  --wr-alloc <0|1> \
  --l1-size <log2(bytes)> --l1-assoc <log2(ways)> --l1-cyc <cycles> \
  --l2-size <log2(bytes)> --l2-assoc <log2(ways)> --l2-cyc <cycles>
```
Example from the spec:
```bash
./cacheSim example.in --mem-cyc 100 --bsize 5 --wr-alloc 1 \
  --l1-size 16 --l1-assoc 3 --l1-cyc 1 \
  --l2-size 20 --l2-assoc 4 --l2-cyc 5
```
Input trace file: each line is r|w <hex_address> (e.g., w 0x1ff91ca8).
Output format (exact):
```
L1miss=<x.xxx> L2miss=<y.yyy> AccTimeAvg=<z.zzz>\n
```
with standard rounding to three decimals.

---

## 🛠️ Build
```bash
g++ -std=c++17 -O2 -Wall cacheSim.cpp -o cacheSim
```
(Or use your provided makefile to produce a cacheSim binary as required.)

---

## 🧱 Implementation Details

**Core structures.** Each cache level is built from sets × ways of Line{valid, dirty, tag, lru}; indexing and tags derive from the block address (>> blockB) split into index = block % sets, tag = block / sets. Probing returns hit/empty/victim with LRU maintenance. 

### Access path
- **L1 hit:** charge tL1, update LRU; writes mark line dirty.
- **L1 miss:**
  - **No-Write-Allocate & write op:** access L2 only; on miss add t_mem; do **not** allocate in L1.
  - **Otherwise (read or Write-Allocate):** look up L2; on hit charge tL1 + tL2, fill L1; on miss charge tL1 + tL2 + t_mem, fill L2 then L1 (inclusive). 

### Inclusive hierarchy
- On **L2 eviction**, invalidate the corresponding L1 line (same block) to preserve inclusion.
- On **L1 eviction of a dirty line**, write back (allocate/update) in L2; a further dirty L2 eviction implies a (background) memory write, which **does not** affect AccTimeAvg per spec. 

### Accounting.
L1miss = mL1 / nL1, L2miss = mL2 / nL2 (with nL2 counting L2 lookups), AccTimeAvg = cycles / nL1. Formatted with three decimals exactly.

---

## 📂 Project Structure
```
cache-sim/
├─ src/
│  └─ cacheSim.cpp          # inclusive 2-level cache simulator (this repo)
├─ tools/
│  └─ makefile              # build script producing 'cacheSim' (as required)
├─ examples/                # (optional) local traces for testing
├─ .gitignore
├─ LICENSE
└─ README.md
```

---

## 🚫 Not Included
- Course handout (CompArch-hw2.pdf) and official example traces/commands;
reproduce locally with your own traces under examples/.
