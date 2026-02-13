/**
 * Initiative Evaluation Layer
 * Evaluates development, tempo, initiative
 */

#ifndef EVAL_INITIATIVE_HPP
#define EVAL_INITIATIVE_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// Initiative evaluation - returns centipawns from White's perspective
int evaluate_initiative(const Board& board);

} // namespace Evaluation

#endif // EVAL_INITIATIVE_HPP
