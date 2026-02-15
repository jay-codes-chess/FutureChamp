# FutureChamp v1.1-alpha

Human-like chess engine with configurable personalities.

## Features

- UCI compatible
- Multiple personality presets (text + JSON)
- TradeBias for controlling trading style
- Evaluation tiering for speed optimization
- LMR, Null Move, Futility pruning
- SEE pruning and check extensions
- Human-like move selection

## Usage

```
FutureChamp.exe
```

## Personality Files

Load via UCI:
- `setoption name Personality value petrosian` (JSON)
- `setoption name PersonalityFile value personalities/tal.txt` (text)

## Options

See UCI output for full list of options.

## Build

-O2 -DNDEBUG -std=c++17 -static
