/**
 * Initiative Evaluation Layer Implementation
 */

#include "initiative.hpp"
#include "material.hpp"
#include "pawn_structure.hpp"

namespace Evaluation {

int evaluate_development(const Board& board, int color) {
    if (!is_opening(board)) return 0;
    
    int score = 0;
    int home_rank = (color == WHITE) ? 0 : 7;
    
    // Penalty for pieces on home rank
    for (int f = 0; f < 8; ++f) {
        int sq = home_rank * 8 + f;
        int p = board.piece_at(sq);
        if (p == KNIGHT || p == BISHOP) score -= 5;
        if (p == ROOK || p == QUEEN) score -= 3;
    }
    
    // Bonus for developed pieces
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != color) continue;
        int p = board.piece_at(sq);
        
        if (p == KNIGHT || p == BISHOP) {
            int rank = sq / 8;
            int file = sq % 8;
            
            if (color == WHITE && rank >= 2 && rank <= 4 && file >= 2 && file <= 5) {
                score += 5;
            } else if (color == BLACK && rank >= 3 && rank <= 5 && file >= 2 && file <= 5) {
                score += 5;
            }
        }
    }
    
    // Castling readiness
    if (board.fullmove_number <= 12) {
        if (board.castling[color][0] || board.castling[color][1]) {
            score += 40;
            if (board.castling[color][0] && board.castling[color][1]) {
                score += 20;
            }
        }
        
        // King moved penalty
        int king_sq = -1;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
                king_sq = sq;
                break;
            }
        }
        
        int king_home_sq = home_rank * 8 + 4;
        if (king_sq != king_home_sq) {
            score -= (king_sq == -1) ? 0 : 30;
        }
        
        // Rook moved from castling square
        if (board.castling[color][1]) {
            int queenside_rook_sq = home_rank * 8;
            if (board.piece_at(queenside_rook_sq) != ROOK || board.color_at(queenside_rook_sq) != color) {
                score -= 25;
            }
        }
        
        if (board.castling[color][0]) {
            int kingside_rook_sq = home_rank * 8 + 7;
            if (board.piece_at(kingside_rook_sq) != ROOK || board.color_at(kingside_rook_sq) != color) {
                score -= 25;
            }
        }
        
        // Castling path clear
        if (color == WHITE && board.castling[WHITE][0]) {
            if (board.is_empty(5) && board.is_empty(6)) score += 15;
            if (board.piece_at(5) != BISHOP || board.color_at(5) != WHITE) score += 10;
            if (board.piece_at(3) != QUEEN || board.color_at(3) != WHITE) score += 5;
        }
        
        if (color == WHITE && board.castling[WHITE][1]) {
            if (board.is_empty(1) && board.is_empty(2) && board.is_empty(3)) score += 15;
            if (board.piece_at(3) != QUEEN || board.color_at(3) != WHITE) score += 10;
            if (board.piece_at(2) != BISHOP || board.color_at(2) != WHITE) score += 5;
        }
        
        if (color == BLACK && board.castling[BLACK][0]) {
            if (board.is_empty(61) && board.is_empty(62)) score += 15;
            if (board.piece_at(61) != BISHOP || board.color_at(61) != BLACK) score += 10;
            if (board.piece_at(59) != QUEEN || board.color_at(59) != BLACK) score += 5;
        }
        
        if (color == BLACK && board.castling[BLACK][1]) {
            if (board.is_empty(57) && board.is_empty(58) && board.is_empty(59)) score += 15;
            if (board.piece_at(59) != QUEEN || board.color_at(59) != BLACK) score += 10;
            if (board.piece_at(58) != BISHOP || board.color_at(58) != BLACK) score += 5;
        }
        
        // Castled king bonus
        if (color == WHITE) {
            if (king_sq == 6) score += 80;
            if (king_sq == 2) score += 70;
        } else {
            if (king_sq == 62) score += 80;
            if (king_sq == 58) score += 70;
        }
    }
    
    // Premature queen development
    if (board.fullmove_number <= 8) {
        int queen_sq = -1;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == QUEEN && board.color_at(sq) == color) {
                queen_sq = sq;
                break;
            }
        }
        
        if (queen_sq != -1) {
            int queen_rank = queen_sq / 8;
            if (queen_rank != home_rank) {
                int queen_file = queen_sq % 8;
                if (queen_file < 2 || queen_file > 5) score -= 20;
                else if (queen_file < 3 || queen_file > 4) score -= 10;
            }
        }
    }
    
    return score;
}

int evaluate_initiative(const Board& board) {
    int score = 0;
    
    score += evaluate_development(board, WHITE);
    score -= evaluate_development(board, BLACK);
    
    // Tempo bonus
    if (board.side_to_move == WHITE) score += 10;
    else score -= 10;
    
    return score;
}

} // namespace Evaluation
