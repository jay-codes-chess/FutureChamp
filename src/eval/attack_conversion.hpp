#pragma once

#include "../utils/board.hpp"

namespace Evaluation {

// Evaluate attack conversion - rewards heavy pieces using opened lines toward enemy king
int evaluate_attack_conversion(
    const Board& board,
    int attack_momentum_score);

} // namespace Evaluation
