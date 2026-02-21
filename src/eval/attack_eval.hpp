/**
 * Aggressive Attack Evaluation
 * King tropism, pawn storm, line opening, and aggressive initiative
 */

#ifndef ATTACK_EVAL_HPP
#define ATTACK_EVAL_HPP

#include "evaluation.hpp"

namespace Evaluation {

// King tropism - reward pieces for proximity to enemy king
int evaluate_king_tropism(const Board& board);

// Opposite castling detection
bool is_opposite_castling(const Board& board);

// Pawn storm bonus (only when opposite castling)
int evaluate_pawn_storm(const Board& board);

// Line opening bonus (only when opposite castling)
int evaluate_line_opening(const Board& board);

// Aggressive initiative - reward coordinated attacks
int evaluate_aggressive_initiative(const Board& board);

// Combined attack evaluation
int evaluate_attacks(const Board& board);

} // namespace Evaluation

#endif // ATTACK_EVAL_HPP
