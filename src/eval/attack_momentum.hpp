#pragma once

#include "../utils/board.hpp"

namespace Evaluation {

// Evaluate attack momentum - rewards sustained attacking pressure
int evaluate_attack_momentum(
    const Board& board,
    int king_tropism,
    int pawn_storm,
    int line_opening,
    int aggressive_initiative);

} // namespace Evaluation
