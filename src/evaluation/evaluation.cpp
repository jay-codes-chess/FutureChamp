/**
 * Human Chess Engine - Evaluation Function
 * Based on Silman's Imbalance Theory + bitboard Board from your project
 */

#include "evaluation.hpp"
#include "../utils/board.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Evaluation {

// ──────────────────────────────────────────────
//   Static data
// ──────────────────────────────────────────────

static StyleWeights current_weights;
static std::string current_style = "classical";

// --- Piece-Square Tables (PST) ---

// reduce knight center bonuses:
static const int KNIGHT_PST[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,   // Reduced center bonus
    -30,   0,  10,  15,  15,  10,   0, -30,   // Reduced center bonus  
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static const int BISHOP_PST[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static const int ROOK_PST[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      0,   0,   5,  10,  10,   5,   0,   0
};

static const int QUEEN_PST[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static const int PAWN_PST[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     50,  50,  50,  50,  50,  50,  50,  50,
     15,  15,  25,  40,  40,  25,  15,  15,   // Increased center values
     10,  10,  15,  35,  35,  15,  10,  10,   // Increased center values
      5,   5,  10,  25,  25,  10,   5,   5,   // Reduced penalties
      0,   0,   0,  15,  15,   0,   0,   0,   // Reduced penalties
      0,   0,   0,   0,   0,   0,   0,   0,   // Reduced back rank penalties
      0,   0,   0,   0,   0,   0,   0,   0
};


// Enhance KING_PST to strongly encourage castling squares:
static const int KING_PST[64] = {
    // Rank 0 (a1-h1) - White's back rank
     20,  30,  10,   0, -30, -50,  30,  20,  // g1 (castled) gets big bonus
    // Rank 1
    -30, -30, -30, -30, -30, -30, -30, -30,
    // Rank 2
    -50, -50, -50, -50, -50, -50, -50, -50,  // Strong penalty for moving forward
    // Rank 3
    -70, -70, -70, -70, -70, -70, -70, -70,
    // Rank 4
    -90, -90, -90, -90, -90, -90, -90, -90,
    // Rank 5
    -90, -90, -90, -90, -90, -90, -90, -90,
    // Rank 6
    -90, -90, -90, -90, -90, -90, -90, -90,
    // Rank 7 (a8-h8) - Black's back rank
    -90, -90, -90, -90, -90, -90, -90, -90
};


// ──────────────────────────────────────────────
//   Helpers
// ──────────────────────────────────────────────

int mirror_square(int sq) { 
    // Mirror only the RANK (vertical flip), keep FILE the same
    // e8 (sq 60) -> e1 (sq 4)
    int file = sq % 8;
    int rank = sq / 8;
    return (7 - rank) * 8 + file;
}

int get_pst_value(int piece_type, int square, int color) {
    int idx = (color == WHITE) ? square : mirror_square(square);
    switch (piece_type) {
        case PAWN:   return PAWN_PST[idx];
        case KNIGHT: return KNIGHT_PST[idx];
        case BISHOP: return BISHOP_PST[idx];
        case ROOK:   return ROOK_PST[idx];
        case QUEEN:  return QUEEN_PST[idx];
        case KING:   return KING_PST[idx];
        default:     return 0;
    }
}

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

bool is_opening(const Board& board) {
    return (count_material(board, WHITE) + count_material(board, BLACK)) > 4000;
}



// ──────────────────────────────────────────────
//   Evaluation terms (improved from evaluation2.cpp)
// ──────────────────────────────────────────────


int evaluate_piece_activity(const Board& board, int color) {
    int score = 0;
	
	 // Center control bonus
    uint64_t attacks = Bitboards::all_attacks(board, color);
    int center_squares[] = {27, 28, 35, 36};  // d4, e4, d5, e5
    for (int sq : center_squares) {
        if (Bitboards::test(attacks, sq)) {
            score += 3;  // Small bonus for attacking center
        }
    }
	
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != color) continue;
        int type = board.piece_at(sq);
        if (type == NO_PIECE || type == KING) continue;

        score += get_pst_value(type, sq, color);

        // **REDUCE KNIGHT BONUS IN OPENING**
        if (type == KNIGHT || type == BISHOP) {
            int rank = sq / 8;
            if (color == WHITE && rank >= 2) score += 8;  // Reduced from 12
            if (color == BLACK && rank <= 5) score += 8;  // Reduced from 12
        }

        // Center control bonus (but not for king - already handled in king_safety)
        int file = sq % 8, rank = sq / 8;
        int dist = std::abs(file - 3) + std::abs(rank - 3);
        if (dist <= 2) score += 5;  // Reduced from 8
    }
    return score;
}


int evaluate_pawn_structure(const Board& board, int color) {
    int score = 0;
	
	  // **HUGE BONUS for center pawns in opening - especially e4/d4**
    if (board.fullmove_number <= 10) {  // Early game
        if (color == WHITE) {
            if (board.piece_at(28) == PAWN && board.color_at(28) == WHITE) score += 150; // e4 - BIG BONUS
            if (board.piece_at(27) == PAWN && board.color_at(27) == WHITE) score += 140; // d4 - BIG BONUS
            if (board.piece_at(29) == PAWN && board.color_at(29) == WHITE) score += 80;  // f4
            if (board.piece_at(26) == PAWN && board.color_at(26) == WHITE) score += 80;  // c4
            
            // Bonus for pawns controlling center
            if (board.piece_at(19) == PAWN && board.color_at(19) == WHITE) score += 30;  // e3
            if (board.piece_at(18) == PAWN && board.color_at(18) == WHITE) score += 30;  // d3
        } else {
            if (board.piece_at(36) == PAWN && board.color_at(36) == BLACK) score += 150; // e5
            if (board.piece_at(35) == PAWN && board.color_at(35) == BLACK) score += 140; // d5
            if (board.piece_at(37) == PAWN && board.color_at(37) == BLACK) score += 80;  // f5
            if (board.piece_at(34) == PAWN && board.color_at(34) == BLACK) score += 80;  // c5
            
            if (board.piece_at(43) == PAWN && board.color_at(43) == BLACK) score += 30;  // e6
            if (board.piece_at(42) == PAWN && board.color_at(42) == BLACK) score += 30;  // d6
        }
    }
	
	

    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) != PAWN || board.color_at(sq) != color) continue;

        int file = sq % 8;
        int rank = sq / 8;
		
		
		 // **NEW: Bonus for controlling center with pawns**
        if (file >= 2 && file <= 5) {  // c,d,e,f files
            int forward_rank = (color == WHITE) ? rank + 1 : rank - 1;
            if (forward_rank >= 0 && forward_rank < 8) {
                int forward_sq = file + forward_rank * 8;
                // Bonus for pawns that can move to center
                if (forward_sq >= 27 && forward_sq <= 36) {  // Center squares
                    score += 15;
                }
            }
        }
		

        // Passed pawn (simplified but better than before)
        bool passed = true;
        int dir = (color == WHITE) ? 8 : -8;
        for (int s = sq + dir; s >= 0 && s < 64; s += dir) {
            if (board.piece_at(s) == PAWN && board.color_at(s) != color) {
                passed = false;
                break;
            }
        }
        if (passed) score += 40 + (color == WHITE ? rank * 5 : (7 - rank) * 5);

        // Isolated pawn
        bool isolated = true;
        if (file > 0) {
            for (int r = 0; r < 8; ++r)
                if (board.piece_at(r*8 + file-1) == PAWN && board.color_at(r*8 + file-1) == color)
                    isolated = false;
        }
        if (file < 7) {
            for (int r = 0; r < 8; ++r)
                if (board.piece_at(r*8 + file+1) == PAWN && board.color_at(r*8 + file+1) == color)
                    isolated = false;
        }
        if (isolated) score -= 20;

        // Doubled pawns
        int count_on_file = 0;
        for (int r = 0; r < 8; ++r)
            if (board.piece_at(r*8 + file) == PAWN && board.color_at(r*8 + file) == color)
                count_on_file++;
        if (count_on_file >= 2) score -= 12 * (count_on_file - 1);
    }
    return score;
}


int evaluate_king_safety(const Board& board, int color) {
    int king_sq = -1;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
            king_sq = sq; break;
        }
    }
    if (king_sq == -1) return -20000;

    int rank = Bitboards::rank_of(king_sq);
    int score = get_pst_value(KING, king_sq, color);
	
    
    // **BIG BONUS for having castling rights in opening**
    if (board.fullmove_number <= 15) {
        if (board.castling[color][0] || board.castling[color][1]) {
            score += 60;  // Big bonus for still having castling rights
        }
        
        // **EVEN BIGGER BONUS for already castled king**
        if (color == WHITE) {
            if (king_sq == 6) score += 120;  // Kingside castled (g1)
            if (king_sq == 2) score += 110;  // Queenside castled (c1)
        } else {
            if (king_sq == 62) score += 120; // Kingside castled (g8)
            if (king_sq == 58) score += 110; // Queenside castled (c8)
        }
    }
	
	
	

    // FIXED: Correct shield direction (in front of the king)
    int shield_rank = (color == WHITE) ? rank + 1 : rank - 1;
    if (shield_rank >= 0 && shield_rank < 8) {
        int file = king_sq % 8;
        for (int df = -1; df <= 1; ++df) {
            int f = file + df;
            if (f < 0 || f > 7) continue;
            int sq = shield_rank * 8 + f;
            if (board.piece_at(sq) == PAWN && board.color_at(sq) == color) {
                score += 18;           // slightly increased
            }
        }
    }

    // STRONG PENALTY: King leaving back rank in opening
    if (is_opening(board)) {
        int back_rank = (color == WHITE) ? 0 : 7;
        if (rank != back_rank) {
            score -= 200;   // Strong penalty - king should stay on back rank
        }
        
        // Bonus for still having castling rights
        if (board.castling[color][0] || board.castling[color][1]) {
            score += 30;
        }
    }

    return score;
}


float evaluate_space(const Board& board, int color) {
    float space = 0.0f;
    
    // Count squares controlled in center and enemy territory
    for (int sq = 0; sq < 64; ++sq) {
        int rank = sq / 8;
        int file = sq % 8;
        
        // For white: ranks 4-7 are important
        // For black: ranks 0-3 are important
        if (color == WHITE && rank >= 4) {
            // Check if square is attacked by our pawns/minor pieces
            uint64_t attacks = Bitboards::all_attacks(board, color);
            if (Bitboards::test(attacks, sq)) {
                space += 0.5f;
                
                // Extra bonus for controlling central squares
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



// In evaluation.cpp - evaluate_development() function

int evaluate_development(const Board& board, int color) {
    if (!is_opening(board)) return 0;

    int score = 0;
    int home_rank = (color == WHITE) ? 0 : 7;
    
    // **1. Small penalty for pieces still on home rank**
    for (int f = 0; f < 8; ++f) {
        int sq = home_rank * 8 + f;
        int p = board.piece_at(sq);
        if (p == KNIGHT || p == BISHOP)
            score -= 5;  // Very small penalty
        if (p == ROOK || p == QUEEN)
            score -= 3;  // Even smaller for major pieces
    }
    
    // **2. Small bonus for knights/bishops that have moved to good squares**
    for (int sq = 0; sq < 64; ++sq) {
        if (board.color_at(sq) != color) continue;
        int p = board.piece_at(sq);
        
        if (p == KNIGHT || p == BISHOP) {
            int rank = sq / 8;
            int file = sq % 8;
            
            // Bonus for being in center/developing squares
            if (color == WHITE && rank >= 2 && rank <= 4 && file >= 2 && file <= 5) {
                score += 5;
            } else if (color == BLACK && rank >= 3 && rank <= 5 && file >= 2 && file <= 5) {
                score += 5;
            }
        }
    }
    
    // **3. CASTLING READINESS - Encourage keeping castling options open**
    if (board.fullmove_number <= 12) {  // Early game focus
        // **A. Bonus for still having castling rights**
        if (board.castling[color][0] || board.castling[color][1]) {
            score += 40;  // Good bonus for having options
            
            // Extra bonus if BOTH castling rights still available
            if (board.castling[color][0] && board.castling[color][1]) {
                score += 20;
            }
        }
        
        // **B. Penalty for moving king before castling**
        int king_sq = -1;
        for (int sq = 0; sq < 64; ++sq) {
            if (board.piece_at(sq) == KING && board.color_at(sq) == color) {
                king_sq = sq;
                break;
            }
        }
        
        int king_home_sq = home_rank * 8 + 4;  // e1/e8
        if (king_sq != king_home_sq && board.castling[color][0] && board.castling[color][1]) {
            // King moved but still has both castling rights? That shouldn't happen, but penalize anyway
            score -= 60;
        } else if (king_sq != king_home_sq) {
            // King has moved from starting square
            score -= 30;
        }
        
        // **C. Penalty for moving rooks that have castling rights**
        if (board.castling[color][1]) {  // Queenside castling still possible
            int queenside_rook_sq = home_rank * 8;  // a1/a8
            if (board.piece_at(queenside_rook_sq) != ROOK || board.color_at(queenside_rook_sq) != color) {
                score -= 25;  // Rook moved from queenside
            }
        }
        
        if (board.castling[color][0]) {  // Kingside castling still possible
            int kingside_rook_sq = home_rank * 8 + 7;  // h1/h8
            if (board.piece_at(kingside_rook_sq) != ROOK || board.color_at(kingside_rook_sq) != color) {
                score -= 25;  // Rook moved from kingside
            }
        }
        
        // **D. Bonus for preparing castling by moving pieces out of the way**
        if (color == WHITE && board.castling[WHITE][0]) {  // White kingside castling possible
            // Bonus if f1, g1 are empty (path for castling)
            if (board.is_empty(5) && board.is_empty(6)) score += 15;
            // Bonus if bishop/queen moved from f1/d1
            if (board.piece_at(5) != BISHOP || board.color_at(5) != WHITE) score += 10;
            if (board.piece_at(3) != QUEEN || board.color_at(3) != WHITE) score += 5;
        }
        
        if (color == WHITE && board.castling[WHITE][1]) {  // White queenside castling possible
            // Bonus if b1, c1, d1 are empty
            if (board.is_empty(1) && board.is_empty(2) && board.is_empty(3)) score += 15;
            // Bonus if queen/bishop moved from d1/c1
            if (board.piece_at(3) != QUEEN || board.color_at(3) != WHITE) score += 10;
            if (board.piece_at(2) != BISHOP || board.color_at(2) != WHITE) score += 5;
        }
        
        // Black side equivalents
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
        
        // **E. BIG bonus for actually castled king position**
        if (color == WHITE) {
            if (king_sq == 6) score += 80;   // Kingside castled (g1)
            if (king_sq == 2) score += 70;   // Queenside castled (c1)
        } else {
            if (king_sq == 62) score += 80;  // Kingside castled (g8)
            if (king_sq == 58) score += 70;  // Queenside castled (c8)
        }
    }
    
    // **4. Discourage premature queen development in opening**
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
            int queen_home_rank = home_rank;
            
            if (queen_rank != queen_home_rank) {
                // Queen has left home rank - small penalty unless it's doing something useful
                int queen_file = queen_sq % 8;
                if (queen_file < 2 || queen_file > 5) {  // Queen on edge files
                    score -= 20;
                } else if (queen_file < 3 || queen_file > 4) {  // Queen on b/g files
                    score -= 10;
                }
                // Queen on c/d/e/f files gets no penalty (central)
            }
        }
    }
    
    return score;
}


void evaluate_minor_pieces(const Board& board, Imbalances& imb) {
    int w_kn = 0, w_bi = 0, b_kn = 0, b_bi = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int type = board.piece_at(sq);
        if (type == KNIGHT) (board.color_at(sq) == WHITE ? w_kn : b_kn)++;
        if (type == BISHOP) (board.color_at(sq) == WHITE ? w_bi : b_bi)++;
    }
    int w_score = w_kn * 32 + w_bi * 33;
    int b_score = b_kn * 32 + b_bi * 33;

    imb.white_has_better_minor = (w_score > b_score + 40);
    imb.black_has_better_minor = (b_score > w_score + 40);
}

// ──────────────────────────────────────────────
//   Public API
// ──────────────────────────────────────────────

// Efficient version that takes a Board directly (for search)

int evaluate(const Board& board) {
    // DEBUG: Print board
    
	#ifdef DEBUG
	std::cout << "EVAL DEBUG: Position: " << board.get_fen() << std::endl;
	#endif
    
    int mat   = count_material(board, WHITE) - count_material(board, BLACK);
    int act   = evaluate_piece_activity(board, WHITE) - evaluate_piece_activity(board, BLACK);
    int pawn  = evaluate_pawn_structure(board, WHITE) - evaluate_pawn_structure(board, BLACK);
    int space = static_cast<int>((evaluate_space(board, WHITE) - evaluate_space(board, BLACK)) * 12);
    int king  = evaluate_king_safety(board, WHITE) - evaluate_king_safety(board, BLACK);
    int dev   = evaluate_development(board, WHITE) - evaluate_development(board, BLACK);

    // DEBUG: Print each component
    #ifdef DEBUG
	std::cout << "EVAL DEBUG: mat=" << mat << " act=" << act << " pawn=" << pawn 
              << " space=" << space << " king=" << king << " dev=" << dev << std::endl;
    #endif
    const auto& w = current_weights;

    int score = 0;
    score += static_cast<int>(mat   * w.material);
    score += static_cast<int>(act   * w.piece_activity);
    score += static_cast<int>(pawn  * w.pawn_structure);
    score += static_cast<int>(space * w.space);
    score += static_cast<int>(king  * w.king_safety);
    score += static_cast<int>(dev   * w.development);
	
	
    // Tempo bonus
    if (board.side_to_move == WHITE) score += 10;
    else                             score -= 10;
    
    #ifdef DEBUG
	std::cout << "EVAL DEBUG: final score=" << score << std::endl;
	#endif
    
    return score;
}


// FEN string version (for UCI commands)
int evaluate(const std::string& fen) {
    Board board;
    if (!fen.empty()) board.set_from_fen(fen);
    return evaluate(board);  // Call the efficient version
}

Imbalances analyze_imbalances(const std::string& fen) {
    Board board;
    if (!fen.empty()) board.set_from_fen(fen);

    Imbalances imb{};
    imb.material_diff     = count_material(board, WHITE) - count_material(board, BLACK);
    imb.white_king_safety = evaluate_king_safety(board, WHITE);
    imb.black_king_safety = evaluate_king_safety(board, BLACK);
    imb.white_space       = evaluate_space(board, WHITE);
    imb.black_space       = evaluate_space(board, BLACK);

    evaluate_minor_pieces(board, imb);

    int wp = evaluate_pawn_structure(board, WHITE);
    int bp = evaluate_pawn_structure(board, BLACK);
    imb.white_weak_pawns = (wp < -40) ? 1 : 0;
    imb.black_weak_pawns = (bp < -40) ? 1 : 0;

    imb.white_has_passed_pawn = (imb.white_space > imb.black_space + 8);
    imb.black_has_passed_pawn = (imb.black_space > imb.white_space + 8);

    if (is_opening(board)) {
        imb.white_development_score = evaluate_development(board, WHITE);
        imb.black_development_score = evaluate_development(board, BLACK);
    }

    return imb;
}

VerbalExplanation explain(int score, const std::string& fen) {
    VerbalExplanation exp;
    Imbalances imb = analyze_imbalances(fen);

    if (imb.material_diff > 120)
        exp.move_reasons.emplace_back("White has a clear material advantage");
    else if (imb.material_diff < -120)
        exp.move_reasons.emplace_back("Black has a clear material advantage");

    if (imb.white_has_better_minor)
        exp.imbalance_notes.emplace_back("White has the superior minor pieces");
    if (imb.black_has_better_minor)
        exp.imbalance_notes.emplace_back("Black has the superior minor pieces");

    if (imb.white_king_safety > imb.black_king_safety + 40)
        exp.imbalance_notes.emplace_back("White's king is significantly safer");
    else if (imb.black_king_safety > imb.white_king_safety + 40)
        exp.imbalance_notes.emplace_back("Black's king is significantly safer");

    // Development only in opening
    Board temp;
    if (!fen.empty() && temp.set_from_fen(fen) && is_opening(temp)) {
        if (imb.white_development_score > imb.black_development_score + 30)
            exp.imbalance_notes.emplace_back("White is ahead in development");
        if (imb.black_development_score > imb.white_development_score + 30)
            exp.imbalance_notes.emplace_back("Black is ahead in development");
    }

    if (score > 40)  exp.move_reasons.emplace_back("White has the better position overall");
    if (score < -40) exp.move_reasons.emplace_back("Black has the better position overall");

    return exp;
}

void initialize() { set_style("classical"); }

void set_style(const std::string& style_name) {
    current_style = style_name;
    
	if (style_name == "classical") {
   current_weights = {
       1.0f,   // material - keep high
       0.3f,   // piece_activity - REDUCED from 0.5f
       0.8f,   // pawn_structure - INCREASED from 0.6f
       0.1f,   // space - keep low
       0.4f,   // initiative
       1.0f,   // king_safety
       0.2f,   // development - REDUCED from 0.3f
       0.4f    // prophylaxis
	};
	
    } else if (style_name == "attacking") {
        current_weights = {0.8f, 0.8f, 0.4f, 0.4f, 1.0f, 0.3f, 0.2f, 0.2f};
    } else if (style_name == "positional") {
        current_weights = {1.0f, 0.6f, 0.8f, 0.6f, 0.3f, 0.5f, 0.4f, 0.6f};
    } else {
        current_weights = {1.0f, 0.5f, 0.5f, 0.3f, 0.4f, 0.6f, 0.3f, 0.4f};
    }
}

std::string get_style_name() { return current_style; }

} // namespace Evaluation