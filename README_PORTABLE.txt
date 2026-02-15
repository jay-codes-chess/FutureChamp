# FutureChamp Portable - Quick Start

## How to Install in Arena

1. Download `FutureChamp-v1.1.1-portable.zip` and extract anywhere
2. In Arena: Engine → Manage Engines → Add
3. Browse to `FutureChamp.exe` and select it
4. Done! The engine is ready to use

## If It Doesn't Start

Run from command line:
```
FutureChamp.exe --selfcheck
```

This will diagnose:
- Whether the executable path is detected
- Whether the personalities folder is found
- Whether perft runs correctly

## Personality Usage

The engine auto-loads personalities from the `personalities/` folder.

### Via UCI (while playing):
```
setoption name Personality value tal
setoption name Personality value petrosian
setoption name Personality value capablanca
setoption name Personality value club1800
```

### Load from specific file:
```
setoption name PersonalityFile value personalities\tal.txt
```

### Set custom personalities folder:
```
setoption name PersonalityDir value C:\ChessEngines\FutureChamp\personalities
```

## UCI Options

All standard UCI options work:
- `setoption name Hash value 128` - Set transposition table size (MB)
- `setoption name Threads value 2` - Set search threads
- `setoption name HumanSelect value false` - Disable human-style move selection

## Troubleshooting

**"Could not start engine" error in Arena:**
- Ensure you downloaded the FULL zip (includes .exe)
- Try running `--selfcheck` from command line
- Make sure no antivirus is blocking the exe

**Engine prints nothing:**
- This is correct! The engine outputs nothing until it receives `uci`
- This ensures clean GUI integration

**Personalities not loading:**
- Check that `personalities/` folder exists next to the .exe
- Use `--selfcheck` to see the detected paths
- Try setting PersonalityDir explicitly

## Version Info

Built: 2026-02-15
Static build: Yes (no DLLs required)
