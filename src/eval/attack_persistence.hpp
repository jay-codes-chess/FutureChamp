#pragma once

#include "../utils/board.hpp"

namespace Evaluation {

// Evaluate attack persistence - discourages retreating attackers when momentum is active
int evaluate_attack_persistence(
    const Board& board,
    int attack_momentum_score);

} // namespace Evaluation
