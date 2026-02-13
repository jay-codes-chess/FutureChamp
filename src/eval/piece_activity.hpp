/**
 * Piece Activity Evaluation Layer
 * Evaluates piece positioning, center control, mobility
 */

#ifndef EVAL_PIECE_ACTIVITY_HPP
#define EVAL_PIECE_ACTIVITY_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// Piece activity evaluation - returns centipawns from White's perspective
int evaluate_piece_activity(const Board& board);

} // namespace Evaluation

#endif // EVAL_PIECE_ACTIVITY_HPP
