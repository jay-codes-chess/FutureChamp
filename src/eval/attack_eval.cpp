/**
 * Aggressive Attack Evaluation Implementation
 * King tropism, pawn storm, line opening, and aggressive initiative
 */

#include "attack_eval.hpp"
#include "king_safety.hpp"
#include "../utils/board.hpp"
#include <cmath>

namespace Evaluation {

// ============================================================================
// KING TROPISM
// ============================================================================

// Manhattan distance between two squares
static int manhattan_distance(int sq1, int sq2) {
    int file1 = sq1 % 8;
    int rank1 = sq1 / 8;
    int file2 = sq2 % 8;
    int rank2 = sq2 / 8;
    return std::abs(file1 - file2) + std::abs(rank1 - rank2);
}

// Get game phase (0 = opening, 100 = endgame)
static int get_game_phase(const Board& board) {
    int phase = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int p = board.piece_at(sq);
        switch (p) {
            case QUEEN:  phase += 4; break;
            case ROOK:   phase += 2; break;
            case BISHOP: 
            case KNIGHT: phase += 1; break;
            default: break;
        }
    }
    return phase;  // 0-24 range, normalize to 0-100
}

// Taper by game phase (1.0 in opening, 0.0 in endgame)
static float get_phase_taper(const Board& board) {
    int phase = get_game_phase(board);
    // 24 = all pieces, 0 = bare kings
    // phase >= 16 = opening/middlegame, phase < 8 = endgame
    if (phase >= 16) return 1.0f;
    if (phase <= 8) return 0.0f;
    return static_cast<float>(phase - 8) / 8.0f;
}

int evaluate_king_tropism(const Board& board) {
    int white_score = 0;
    int black_score = 0;
    
    // Find king positions
    int white_king_sq = -1;
    int black_king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_sq = sq;
            else black_king_sq = sq;
        }
    }
    
    if (white_king_sq == -1 || black_king_sq == -1) return 0;
    
    float taper = get_phase_taper(board);
    if (taper <= 0.0f) return 0;
    
    // Evaluate each piece's proximity to enemy king
    for (int sq = 0; sq < 64; ++sq) {
        int p = board.piece_at(sq);
        if (p == PAWN || p == KING) continue;  // Ignore pawns and kings
        
        int color = board.color_at(sq);
        int enemy_king_sq = (color == WHITE) ? black_king_sq : white_king_sq;
        
        int dist = manhattan_distance(sq, enemy_king_sq);
        
        int bonus = 0;
        if (p == QUEEN) {
            if (dist <= 3) bonus = 6;
            else if (dist == 4) bonus = 3;
        } else if (p == ROOK) {
            if (dist <= 3) bonus = 4;
            else if (dist == 4) bonus = 2;
        } else if (p == BISHOP) {
            if (dist <= 3) bonus = 5;
            else if (dist == 4) bonus = 2;
        } else if (p == KNIGHT) {
            if (dist <= 3) bonus = 5;
            else if (dist == 4) bonus = 2;
        }
        
        if (color == WHITE) white_score += bonus;
        else black_score += bonus;
    }
    
    // Apply phase taper and return difference
    int tropism = static_cast<int>((white_score - black_score) * taper);
    return tropism;
}

// ============================================================================
// OPPOSITE CASTLING DETECTION
// ============================================================================

// Determine which side a king is on (queenside = file <= 3, kingside = file >= 4)
static int get_king_side(int king_sq) {
    int file = king_sq % 8;
    return (file <= 3) ? 0 : 1;  // 0 = queenside, 1 = kingside
}

// Check if queens are still on the board
static bool queens_on_board(const Board& board) {
    bool white_queen = false;
    bool black_queen = false;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == QUEEN) {
            if (board.color_at(sq) == WHITE) white_queen = true;
            else black_queen = true;
        }
    }
    return white_queen && black_queen;
}

// Check if king is on back rank
static bool king_on_back_rank(int king_sq, int color) {
    int rank = king_sq / 8;
    int expected_rank = (color == WHITE) ? 0 : 7;
    return rank == expected_rank;
}

bool is_opposite_castling(const Board& board) {
    // Find king positions
    int white_king_sq = -1;
    int black_king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_sq = sq;
            else black_king_sq = sq;
        }
    }
    
    if (white_king_sq == -1 || black_king_sq == -1) return false;
    
    // Both kings must be on back rank
    if (!king_on_back_rank(white_king_sq, WHITE)) return false;
    if (!king_on_back_rank(black_king_sq, BLACK)) return false;
    
    // Queens must still be on board
    if (!queens_on_board(board)) return false;
    
    // Game must be in middlegame (phase >= 12 means some pieces exchanged)
    int phase = get_game_phase(board);
    if (phase < 12) return false;
    
    // Kings must be on opposite sides
    int white_side = get_king_side(white_king_sq);
    int black_side = get_king_side(black_king_sq);
    
    return white_side != black_side;
}

// ============================================================================
// PAWN STORM BONUS
// ============================================================================

// Get the file (0-7) from a square
static int get_file(int sq) {
    return sq % 8;
}

// Get the rank (0-7) from a square  
static int get_rank(int sq) {
    return sq / 8;
}

// Check if a pawn attacks a specific square
static bool pawn_attacks_square(const Board& board, int pawn_sq, int target_sq, int pawn_color) {
    int pawn_file = get_file(pawn_sq);
    int pawn_rank = get_rank(pawn_sq);
    int target_file = get_file(target_sq);
    int target_rank = get_rank(target_sq);
    
    int direction = (pawn_color == WHITE) ? 1 : -1;
    
    // White pawns attack forward-left and forward-right
    // Black pawns attack backward-left and backward-right
    return (target_file == pawn_file - 1 && target_rank == pawn_rank + direction && pawn_color == WHITE) ||
           (target_file == pawn_file + 1 && target_rank == pawn_rank + direction && pawn_color == WHITE) ||
           (target_file == pawn_file - 1 && target_rank == pawn_rank + direction && pawn_color == BLACK) ||
           (target_file == pawn_file + 1 && target_rank == pawn_rank + direction && pawn_color == BLACK);
}

int evaluate_pawn_storm(const Board& board) {
    if (!is_opposite_castling(board)) return 0;
    
    int white_score = 0;
    int black_score = 0;
    
    // Find king positions
    int white_king_sq = -1;
    int black_king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_sq = sq;
            else black_king_sq = sq;
        }
    }
    
    int white_king_file = get_file(white_king_sq);
    int black_king_file = get_file(black_king_sq);
    
    // Define pawn storm files based on king position
    // If enemy king kingside (file >= 4), storm g/h/f pawns
    // If enemy king queenside (file <= 3), storm a/b/c pawns
    
    // Evaluate white pawns
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) != PAWN || board.color_at(sq) != WHITE) continue;
        
        int file = get_file(sq);
        int rank = get_rank(sq);
        
        // Check if this pawn is storming toward black king
        if (black_king_file >= 4) {
            // Black king kingside - look at g/h/f files (files 5,6,7)
            if (file >= 5) {
                // Bonus based on rank advancement
                int advancement = rank;  // 0-7 for white
                int storm_bonus = advancement * 2;  // Higher = more advanced
                
                // Extra bonus if attacks king zone (squares around king)
                int king_zone_bonus = 0;
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        int zone_sq = black_king_sq + dx + dy * 8;
                        if (zone_sq >= 0 && zone_sq < 64) {
                            if (pawn_attacks_square(board, sq, zone_sq, WHITE)) {
                                king_zone_bonus = 5;
                                break;
                            }
                        }
                    }
                }
                white_score += storm_bonus + king_zone_bonus;
            }
        } else {
            // Black king queenside - look at a/b/c files (files 0,1,2)
            if (file <= 2) {
                int advancement = rank;
                int storm_bonus = advancement * 2;
                
                int king_zone_bonus = 0;
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        int zone_sq = black_king_sq + dx + dy * 8;
                        if (zone_sq >= 0 && zone_sq < 64) {
                            if (pawn_attacks_square(board, sq, zone_sq, WHITE)) {
                                king_zone_bonus = 5;
                                break;
                            }
                        }
                    }
                }
                white_score += storm_bonus + king_zone_bonus;
            }
        }
    }
    
    // Evaluate black pawns (mirror logic)
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) != PAWN || board.color_at(sq) != BLACK) continue;
        
        int file = get_file(sq);
        int rank = get_rank(sq);
        
        // Check if this pawn is storming toward white king
        if (white_king_file >= 4) {
            // White king kingside - storm g/h/f files
            if (file >= 5) {
                int advancement = 7 - rank;  // Count from black's perspective
                int storm_bonus = advancement * 2;
                
                int king_zone_bonus = 0;
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        int zone_sq = white_king_sq + dx + dy * 8;
                        if (zone_sq >= 0 && zone_sq < 64) {
                            if (pawn_attacks_square(board, sq, zone_sq, BLACK)) {
                                king_zone_bonus = 5;
                                break;
                            }
                        }
                    }
                }
                black_score += storm_bonus + king_zone_bonus;
            }
        } else {
            // White king queenside - storm a/b/c files
            if (file <= 2) {
                int advancement = 7 - rank;
                int storm_bonus = advancement * 2;
                
                int king_zone_bonus = 0;
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        int zone_sq = white_king_sq + dx + dy * 8;
                        if (zone_sq >= 0 && zone_sq < 64) {
                            if (pawn_attacks_square(board, sq, zone_sq, BLACK)) {
                                king_zone_bonus = 5;
                                break;
                            }
                        }
                    }
                }
                black_score += storm_bonus + king_zone_bonus;
            }
        }
    }
    
    // Cap at 40 and return difference
    white_score = std::min(white_score, 40);
    black_score = std::min(black_score, 40);
    
    return white_score - black_score;
}

// ============================================================================
// LINE OPENING BONUS
// ============================================================================

// Check if a file is open (no pawns)
static bool is_file_open(const Board& board, int file) {
    for (int rank = 0; rank < 8; ++rank) {
        int sq = rank * 8 + file;
        if (board.piece_at(sq) == PAWN) return false;
    }
    return true;
}

// Check if a file is semi-open for a color (no pawns of that color)
static bool is_file_semi_open(const Board& board, int file, int color) {
    int start_rank = (color == WHITE) ? 0 : 7;
    int end_rank = (color == WHITE) ? 8 : -1;
    int dir = (color == WHITE) ? 1 : -1;
    
    for (int rank = start_rank; rank != end_rank; rank += dir) {
        int sq = rank * 8 + file;
        if (board.piece_at(sq) == PAWN && board.color_at(sq) == color) return false;
    }
    return true;
}

int evaluate_line_opening(const Board& board) {
    if (!is_opposite_castling(board)) return 0;
    
    int white_score = 0;
    int black_score = 0;
    
    // Find king positions
    int white_king_sq = -1;
    int black_king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_sq = sq;
            else black_king_sq = sq;
        }
    }
    
    int white_king_file = get_file(white_king_sq);
    int black_king_file = get_file(black_king_sq);
    
    // Check files toward enemy king
    // White attacking black: check files around black king
    for (int file = std::max(0, black_king_file - 1); file <= std::min(7, black_king_file + 1); ++file) {
        bool open = is_file_open(board, file);
        bool semi_open_white = is_file_semi_open(board, file, WHITE);
        
        if (open) white_score += 10;
        else if (semi_open_white) white_score += 5;
        
        // Bonus if rook or queen on this file
        for (int rank = 0; rank < 8; ++rank) {
            int sq = rank * 8 + file;
            int p = board.piece_at(sq);
            if ((p == ROOK || p == QUEEN) && board.color_at(sq) == WHITE) {
                white_score += 8;
            }
        }
    }
    
    // Black attacking white
    for (int file = std::max(0, white_king_file - 1); file <= std::min(7, white_king_file + 1); ++file) {
        bool open = is_file_open(board, file);
        bool semi_open_black = is_file_semi_open(board, file, BLACK);
        
        if (open) black_score += 10;
        else if (semi_open_black) black_score += 5;
        
        // Bonus if rook or queen on this file
        for (int rank = 0; rank < 8; ++rank) {
            int sq = rank * 8 + file;
            int p = board.piece_at(sq);
            if ((p == ROOK || p == QUEEN) && board.color_at(sq) == BLACK) {
                black_score += 8;
            }
        }
    }
    
    // Bonus for broken pawn shield near enemy king
    // Check pawn shield files
    for (int file = std::max(0, black_king_file - 1); file <= std::min(7, black_king_file + 1); ++file) {
        // Check if white pawns are broken (gaps in pawn shield)
        int shield_rank = 1;  // White pawns on rank 1-2
        bool has_pawn = false;
        for (int r = 0; r < 3; ++r) {
            int sq = r * 8 + file;
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == WHITE) {
                has_pawn = true;
                break;
            }
        }
        if (!has_pawn) white_score += 3;  // Broken shield
    }
    
    for (int file = std::max(0, white_king_file - 1); file <= std::min(7, white_king_file + 1); ++file) {
        bool has_pawn = false;
        for (int r = 5; r < 8; ++r) {
            int sq = r * 8 + file;
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == BLACK) {
                has_pawn = true;
                break;
            }
        }
        if (!has_pawn) black_score += 3;
    }
    
    // Cap at 35
    white_score = std::min(white_score, 35);
    black_score = std::min(black_score, 35);
    
    return white_score - black_score;
}

// ============================================================================
// AGGRESSIVE INITIATIVE
// ============================================================================

// Count pieces attacking king zone
static int count_king_zone_attackers(const Board& board, int king_sq, int attacker_color) {
    int count = 0;
    
    // King zone = 3x3 area around king
    int king_file = get_file(king_sq);
    int king_rank = get_rank(king_sq);
    
    for (int df = -1; df <= 1; ++df) {
        for (int dr = -1; dr <= 1; ++dr) {
            int file = king_file + df;
            int rank = king_rank + dr;
            if (file < 0 || file > 7 || rank < 0 || rank > 7) continue;
            
            int sq = rank * 8 + file;
            int p = board.piece_at(sq);
            if (p == 0) continue;
            
            // Check if this piece attacks the square (simplified)
            int color = board.color_at(sq);
            if (color != attacker_color) continue;
            
            // Rough approximation - in real engine would use attack bitboards
            // For now, count any non-pawn, non-king piece in zone as "attacking"
            if (p != PAWN && p != KING) {
                count++;
            }
        }
    }
    
    return count;
}

// Check if pieces are developed (past starting rank)
static bool is_piece_developed(const Board& board, int color) {
    int start_rank = (color == WHITE) ? 0 : 7;
    int develop_rank = (color == WHITE) ? 2 : 5;
    
    int developed_count = 0;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != color) continue;
        int p = board.piece_at(sq);
        if (p == KNIGHT || p == BISHOP) {
            int rank = get_rank(sq);
            if ((color == WHITE && rank >= develop_rank) || 
                (color == BLACK && rank <= develop_rank)) {
                developed_count++;
            }
        }
    }
    return developed_count >= 2;
}

// Simple check if king is safe (based on pawn shield and position)
static bool is_king_safe(const Board& board, int color) {
    int king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
            king_sq = sq;
            break;
        }
    }
    if (king_sq == -1) return false;
    
    int file = get_file(king_sq);
    int rank = get_rank(king_sq);
    
    // Check for pawn shield
    int shield_rank = (color == WHITE) ? rank + 1 : rank - 1;
    if (shield_rank >= 0 && shield_rank < 8) {
        int pawns_near = 0;
        for (int df = -1; df <= 1; ++df) {
            int f = file + df;
            if (f < 0 || f > 7) continue;
            int sq = shield_rank * 8 + f;
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == color) {
                pawns_near++;
            }
        }
        // King is safe if has pawn shield
        if (pawns_near >= 2) return true;
    }
    
    // Also check if king is castled
    if (color == WHITE && (king_sq == 6 || king_sq == 2)) return true;
    if (color == BLACK && (king_sq == 62 || king_sq == 58)) return true;
    
    return false;
}

int evaluate_aggressive_initiative(const Board& board) {
    if (!is_opposite_castling(board)) return 0;
    
    int white_score = 0;
    int black_score = 0;
    
    // Find king positions
    int white_king_sq = -1;
    int black_king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_sq = sq;
            else black_king_sq = sq;
        }
    }
    
    // Check if own king is safe
    bool white_king_safe = is_king_safe(board, WHITE);
    bool black_king_safe = is_king_safe(board, BLACK);
    
    // White's aggressive initiative
    if (is_piece_developed(board, WHITE)) {
        int attackers = count_king_zone_attackers(board, black_king_sq, WHITE);
        if (attackers >= 2) {
            white_score += attackers * 5;  // Reward coordinated attack
        }
        
        // Bonus if there's pressure (use king tropism as signal)
        int tropism = evaluate_king_tropism(board);
        if (tropism > 10) {
            white_score += 5;
        }
    }
    
    // Black's aggressive initiative
    if (is_piece_developed(board, BLACK)) {
        int attackers = count_king_zone_attackers(board, white_king_sq, BLACK);
        if (attackers >= 2) {
            black_score += attackers * 5;
        }
        
        int tropism = evaluate_king_tropism(board);
        if (tropism < -10) {
            black_score += 5;
        }
    }
    
    // Penalty for pushing pawns while own king is unsafe
    if (!white_king_safe) {
        // Count advanced white pawns
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == WHITE) {
                int rank = get_rank(sq);
                if (rank >= 4) {  // Advanced pawn
                    white_score -= 3;
                }
            }
        }
    }
    
    if (!black_king_safe) {
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == BLACK) {
                int rank = get_rank(sq);
                if (rank <= 3) {
                    black_score -= 3;
                }
            }
        }
    }
    
    // Cap at 30
    white_score = std::min(white_score, 30);
    black_score = std::min(black_score, 30);
    
    return white_score - black_score;
}

// ============================================================================
// COMBINED ATTACK EVALUATION
// ============================================================================

int evaluate_attacks(const Board& board) {
    int score = 0;
    
    // All terms are already symmetric (white - black)
    score += evaluate_king_tropism(board);
    score += evaluate_pawn_storm(board);
    score += evaluate_line_opening(board);
    score += evaluate_aggressive_initiative(board);
    
    return score;
}

} // namespace Evaluation
