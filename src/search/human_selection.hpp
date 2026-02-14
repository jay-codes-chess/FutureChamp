/**
 * Human Selection - Stochastic root move selection
 * 
 * Implements human-like move selection at the root:
 * - Collect top candidate moves
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
    
    CandidateMove(int m, int s) : move(m), score(s), probability(0.0), weight(0.0) {}
};

// Human selection functions
namespace HumanSelection {

// Collect candidate moves at root position
// Returns vector of (move, score) pairs sorted by score descending
std::vector<CandidateMove> collect_candidates(
    void* board_ptr, 
    int candidate_margin_cp, 
    int candidate_moves_max,
    int max_depth = 3  // Shallow search for candidates
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
    int random_seed,
    bool debug_output
);

// Utility: Calculate variance score for a move (higher for tactical moves)
int calculate_variance_score(void* board_ptr, int move);

// Seeded random number generator
double seeded_random(int seed);

} // namespace HumanSelection

#endif // HUMAN_SELECTION_HPP
