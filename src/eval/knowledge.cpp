/**
 * Knowledge Registry Implementation
 * Hard-coded strategic concepts from chess knowledge
 */

#include "knowledge.hpp"
#include "params.hpp"
#include "../utils/board.hpp"

// Simple square helpers (board.hpp doesn't have square namespace)
inline int sq_file(int sq) { return sq % 8; }
inline int sq_rank(int sq) { return sq / 8; }
inline bool sq_is_ok(int sq) { return sq >= 0 && sq < 64; }
inline bool sq_is_dark(int sq) { return (sq_file(sq) + sq_rank(sq)) % 2 == 1; }

namespace Evaluation {

// Helper: Is square attacked by enemy pawns?
static bool is_pawn_attacked(const Board& board, int sq, int by_color) {
    int enemy = by_color == WHITE ? BLACK : WHITE;
    
    // Check pawn attacks
    int direction = by_color == WHITE ? -1 : 1;
    int left = (sq_file(sq) - 1) + 8 * (sq_rank(sq) + direction);
    int right = (sq_file(sq) + 1) + 8 * (sq_rank(sq) + direction);
    
    if (sq_is_ok(left) && board.piece_at(left) == PAWN && board.color_at(left) == enemy) return true;
    if (sq_is_ok(right) && board.piece_at(right) == PAWN && board.color_at(right) == enemy) return true;
    
    return false;
}

// Bad Bishop: Bishop blocked by own pawns on same color squares
int eval_bad_bishop(const Board& board, const Params& params) {
    int score = 0;
    
    for (int color = WHITE; color <= BLACK; ++color) {
        int sign = color == WHITE ? 1 : -1;
        
        // Find bishops
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) != BISHOP || board.color_at(sq) != color) continue;
            
            // Count own pawns blocking this bishop
            int blockers = 0;
            int color_of_bishop = sq_is_dark(sq) ? 1 : 0;
            
            for (int psq = 0; psq < 64; ++psq) {
                if (board.piece_at(psq) != PAWN || board.color_at(psq) != color) continue;
                if ((sq_is_dark(psq) ? 1 : 0) == color_of_bishop) {
                    blockers++;
                }
            }
            
            if (blockers >= 3) {
                score -= sign * 35;  // -35 cp for severely blocked
            } else if (blockers >= 2) {
                score -= sign * 20;
            } else if (blockers >= 1) {
                score -= sign * 10;
            }
        }
    }
    
    return score * params.concept_bad_bishop_weight / 100;
}

// Good Knight vs Bad Bishop: Closed pawn structures favor knights
int eval_knight_vs_bad_bishop(const Board& board, const Params& params) {
    int score = 0;
    
    // Count knights and bishops for each side
    int white_knights = 0, black_knights = 0;
    int white_bishops = 0, black_bishops = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        int color = board.color_at(sq);
        int piece = board.piece_at(sq);
        
        if (color == WHITE) {
            if (piece == KNIGHT) white_knights++;
            else if (piece == BISHOP) white_bishops++;
        } else if (color == BLACK) {
            if (piece == KNIGHT) black_knights++;
            else if (piece == BISHOP) black_bishops++;
        }
    }
    
    // If white has knight + black has bishop
    if (white_knights > 0 && black_bishops > 0) {
        int black_bad_bishops = 0;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) != BISHOP || board.color_at(sq) != BLACK) continue;
            int color_of_bishop = sq_is_dark(sq) ? 1 : 0;
            int blockers = 0;
            for (int psq = 0; psq < 64; ++psq) {
                if (board.piece_at(psq) != PAWN || board.color_at(psq) != BLACK) continue;
                if ((sq_is_dark(psq) ? 1 : 0) == color_of_bishop) blockers++;
            }
            if (blockers >= 2) black_bad_bishops++;
        }
        
        if (black_bad_bishops > 0) {
            score += 25 * black_bad_bishops;
        }
    }
    
    // If black has knight + white has bishop
    if (black_knights > 0 && white_bishops > 0) {
        int white_bad_bishops = 0;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) != BISHOP || board.color_at(sq) != WHITE) continue;
            int color_of_bishop = sq_is_dark(sq) ? 1 : 0;
            int blockers = 0;
            for (int psq = 0; psq < 64; ++psq) {
                if (board.piece_at(psq) != PAWN || board.color_at(psq) != WHITE) continue;
                if ((sq_is_dark(psq) ? 1 : 0) == color_of_bishop) blockers++;
            }
            if (blockers >= 2) white_bad_bishops++;
        }
        
        if (white_bad_bishops > 0) {
            score -= 25 * white_bad_bishops;
        }
    }
    
    return score;
}

// Rook on 7th rank
int eval_rook_on_7th(const Board& board, const Params& params) {
    int score = 0;
    
    // White rooks on rank 6 or 7 (if black's pawns there)
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) != ROOK) continue;
        
        int rank = sq_rank(sq);
        int color = board.color_at(sq);
        
        if (color == WHITE && rank >= 5) {
            bool enemy_pawns_near = false;
            for (int esq = 0; esq < 64; ++esq) {
                if (board.piece_at(esq) != PAWN || board.color_at(esq) != BLACK) continue;
                if (sq_rank(esq) >= 5) {
                    enemy_pawns_near = true;
                    break;
                }
            }
            if (enemy_pawns_near) score += 20;
        }
        
        if (color == BLACK && rank <= 2) {
            bool enemy_pawns_near = false;
            for (int esq = 0; esq < 64; ++esq) {
                if (board.piece_at(esq) != PAWN || board.color_at(esq) != WHITE) continue;
                if (sq_rank(esq) <= 2) {
                    enemy_pawns_near = true;
                    break;
                }
            }
            if (enemy_pawns_near) score -= 20;
        }
    }
    
    return score;
}

// Space advantage: Count squares controlled in enemy half
int eval_space_advantage(const Board& board, const Params& params) {
    int white_space = 0, black_space = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        int rank = sq_rank(sq);
        int color = board.color_at(sq);
        int piece = board.piece_at(sq);
        
        if (color == WHITE && piece != NO_PIECE && piece != PAWN) {
            if (rank >= 4) white_space++;  // White controls enemy (black) half
        }
        if (color == BLACK && piece != NO_PIECE && piece != PAWN) {
            if (rank <= 3) black_space++;  // Black controls enemy (white) half
        }
    }
    
    int diff = white_space - black_space;
    int score = 0;
    
    if (diff > 0) score = std::min(diff * 5, 40);   // +5 to +40 cp
    else score = std::max(diff * 5, -40);         // -5 to -40 cp
    
    return score * params.concept_space_weight / 100;
}

// Knight Outpost: Knight on square not attackable by enemy pawn, supported by own pawn
int eval_knight_outpost(const Board& board, const Params& params) {
    int score = 0;
    
    for (int color = WHITE; color <= BLACK; ++color) {
        int sign = color == WHITE ? 1 : -1;
        
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) != KNIGHT || board.color_at(sq) != color) continue;
            
            // Not attackable by enemy pawn
            if (is_pawn_attacked(board, sq, color)) continue;
            
            // Supported by friendly pawn
            int direction = color == WHITE ? 1 : -1;
            int support_rank = sq_rank(sq) + direction;
            
            if (support_rank < 0 || support_rank > 7) continue;
            
            bool supported = false;
            for (int f = -1; f <= 1; ++f) {
                int file = sq_file(sq) + f;
                if (file < 0 || file > 7) continue;
                int pawn_sq = file + 8 * support_rank;
                if (sq_is_ok(pawn_sq) && board.piece_at(pawn_sq) == PAWN && board.color_at(pawn_sq) == color) {
                    supported = true;
                    break;
                }
            }
            
            if (supported) {
                // Bonus based on rank (closer to enemy king = better)
                int rank = sq_rank(sq);
                int bonus = color == WHITE ? rank : (7 - rank);
                score += sign * (15 + bonus * 5);  // +15 to +40 cp
            }
        }
    }
    
    return score * params.concept_outpost_weight / 100;
}

// Exchange sacrifice compensation - when down material but have positional compensation
int eval_exchange_sac_compensation(const Board& board, const Params& params) {
    if (params.concept_exchange_sac_weight == 0) return 0;
    
    int score = 0;
    
    // Count material for each side (R=5, B/N=3, P=1)
    int white_mat = 0, black_mat = 0;
    int white_minors = 0, black_minors = 0;
    int white_rooks = 0, black_rooks = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        int piece = board.piece_at(sq);
        int color = board.color_at(sq);
        
        if (color == WHITE) {
            if (piece == PAWN) white_mat += 1;
            else if (piece == KNIGHT || piece == BISHOP) { white_mat += 3; white_minors++; }
            else if (piece == ROOK) { white_mat += 5; white_rooks++; }
            else if (piece == QUEEN) white_mat += 9;
        } else if (color == BLACK) {
            if (piece == PAWN) black_mat += 1;
            else if (piece == KNIGHT || piece == BISHOP) { black_mat += 3; black_minors++; }
            else if (piece == ROOK) { black_mat += 5; black_rooks++; }
            else if (piece == QUEEN) black_mat += 9;
        }
    }
    
    // White is down exchange (R for minor)
    if (white_rooks > 0 && black_minors > white_minors && 
        (black_mat - white_mat) >= 2 && (black_mat - white_mat) <= 4) {
        
        int compensation = 0;
        
        // Check for outpost/strong squares
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == KNIGHT && board.color_at(sq) == WHITE) {
                int rank = sq_rank(sq);
                if (rank >= 4 && !is_pawn_attacked(board, sq, WHITE)) {
                    compensation += 20;
                }
            }
        }
        
        // Bishop pair bonus
        if (white_minors >= 2) compensation += 15;
        
        // Space/central control
        int white_space = 0;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.color_at(sq) == WHITE && board.piece_at(sq) != NO_PIECE && board.piece_at(sq) != PAWN) {
                if (sq_rank(sq) >= 4) white_space++;
            }
        }
        if (white_space > 8) compensation += 20;
        
        score += compensation;
    }
    
    // Black is down exchange
    if (black_rooks > 0 && white_minors > black_minors && 
        (white_mat - black_mat) >= 2 && (white_mat - black_mat) <= 4) {
        
        int compensation = 0;
        
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == KNIGHT && board.color_at(sq) == BLACK) {
                int rank = sq_rank(sq);
                if (rank <= 3 && !is_pawn_attacked(board, sq, BLACK)) {
                    compensation += 20;
                }
            }
        }
        
        if (black_minors >= 2) compensation += 15;
        
        int black_space = 0;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.color_at(sq) == BLACK && board.piece_at(sq) != NO_PIECE && board.piece_at(sq) != PAWN) {
                if (sq_rank(sq) <= 3) black_space++;
            }
        }
        if (black_space > 8) compensation += 20;
        
        score -= compensation;
    }
    
    return score * params.concept_exchange_sac_weight / 100;
}

// Weak color complex - multiple weak squares of same color
int eval_weak_color_complex(const Board& board, const Params& params) {
    if (params.concept_color_complex_weight == 0) return 0;
    
    int score = 0;
    
    // Count weak squares near kings
    for (int color = WHITE; color <= BLACK; ++color) {
        int sign = color == WHITE ? 1 : -1;
        
        // Find king position
        int king_sq = -1;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
                king_sq = sq;
                break;
            }
        }
        if (king_sq < 0) continue;
        
        // Check adjacent squares for weakness (no pawn protection, not our color)
        bool is_dark_king = sq_is_dark(king_sq);
        int weak_dark = 0, weak_light = 0;
        
        // Check 8 squares around king
        int king_file = sq_file(king_sq);
        int king_rank = sq_rank(king_sq);
        
        for (int df = -1; df <= 1; ++df) {
            for (int dr = -1; dr <= 1; ++dr) {
                if (df == 0 && dr == 0) continue;
                int f = king_file + df;
                int r = king_rank + dr;
                if (f < 0 || f > 7 || r < 0 || r > 7) continue;
                
                int sq = f + 8 * r;
                bool sq_dark = sq_is_dark(sq);
                
                // If no pawn protection and opposite color to king, it's weak
                bool has_pawn_protection = false;
                int direction = color == WHITE ? 1 : -1;
                int check_sq = f + 8 * (r + direction);
                if (sq_is_ok(check_sq) && board.piece_at(check_sq) == PAWN && board.color_at(check_sq) == color) {
                    has_pawn_protection = true;
                }
                
                if (!has_pawn_protection) {
                    if (sq_dark != is_dark_king) weak_light++;
                    else weak_dark++;
                }
            }
        }
        
        // If multiple weak squares of same color, apply penalty
        if (weak_dark >= 3 || weak_light >= 3) {
            score -= sign * 30;
        } else if (weak_dark >= 2 || weak_light >= 2) {
            score -= sign * 15;
        }
    }
    
    return score * params.concept_color_complex_weight / 100;
}

// Pawn lever timing - recognizing lever opportunities
int eval_pawn_lever_timing(const Board& board, const Params& params) {
    if (params.concept_pawn_lever_weight == 0) return 0;
    
    int score = 0;
    
    // Check for lever opportunities (pawn can attack enemy chain base)
    for (int color = WHITE; color <= BLACK; ++color) {
        int sign = color == WHITE ? 1 : -1;
        int direction = color == WHITE ? 1 : -1;
        
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) != PAWN || board.color_at(sq) != color) continue;
            
            int file = sq_file(sq);
            int rank = sq_rank(sq);
            
            // Check if can capture left or right (lever)
            int left_file = file - 1;
            int right_file = file + 1;
            int target_rank = rank + direction;
            
            if (left_file >= 0 && target_rank >= 0 && target_rank <= 7) {
                int target_sq = left_file + 8 * target_rank;
                int target_piece = board.piece_at(target_sq);
                int target_color = board.color_at(target_sq);
                int enemy = color == WHITE ? BLACK : WHITE;
                
                // If target is enemy pawn, this is a lever
                if (target_piece == PAWN && target_color == enemy) {
                    score += sign * 10;  // Small bonus for having lever opportunity
                }
            }
            
            if (right_file <= 7 && target_rank >= 0 && target_rank <= 7) {
                int target_sq = right_file + 8 * target_rank;
                int target_piece = board.piece_at(target_sq);
                int target_color = board.color_at(target_sq);
                int enemy = color == WHITE ? BLACK : WHITE;
                
                if (target_piece == PAWN && target_color == enemy) {
                    score += sign * 10;
                }
            }
        }
    }
    
    return score * params.concept_pawn_lever_weight / 100;
}

// Initiative persistence - tempo and development advantage
int eval_initiative_persistence(const Board& board, const Params& params) {
    if (params.concept_initiative_persist_weight == 0) return 0;
    
    int score = 0;
    
    // Count developed pieces (not on starting squares)
    int white_developed = 0, black_developed = 0;
    
    // Knights: normally on g1/b1 (white) or g8/b8 (black)
    int white_knight_squares[2] = {6, 1};  // g1, b1
    int black_knight_squares[2] = {62, 57}; // g8, b8
    
    for (int sq = 0; sq < 64; ++sq) {
        int piece = board.piece_at(sq);
        int color = board.color_at(sq);
        
        if (color == WHITE) {
            if (piece == KNIGHT && sq != 1 && sq != 6) white_developed++;
            if (piece == BISHOP && sq != 2 && sq != 5) white_developed++;
        } else if (color == BLACK) {
            if (piece == KNIGHT && sq != 57 && sq != 62) black_developed++;
            if (piece == BISHOP && sq != 58 && sq != 61) black_developed++;
        }
    }
    
    // Development lead gives initiative
    // Score positive = white has initiative, negative = black has initiative
    int dev_diff = white_developed - black_developed;
    if (dev_diff > 0) {
        // White has more development - positive score for white
        score += dev_diff * 15;
    } else if (dev_diff < 0) {
        // Black has more development - negative score from white's perspective
        score += dev_diff * 15;  // dev_diff is already negative
    }
    
    // King in center (not castled) - temporary initiative
    // Score positive = white initiative, negative = black initiative
    int white_king_file = -1, black_king_file = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING) {
            if (board.color_at(sq) == WHITE) white_king_file = sq_file(sq);
            else black_king_file = sq_file(sq);
        }
    }
    
    // King on e1/d1 or e8/d8 (not castled) gives initiative
    if (white_king_file >= 2 && white_king_file <= 5 && board.color_at(4) == WHITE) {
        score += 10;  // White king in center - white has initiative
    }
    if (black_king_file >= 2 && black_king_file <= 5 && board.color_at(60) == BLACK) {
        score -= 10;  // Black king in center - black has initiative (negative from white POV)
    }
    
    return score * params.concept_initiative_persist_weight / 100;
}

// Main knowledge evaluation - combines all concepts
int evaluate_knowledge(const Board& board, const Params& params) {
    if (params.w_knowledge_concepts == 0) return 0;
    
    int score = 0;
    score += eval_knight_outpost(board, params);
    score += eval_bad_bishop(board, params);
    score += eval_knight_vs_bad_bishop(board, params);
    score += eval_rook_on_7th(board, params);
    score += eval_space_advantage(board, params);
    
    // Master concepts
    score += eval_exchange_sac_compensation(board, params);
    score += eval_weak_color_complex(board, params);
    score += eval_pawn_lever_timing(board, params);
    score += eval_initiative_persistence(board, params);
    
    // Apply global knowledge weight
    score = score * params.w_knowledge_concepts / 100;
    
    return score;
}

} // namespace Evaluation
