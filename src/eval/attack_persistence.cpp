/**
 * Attack Persistence Evaluation
 * Discourages retreating attackers when attack momentum is active
 */

#include "attack_persistence.hpp"

namespace Evaluation {

// Manhattan distance between two squares
static int manhattan_distance(int sq1, int sq2) {
    int file1 = sq1 % 8;
    int rank1 = sq1 / 8;
    int file2 = sq2 % 8;
    int rank2 = sq2 / 8;
    return std::abs(file1 - file2) + std::abs(rank1 - rank2);
}

// Get king zone squares (8 surrounding squares)
static void get_king_zone(int king_sq, int zone[9]) {
    int king_file = king_sq % 8;
    int king_rank = king_sq / 8;
    int idx = 0;
    for (int df = -1; df <= 1; df++) {
        for (int dr = -1; dr <= 1; dr++) {
            int f = king_file + df;
            int r = king_rank + dr;
            if (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
                zone[idx++] = r * 8 + f;
            }
        }
    }
    zone[idx] = -1;  // terminator
}

// Check if square is in king zone
static bool in_king_zone(int sq, const int zone[9]) {
    for (int i = 0; zone[i] != -1; i++) {
        if (sq == zone[i]) return true;
    }
    return false;
}

// Count attackers near enemy king
static int count_attackers_near_king(const Board& board, int color, int enemy_king_sq) {
    int count = 0;
    int enemy_zone[9];
    get_king_zone(enemy_king_sq, enemy_zone);
    
    for (int sq = 0; sq < 64; sq++) {
        int p = board.piece_at(sq);
        if (p != QUEEN && p != ROOK && p != BISHOP && p != KNIGHT) continue;
        if (board.color_at(sq) != color) continue;
        
        // Check if in king zone or within distance 3
        bool in_zone = in_king_zone(sq, enemy_zone);
        int dist = manhattan_distance(sq, enemy_king_sq);
        
        if (in_zone || dist <= 3) {
            count++;
        }
    }
    
    return count;
}

// Check if queens are on board
static bool both_queens_on_board(const Board& board) {
    bool white_queen = false, black_queen = false;
    for (int sq = 0; sq < 64; sq++) {
        if (board.piece_at(sq) == QUEEN) {
            if (board.color_at(sq) == WHITE) white_queen = true;
            else black_queen = true;
        }
    }
    return white_queen && black_queen;
}

// Compute game phase
static int get_game_phase(const Board& board) {
    int phase = 0;
    for (int sq = 0; sq < 64; sq++) {
        int p = board.piece_at(sq);
        switch (p) {
            case QUEEN: phase += 4; break;
            case ROOK:  phase += 2; break;
            case BISHOP: 
            case KNIGHT: phase += 1; break;
            default: break;
        }
    }
    return phase;
}

int evaluate_attack_persistence(
    const Board& board,
    int attack_momentum_score)
{
    // Must have attack momentum to apply
    if (std::abs(attack_momentum_score) < 15) {
        return 0;
    }
    
    // Phase gate: only in middlegame-ish positions
    // Require queens on board AND phase >= 10
    if (!both_queens_on_board(board)) {
        return 0;
    }
    
    int phase = get_game_phase(board);
    if (phase < 10) {
        return 0;
    }
    
    // Find both kings
    int white_king_sq = -1, black_king_sq = -1;
    for (int sq = 0; sq < 64; sq++) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_sq = sq;
            else black_king_sq = sq;
        }
    }
    
    if (white_king_sq == -1 || black_king_sq == -1) {
        return 0;
    }
    
    int persistence = 0;
    
    // White's attack persistence: if attacking (momentum > 0), reward attackers near black king
    if (attack_momentum_score > 0) {
        int attackers = count_attackers_near_king(board, WHITE, black_king_sq);
        persistence += std::min(attackers * 4, 24);  // cap at +24
    }
    
    // Black's attack persistence: if attacking (momentum < 0), reward attackers near white king
    if (attack_momentum_score < 0) {
        int attackers = count_attackers_near_king(board, BLACK, white_king_sq);
        persistence -= std::min(attackers * 4, 24);  // cap at -24
    }
    
    // Hard cap
    if (persistence > 24) persistence = 24;
    if (persistence < -24) persistence = -24;
    
    return persistence;
}

} // namespace Evaluation
