/**
 * Attack Conversion Evaluation
 * Rewards heavy pieces using opened lines toward enemy king when attack momentum is active
 */

#include "attack_conversion.hpp"

namespace Evaluation {

// Get file and rank from square
static int get_file(int sq) { return sq % 8; }
static int get_rank(int sq) { return sq / 8; }

// Find king square for a color
static int find_king(const Board& board, int color) {
    for (int sq = 0; sq < 64; sq++) {
        if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
            return sq;
        }
    }
    return -1;
}

// Check if queens are on board
static bool both_queens_on_board(const Board& board) {
    bool wq = false, bq = false;
    for (int sq = 0; sq < 64; sq++) {
        if (board.piece_at(sq) == QUEEN) {
            if (board.color_at(sq) == WHITE) wq = true;
            else bq = true;
        }
    }
    return wq && bq;
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

// Check if file is open (no pawns of either color)
static bool is_file_open(const Board& board, int file) {
    for (int r = 0; r < 8; r++) {
        if (board.piece_at(r * 8 + file) == PAWN) return false;
    }
    return true;
}

// Check if file is semi-open for a color (no pawns of that color)
static bool is_file_semi_open(const Board& board, int file, int color) {
    int start_rank = (color == WHITE) ? 0 : 7;
    int end_rank = (color == WHITE) ? 8 : -1;
    int dir = (color == WHITE) ? 1 : -1;
    for (int r = start_rank; r != end_rank; r += dir) {
        int sq = r * 8 + file;
        if (board.piece_at(sq) == PAWN && board.color_at(sq) == color) return false;
    }
    return true;
}

// Check if horizontal path between two files on same rank is clear
static bool is_horizontal_clear(const Board& board, int from_file, int to_file, int rank) {
    int start = std::min(from_file, to_file);
    int end = std::max(from_file, to_file);
    for (int f = start + 1; f < end; f++) {
        if (board.piece_at(rank * 8 + f) != NO_PIECE) return false;
    }
    return true;
}

// Check if diagonal from sq1 to sq2 is clear
static bool is_diagonal_clear(const Board& board, int sq1, int sq2) {
    int f1 = get_file(sq1), r1 = get_rank(sq1);
    int f2 = get_file(sq2), r2 = get_rank(sq2);
    if (std::abs(f2 - f1) != std::abs(r2 - r1)) return false; // Not diagonal
    
    int df = (f2 > f1) ? 1 : -1;
    int dr = (r2 > r1) ? 1 : -1;
    int f = f1 + df, r = r1 + dr;
    while (f != f2 || r != r2) {
        if (board.piece_at(r * 8 + f) != NO_PIECE) return false;
        f += df; r += dr;
    }
    return true;
}

// Get king zone squares
static void get_king_zone(int king_sq, bool zone[64]) {
    int f = get_file(king_sq), r = get_rank(king_sq);
    for (int df = -1; df <= 1; df++) {
        for (int dr = -1; dr <= 1; dr++) {
            int nf = f + df, nr = r + dr;
            if (nf >= 0 && nf <= 7 && nr >= 0 && nr <= 7) {
                zone[nr * 8 + nf] = true;
            }
        }
    }
}

// Count rook/queen on a file
static int count_rq_on_file(const Board& board, int color, int file) {
    int count = 0;
    for (int r = 0; r < 8; r++) {
        int sq = r * 8 + file;
        int p = board.piece_at(sq);
        if (board.color_at(sq) == color && (p == ROOK || p == QUEEN)) {
            count++;
        }
    }
    return count;
}

// Check if rook is on rank 2/3 (white) or 5/6 (black) and can move horizontally to king side
static int rook_lift_bonus(const Board& board, int color, int king_file) {
    int bonus = 0;
    int target_start = king_file - 1;
    int target_end = king_file + 1;
    if (target_start < 0) target_start = 0;
    if (target_end > 7) target_end = 7;
    
    for (int sq = 0; sq < 64; sq++) {
        if (board.piece_at(sq) != ROOK) continue;
        if (board.color_at(sq) != color) continue;
        
        int r = get_rank(sq);
        bool good_rank = (color == WHITE && (r == 2 || r == 3)) ||
                        (color == BLACK && (r == 5 || r == 4));
        if (!good_rank) continue;
        
        // Check horizontal path to king-side files
        for (int f = target_start; f <= target_end; f++) {
            if (is_horizontal_clear(board, get_file(sq), f, r)) {
                bonus += 6;
                break; // Only count once per rook
            }
        }
    }
    return std::min(bonus, 12);
}

// Count diagonal attackers hitting king zone
static int diagonal_alignment_bonus(const Board& board, int color, int enemy_king_sq) {
    int bonus = 0;
    bool enemy_zone[64] = {false};
    get_king_zone(enemy_king_sq, enemy_zone);
    
    for (int sq = 0; sq < 64; sq++) {
        int p = board.piece_at(sq);
        if (p != BISHOP && p != QUEEN) continue;
        if (board.color_at(sq) != color) continue;
        
        // Check if this piece has clear diagonal to any zone square
        for (int zone_sq = 0; zone_sq < 64; zone_sq++) {
            if (!enemy_zone[zone_sq]) continue;
            if (is_diagonal_clear(board, sq, zone_sq)) {
                bonus += 4;
                break; // Count each piece once
            }
        }
    }
    return std::min(bonus, 12);
}

int evaluate_attack_conversion(
    const Board& board,
    int attack_momentum_score)
{
    // Must have attack momentum
    if (std::abs(attack_momentum_score) < 15) {
        return 0;
    }
    
    // Phase gate: require queens on board and reasonable phase
    if (!both_queens_on_board(board)) {
        return 0;
    }
    int phase = get_game_phase(board);
    if (phase < 10) {
        return 0;
    }
    
    // Find kings
    int wk = find_king(board, WHITE);
    int bk = find_king(board, BLACK);
    if (wk == -1 || bk == -1) return 0;
    
    int wk_file = get_file(wk);
    int bk_file = get_file(bk);
    
    int conversion = 0;
    
    // White's conversion bonus (attacking black)
    if (attack_momentum_score > 0) {
        int white_bonus = 0;
        
        // Check king-side files (bk_file - 1, bk_file, bk_file + 1)
        for (int df = -1; df <= 1; df++) {
            int f = bk_file + df;
            if (f < 0 || f > 7) continue;
            
            // File openness
            if (is_file_open(board, f)) {
                white_bonus += 8;
            } else if (is_file_semi_open(board, f, WHITE)) {
                white_bonus += 4;
            }
            
            // Rook/queen on file
            int rq = count_rq_on_file(board, WHITE, f);
            if (rq >= 1) {
                // Check if rook or queen
                for (int r = 0; r < 8; r++) {
                    int sq = r * 8 + f;
                    int p = board.piece_at(sq);
                    if (board.color_at(sq) == WHITE) {
                        if (p == ROOK) white_bonus += 10;
                        else if (p == QUEEN) white_bonus += 7;
                    }
                }
            }
        }
        
        // Battery bonus: rook + queen same file
        for (int f = std::max(0, bk_file - 1); f <= std::min(7, bk_file + 1); f++) {
            bool has_rook = false, has_queen = false;
            for (int r = 0; r < 8; r++) {
                int sq = r * 8 + f;
                if (board.color_at(sq) == WHITE) {
                    if (board.piece_at(sq) == ROOK) has_rook = true;
                    else if (board.piece_at(sq) == QUEEN) has_queen = true;
                }
            }
            if (has_rook && has_queen) white_bonus += 6;
        }
        
        // Rook lift bonus
        white_bonus += rook_lift_bonus(board, WHITE, bk_file);
        
        // Diagonal alignment
        white_bonus += diagonal_alignment_bonus(board, WHITE, bk);
        
        white_bonus = std::min(white_bonus, 35);
        conversion += white_bonus;
    }
    
    // Black's conversion bonus (attacking white)
    if (attack_momentum_score < 0) {
        int black_bonus = 0;
        
        // Check king-side files (wk_file - 1, wk_file, wk_file + 1)
        for (int df = -1; df <= 1; df++) {
            int f = wk_file + df;
            if (f < 0 || f > 7) continue;
            
            // File openness
            if (is_file_open(board, f)) {
                black_bonus += 8;
            } else if (is_file_semi_open(board, f, BLACK)) {
                black_bonus += 4;
            }
            
            // Rook/queen on file
            int rq = count_rq_on_file(board, BLACK, f);
            if (rq >= 1) {
                for (int r = 0; r < 8; r++) {
                    int sq = r * 8 + f;
                    int p = board.piece_at(sq);
                    if (board.color_at(sq) == BLACK) {
                        if (p == ROOK) black_bonus += 10;
                        else if (p == QUEEN) black_bonus += 7;
                    }
                }
            }
        }
        
        // Battery bonus
        for (int f = std::max(0, wk_file - 1); f <= std::min(7, wk_file + 1); f++) {
            bool has_rook = false, has_queen = false;
            for (int r = 0; r < 8; r++) {
                int sq = r * 8 + f;
                if (board.color_at(sq) == BLACK) {
                    if (board.piece_at(sq) == ROOK) has_rook = true;
                    else if (board.piece_at(sq) == QUEEN) has_queen = true;
                }
            }
            if (has_rook && has_queen) black_bonus += 6;
        }
        
        // Rook lift bonus
        black_bonus += rook_lift_bonus(board, BLACK, wk_file);
        
        // Diagonal alignment
        black_bonus += diagonal_alignment_bonus(board, BLACK, wk);
        
        black_bonus = std::min(black_bonus, 35);
        conversion -= black_bonus;
    }
    
    // Hard cap
    if (conversion > 35) conversion = 35;
    if (conversion < -35) conversion = -35;
    
    return conversion;
}

} // namespace Evaluation
