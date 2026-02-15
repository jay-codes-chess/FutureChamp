/**
 * Pawn Structure Evaluation Layer Implementation
 */

#include "pawn_structure.hpp"
#include "material.hpp"
#include <cmath>
#include <vector>

namespace Evaluation {

// **PAWN HASH CACHE**
static const int PAWN_HASH_SIZE = 16384;  // 2^14 entries
static std::vector<PawnHashEntry> pawn_hash;

void init_pawn_hash(int size) {
    pawn_hash.resize(size);
    for (auto& e : pawn_hash) {
        e.key = 0;
        e.score = 0;
        e.valid = false;
    }
}

void clear_pawn_hash() {
    for (auto& e : pawn_hash) {
        e.valid = false;
    }
}

int probe_pawn_hash(uint64_t key, int& score) {
    if (pawn_hash.empty()) return 0;
    int idx = key & (pawn_hash.size() - 1);
    if (pawn_hash[idx].valid && pawn_hash[idx].key == key) {
        score = pawn_hash[idx].score;
        return 1;  // Hit
    }
    return 0;  // Miss
}

void store_pawn_hash(uint64_t key, int score) {
    if (pawn_hash.empty()) return;
    int idx = key & (pawn_hash.size() - 1);
    pawn_hash[idx].key = key;
    pawn_hash[idx].score = score;
    pawn_hash[idx].valid = true;
}

int evaluate_pawns_for_color(const Board& board, int color);

// is_opening is defined in evaluation.cpp
// Using forward declaration to avoid circular dependency

int evaluate_pawn_structure(const Board& board) {
    // **PAWN HASH**: Try to use cached result
    uint64_t pawn_key = (board.pieces[PAWN] << 0) ^ (board.colors[WHITE] << 16) ^ (board.colors[BLACK] << 32);
    int cached_score = 0;
    if (probe_pawn_hash(pawn_key, cached_score)) {
        return cached_score;
    }
    
    int score = 0;
    
    // Early game center pawn bonus
    if (board.fullmove_number <= 10) {
        if (board.side_to_move == WHITE) {
            if (board.piece_at(28) == PAWN && board.color_at(28) == WHITE) score += 150;
            if (board.piece_at(27) == PAWN && board.color_at(27) == WHITE) score += 140;
            if (board.piece_at(29) == PAWN && board.color_at(29) == WHITE) score += 80;
            if (board.piece_at(26) == PAWN && board.color_at(26) == WHITE) score += 80;
            if (board.piece_at(19) == PAWN && board.color_at(19) == WHITE) score += 30;
            if (board.piece_at(18) == PAWN && board.color_at(18) == WHITE) score += 30;
        } else {
            if (board.piece_at(36) == PAWN && board.color_at(36) == BLACK) score -= 150;
            if (board.piece_at(35) == PAWN && board.color_at(35) == BLACK) score -= 140;
            if (board.piece_at(37) == PAWN && board.color_at(37) == BLACK) score -= 80;
            if (board.piece_at(34) == PAWN && board.color_at(34) == BLACK) score -= 80;
            if (board.piece_at(43) == PAWN && board.color_at(43) == BLACK) score -= 30;
            if (board.piece_at(42) == PAWN && board.color_at(42) == BLACK) score -= 30;
        }
    }
    
    // Evaluate pawns for each color
    score += evaluate_pawns_for_color(board, WHITE);
    score -= evaluate_pawns_for_color(board, BLACK);
    
    // **PAWN HASH**: Store result
    store_pawn_hash(pawn_key, score);
    
    return score;
}

int evaluate_pawns_for_color(const Board& board, int color) {
    int score = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) != PAWN || board.color_at(sq) != color) continue;
        
        int file = sq % 8;
        int rank = sq / 8;
        
        // Center control bonus
        if (file >= 2 && file <= 5) {
            int forward_rank = (color == WHITE) ? rank + 1 : rank - 1;
            if (forward_rank >= 0 && forward_rank < 8) {
                int forward_sq = file + forward_rank * 8;
                if (forward_sq >= 27 && forward_sq <= 36) {
                    score += 15;
                }
            }
        }
        
        // Passed pawn
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

} // namespace Evaluation
