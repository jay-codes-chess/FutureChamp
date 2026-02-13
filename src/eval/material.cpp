/**
 * Material Evaluation Layer Implementation
 */

#include "material.hpp"
#include "params.hpp"

namespace Evaluation {

int count_material(const Board& board, int color) {
    const auto& params = get_params();
    int mat = 0;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != color) continue;
        int type = board.piece_at(sq);
        switch (type) {
            case PAWN:   mat += PAWN_VALUE;   break;
            case KNIGHT: mat += KNIGHT_VALUE + params.knight_value_bias; break;
            case BISHOP: mat += BISHOP_VALUE + params.bishop_value_bias; break;
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
