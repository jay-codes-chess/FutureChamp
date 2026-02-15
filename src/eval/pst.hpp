/**
 * Piece-Square Tables (PST)
 * Provides positional bonuses for pieces on specific squares
 * Supports tapered evaluation (opening/endgame interpolation)
 */

#ifndef EVAL_PST_HPP
#define EVAL_PST_HPP

#include "../utils/board.hpp"

namespace Evaluation {

// Piece-square tables (from White's perspective)
// Index: 0=a1, 63=h8
// Opening and endgame tables for each piece type

// Knights: prefer center squares, penalize edges/corners
extern const int PST_KNIGHT_OPENING[64];
extern const int PST_KNIGHT_ENDGAME[64];

// Bishops: prefer long diagonals/center
extern const int PST_BISHOP_OPENING[64];
extern const int PST_BISHOP_ENDGAME[64];

// Rooks: prefer 7th rank, open files
extern const int PST_ROOK_OPENING[64];
extern const int PST_ROOK_ENDGAME[64];

// Queens: mild center early, avoid early adventure
extern const int PST_QUEEN_OPENING[64];
extern const int PST_QUEEN_ENDGAME[64];

// Kings: prefer castled squares in opening, center in endgame
extern const int PST_KING_OPENING[64];
extern const int PST_KING_ENDGAME[64];

// Pawns: standard pawn structure bonuses
extern const int PST_PAWN_OPENING[64];
extern const int PST_PAWN_ENDGAME[64];

// Phase calculation constants
constexpr int PHASE_NB = 1;  // Knight/bishop
constexpr int PHASE_ROOK = 2;
constexpr int PHASE_QUEEN = 4;
constexpr int MAX_PHASE = 24;  // 2N + 2B + 2R + 1Q * 4 = 24

// Compute game phase from board
int compute_phase(const Board& board);

// Mirror square for black (a1->a8, etc.)
int mirror_square(int sq);

// Distance to nearest center square (d4,e4,d5,e5)
extern int dist_to_center[64];

// Evaluate PST for a board position
int evaluate_pst(const Board& board);

// Evaluate PST for a specific color (internal helper)
int evaluate_pst_for_color(const Board& board, int color, int phase);

} // namespace Evaluation

#endif // EVAL_PST_HPP
