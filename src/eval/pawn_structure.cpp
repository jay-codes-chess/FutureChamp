/**
 * Pawn Structure Evaluation Layer Implementation
 */

#include "pawn_structure.hpp"
#include "material.hpp"
#include <cmath>
#include <iostream>

namespace Evaluation {

// Pawn hash table
static const int PAWN_HASH_SIZE = 1 << 16;  // 65536 entries

struct PawnHashEntry {
    uint64_t key;
    int pawn_score;
    bool valid;
};

static PawnHashEntry* pawn_hash_table = nullptr;
static uint64_t pawn_hash_probes = 0;
static uint64_t pawn_hash_hits = 0;

// Compute pawn-only Zobrist key
uint64_t compute_pawn_hash(const Board& board) {
    // Use only pawn positions for pawn hash
    // We can derive this from the board's full hash, but simpler to compute fresh
    static bool initialized = false;
    static uint64_t pawn_keys[2][64];
    
    if (!initialized) {
        uint64_t seed = 0xDEADBEEF12345678ULL;
        for (int sq = 0; sq < 64; sq++) {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            pawn_keys[0][sq] = seed;
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            pawn_keys[1][sq] = seed;
        }
        initialized = true;
    }
    
    uint64_t key = 0;
    for (int sq = 0; sq < 64; sq++) {
        if (board.piece_at(sq) == PAWN) {
            int c = board.color_at(sq);
            key ^= pawn_keys[c][sq];
        }
    }
    return key;
}

// Initialize pawn hash table
void init_pawn_hash() {
    if (pawn_hash_table == nullptr) {
        pawn_hash_table = new PawnHashEntry[PAWN_HASH_SIZE];
    }
    for (int i = 0; i < PAWN_HASH_SIZE; i++) {
        pawn_hash_table[i].key = 0;
        pawn_hash_table[i].pawn_score = 0;
        pawn_hash_table[i].valid = false;
    }
    pawn_hash_probes = 0;
    pawn_hash_hits = 0;
}

// Get pawn hash statistics
void print_pawn_hash_stats() {
    if (pawn_hash_probes > 0) {
        int hitrate = static_cast<int>((pawn_hash_hits * 100) / pawn_hash_probes);
        std::cerr << "info string pawn_hash hitrate=" << hitrate << "% probes=" << pawn_hash_probes << " hits=" << pawn_hash_hits << std::endl;
    }
}

// Probe pawn hash
static int probe_pawn_hash(uint64_t key, int& score) {
    pawn_hash_probes++;
    int idx = key & (PAWN_HASH_SIZE - 1);
    if (pawn_hash_table[idx].valid && pawn_hash_table[idx].key == key) {
        pawn_hash_hits++;
        score = pawn_hash_table[idx].pawn_score;
        return 1;  // Hit
    }
    return 0;  // Miss
}

// Store in pawn hash
static void store_pawn_hash(uint64_t key, int score) {
    int idx = key & (PAWN_HASH_SIZE - 1);
    pawn_hash_table[idx].key = key;
    pawn_hash_table[idx].pawn_score = score;
    pawn_hash_table[idx].valid = true;
}

int evaluate_pawns_for_color(const Board& board, int color);

// is_opening is defined in evaluation.cpp
// Using forward declaration to avoid circular dependency

int evaluate_pawn_structure(const Board& board) {
    // Initialize pawn hash on first call
    static bool initialized = false;
    if (!initialized) {
        init_pawn_hash();
        initialized = true;
    }
    
    // Probe pawn hash
    uint64_t pawn_key = compute_pawn_hash(board);
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
    
    // Store in pawn hash
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
