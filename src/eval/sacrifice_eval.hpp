#pragma once

#include "../utils/board.hpp"

namespace Evaluation {

// Evaluate sacrifice justification - reduces penalty for material down when attacking
int evaluate_sacrifice_justification(
    const Board& board,
    int material_score,
    int attack_momentum_score);

// Evaluate exchange sac tolerance - reduces penalty for exchange down when attacking
int evaluate_exchange_sac_tolerance(
    const Board& board,
    int attack_momentum_score,
    int line_opening_score,
    int king_tropism_score);

} // namespace Evaluation
