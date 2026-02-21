#pragma once

#include "../utils/board.hpp"

namespace Evaluation {

// Evaluate sacrifice justification - reduces penalty for material down when attacking
int evaluate_sacrifice_justification(
    const Board& board,
    int material_score,
    int attack_momentum_score);

} // namespace Evaluation
