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
    
    return score * params.bad_bishop_penalty / 100;
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
    
    return score * params.space_concept_weight / 100;
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
    
    return score * params.outpost_bonus / 100;
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
    
    // Apply global knowledge weight
    score = score * params.w_knowledge_concepts / 100;
    
    return score;
}

} // namespace Evaluation
