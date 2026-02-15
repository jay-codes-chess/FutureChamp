/**
 * King Safety Evaluation Layer Implementation
 */

#include "king_safety.hpp"
#include "pawn_structure.hpp"
#include "../uci/uci.hpp"

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
int evaluate_king_danger_for_color(const Board& board, int color);

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

// King danger - how unsafe is the ENEMY king (positive = enemy king in danger)
// Computed from White's perspective
int evaluate_king_danger(const Board& board) {
    int white_danger = evaluate_king_danger_for_color(board, BLACK);
    int black_danger = evaluate_king_danger_for_color(board, WHITE);
    return white_danger - black_danger;
}

int evaluate_king_danger_for_color(const Board& board, int enemy_color) {
    // Find enemy king
    int enemy_king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING && board.color_at(sq) == enemy_color) {
            enemy_king_sq = sq;
            break;
        }
    }
    if (enemy_king_sq == -1) return 0;
    
    int our_color = 1 - enemy_color;
    int king_file = enemy_king_sq % 8;
    int king_rank = enemy_king_sq / 8;
    
    // FAST: Pawn shield penalty
    int shield_penalty = 0;
    int king_side = (enemy_king_sq > 27) ? 1 : 0;
    
    if (king_side) {
        int start_file = (enemy_color == WHITE) ? 5 : 0;
        for (int f = start_file; f < start_file + 3; ++f) {
            int rank = (enemy_color == WHITE) ? (enemy_king_sq < 16 ? 0 : 1) : (enemy_king_sq >= 48 ? 7 : 6);
            int sq = rank * 8 + f;
            if (board.piece_at(sq) != PAWN || board.color_at(sq) != enemy_color) {
                shield_penalty += UCI::options.king_danger_shield_penalty;
            }
        }
    } else {
        int start_file = (enemy_color == WHITE) ? 1 : 5;
        for (int f = start_file; f < start_file + 3; ++f) {
            int rank = (enemy_color == WHITE) ? (enemy_king_sq < 16 ? 0 : 1) : (enemy_king_sq >= 48 ? 7 : 6);
            int sq = rank * 8 + f;
            if (board.piece_at(sq) != PAWN || board.color_at(sq) != enemy_color) {
                shield_penalty += UCI::options.king_danger_shield_penalty;
            }
        }
    }
    
    // FAST: Open/semi-open files toward king
    int file_penalty = 0;
    for (int f = king_file - 2; f <= king_file + 2; ++f) {
        if (f < 0 || f > 7) continue;
        bool open = true, semi = true;
        for (int r = 0; r < 8; ++r) {
            int sq = r * 8 + f;
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == enemy_color) {
                open = false; break;
            }
            if (board.piece_at(sq) == PAWN) semi = false;
        }
        if (open) file_penalty += 30;
        else if (semi) file_penalty += 15;
    }
    
    // MED: Ring attacks
    int ring_attacks = 0;
    for (int dr = -2; dr <= 2; ++dr) {
        for (int df = -2; df <= 2; ++df) {
            if (dr == 0 && df == 0) continue;
            int r = king_rank + dr;
            int f = king_file + df;
            if (r < 0 || r > 7 || f < 0 || f > 7) continue;
            int sq = r * 8 + f;
            if (board.color_at(sq) == our_color) {
                int piece = board.piece_at(sq);
                if (piece == QUEEN) ring_attacks += 10;
                else if (piece == ROOK) ring_attacks += 7;
                else if (piece == BISHOP || piece == KNIGHT) ring_attacks += 5;
            }
        }
    }
    
    // Queen proximity bonus
    int queen_bonus = 0;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == QUEEN && board.color_at(sq) == our_color) {
            int dist = std::max(std::abs((sq % 8) - king_file), std::abs((sq / 8) - king_rank));
            if (dist <= 4) queen_bonus += (5 - dist) * 5;
        }
    }
    
    int total_danger = shield_penalty + file_penalty;
    total_danger += ring_attacks * UCI::options.king_danger_ring_bonus / 100;
    total_danger += queen_bonus;
    
    return total_danger;
}

} // namespace Evaluation
