/**
 * Piece-Square Tables Implementation
 */

#include "pst.hpp"
#include <algorithm>

namespace Evaluation {

// Precomputed distance to center squares (d4,e4,d5,e5)
int dist_to_center[64] = {
    6,5,4,3,3,4,5,6,
    5,4,3,2,2,3,4,5,
    4,3,2,1,1,2,3,4,
    3,2,1,0,0,1,2,3,
    3,2,1,0,0,1,2,3,
    4,3,2,1,1,2,3,4,
    5,4,3,2,2,3,4,5,
    6,5,4,3,3,4,5,6
};

// Knight PST - Opening: strong center preference
const int PST_KNIGHT_OPENING[64] = {
    -25,-15,-10,-10,-10,-10,-15,-25,
    -15,-10,  5, 10, 10,  5,-10,-15,
    -10,  5, 15, 20, 20, 15,  5,-10,
    -10, 10, 20, 25, 25, 20, 10,-10,
    -10, 10, 20, 25, 25, 20, 10,-10,
    -10,  5, 15, 20, 20, 15,  5,-10,
    -15,-10,  5, 10, 10,  5,-10,-15,
    -25,-15,-10,-10,-10,-10,-15,-25
};

// Knight PST - Endgame: similar but slightly less aggressive
const int PST_KNIGHT_ENDGAME[64] = {
    -20,-15,-10, -8, -8,-10,-15,-20,
    -15,-10,  0,  5,  5,  0,-10,-15,
    -10,  0, 10, 15, 15, 10,  0,-10,
     -8,  5, 15, 20, 20, 15,  5, -8,
     -8,  5, 15, 20, 20, 15,  5, -8,
    -10,  0, 10, 15, 15, 10,  0,-10,
    -15,-10,  0,  5,  5,  0,-10,-15,
    -20,-15,-10, -8, -8,-10,-15,-20
};

// Bishop PST - Opening: prefer long diagonals/center
const int PST_BISHOP_OPENING[64] = {
    -10, -5, -5, -5, -5, -5, -5,-10,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  5,  8,  8,  5,  0, -5,
    -5,  0,  8, 12, 12,  8,  0, -5,
    -5,  0,  8, 12, 12,  8,  0, -5,
    -5,  0,  5,  8,  8,  5,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -10, -5, -5, -5, -5, -5, -5,-10
};

// Bishop PST - Endgame: prefer center
const int PST_BISHOP_ENDGAME[64] = {
    -10, -5, -5, -5, -5, -5, -5,-10,
    -5,  0,  2,  2,  2,  2,  0, -5,
    -5,  2,  5,  8,  8,  5,  2, -5,
    -5,  2,  8, 10, 10,  8,  2, -5,
    -5,  2,  8, 10, 10,  8,  2, -5,
    -5,  2,  5,  8,  8,  5,  2, -5,
    -5,  0,  2,  2,  2,  2,  0, -5,
    -10, -5, -5, -5, -5, -5, -5,-10
};

// Rook PST - Opening: small 7th rank bonus
const int PST_ROOK_OPENING[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
     -2,  0,  0,  0,  0,  0,  0, -2,
     -2,  0,  0,  0,  0,  0,  0, -2,
     -2,  0,  0,  0,  0,  0,  0, -2,
     -2,  0,  0,  0,  0,  0,  0, -2,
      3,  5,  5,  8,  8,  5,  5,  3,
      5, 10, 10, 15, 15, 10, 10,  5
};

// Rook PST - Endgame: prefer 7th rank more
const int PST_ROOK_ENDGAME[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
     -2,  0,  0,  0,  0,  0,  0, -2,
     -2,  0,  0,  0,  0,  0,  0, -2,
     -2,  0,  0,  0,  0,  0,  0, -2,
     -2,  0,  0,  0,  0,  0,  0, -2,
      5, 10, 10, 15, 15, 10, 10,  5,
     10, 20, 20, 25, 25, 20, 20, 10
};

// Queen PST - Opening: avoid early central commitment
const int PST_QUEEN_OPENING[64] = {
    -10, -5, -5, -5, -5, -5, -5,-10,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  2,  5,  5,  2,  0, -5,
    -5,  0,  5,  8,  8,  5,  0, -5,
    -5,  0,  5,  8,  8,  5,  0, -5,
    -5,  0,  2,  5,  5,  2,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -10, -5, -5, -5, -5, -5, -5,-10
};

// Queen PST - Endgame: prefers center slightly
const int PST_QUEEN_ENDGAME[64] = {
    -10, -5, -2, -2, -2, -2, -5,-10,
    -5,  0,  2,  2,  2,  2,  0, -5,
    -2,  2,  5,  5,  5,  5,  2, -2,
    -2,  2,  5, 10, 10,  5,  2, -2,
    -2,  2,  5, 10, 10,  5,  2, -2,
    -2,  2,  5,  5,  5,  5,  2, -2,
    -5,  0,  2,  2,  2,  2,  0, -5,
    -10, -5, -2, -2, -2, -2, -5,-10
};

// King PST - Opening: prefer castled position
const int PST_KING_OPENING[64] = {
    -30,-20,-10, -5, -5,-10,-20,-30,
    -20,-10,  0,  5,  5,  0,-10,-20,
    -10,  0, 10, 20, 20, 10,  0,-10,
     -5,  5, 20, 30, 30, 20,  5, -5,
     -5,  5, 20, 30, 30, 20,  5, -5,
    -10,  0, 10, 20, 20, 10,  0,-10,
    -20,-10,  0,  5,  5,  0,-10,-20,
    -30,-20,-10, -5, -5,-10,-20,-30
};

// King PST - Endgame: prefer center for mating
const int PST_KING_ENDGAME[64] = {
    -20,-15,-10, -5, -5,-10,-15,-20,
    -15,-10, -5,  0,  0, -5,-10,-15,
    -10, -5,  5, 10, 10,  5, -5,-10,
     -5,  0, 10, 20, 20, 10,  0, -5,
     -5,  0, 10, 20, 20, 10,  0, -5,
    -10, -5,  5, 10, 10,  5, -5,-10,
    -15,-10, -5,  0,  0, -5,-10,-15,
    -20,-15,-10, -5, -5,-10,-15,-20
};

// Pawn PST - Opening: standard structure
const int PST_PAWN_OPENING[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
     -5, -5, -5, -5, -5, -5, -5, -5,
      0,  0,  0,  0,  0,  0,  0,  0,
      5,  5,  5,  5,  5,  5,  5,  5,
     10, 10, 10, 10, 10, 10, 10, 10,
     15, 15, 15, 15, 15, 15, 15, 15,
      0,  0,  0,  0,  0,  0,  0,  0
};

// Pawn PST - Endgame: advance
const int PST_PAWN_ENDGAME[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      5,  5,  5,  5,  5,  5,  5,  5,
     10, 10, 10, 10, 10, 10, 10, 10,
     15, 15, 15, 15, 15, 15, 15, 15,
     20, 20, 20, 20, 20, 20, 20, 20
};

// Mirror square for black (flip rank)
int mirror_square(int sq) {
    return (sq ^ 56);
}

// Compute game phase (0=opening, MAX_PHASE=endgame)
int compute_phase(const Board& board) {
    int phase = MAX_PHASE;
    
    // Count major pieces
    uint64_t queens = board.pieces[QUEEN];
    uint64_t rooks = board.pieces[ROOK];
    uint64_t bishops = board.pieces[BISHOP];
    uint64_t knights = board.pieces[KNIGHT];
    
    phase -= 4 * __builtin_popcountll(queens);
    phase -= 2 * __builtin_popcountll(rooks);
    phase -= 1 * __builtin_popcountll(bishops);
    phase -= 1 * __builtin_popcountll(knights);
    
    return std::max(0, phase);
}

// Get PST value for a piece on a square (white's perspective)
int get_pst_value(int piece, int sq, bool is_white, const int* opening_table, const int* endgame_table, int phase) {
    // Convert to white's perspective
    int psq = is_white ? sq : mirror_square(sq);
    
    int open_score = opening_table[psq];
    int end_score = endgame_table[psq];
    
    // Tapered interpolation
    return (open_score * phase + end_score * (MAX_PHASE - phase)) / MAX_PHASE;
}

// Evaluate PST for a specific color
int evaluate_pst_for_color(const Board& board, int color, int phase) {
    int score = 0;
    int our_color = color;
    
    // Pawns
    uint64_t pawns = board.pieces[PAWN] & board.colors[our_color];
    while (pawns) {
        int sq = __builtin_ctzll(pawns);
        pawns &= pawns - 1;
        score += get_pst_value(PAWN, sq, true, PST_PAWN_OPENING, PST_PAWN_ENDGAME, phase);
    }
    
    // Knights
    uint64_t knights = board.pieces[KNIGHT] & board.colors[our_color];
    while (knights) {
        int sq = __builtin_ctzll(knights);
        knights &= knights - 1;
        score += get_pst_value(KNIGHT, sq, true, PST_KNIGHT_OPENING, PST_KNIGHT_ENDGAME, phase);
    }
    
    // Bishops
    uint64_t bishops = board.pieces[BISHOP] & board.colors[our_color];
    while (bishops) {
        int sq = __builtin_ctzll(bishops);
        bishops &= bishops - 1;
        score += get_pst_value(BISHOP, sq, true, PST_BISHOP_OPENING, PST_BISHOP_ENDGAME, phase);
    }
    
    // Rooks
    uint64_t rooks = board.pieces[ROOK] & board.colors[our_color];
    while (rooks) {
        int sq = __builtin_ctzll(rooks);
        rooks &= rooks - 1;
        score += get_pst_value(ROOK, sq, true, PST_ROOK_OPENING, PST_ROOK_ENDGAME, phase);
    }
    
    // Queens
    uint64_t queens = board.pieces[QUEEN] & board.colors[our_color];
    while (queens) {
        int sq = __builtin_ctzll(queens);
        queens &= queens - 1;
        score += get_pst_value(QUEEN, sq, true, PST_QUEEN_OPENING, PST_QUEEN_ENDGAME, phase);
    }
    
    // King
    uint64_t king = board.pieces[KING] & board.colors[our_color];
    if (king) {
        int sq = __builtin_ctzll(king);
        score += get_pst_value(KING, sq, true, PST_KING_OPENING, PST_KING_ENDGAME, phase);
    }
    
    return score;
}

// Main PST evaluation function
int evaluate_pst(const Board& board) {
    int phase = compute_phase(board);
    
    int white_score = evaluate_pst_for_color(board, WHITE, phase);
    int black_score = evaluate_pst_for_color(board, BLACK, phase);
    
    // Return from white's perspective
    return white_score - black_score;
}

} // namespace Evaluation
