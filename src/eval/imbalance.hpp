/**
 * Imbalance Evaluation Layer
 * Evaluates strategic imbalances between sides
 */

#ifndef EVAL_IMBALANCE_HPP
#define EVAL_IMBALANCE_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// Imbalance evaluation - returns centipawns from White's perspective
int evaluate_imbalance(const Board& board);

} // namespace Evaluation

#endif // EVAL_IMBALANCE_HPP
