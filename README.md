# Human Chess Engine ♟️

A human-like chess engine that thinks like a coach, not a calculator.

## Philosophy

Unlike Stockfish which plays perfect chess through brute-force search, this engine:
- **Evaluates positions like a human** using Silman's Imbalance Theory
- **Thinks strategically** using Russian School methodology  
- **Calculates selectively** using MCTS for human-like move selection
- **Explains its moves** with verbal PV output for teaching

## Key Features

### 🎯 Human-Like Evaluation
- Imbalance-based evaluation (material, piece activity, pawn structure, space, initiative)
- Style-tunable weights (Classical, Attacking, Tactical, Positional, etc.)
- Prophylactic thinking from Russian Chess School

### 🧠 Teaching Focus
- Verbal PV output explaining WHY each move is played
- Imbalance analysis in comments
- Pattern recognition annotations
- Skill levels for students

### ⚡ Modern Architecture
- C++ for performance
- MCTS or Alpha-Beta search (selectable)
- UCI protocol compatible
- Style profiles via UCI options

## Architecture

```
human-chess-engine/
├── src/
│   ├── main.cpp              # Entry point
│   ├── evaluation/           # Silman-based evaluation ✓
│   │   ├── evaluation.hpp
│   │   └── evaluation.cpp
│   ├── search/              # MCTS/Alpha-Beta (placeholder)
│   │   ├── search.hpp
│   │   └── search.cpp
│   ├── uci/                 # UCI protocol ✓
│   │   ├── uci.hpp
│   │   └── uci.cpp
│   └── utils/               # Board representation ✓
│       ├── board.hpp
│       └── board.cpp
├── data/
│   └── opening_book/
├── docs/
│   └── style_guide.md        # Evaluation philosophy
└── Makefile
```

## Current Status

### ✅ Working
- Bitboard representation (fast 64-bit boards)
- FEN parsing and generation
- Move generation for all pieces
- UCI protocol integration
- Basic evaluation (material, activity, pawns, space)
- Style profiles (Classical, Attacking, Tactical, Positional, Technical)

### 🔄 In Progress
- Search algorithm (MCTS/Alpha-Beta)
- Verbal PV explanations
- Checkmate detection

### 📋 Roadmap
- Proper Zobrist hashing
- Transposition tables
- Quiescence search
- Opening book integration

## Knowledge Base

Built on concepts from:
- Jeremy Silman - "How to Reassess Your Chess"
- Konstantin Sakaev - "Russian Chess School Vol 1 & 2"
- M.I. Shereshevsky - "Endgame Strategy"
- Alexander Kotov - "Play Like a Grandmaster"
- Vladimir Vukovic - "The Art of Attack"
- Neil McDonald - "Positional Sacrifices"
- Ivan Sokolov - "Sacrifice and Initiative"

## License

MIT License - Open source for the chess community!

## Author

Built with ❤️ by Brendan and Jay

---

*"Chess is a conversation with the board. Our engine teaches you how to listen."*
