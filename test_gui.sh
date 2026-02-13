#!/bin/bash
# GUI Test Script for Human Chess Engine
# Usage: ./test_gui.sh [depth]

ENGINE_DIR="/Users/john/.openclaw/workspace/human-chess-engine"
ENGINE="$ENGINE_DIR/human-chess-engine"
DEPTH=${1:-4}

echo "=============================================="
echo "Human Chess Engine - GUI Test"
echo "=============================================="
echo ""
echo "Engine: $ENGINE"
echo "Test depth: $DEPTH"
echo ""
echo "Testing UCI protocol..."
echo ""

# Test UCI handshake
echo -e "uci\nquit" | "$ENGINE" | grep -E "^(id|name|uciok)"

echo ""
echo "Testing move generation..."
echo -e "uci\nposition startpos\ngo depth $DEPTH\nquit" | "$ENGINE" | grep -E "^(info|bestmove)"

echo ""
echo "=============================================="
echo "To connect to a GUI (Fritz/Arena/Cute Chess):"
echo ""
echo "1. Add engine: $ENGINE"
echo "2. Protocol: UCI"
echo "3. Working directory: $ENGINE_DIR"
echo ""
echo "Recommended options:"
echo "  - PlayingStyle: classical"
echo "  - SkillLevel: 10"
echo "  - UseMCTS: false (alpha-beta mode)"
echo "=============================================="
