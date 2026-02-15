# FutureChamp ♟️  
*A human-like chess engine with style presets and "coach-ish" decision-making.*

## What this is
FutureChamp is a UCI chess engine that supports:
- **Style presets ("personalities")** for different playstyles (Tal, Petrosian, Capablanca, etc.)
- **Human-like move selection** (stochastic root choice + guardrails)
- **Tunable knobs** for risk, sacrifices, simplicity, and trading pieces

It's built for **interesting, instructive games** — not just maximum brute-force perfection.

---

## Key Features (Current)
### ✅ UCI + Engine Core
- Clean UCI protocol (no stdout banner before `uciok`)
- Perft verified: **Perft(5) = 4,865,609** (matches Stockfish)
- Transposition table (TT), zobrist hashing, move ordering

### ✅ Strength + Search
- Quiescence search (qsearch) with SEE filtering + pruning
- LMR (Late Move Reductions)
- Null move pruning
- Futility pruning
- SEE pruning in main search
- Selective check extensions

### ✅ "Human" Style Layer
- Root "HumanSelect" stochastic picker with guardrails (floor/top-k/opening sanity)
- Personalities load instantly (supports **.txt** Rodent-style and **.json** legacy)
- TradeBias: influences trade preference (ordering / evaluation / qsearch filtering / HumanSelect bias)

---

## Download
Grab the latest release here:
- **v1.1-alpha**: https://github.com/jay-codes-chess/FutureChamp/releases/tag/v1.1-alpha

---

## Quickstart (UCI GUI)
1) Download the zip from Releases and extract it
2) In Arena (or your favourite GUI): add `FutureChamp.exe` as a UCI engine
3) Recommended: keep `HumanSelect=false` while you're testing strength

---

## Personalities
### Load by name
If you have `personalities/` next to the exe:
- `setoption name Personality value tal`
- `setoption name Personality value petrosian`

### Load by file
- `setoption name PersonalityFile value personalities/tal.txt`

**Text personality format**:
- One setting per line: `Key = Value`
- Lines starting `#` or `//` are comments
- Unknown keys are ignored safely

Example:
```txt
Name = Tal
RiskAppetite = 180
SacrificeBias = 200
SimplicityBias = 60
HumanTemperature = 140
TradeBias = 60
```

---

## Build from Source
### Windows (MinGW)
```bash
g++ -O2 -DNDEBUG -std=c++17 -static src/main.cpp src/eval/*.cpp src/search/*.cpp src/uci/*.cpp src/utils/*.cpp src/movegen.cpp -o FutureChamp.exe
```

---

## Why "FutureChamp"?
The goal is **teaching chess through example**. Instead of perfect moves, FutureChamp makes *strategic* moves — the kind a coach would recommend — while explaining the reasoning through PV output.

Play against it to see **why** a move is good, not just **that** it's good.

---

## Author
Created by Brendan & Jay
