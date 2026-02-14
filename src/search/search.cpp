/**
 * Search Module - Alpha-Beta with Human-Like Selectivity
 * 
 * ENHANCEMENTS:
 * - Fixed illegal move bug with better TT validation
 * - Added SEE (Static Exchange Evaluation)
 * - Added null move pruning
 * - Improved checkmate detection with proper mate scoring
 * - Enhanced killer move heuristic
 * - Better quiescence search
 */

#include "search.hpp"
#include "human_selection.hpp"
#include "../utils/board.hpp"
#include "../eval/evaluation.hpp"
#include "../eval/params.hpp"
#include "../uci/uci.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>
#include <limits>
#include <unordered_map>
#include <string>

namespace Search {

// Search diagnostics
SearchDiagnostics g_diag;

// Search state
static bool stop_search = false;
static int search_depth = 10;
static int max_depth = 20;
static long nodes_searched = 0;
static int best_move_found = 0;
static int search_score = 0;

// Mate scores
constexpr int MATE_SCORE = 30000;
constexpr int MATE_BOUND = MATE_SCORE - 1000;

// Position history for repetition detection
static std::vector<uint64_t> position_history;

// Time management
static std::chrono::steady_clock::time_point start_time;
static int max_time_ms = 30000;
static int optimal_time_ms = 3000;
static int panic_time_ms = 5000;


// Add this helper function near the top of search.cpp
int mirror_square(int sq) {
    int file = sq % 8;
    int rank = sq / 8;
    return (7 - rank) * 8 + file;
}


bool is_opening(const Board& board) {
    // Simple check: if both sides have more than a certain amount of material
    int white_mat = 0, black_mat = 0;
    for (int sq = 0; sq < 64; sq++) {
        int piece = board.piece_at(sq);
        if (piece == NO_PIECE) continue;
        
        int value = 0;
        switch(piece) {
            case PAWN: value = 100; break;
            case KNIGHT: value = 320; break;
            case BISHOP: value = 330; break;
            case ROOK: value = 500; break;
            case QUEEN: value = 900; break;
            default: value = 0;
        }
        
        if (board.color_at(sq) == WHITE) {
            white_mat += value;
        } else {
            black_mat += value;
        }
    }
    
    // Opening: both sides have most pieces
    return (white_mat + black_mat) > 4000;
}


// Transposition table
struct TTEntry {
    uint64_t hash;
    int depth;
    int score;
    int move;
    uint8_t flag;  // 0=empty, 1=alpha, 2=beta, 3=exact
};

static const int TT_SIZE = 1 << 20;  // 1M entries
static TTEntry* transposition_table;

// Killer moves - moves that caused cutoffs at sibling nodes
static int killer_moves[64][2];  // [depth][2 killer moves]

// History heuristic - scores moves based on success
static int history_scores[64][64];  // [from][to] history score

// Update history score
void update_history(int move, int depth, int bonus) {
    int from = Bitboards::move_from(move);
    int to = Bitboards::move_to(move);
    history_scores[from][to] += bonus;
    
    // Cap history scores to prevent overflow
    if (history_scores[from][to] > 10000) {
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                history_scores[i][j] = std::max(0, history_scores[i][j] / 2);
            }
        }
    }
}

// Initialize search
void initialize() {
    transposition_table = new TTEntry[TT_SIZE];
    for (int i = 0; i < TT_SIZE; i++) {
        transposition_table[i].hash = 0;
        transposition_table[i].depth = 0;
        transposition_table[i].score = 0;
        transposition_table[i].move = 0;
        transposition_table[i].flag = 0;
    }
    // Initialize history and killer tables
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 2; j++) {
            killer_moves[i][j] = 0;
        }
        for (int j = 0; j < 64; j++) {
            history_scores[i][j] = 0;
        }
    }
}

// Get TT index
int tt_index(uint64_t hash) {
    return hash & (TT_SIZE - 1);
}

// Store in transposition table
void tt_store(uint64_t hash, int depth, int score, int move, int flag) {
    g_diag.ttStores++;
    int idx = tt_index(hash);
    
    // Count collision if overwriting different key
    if (transposition_table[idx].hash != 0 && transposition_table[idx].hash != hash) {
        g_diag.ttCollisions++;
    }
    
    // Simple replacement strategy - always replace (could be improved)
    transposition_table[idx].hash = hash;
    transposition_table[idx].depth = depth;
    transposition_table[idx].score = score;
    transposition_table[idx].move = move;
    transposition_table[idx].flag = flag;
}

// Probe transposition table
bool tt_probe(uint64_t hash, int depth, int& score, int& move) {
    int idx = tt_index(hash);
    g_diag.ttProbes++;
    
    // Count collision if entry exists but key doesn't match
    if (transposition_table[idx].hash != 0 && transposition_table[idx].hash != hash) {
        g_diag.ttCollisions++;
    }
    
    // CRITICAL: Verify hash matches exactly to avoid hash collisions
    if (transposition_table[idx].hash == hash && 
        transposition_table[idx].depth >= depth &&
        transposition_table[idx].flag != 0) {  // Ensure entry is valid
        score = transposition_table[idx].score;
        move = transposition_table[idx].move;
        g_diag.ttHits++;
        return true;
    }
    return false;
}

// Get current time
int get_elapsed_ms() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
}

// Check if we should stop
bool should_stop() {
    if (stop_search) return true;
    
    int elapsed = get_elapsed_ms();
    
    // Time pressure checks
    if (elapsed > max_time_ms) return true;
    if (elapsed > panic_time_ms && search_depth > 3) return true;
    
    return false;
}

// Check for draw by 50-move rule
bool is_fifty_move_draw(const Board& board) {
    return board.halfmove_clock >= 100;  // 50 moves = 100 half-moves
}

// Check for draw by repetition
bool is_repetition_draw(const Board& board) {
    // Count how many times this position has occurred
    int count = 0;
    for (uint64_t hash : position_history) {
        if (hash == board.hash) {
            count++;
            if (count >= 2) return true;  // 3-fold (current + 2 previous)
        }
    }
    return false;
}

// Check for insufficient material
bool is_insufficient_material(const Board& board) {
    // Count pieces
    int white_pawns = Bitboards::popcount(board.pieces[PAWN] & board.colors[WHITE]);
    int black_pawns = Bitboards::popcount(board.pieces[PAWN] & board.colors[BLACK]);
    int white_knights = Bitboards::popcount(board.pieces[KNIGHT] & board.colors[WHITE]);
    int black_knights = Bitboards::popcount(board.pieces[KNIGHT] & board.colors[BLACK]);
    int white_bishops = Bitboards::popcount(board.pieces[BISHOP] & board.colors[WHITE]);
    int black_bishops = Bitboards::popcount(board.pieces[BISHOP] & board.colors[BLACK]);
    int white_rooks = Bitboards::popcount(board.pieces[ROOK] & board.colors[WHITE]);
    int black_rooks = Bitboards::popcount(board.pieces[ROOK] & board.colors[BLACK]);
    int white_queens = Bitboards::popcount(board.pieces[QUEEN] & board.colors[WHITE]);
    int black_queens = Bitboards::popcount(board.pieces[QUEEN] & board.colors[BLACK]);
    
    // Any pawns, rooks, or queens = not insufficient material
    if (white_pawns + black_pawns + white_rooks + black_rooks + white_queens + black_queens > 0) {
        return false;
    }
    
    int white_minor = white_knights + white_bishops;
    int black_minor = black_knights + black_bishops;
    
    // K vs K
    if (white_minor == 0 && black_minor == 0) return true;
    
    // K+N vs K or K+B vs K
    if ((white_minor == 1 && black_minor == 0) || (white_minor == 0 && black_minor == 1)) {
        return true;
    }
    
    // K+B vs K+B (same color bishops)
    if (white_bishops == 1 && black_bishops == 1 && white_knights == 0 && black_knights == 0) {
        // Check if bishops are on same color squares
        uint64_t white_bishop_bb = board.pieces[BISHOP] & board.colors[WHITE];
        uint64_t black_bishop_bb = board.pieces[BISHOP] & board.colors[BLACK];
        int white_bishop_sq = Bitboards::lsb(white_bishop_bb);
        int black_bishop_sq = Bitboards::lsb(black_bishop_bb);
        
        int white_color = Bitboards::color_of(white_bishop_sq);
        int black_color = Bitboards::color_of(black_bishop_sq);
        
        if (white_color == black_color) return true;
    }
    
    return false;
}

// Evaluate position (using our Silman-based evaluation)
int evaluate_position(const Board& board, int color) {
    // Check for draws first
    if (is_fifty_move_draw(board)) return 0;
    if (is_repetition_draw(board)) return 0;
    if (is_insufficient_material(board)) return 0;
    
    int score = Evaluation::evaluate(board);
    return (color == WHITE) ? score : -score;
}

// Make move on board
Board make_move(const Board& board, int move) {
    Board new_board = board;
    int from = Bitboards::move_from(move);
    int to = Bitboards::move_to(move);
    int flags = Bitboards::move_flags(move);
    int promo = Bitboards::move_promotion(move);
    
    int piece = board.piece_at(from);
    int color = board.color_at(from);
    int captured = board.piece_at(to);
    
    // Increment halfmove clock (reset later if pawn move or capture)
    new_board.halfmove_clock++;
    
    // Handle castling
    if (flags == MOVE_CASTLE) {
        // Move king
        new_board.remove_piece(from);
        new_board.add_piece(to, KING, color);
        
        // Move rook
        if (to > from) {
            // Kingside
            int rook_from = from + 3;
            int rook_to = from + 1;
            new_board.remove_piece(rook_from);
            new_board.add_piece(rook_to, ROOK, color);
        } else {
            // Queenside
            int rook_from = from - 4;
            int rook_to = from - 1;
            new_board.remove_piece(rook_from);
            new_board.add_piece(rook_to, ROOK, color);
        }
        
        // Remove castling rights
        new_board.castling[color][0] = false;
        new_board.castling[color][1] = false;
        
        // Reset halfmove (castling resets it)
        new_board.halfmove_clock = 0;
    }
    // Handle en passant
    else if (flags == MOVE_EN_PASSANT) {
        new_board.remove_piece(from);
        new_board.add_piece(to, PAWN, color);
        
        // Remove captured pawn
        int captured_sq = to + (color == WHITE ? -8 : 8);
        new_board.remove_piece(captured_sq);
        
        new_board.en_passant_square = -1;
        
        // Capture - reset halfmove clock
        new_board.halfmove_clock = 0;
    }
    // Handle promotion
    else if (flags == MOVE_PROMOTION) {
        new_board.remove_piece(from);
        
        // Remove captured piece if any
        if (captured != NO_PIECE) {
            new_board.remove_piece(to);
        }
        
        // Promotion pieces: 0=Knight, 1=Bishop, 2=Rook, 3=Queen
        int promo_piece = KNIGHT;
        switch (promo) {
            case 0: promo_piece = KNIGHT; break;
            case 1: promo_piece = BISHOP; break;
            case 2: promo_piece = ROOK; break;
            case 3: promo_piece = QUEEN; break;
        }
        
        new_board.add_piece(to, promo_piece, color);
        new_board.en_passant_square = -1;
        
        // Pawn move or capture - reset halfmove clock
        new_board.halfmove_clock = 0;
    }
    // Normal move
    else {
        // Move piece
        new_board.remove_piece(from);
        
        // Remove captured piece if any (must do this BEFORE add_piece)
        if (captured != NO_PIECE && captured != KING) {
            new_board.remove_piece(to);
            // Capture - reset halfmove clock
            new_board.halfmove_clock = 0;
        }
        
        new_board.add_piece(to, piece, color);
        
        // Handle pawn double push for en passant
        if (piece == PAWN) {
            // Pawn move - reset halfmove clock
            new_board.halfmove_clock = 0;
            
            if (std::abs(to - from) == 16) {
                new_board.en_passant_square = (from + to) / 2;
            } else {
                new_board.en_passant_square = -1;
            }
        } else {
            new_board.en_passant_square = -1;
        }
        
        // Update castling rights for king moves
        if (piece == KING) {
            new_board.castling[color][0] = false;
            new_board.castling[color][1] = false;
        }
        
        // Update castling rights for rook moves
        if (piece == ROOK) {
            if (color == WHITE) {
                if (from == 0) new_board.castling[WHITE][1] = false;
                if (from == 7) new_board.castling[WHITE][0] = false;
            } else {
                if (from == 56) new_board.castling[BLACK][1] = false;
                if (from == 63) new_board.castling[BLACK][0] = false;
            }
        }
        
        // Update castling rights if rook is captured
        if (captured == ROOK) {
            if (to == 0) new_board.castling[WHITE][1] = false;
            if (to == 7) new_board.castling[WHITE][0] = false;
            if (to == 56) new_board.castling[BLACK][1] = false;
            if (to == 63) new_board.castling[BLACK][0] = false;
        }
    }
    
    // Switch side to move
    new_board.side_to_move = 1 - color;
    
    // Update fullmove number (increment after Black's move)
    if (color == BLACK) {
        new_board.fullmove_number++;
    }
    
    new_board.compute_hash();
    
    return new_board;
}

// Generate pseudo-legal moves
std::vector<int> generate_moves(const Board& board) {
	
    return board.generate_moves();
}

// Check if move is legal (king not in check after move)
bool is_legal(const Board& board, int move) {
    // Make the move on a copy
    Board test_board = make_move(board, move);
    
    // Check if OUR king is in check after the move
    int our_color = board.side_to_move;
    
    // Find our king in the new position
    int king_sq = -1;
    for (int sq = 0; sq < 64; sq++) {
        if (test_board.piece_at(sq) == KING && test_board.color_at(sq) == our_color) {
            king_sq = sq;
            break;
        }
    }
    
    if (king_sq == -1) {
        // No king? Should never happen
        return false;
    }
    
    // Check if king is attacked
    return !Bitboards::is_square_attacked(test_board, king_sq, 1 - our_color);
}


// Get piece value for ordering
int get_piece_value(int piece) {
    switch(piece) {
        case PAWN: return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK: return 500;
        case QUEEN: return 900;
        default: return 0;
    }
}

// **NEW**: Static Exchange Evaluation - evaluate material exchanges on a square
// Returns estimated material gain/loss from a capture sequence

// In search.cpp, replace the entire see() function with this simple version:
int see(const Board& board, int move) {
    int from = Bitboards::move_from(move);
    int to = Bitboards::move_to(move);
    
    int attacker = board.piece_at(from);
    int victim = board.piece_at(to);
    
    if (victim == NO_PIECE) {
        // Not a capture
        if (Bitboards::is_promotion(move)) {
            int promo = Bitboards::move_promotion(move);
            int promo_values[] = {320, 330, 500, 900};  // N, B, R, Q
            return promo_values[promo] - 100;  // Replace pawn with promoted piece
        }
        return 0;
    }
    
    // Simple material exchange
    int attacker_value = get_piece_value(attacker);
    int victim_value = get_piece_value(victim);
    
    // Basic rule: capturing equal or lower value is OK
    // Capturing higher value is good
    int gain = victim_value - attacker_value;
    
    // If we're losing material (negative gain), it's usually bad
    // But there might be exceptions (checks, forks, etc.)
    return gain;
}



// Score move for ordering (used in order_moves)
int score_move_for_order(Board& board, int move, int tt_move, int depth) {
    // TT move is best
    if (tt_move != 0 && move == tt_move) return 1000000;
    
    int score = 0;
    int from = Bitboards::move_from(move);
    int to = Bitboards::move_to(move);
    int piece = board.piece_at(from);
    int captured = board.piece_at(to);
	int flags = Bitboards::move_flags(move);
    
    // **MASSIVE BONUS for e4/d4 in first few moves**
    if (board.fullmove_number <= 3) {
        if (piece == PAWN) {
            int target = (board.side_to_move == WHITE) ? to : mirror_square(to);
            // e4/d4 for white, e5/d5 for black
            if ((board.side_to_move == WHITE && (target == 28 || target == 27)) ||
                (board.side_to_move == BLACK && (target == 36 || target == 35))) {
                return 200000;  // HUGE bonus - should be picked first
            }
        }
    }
    
    // **BIG BONUS for castling**
    if (flags == MOVE_CASTLE) {
        if (board.fullmove_number <= 10) {
            return 150000;  // Big bonus for castling early
        }
        return 100000;      // Still good bonus later
    }
    
    // **BONUS for moves that preserve castling rights**
    if (board.fullmove_number <= 10) {
        if (board.castling[board.side_to_move][0] || board.castling[board.side_to_move][1]) {
            // Bonus for not moving king or rooks
            if (piece == KING) score -= 50000;  // Penalty for moving king before castling
            if (piece == ROOK) {
                // Check if it's a rook that has castling rights
                if (board.side_to_move == WHITE) {
                    if (from == 0 && board.castling[WHITE][1]) score -= 30000;  // a1 rook
                    if (from == 7 && board.castling[WHITE][0]) score -= 30000;  // h1 rook
                } else {
                    if (from == 56 && board.castling[BLACK][1]) score -= 30000; // a8 rook
                    if (from == 63 && board.castling[BLACK][0]) score -= 30000; // h8 rook
                }
            }
        }
    
	
	
	
	 if (piece == KNIGHT) {
        // Small bonus for developing knights
        score += 2000;  // Reduced from larger bonus
    }
	
	
}
    
    // 1. Captures - use simple MVV-LVA
    if (captured != NO_PIECE) {
        score = 90000;
        score += get_piece_value(captured) * 10;
        score -= get_piece_value(piece);
        return score;
    }
    
    // 2. Promotions
    if (Bitboards::is_promotion(move)) {
        score = 95000;
        return score;
    }
    
    // 3. Killer moves
    if (depth >= 0 && depth < 64) {
        if (move == killer_moves[depth][0]) return 85000;
        if (move == killer_moves[depth][1]) return 84000;
    }
    
    // 4. Castling
    if (Bitboards::is_castle(move)) {
        return 80000;
    }
    
    // 5. Simple history
    score = history_scores[from][to];
    
    // 6. Bonus for moving toward center in opening
    if (board.fullmove_number <= 20) {
        int to_file = Bitboards::file_of(to);
        int to_rank = Bitboards::rank_of(to);
        int center_dist = std::abs(to_file - 3) + std::abs(to_rank - 3);
        if (center_dist <= 3) {
            score += (3 - center_dist) * 10;
        }
        
        
		
		if (piece == KING) {
    // King moves - usually bad unless castling or forced
    if (captured == NO_PIECE) {
        return -100000;  // Strong penalty for king moves
    }
}

// Add penalty for moving king into center in opening
if (board.fullmove_number <= 10) {
    if (piece == KING) {
        int to_rank = Bitboards::rank_of(to);
        int back_rank = (board.side_to_move == WHITE) ? 0 : 7;
        if (to_rank != back_rank) {
            return -50000;  // Huge penalty for king leaving back rank
        }
    }
}
		
		
		
		
		
		// Extra bonus for developing knights/bishops
        if (piece == KNIGHT || piece == BISHOP) {
            int from_rank = Bitboards::rank_of(from);
            if ((board.side_to_move == WHITE && from_rank == 0) ||
                (board.side_to_move == BLACK && from_rank == 7)) {
                score += 30; // Bonus for moving off back rank
            }
            
            // Specific bonus for knights to good squares
            if (piece == KNIGHT) {
                if ((board.side_to_move == WHITE && (to == 16 || to == 18 || to == 21 || to == 23)) ||
                    (board.side_to_move == BLACK && (to == 40 || to == 42 || to == 45 || to == 47))) {
                    score += 20; // Extra bonus for natural knight squares
                }
            }
        }
    }
    
    return score;
}


// Order moves for better alpha-beta performance

void order_moves(std::vector<int>& moves, Board& board, int tt_move, int depth) {
    if (moves.empty()) return;
    
    // Score all moves
    std::vector<std::pair<int, int>> scored_moves;  // (score, move)
    for (int move : moves) {
        int score = score_move_for_order(board, move, tt_move, depth);
        scored_moves.push_back({score, move});
    }
    
    // Sort by score (descending)
    std::sort(scored_moves.begin(), scored_moves.end(),
              [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                  return a.first > b.first;
              });
    
    // Update moves vector
    moves.clear();
    for (const auto& sm : scored_moves) {
        moves.push_back(sm.second);
    }
}


// Generate candidate moves (Kotov method)
std::vector<int> generate_candidates(const Board& board) {
    auto all_moves = generate_moves(board);
    std::vector<int> legal_moves;
    
    #ifdef DEBUG
	std::cout << "DEBUG: Position FEN: " << board.get_fen() << std::endl;
    std::cout << "DEBUG: All pseudo-legal moves: " << all_moves.size() << std::endl;
	#endif
    
    for (int move : all_moves) {
        if (is_legal(board, move)) {
            legal_moves.push_back(move);
            #ifdef DEBUG
			std::cout << "DEBUG: Legal move: " << Bitboards::move_to_uci(move) << std::endl;
			#endif
        }
    }
    
    #ifdef DEBUG
	std::cout << "DEBUG: Total legal moves: " << legal_moves.size() << std::endl;
	#endif
    return legal_moves;
}


// **QSEARCH v2**: Enhanced quiescence search with in-check evasions + capture filtering + delta pruning
int quiescence_search(Board& board, int alpha, int beta, int color) {
    nodes_searched++;
    g_diag.qnodes++;
    
    // Check for stop
    if (should_stop()) {
        return evaluate_position(board, color);
    }
    
    // Check if in check - MUST generate evasions
    bool in_check = board.is_in_check(color);
    
    // Stand-pat evaluation - only valid when NOT in check
    int stand_pat = evaluate_position(board, color);
    
    if (!in_check) {
        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) {
            alpha = stand_pat;
            g_diag.alphaImproves++;
        }
        
        // Delta pruning: if we're so far behind that even capturing a queen won't help
        const int BIG_DELTA = 975;  // Queen value + margin
        if (stand_pat < alpha - BIG_DELTA) {
            g_diag.qDeltaPruned++;
            return alpha;
        }
    }
    
    // Generate moves based on whether we're in check
    std::vector<int> moves;
    
    if (in_check) {
        // Generate ALL legal moves (evasions)
        auto all_moves = generate_moves(board);
        for (int move : all_moves) {
            if (is_legal(board, move)) {
                moves.push_back(move);
                g_diag.qEvasions++;
            }
        }
    } else {
        // Generate only tactical moves: captures and promotions
        auto all_moves = generate_moves(board);
        
        for (int move : all_moves) {
            if (!is_legal(board, move)) continue;
            
            int to = Bitboards::move_to(move);
            int captured = board.piece_at(to);
            
            // Only include captures and promotions
            if (captured != NO_PIECE || Bitboards::is_promotion(move)) {
                // **SEE filter**: skip obviously bad captures (losing more than a pawn)
                int see_score = see(board, move);
                if (see_score < -100) {
                    g_diag.qCapturesSkippedSEE++;
                    continue;  // Skip bad captures
                }
                moves.push_back(move);
            }
        }
    }
    
    // If no moves (shouldn't happen in legal chess), return eval
    if (moves.empty()) {
        return stand_pat;
    }
    
    // Order moves: promotions first, then by MVV-LVA + SEE
    std::sort(moves.begin(), moves.end(), [&](int a, int b) {
        int from_a = Bitboards::move_from(a);
        int to_a = Bitboards::move_to(a);
        int from_b = Bitboards::move_from(b);
        int to_b = Bitboards::move_to(b);
        
        int piece_a = board.piece_at(from_a);
        int captured_a = board.piece_at(to_a);
        int piece_b = board.piece_at(from_b);
        int captured_b = board.piece_at(to_b);
        
        // Promotions get highest bonus
        bool prom_a = Bitboards::is_promotion(a);
        bool prom_b = Bitboards::is_promotion(b);
        if (prom_a && !prom_b) return true;
        if (!prom_a && prom_b) return false;
        
        // MVV-LVA score
        int score_a = get_piece_value(captured_a) * 10 - get_piece_value(piece_a);
        int score_b = get_piece_value(captured_b) * 10 - get_piece_value(piece_b);
        
        // Add SEE scores
        score_a += see(board, a);
        score_b += see(board, b);
        
        return score_a > score_b;
    });
    
    // Search moves
    for (int move : moves) {
        if (should_stop()) {
            return alpha;
        }
        
        if (!in_check) {
            g_diag.qCapturesSearched++;
        }
        
        Board new_board = make_move(board, move);
        int score = -quiescence_search(new_board, -beta, -alpha, 1 - color);
        
        if (score > alpha) {
            alpha = score;
            if (alpha >= beta) {
                return beta;
            }
        }
    }
    
    return alpha;
}

// **ENHANCED**: Alpha-beta search with null move pruning and better mate detection
int alpha_beta(Board& board, int depth, int alpha, int beta, int color, bool allow_null = true) {
    nodes_searched++;
    g_diag.nodes++;
    
    // Check for time
    if (should_stop()) {
        return 0;
    }
    
    // **IMPROVED**: Mate distance pruning
    // Don't search for mates that are further than current depth
    int mate_alpha = std::max(alpha, -MATE_SCORE + (max_depth - depth));
    int mate_beta = std::min(beta, MATE_SCORE - (max_depth - depth));
    if (mate_alpha >= mate_beta) {
        return mate_alpha;
    }
    
    // Check for draws
    if (is_fifty_move_draw(board)) return 0;
    if (is_repetition_draw(board)) return 0;
    if (is_insufficient_material(board)) return 0;
    
    // Check transposition table
    int tt_score, tt_move = 0;
    bool tt_hit = tt_probe(board.hash, depth, tt_score, tt_move);
    
    // **FIXED**: Validate TT move against ALL legal moves, not just candidates
    if (tt_hit && tt_move != 0) {
        bool tt_move_is_legal = false;
        auto all_legal_moves = generate_moves(board);
        for (int legal_move : all_legal_moves) {
            if (is_legal(board, legal_move) && legal_move == tt_move) {
                tt_move_is_legal = true;
                break;
            }
        }
        // If TT move is not legal for this position, discard it
        if (!tt_move_is_legal) {
            tt_move = 0;
            tt_hit = false;  // Don't use score either
        }
    }
    
    // Use TT score if valid
    if (tt_hit && depth > 0) {
        if (tt_score >= beta) {
            g_diag.betaCutoffs++;
            return beta;
        }
        if (tt_score <= alpha) return alpha;
    }
    
    // Terminal position or max depth - use quiescence search
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, color);
    }
    
    bool in_check = board.is_in_check(color);
    
    // **NEW**: Null Move Pruning - if we can give opponent a free move and still beat beta
    // Don't do null move if:
    // - We're in check
    // - Already did null move in parent
    // - In endgame (low material)
    // - Depth too low
    if (allow_null && !in_check && depth >= 3) {
        // Check if we have enough material for null move (not in endgame)
        int our_material = 0;
        for (int sq = 0; sq < 64; sq++) {
            if (board.color_at(sq) == color) {
                int piece = board.piece_at(sq);
                our_material += get_piece_value(piece);
            }
        }
        
        if (our_material > 400) {  // More than a rook
            // Make a null move (pass the turn)
            Board null_board = board;
            null_board.side_to_move = 1 - color;
            null_board.en_passant_square = -1;
            null_board.compute_hash();
            
            // Search with reduced depth
            int R = 2;  // Reduction factor
            int null_score = -alpha_beta(null_board, depth - 1 - R, -beta, -beta + 1, 1 - color, false);
            
            if (null_score >= beta) {
                // Null move caused a beta cutoff
                g_diag.betaCutoffs++;
                return beta;
            }
        }
    }
    
    // Generate candidate moves
    auto moves = generate_candidates(board);
    
    if (moves.empty()) {
        // No legal moves - stalemate or checkmate
        if (in_check) {
            // **IMPROVED**: Return mate score adjusted by depth
            return -MATE_SCORE + (max_depth - depth);
        }
        return 0;  // Stalemate
    }
    
    // **IMPROVED**: Check extension - search deeper when in check
    if (in_check && depth < max_depth) {
        depth++;
    }
    
    // Order moves
    order_moves(moves, board, tt_move, depth);
    
    int best_move = moves[0];
    int best_score = -std::numeric_limits<int>::max();
    int flag = 1;  // alpha
    
    // Search all moves
    for (int move : moves) {
        if (should_stop()) return 0;
        
        Board new_board = make_move(board, move);
        
        // Add current position to history for repetition detection
        position_history.push_back(board.hash);
        
        int score = -alpha_beta(new_board, depth - 1, -beta, -alpha, 1 - color, true);
        
        // Remove from history
        position_history.pop_back();
        
        if (score > best_score) {
            best_score = score;
            best_move = move;
            
            if (score > alpha) {
                alpha = score;
                g_diag.alphaImproves++;
                flag = 3;  // exact
                
                if (score >= beta) {
                    flag = 2;  // beta cutoff
                    g_diag.betaCutoffs++;
                    
                    // **ENHANCED**: Update killer moves (only for quiet moves)
                    int to = Bitboards::move_to(move);
                    if (board.piece_at(to) == NO_PIECE && depth >= 0 && depth < 64) {
                        // Not a capture - update killers
                        if (killer_moves[depth][0] != move) {
                            killer_moves[depth][1] = killer_moves[depth][0];
                            killer_moves[depth][0] = move;
                        }
                    }
                    
                    // Update history
                    update_history(move, depth, depth * depth);
                    break;
                }
            }
        }
    }
    
    // Store in transposition table
    tt_store(board.hash, depth, best_score, best_move, flag);
    
    return best_score;
}

// Extract PV (principal variation) from transposition table
std::vector<std::string> extract_pv(Board board, int max_depth) {
    std::vector<std::string> pv;
    std::vector<uint64_t> seen_positions;
    
    for (int i = 0; i < max_depth && i < 10; i++) {
        // Check for repetition
        bool repeated = false;
        for (uint64_t hash : seen_positions) {
            if (hash == board.hash) {
                repeated = true;
                break;
            }
        }
        if (repeated) break;
        
        seen_positions.push_back(board.hash);
        
        // Get move from TT
        int idx = tt_index(board.hash);
        if (transposition_table[idx].hash != board.hash) {
            break;  // No entry for this position
        }
        
        int move = transposition_table[idx].move;
        if (move == 0) break;
        
        // **FIXED**: Verify move is legal for THIS position
        auto legal_moves = board.generate_moves();
        bool found = false;
        for (int legal_move : legal_moves) {
            if (legal_move == move && is_legal(board, legal_move)) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Move is not legal for this position - stop PV extraction
            break;
        }
        
        // Add move to PV
        pv.push_back(Bitboards::move_to_uci(move));
        
        // Make the move
        board = make_move(board, move);
    }
    
    return pv;
}


// Calculate thinking time based on position
int calculate_think_time(const Board& board, int base_time) {
    auto imbalances = Evaluation::analyze_imbalances(board.get_fen());
    
    float complexity = 1.0f;
    
    // King safety concerns
    if (imbalances.white_king_safety < 0 || imbalances.black_king_safety < 0) {
        complexity += 0.5f;
    }
    
    // Many imbalances = complex
    if (std::abs(imbalances.material_diff) > 200) {
        complexity += 0.3f;
    }
    
    // Passed pawns increase complexity
    if (imbalances.white_has_passed_pawn || imbalances.black_has_passed_pawn) {
        complexity += 0.3f;
    }
    
    if (Evaluation::is_opening(board)) {  
        complexity *= 0.7f;
    }
    
    return static_cast<int>(base_time * complexity);
}




// Main search function with iterative deepening

SearchResult search(const std::string& fen, int max_time_ms_param, int max_search_depth) {
    Board board;
    board.set_from_fen(fen);
    
    SearchResult result = {};
    result.time_ms = max_time_ms_param;
    result.depth = max_search_depth;
    result.nodes = 0;
    result.score = 0;
    result.best_move = 0;
    
    // Clear position history for new search
    position_history.clear();
    
    // Set time
    max_time_ms = max_time_ms_param;
    start_time = std::chrono::steady_clock::now();
    stop_search = false;
    nodes_searched = 0;
    
    // Reset diagnostics if debug tracing enabled
    if (UCI::options.debug_search_trace) {
        g_diag = SearchDiagnostics{};
        g_diag.rootKeyNonZero = (board.hash != 0);
        g_diag.ttEntries = TT_SIZE;
    }
    
    // **IMPROVED: Ensure we search at least to depth 3**
    int min_depth = 3;
    
    // Iterative deepening: search depth 1, 2, 3... up to max_search_depth
    for (int depth = 1; depth <= max_search_depth; depth++) {
        if (should_stop()) break;
        
        search_depth = depth;
        
        // Search at this depth
        int score = alpha_beta(board, depth, -MATE_SCORE, MATE_SCORE, board.side_to_move, true);
        
        result.score = score;
        result.depth = depth;
        
        // Get best move from TT with proper validation
        int idx = tt_index(board.hash);
        int tt_candidate = 0;
        
        // Verify TT entry is for correct position
        if (transposition_table[idx].hash == board.hash && transposition_table[idx].move != 0) {
            tt_candidate = transposition_table[idx].move;
            
            // Validate move is legal
            auto legal_moves = board.generate_moves();
            bool found = false;
            for (int legal_move : legal_moves) {
                if (is_legal(board, legal_move) && legal_move == tt_candidate) {
                    found = true;
                    result.best_move = tt_candidate;
                    break;
                }
            }
            
            if (!found) {
                // TT move is not legal - find any legal move as fallback
                for (int legal_move : legal_moves) {
                    if (is_legal(board, legal_move)) {
                        result.best_move = legal_move;
                        break;
                    }
                }
            }
        } else if (result.best_move == 0) {
            // No TT move, find any legal move
            auto legal_moves = board.generate_moves();
            for (int legal_move : legal_moves) {
                if (is_legal(board, legal_move)) {
                    result.best_move = legal_move;
                    break;
                }
            }
        }
        
		
		if (result.best_move != 0) {
    // Validate the move is actually legal
    bool legal = false;
    auto all_moves = board.generate_moves();
    for (int legal_move : all_moves) {
        if (is_legal(board, legal_move) && legal_move == result.best_move) {
            legal = true;
            break;
        }
    }
    if (!legal) {
        result.best_move = 0;  // Discard illegal TT move
    }
}
		
		
		
        // Output info after each depth completes
        int elapsed = get_elapsed_ms();
        
        // Extract full PV line from transposition table
        Board pv_board = board;
        std::vector<std::string> pv_line = extract_pv(pv_board, depth);
        
        // Build PV string
        std::string pv_str;
        for (size_t i = 0; i < pv_line.size(); i++) {
            if (i > 0) pv_str += " ";
            pv_str += pv_line[i];
        }
        // Fallback to just best move if PV extraction failed
        if (pv_str.empty() && result.best_move != 0) {
            pv_str = Bitboards::move_to_uci(result.best_move);
        }
        
        // Format mate scores properly
        std::string score_str;
        if (score > MATE_BOUND) {
            int moves_to_mate = (MATE_SCORE - score + 1) / 2;
            score_str = "mate " + std::to_string(moves_to_mate);
        } else if (score < -MATE_BOUND) {
            int moves_to_mate = (MATE_SCORE + score + 1) / 2;
            score_str = "mate -" + std::to_string(moves_to_mate);
        } else {
            score_str = "cp " + std::to_string(score);
        }
        
        std::cout << "info depth " << depth 
                  << " score " << score_str
                  << " nodes " << nodes_searched
                  << " time " << elapsed
                  << " nps " << (elapsed > 0 ? (nodes_searched * 1000 / elapsed) : 0)
                  << " pv " << pv_str
                  << std::endl;
        std::cout.flush();
        
        // **NEW: Don't stop too early in opening**
        // In opening (first 5 moves), search to at least depth 4
        if (depth < min_depth && board.fullmove_number <= 5) {
            continue; // Keep searching deeper
        }
        
        // Check time
        if (should_stop()) break;
    }
    
    result.nodes = nodes_searched;
    result.time_ms = get_elapsed_ms();
    
    // Output root eval trace if enabled
    if (Evaluation::get_debug_trace()) {
        Evaluation::evaluate_at_root(board);
    }
    
    // Apply human selection at root if enabled
    const auto& params = Evaluation::get_params();
    if (params.human_select && result.best_move != 0 && !stop_search) {
        // Calculate current ply (halfmove clock is plies since last capture/pawn)
        int current_ply = board.halfmove_clock;
        
        // Collect candidate moves with guardrails
        auto candidates = HumanSelection::collect_candidates(
            &board,
            params.candidate_margin_cp,
            params.candidate_moves_max,
            3,  // Shallow search for candidates
            params.human_hard_floor_cp,
            params.human_opening_sanity,
            params.human_topk_override,
            current_ply,
            params.debug_human_pick
        );
        
        if (!candidates.empty() && candidates.size() > 1) {
            // Get best score from candidates
            int best_score = candidates[0].score;
            
            // Pick human move
            int human_move = HumanSelection::pick_human_move(
                &board,
                candidates,
                best_score,
                params.human_temperature,
                params.human_noise_cp,
                params.risk_appetite,
                params.sacrifice_bias,
                params.simplicity_bias,
                params.random_seed,
                params.debug_human_pick
            );
            
            if (human_move != 0) {
                // CRITICAL: Verify the chosen move is LEGAL in current position
                bool move_is_legal = false;
                auto all_moves = board.generate_moves();
                for (int m : all_moves) {
                    if (m == human_move && is_legal(board, m)) {
                        move_is_legal = true;
                        break;
                    }
                }
                
                if (!move_is_legal) {
                    // CRITICAL ERROR: Illegal move selected - abort!
                    std::cerr << "FATAL: Illegal move selected by human selection!" << std::endl;
                    std::cerr << "FEN: " << board.get_fen() << std::endl;
                    std::cerr << "Move: " << Bitboards::move_to_uci(human_move) << std::endl;
                    std::cerr << "Legal moves:";
                    for (int m : all_moves) {
                        if (is_legal(board, m)) {
                            std::cerr << " " << Bitboards::move_to_uci(m);
                        }
                    }
                    std::cerr << std::endl;
                    // Fall back to best move from search
                    human_move = 0;
                }
                
                if (human_move != 0) {
                    // Use human-selected move but keep PV from best move
                    result.best_move = human_move;
                }
            }
        }
    }
    
    // FINAL SAFETY CHECK: Verify best_move is legal before returning
    if (result.best_move != 0) {
        bool best_is_legal = false;
        auto all_moves = board.generate_moves();
        for (int m : all_moves) {
            if (m == result.best_move && is_legal(board, m)) {
                best_is_legal = true;
                break;
            }
        }
        
        if (!best_is_legal) {
            std::cerr << "FATAL: Engine returned illegal best_move!" << std::endl;
            std::cerr << "FEN: " << board.get_fen() << std::endl;
            std::cerr << "Move: " << Bitboards::move_to_uci(result.best_move) << std::endl;
            // Find a legal move as fallback
            for (int m : all_moves) {
                if (is_legal(board, m)) {
                    result.best_move = m;
                    break;
                }
            }
        }
    }
    
    return result;
}


void stop() {
    stop_search = true;
}

bool is_searching() {
    return !stop_search && get_elapsed_ms() < max_time_ms;
}

// Helper function for UCI position command - apply a UCI move to a FEN string
std::string apply_uci_move(const std::string& fen, const std::string& uci_move) {
    Board board;
    board.set_from_fen(fen);
    
    // Generate all moves
    auto all_moves = board.generate_moves();
    
    // Filter to legal moves AND convert to UCI strings at the same time
    std::vector<std::pair<int, std::string>> legal_moves_with_uci;
    for (int move : all_moves) {
        if (is_legal(board, move)) {
            std::string move_uci = Bitboards::move_to_uci(move);
            legal_moves_with_uci.push_back({move, move_uci});
        }
    }
    
    // Find the move that matches the UCI string
    int matched_move = 0;
    for (const auto& pair : legal_moves_with_uci) {
        if (pair.second == uci_move) {
            matched_move = pair.first;
            break;
        }
    }
    
    if (matched_move == 0) {
        // Could not find move - return unchanged FEN
        return fen;
    }
    
    // Apply the move
    Board new_board = make_move(board, matched_move);
    
    // Update move counters
    new_board.fullmove_number = board.fullmove_number + (board.side_to_move == BLACK ? 1 : 0);
    
    return new_board.get_fen();
}

void set_threads(int n) {
    // For single-threaded alpha-beta, threads not used
    (void)n;
}

void set_hash_size(int mb) {
    // Resize transposition table
    delete[] transposition_table;
    int new_size = 1 << (20 + mb / 64);  // Scale with MB
    transposition_table = new TTEntry[new_size];
}

void set_use_mcts(bool use) {
    (void)use;  // Not used in alpha-beta implementation
}

void set_depth_limit(int depth) {
    max_depth = depth;
}

// Forward declaration
static uint64_t perft_recursive(Board& board, int depth);

// Perft divide - shows count for each root move
void perft_divide(Board& board, int depth) {
    std::cout << "Perft Divide at depth " << depth << std::endl;
    std::cout << "Position: " << board.get_fen() << std::endl;
    std::cout << std::endl;
    
    auto moves = board.generate_moves();
    
    uint64_t total = 0;
    for (int move : moves) {
        if (!is_legal(board, move)) {
            continue;
        }
        
        Board temp = make_move(board, move);
        uint64_t count = perft_recursive(temp, depth - 1);
        
        std::string uci = Bitboards::move_to_uci(move);
        std::cout << uci << ": " << count << std::endl;
        
        total += count;
    }
    
    std::cout << std::endl;
    std::cout << "Total: " << total << std::endl;
}

// Perft (performance test) - counts leaf nodes at given depth
uint64_t perft_recursive(Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }
    
    uint64_t nodes = 0;
    auto moves = board.generate_moves();
    
    for (int move : moves) {
        // Check if move is LEGAL before recursing
        if (!is_legal(board, move)) {
            continue;
        }
        
        // DEBUG: Snapshot board state before make_move
        std::string fen_before = board.get_fen();
        uint64_t hash_before = board.hash;
        
        // Make move
        Board temp = make_move(board, move);
        
        // DEBUG: Verify board state is valid after make_move
        // Check king exists for both sides
        bool white_king_found = false, black_king_found = false;
        for (int sq = 0; sq < 64; sq++) {
            if (temp.piece_at(sq) == KING) {
                if (temp.color_at(sq) == WHITE) white_king_found = true;
                else black_king_found = true;
            }
        }
        if (!white_king_found || !black_king_found) {
            std::cerr << "ERROR: King missing after move " << Bitboards::move_to_uci(move) << std::endl;
            std::cerr << "FEN before: " << fen_before << std::endl;
            std::cerr << "FEN after: " << temp.get_fen() << std::endl;
        }
        
        // DEBUG: Undo move and verify state restored
        // Find reverse move (this is approximate - we just check state)
        // Actually, let's just check that temp side_to_move is correct
        if (temp.side_to_move == board.side_to_move) {
            std::cerr << "ERROR: Side to move not switched after move " << Bitboards::move_to_uci(move) << std::endl;
            std::cerr << "FEN before: " << fen_before << std::endl;
            std::cerr << "FEN after: " << temp.get_fen() << std::endl;
        }
        
        if (depth > 1) {
            nodes += perft_recursive(temp, depth - 1);
        } else {
            nodes++;
        }
    }
    
    return nodes;
}

// Reference perft values from known correct implementations
// Perft 3 from start position: 8902
// These are the per-move counts at depth 3 (perft 2 from each child position)
static const std::unordered_map<std::string, int> perft3_reference = {
    {"a2a3", 380}, {"a2a4", 420}, {"b2b3", 420}, {"b2b4", 421},
    {"c2c3", 420}, {"c2c4", 441}, {"d2d3", 539}, {"d2d4", 560},
    {"e2e3", 580}, {"e2e4", 579}, {"f2f3", 380}, {"f2f4", 401},
    {"g2g3", 420}, {"g2g4", 421}, {"h2h3", 380}, {"h2h4", 420},
    {"b1a3", 400}, {"b1c3", 440}, {"g1f3", 440}, {"g1h3", 400}
};

void perft(Board& board, int depth) {
    std::cout << "Perft to depth " << depth << std::endl;
    std::cout << "Position: " << board.get_fen() << std::endl;
    std::cout << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    uint64_t total_nodes = 0;
    auto moves = board.generate_moves();
    
    // First, print perft 1 for each move
    std::cout << "Move        | Count     | %" << std::endl;
    std::cout << "------------|-----------|----" << std::endl;
    
    for (int move : moves) {
        // Check if move is LEGAL before counting
        if (!is_legal(board, move)) {
            // Print illegal moves for debugging - show WHY it's illegal
            std::string move_uci = Bitboards::move_to_uci(move);
            int from = Bitboards::move_from(move);
            int to = Bitboards::move_to(move);
            int flags = Bitboards::move_flags(move);
            
            // Make the move and check
            Board temp = make_move(board, move);
            bool king_in_check = temp.is_in_check(temp.side_to_move);
            
            std::cout << move_uci << " (ILLEGAL - ";
            if (king_in_check) std::cout << "king in check)";
            else std::cout << "unknown)";
            std::cout << std::endl;
            continue;
        }
        
        uint64_t count = 1;
        if (depth > 1) {
            Board temp = make_move(board, move);
            count = perft_recursive(temp, depth - 1);
        }
        
        std::string move_uci = Bitboards::move_to_uci(move);
        
        // Compare with reference if available (depth 3 = perft2 from each child)
        auto it = perft3_reference.find(move_uci);
        if (it != perft3_reference.end()) {
            if (depth == 3) {
                if (count != it->second) {
                    std::cout << move_uci << std::string(12 - move_uci.length(), ' ') << "| " << count << " *** MISMATCH (expected " << it->second << ") ***" << std::endl;
                } else {
                    std::cout << move_uci << std::string(12 - move_uci.length(), ' ') << "| " << count << " (ok)" << std::endl;
                }
            } else {
                std::cout << move_uci << std::string(12 - move_uci.length(), ' ') << "| " << count << std::endl;
            }
        } else {
            std::cout << move_uci << std::string(12 - move_uci.length(), ' ') << "| " << count << std::endl;
        }
        
        total_nodes += count;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "------------|-----------|----" << std::endl;
    std::cout << "Total nodes:   " << total_nodes << std::endl;
    std::cout << "Time:          " << duration.count() << " ms" << std::endl;
    std::cout << "Nodes/second:  " << (duration.count() > 0 ? total_nodes / (duration.count() / 1000.0) : 0) << std::endl;
}

} // namespace Search
