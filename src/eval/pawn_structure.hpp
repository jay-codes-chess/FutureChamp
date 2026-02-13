/**
 * Pawn Structure Evaluation Layer
 * Evaluates pawn weaknesses, passed pawns, isolated/doubled pawns
 */

#ifndef EVAL_PAWN_STRUCTURE_HPP
#define EVAL_PAWN_STRUCTURE_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// Pawn structure evaluation - returns centipawns from White's perspective
int evaluate_pawn_structure(const Board& board);

// Internal helper declarations (extracted from evaluation.cpp)
bool is_opening(const Board& board);

} // namespace Evaluation

#endif // EVAL_PAWN_STRUCTURE_HPP
