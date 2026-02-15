# FutureChamp Chess Engine

FutureChamp is a human-style chess engine designed to emulate real player personalities, strategic preferences, and decision-making patterns rather than playing purely for maximum engine strength.

Unlike traditional engines, FutureChamp can play like Tal, Petrosian, Capablanca, or custom personalities defined via parameter tuning.

Current estimated strength: ~2000–2300 Elo (rapid), improving rapidly.

---

## Core Features

### Human-Style Play
- Stochastic move selection with temperature control  
- Risk appetite and sacrifice bias controls  
- Personality-driven decision making  

### Personality System
- Load personality via simple text files  
- Supports Tal, Petrosian, Capablanca, club-level presets  
- Fully customizable parameters  

### Advanced Search
- Alpha-Beta with aspiration windows  
- Late Move Reductions (LMR)  
- Null Move Pruning  
- Futility Pruning  
- SEE pruning  
- Selective check extensions  
- Transposition table  

### Human Selection System
- Candidate move filtering  
- Softmax probabilistic selection  
- Guardrails to prevent unrealistic blunders  

### Evaluation System
- Multi-layer positional evaluation  
- Knowledge-based concepts from chess literature  
- Pawn hash cache  
- Tiered evaluation for speed  

### Performance
- ~85k–95k NPS on modern CPU  
- Perft verified to depth 5  
- Stable search with no illegal moves  

---

## Personality System

Example personality file:

**personalities/tal.txt**

```txt
RiskAppetite = 180
SacrificeBias = 200
TradeBias = 60
HumanTemperature = 140
```

Load personality via UCI:

```
setoption name PersonalityFile value personalities/tal.txt
```

---

## Build Instructions (Windows)

Using MSYS2 MinGW64:

```bash
g++ -O2 -DNDEBUG -std=c++17 -static src/*.cpp src/eval/*.cpp src/search/*.cpp -o FutureChamp.exe
```

---

## Running

FutureChamp supports full UCI protocol.

Example:

```
FutureChamp.exe
```

Then:

```
uci
isready
position startpos
go depth 6
```

---

## Project Goals

FutureChamp aims to become the world's strongest human-style chess engine.

Focus areas:

- Human realism  
- Personality emulation  
- Strategic understanding  
- Configurable playing styles  

Target strength goal: 2500+ Elo.

---

## Repository Structure

```
src/
  main.cpp
  uci/
  eval/
  search/
  utils/
personalities/
knowledge/
docs/
```

---

## Author

Created by Brendan & Jay
