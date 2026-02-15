/**
 * King Safety Evaluation Layer
 * Evaluates king position, castling rights, pawn shield
 */

#ifndef EVAL_KING_SAFETY_HPP
#define EVAL_KING_SAFETY_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// King safety evaluation - returns centipawns from White's perspective
int evaluate_king_safety(const Board& board);

// King danger - enemy king danger (positive = enemy king unsafe)
int evaluate_king_danger(const Board& board);

} // namespace Evaluation

#endif // EVAL_KING_SAFETY_HPP
