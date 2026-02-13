/**
 * Material Evaluation Layer Implementation
 */

#include "material.hpp"

namespace Evaluation {

int count_material(const Board& board, int color) {
    int mat = 0;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != color) continue;
        int type = board.piece_at(sq);
        switch (type) {
            case PAWN:   mat += PAWN_VALUE;   break;
            case KNIGHT: mat += KNIGHT_VALUE; break;
            case BISHOP: mat += BISHOP_VALUE; break;
            case ROOK:   mat += ROOK_VALUE;   break;
            case QUEEN:  mat += QUEEN_VALUE;  break;
        }
    }
    return mat;
}

int evaluate_material(const Board& board) {
    return count_material(board, WHITE) - count_material(board, BLACK);
}

} // namespace Evaluation
