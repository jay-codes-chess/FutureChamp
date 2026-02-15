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
#include <cstdint>
#include <chrono>
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
    
    // Hotpath counters
    uint64_t makeMoveCalls = 0;
    uint64_t unmakeMoveCalls = 0;
    uint64_t boardCopies = 0;
    
    // Evaluation profiling
    uint64_t evalCalls = 0;
    uint64_t evalTimeNs = 0;
    
    // QSearch eval mode counters
    uint64_t qevalFast = 0;
    uint64_t qevalMed = 0;
    
    // Move ordering stats
    uint64_t killerHits = 0;
    uint64_t historyHits = 0;
    
    // Copy attribution counters
    uint64_t copies_make_return = 0;
    uint64_t copies_board_clone = 0;
    uint64_t copies_nullmove = 0;
    uint64_t copies_legality = 0;
    uint64_t copies_qsearch = 0;
    uint64_t copies_pv = 0;
    uint64_t copies_other = 0;
    
    // Timing buckets (root only, in microseconds)
    uint64_t t_movegen = 0;
    uint64_t t_makeunmake = 0;
    uint64_t t_eval = 0;
    uint64_t t_legality = 0;
    
    std::chrono::steady_clock::time_point searchStartTime;
    std::chrono::steady_clock::time_point searchEndTime;
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

// **MOVE ORDERING TABLES**
static const int MAX_PLY = 64;
static const int MAX_SQ = 64;

// Killer moves: [ply][2]
extern int killers[MAX_PLY][2];

// History heuristic: [side][from][to]
extern int history[2][MAX_SQ][MAX_SQ];

// Initialize search (creates transposition table)
void initialize();
void perft(Board& board, int depth);
void perft_divide(Board& board, int depth);
bool is_legal(Board& board, int move);

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
