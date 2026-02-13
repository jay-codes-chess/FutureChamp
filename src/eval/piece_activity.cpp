/**
 * Piece Activity Evaluation Layer Implementation
 */

#include "piece_activity.hpp"
#include "pawn_structure.hpp"
#include <cmath>

namespace Evaluation {

// Piece-Square Tables
static const int KNIGHT_PST[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static const int BISHOP_PST[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static const int ROOK_PST[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      0,   0,   5,  10,  10,   5,   0,   0
};

static const int QUEEN_PST[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static const int PAWN_PST[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     50,  50,  50,  50,  50,  50,  50,  50,
     15,  15,  25,  40,  40,  25,  15,  15,
     10,  10,  15,  35,  35,  15,  10,  10,
      5,   5,  10,  25,  25,  10,   5,   5,
      0,   0,   0,  15,  15,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};

static int mirror_square(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    return (7 - rank) * 8 + file;
}

int get_pst_value(int piece_type, int square, int color) {
    int idx = (color == WHITE) ? square : mirror_square(square);
    switch (piece_type) {
        case PAWN:   return PAWN_PST[idx];
        case KNIGHT: return KNIGHT_PST[idx];
        case BISHOP: return BISHOP_PST[idx];
        case ROOK:   return ROOK_PST[idx];
        case QUEEN:  return QUEEN_PST[idx];
        default:     return 0;
    }
}

int evaluate_piece_activity(const Board& board) {
    int score = 0;
    
    // Center control bonus
    uint64_t attacks = Bitboards::all_attacks(board, WHITE);
    int center_squares[] = {27, 28, 35, 36};
    for (int sq : center_squares) {
        if (Bitboards::test(attacks, sq)) {
            score += 3;
        }
    }
    
    // White piece activity
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != WHITE) continue;
        int type = board.piece_at(sq);
        if (type == NO_PIECE || type == KING) continue;
        
        score += get_pst_value(type, sq, WHITE);
        
        // Reduced opening bonus for knights/bishops
        if (type == KNIGHT || type == BISHOP) {
            int rank = sq / 8;
            if (rank >= 2) score += 8;
        }
        
        // Center control
        int file = sq % 8, rank = sq / 8;
        int dist = std::abs(file - 3) + std::abs(rank - 3);
        if (dist <= 2) score += 5;
    }
    
    // Black piece activity (subtract)
    attacks = Bitboards::all_attacks(board, BLACK);
    for (int sq : center_squares) {
        if (Bitboards::test(attacks, sq)) {
            score -= 3;
        }
    }
    
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != BLACK) continue;
        int type = board.piece_at(sq);
        if (type == NO_PIECE || type == KING) continue;
        
        score -= get_pst_value(type, sq, BLACK);
        
        if (type == KNIGHT || type == BISHOP) {
            int rank = sq / 8;
            if (rank <= 5) score -= 8;
        }
        
        int file = sq % 8, rank = sq / 8;
        int dist = std::abs(file - 3) + std::abs(rank - 3);
        if (dist <= 2) score -= 5;
    }
    
    return score;
}

} // namespace Evaluation
