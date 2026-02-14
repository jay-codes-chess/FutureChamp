#pragma once

#include "utils/board.hpp"

struct MoveList {
    int moves[256];
    int count;
    
    MoveList() : count(0) {}
    
    void clear() { count = 0; }
    void add(int move) { moves[count++] = move; }
    int& operator[](int i) { return moves[i]; }
    int operator[](int i) const { return moves[i]; }
    
    // For range-based for loops
    int* begin() { return moves; }
    int* end() { return moves + count; }
    const int* begin() const { return moves; }
    const int* end() const { return moves + count; }
};

// Generate moves into fixed buffer
void generate_moves(const Board& board, MoveList& list);
