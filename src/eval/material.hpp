/**
 * Material Evaluation Layer
 * Pure material count - no positional bonuses
 */

#ifndef EVAL_MATERIAL_HPP
#define EVAL_MATERIAL_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// Piece values (centipawns)
constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;
constexpr int KING_VALUE = 0;

// Material evaluation - returns centipawns from White's perspective
int evaluate_material(const Board& board);

// Count material for a specific color
int count_material(const Board& board, int color);

} // namespace Evaluation

#endif // EVAL_MATERIAL_HPP
