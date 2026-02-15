/**
 * Human Selection - Stochastic root move selection
 * 
 * Implements human-like move selection at the root:
 * - Collect top candidate moves
 * - Apply guardrails (hard floor, opening sanity, topK)
 * - Apply temperature-based sampling
 * - Add noise, risk appetite, sacrifice bias, simplicity bias
 */

#ifndef HUMAN_SELECTION_HPP
#define HUMAN_SELECTION_HPP

#include <vector>
#include <utility>  // for std::pair

// A candidate move with score and probability
struct CandidateMove {
    int move;
    int score;           // Original search score in centipawns
    double probability;   // Probability of being selected
    double weight;       // Weight after applying all factors
    
    CandidateMove() : move(0), score(0), probability(0.0), weight(0.0) {}
    CandidateMove(int m, int s) : move(m), score(s), probability(0.0), weight(0.0) {}
};

// Human selection functions
namespace HumanSelection {

// Collect candidate moves at root position with guardrails
// Returns vector of (move, score) pairs sorted by score descending
std::vector<CandidateMove> collect_candidates(
    void* board_ptr, 
    int candidate_margin_cp, 
    int candidate_moves_max,
    int max_depth = 3,
    int hard_floor_cp = 200,
    int opening_sanity = 120,
    int topk_override = 0,
    int current_ply = 0,
    bool debug_output = false
);

// Apply human selection heuristics and pick a move
int pick_human_move(
    void* board_ptr,
    std::vector<CandidateMove>& candidates,
    int best_score,
    int human_temperature,
    int human_noise_cp,
    int risk_appetite,
    int sacrifice_bias,
    int simplicity_bias,
    int trade_bias,
    int random_seed,
    bool debug_output
);

// Utility: Check if move is an edge move in opening
bool is_edge_move_opening(int move, void* board_ptr);

// Seeded random number generator
double seeded_random(int seed);

} // namespace HumanSelection

#endif // HUMAN_SELECTION_HPP
