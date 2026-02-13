/**
 * Imbalance Evaluation Layer Implementation
 */

#include "imbalance.hpp"
#include "material.hpp"
#include "pawn_structure.hpp"
#include <cmath>

namespace Evaluation {

float evaluate_space(const Board& board, int color) {
    float space = 0.0f;
    
    for (int sq = 0; sq < 64; ++sq) {
        int rank = sq / 8;
        int file = sq % 8;
        
        if (color == WHITE && rank >= 4) {
            uint64_t attacks = Bitboards::all_attacks(board, color);
            if (Bitboards::test(attacks, sq)) {
                space += 0.5f;
                if (file >= 2 && file <= 5) {
                    space += 0.5f;
                }
            }
        } else if (color == BLACK && rank <= 3) {
            uint64_t attacks = Bitboards::all_attacks(board, color);
            if (Bitboards::test(attacks, sq)) {
                space += 0.5f;
                if (file >= 2 && file <= 5) {
                    space += 0.5f;
                }
            }
        }
    }
    
    return space;
}

int evaluate_minor_pieces(const Board& board, int color) {
    int w_kn = 0, w_bi = 0, b_kn = 0, b_bi = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int type = board.piece_at(sq);
        if (type == KNIGHT) (board.color_at(sq) == WHITE ? w_kn : b_kn)++;
        if (type == BISHOP) (board.color_at(sq) == WHITE ? w_bi : b_bi)++;
    }
    
    if (color == WHITE) {
        return w_kn * 32 + w_bi * 33 - (b_kn * 32 + b_bi * 33);
    } else {
        return b_kn * 32 + b_bi * 33 - (w_kn * 32 + w_bi * 33);
    }
}

int evaluate_imbalance(const Board& board) {
    int score = 0;
    
    // Space imbalance
    float white_space = evaluate_space(board, WHITE);
    float black_space = evaluate_space(board, BLACK);
    score += static_cast<int>((white_space - black_space) * 12);
    
    // Minor piece imbalance
    score += evaluate_minor_pieces(board, WHITE);
    
    return score;
}

} // namespace Evaluation
