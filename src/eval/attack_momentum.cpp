/**
 * Attack Momentum Evaluation
 * Rewards sustained attacking pressure across evaluation terms
 */

#include "attack_momentum.hpp"

namespace Evaluation {

int evaluate_attack_momentum(
    const Board& board,
    int tropism,
    int pawn_storm,
    int line_opening,
    int initiative)
{
    // Combined attacking pressure
    int pressure = tropism + pawn_storm + line_opening + initiative;

    int bonus = 0;

    // Activate only when real attack exists
    if (pressure > 40)
        bonus += (pressure - 40) / 2;

    // Strong attacking phase bonus
    if (pressure > 80)
        bonus += 10;

    // Safety cap
    if (bonus > 30)
        bonus = 30;

    return bonus;
}

} // namespace Evaluation
