/**
 * King Safety Evaluation Layer Implementation
 */

#include "king_safety.hpp"
#include "pawn_structure.hpp"

namespace Evaluation {

static const int KING_PST[64] = {
     20,  30,  10,   0, -30, -50,  30,  20,
    -30, -30, -30, -30, -30, -30, -30, -30,
    -50, -50, -50, -50, -50, -50, -50, -50,
    -70, -70, -70, -70, -70, -70, -70, -70,
    -90, -90, -90, -90, -90, -90, -90, -90,
    -90, -90, -90, -90, -90, -90, -90, -90,
    -90, -90, -90, -90, -90, -90, -90, -90,
    -90, -90, -90, -90, -90, -90, -90, -90
};

// Local helper - mirror square for black pieces
static int mirror_square(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    return (7 - rank) * 8 + file;
}

int evaluate_king_safety_for_color(const Board& board, int color);

int evaluate_king_safety(const Board& board) {
    int score = 0;
    
    score += evaluate_king_safety_for_color(board, WHITE);
    score -= evaluate_king_safety_for_color(board, BLACK);
    
    return score;
}

int evaluate_king_safety_for_color(const Board& board, int color) {
    int king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
            king_sq = sq; break;
        }
    }
    if (king_sq == -1) return -20000;
    
    int rank = Bitboards::rank_of(king_sq);
    int score = (color == WHITE) ? KING_PST[king_sq] : KING_PST[mirror_square(king_sq)];
    
    // Castling bonus in early game
    if (board.fullmove_number <= 15) {
        if (board.castling[color][0] || board.castling[color][1]) {
            score += 60;
        }
        
        if (color == WHITE) {
            if (king_sq == 6) score += 120;
            if (king_sq == 2) score += 110;
        } else {
            if (king_sq == 62) score += 120;
            if (king_sq == 58) score += 110;
        }
    }
    
    // Pawn shield
    int shield_rank = (color == WHITE) ? rank + 1 : rank - 1;
    if (shield_rank >= 0 && shield_rank < 8) {
        int file = king_sq % 8;
        for (int df = -1; df <= 1; ++df) {
            int f = file + df;
            if (f < 0 || f > 7) continue;
            int sq = shield_rank * 8 + f;
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == color) {
                score += 18;
            }
        }
    }
    
    // Penalty for leaving back rank in opening
    if (is_opening(board)) {
        int back_rank = (color == WHITE) ? 0 : 7;
        if (rank != back_rank) {
            score -= 200;
        }
        if (board.castling[color][0] || board.castling[color][1]) {
            score += 30;
        }
    }
    
    return score;
}

} // namespace Evaluation
