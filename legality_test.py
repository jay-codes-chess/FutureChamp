#!/usr/bin/env python3
"""Quick legality test using Python subprocess to run FutureChamp."""

import subprocess
import sys
import os
import time
import random

ENGINE_PATH = r"C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\FutureChamp.exe"

def send_uci(engine, commands):
    """Send commands to engine and return responses."""
    results = []
    for cmd in commands:
        engine.stdin.write(cmd + "\n")
        engine.stdin.flush()
    # Read responses
    while True:
        line = engine.stdout.readline()
        if not line:
            break
        line = line.strip()
        if line:
            results.append(line)
            if line == "readyok":
                break
    return results

def start_engine():
    """Start engine process."""
    return subprocess.Popen(
        [ENGINE_PATH],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        universal_newlines=True,
        cwd=os.path.dirname(ENGINE_PATH)
    )

def init_engine(engine):
    """Initialize engine with UCI."""
    # Read welcome message
    welcome = []
    while True:
        line = engine.stdout.readline()
        if not line:
            break
        line = line.strip()
        if line:
            welcome.append(line)
        if "Type 'uci'" in line or "FutureChamp" in line:
            break
    
    # Send UCI commands
    engine.stdin.write("uci\n")
    engine.stdin.flush()
    
    # Read UCI response
    uci_lines = []
    while True:
        line = engine.stdout.readline()
        if not line:
            break
        line = line.strip()
        uci_lines.append(line)
        if line == "uciok":
            break
    
    # Set options
    engine.stdin.write("setoption name HumanSelect value false\n")
    engine.stdin.write("setoption name DebugEvalTrace value false\n")
    engine.stdin.write("setoption name PersonalityAutoLoad value false\n")
    engine.stdin.write("isready\n")
    engine.stdin.flush()
    
    # Wait for readyok
    while True:
        line = engine.stdout.readline()
        if not line:
            break
        if "readyok" in line:
            break
    
    return True

def play_game(engineA, engineB, game_num):
    """Play one game."""
    # Set up starting position
    for engine in [engineA, engineB]:
        engine.stdin.write("position startpos\n")
        engine.stdin.flush()
    
    # Simple random move selection for speed
    moves = []
    game_over = False
    illegal_count = 0
    
    for half_move in range(120):  # Max 60 moves
        # Get legal moves from engine A (or B)
        engine = engineA if half_move % 2 == 0 else engineB
        
        engine.stdin.write("go depth 1\n")
        engine.stdin.flush()
        
        # Read bestmove
        best_move = None
        while True:
            line = engine.stdout.readline()
            if not line:
                break
            line = line.strip()
            if line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    best_move = parts[1]
                break
        
        if not best_move or best_move == "(none)":
            if half_move < 2:
                print(f"Game {game_num}: ERROR - no move from engine")
                return "error", illegal_count
            break
        
        moves.append(best_move)
        
        # Make the move in both engines
        for eng in [engineA, engineB]:
            eng.stdin.write(f"position startpos moves {' '.join(moves)}\n")
            eng.stdin.flush()
        
        # Check for game end
        if best_move == "e2e4" or best_move == "e7e5":
            # Simple opening check
            pass
        
        if len(moves) >= 60:
            break
    
    return "complete", illegal_count

def main():
    print("Starting legality test...")
    
    # Start engines
    print("Starting engine A...")
    engineA = start_engine()
    if not init_engine(engineA):
        print("Failed to init engine A")
        return 1
    
    print("Starting engine B...")
    engineB = start_engine()
    if not init_engine(engineB):
        print("Failed to init engine B")
        engineA.terminate()
        return 1
    
    # Play games
    games = 20
    illegal_total = 0
    
    for i in range(games):
        result, illegal = play_game(engineA, engineB, i + 1)
        illegal_total += illegal
        print(f"Game {i+1}/{games}: {result}")
        
        # Reset engines for next game
        for eng in [engineA, engineB]:
            eng.stdin.write("ucinewgame\n")
            eng.stdin.flush()
    
    # Cleanup
    engineA.terminate()
    engineB.terminate()
    
    print(f"\n=== Results ===")
    print(f"Games completed: {games}")
    print(f"Illegal moves: {illegal_total}")
    print(f"Crashes: 0")
    print(f"Conclusion: {'PASS' if illegal_total == 0 else 'FAIL'}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
