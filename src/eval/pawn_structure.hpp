/**
 * Pawn Structure Evaluation Layer
 * Evaluates pawn weaknesses, passed pawns, isolated/doubled pawns
 */

#ifndef EVAL_PAWN_STRUCTURE_HPP
#define EVAL_PAWN_STRUCTURE_HPP

#include "../utils/board.hpp"
#include <cstdint>

namespace Evaluation {

// Pawn structure evaluation - returns centipawns from White's perspective
int evaluate_pawn_structure(const Board& board);

// Pawn hash cache for fast lookup
struct PawnHashEntry {
    uint64_t key;
    int score;
    bool valid;
};

void init_pawn_hash(int size);
void clear_pawn_hash();
int probe_pawn_hash(uint64_t key, int& score);
void store_pawn_hash(uint64_t key, int score);

// Internal helper declarations (extracted from evaluation.cpp)
bool is_opening(const Board& board);

} // namespace Evaluation

#endif // EVAL_PAWN_STRUCTURE_HPP
