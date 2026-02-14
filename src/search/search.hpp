/**
 * Search Module - Alpha-Beta with Human-Like Selectivity
 * 
 * Implements selective search that mirrors human thinking:
 * - Uses candidate move generation (Kotov method)
 * - Selective depth based on position complexity
 * - Time-aware thinking patterns
 * 
 * Reference: Alexander Kotov "Play Like a Grandmaster"
 */

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "../utils/board.hpp"

namespace Search {

// Search diagnostics structure
struct SearchDiagnostics {
    uint64_t nodes = 0;
    uint64_t qnodes = 0;
    uint64_t qEvasions = 0;
    uint64_t qCapturesSearched = 0;
    uint64_t qCapturesSkippedSEE = 0;
    uint64_t qDeltaPruned = 0;
    uint64_t ttProbes = 0;
    uint64_t ttHits = 0;
    uint64_t ttStores = 0;
    uint64_t ttCollisions = 0;
    uint64_t ttEntries = 0;
    bool rootKeyNonZero = false;
    uint64_t betaCutoffs = 0;
    uint64_t alphaImproves = 0;
};

// Global diagnostics instance
extern SearchDiagnostics g_diag;

struct SearchResult {
    int best_move;  // encoded move (from << 6) | to
    int score;  // centipawns
    int depth;
    long nodes;
    double time_ms;
    std::vector<std::string> pv;
};

// Initialize search (creates transposition table)
void initialize();
void perft(Board& board, int depth);
void perft_divide(Board& board, int depth);
bool is_legal(const Board& board, int move);

// Set search parameters
void set_threads(int n);
void set_hash_size(int mb);
void set_use_mcts(bool use_mcts);
void set_depth_limit(int depth);

// Search position with time limit and optional depth
SearchResult search(const std::string& fen, int max_time_ms = 30000, int max_search_depth = 10);

// Stop search
void stop();

// Check if search is running
bool is_searching();

// Helper function to apply a UCI move to a FEN string (for position command)
std::string apply_uci_move(const std::string& fen, const std::string& uci_move);

} // namespace Search

#endif // SEARCH_HPP
