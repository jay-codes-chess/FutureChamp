/**
 * Parameters Registry
 * Single source of truth for all tunable engine parameters
 * Updated via UCI setoption commands
 */

#ifndef EVAL_PARAMS_HPP
#define EVAL_PARAMS_HPP

#include <string>

namespace Evaluation {

// Parameter registry - all tunable values
struct Params {
    // === CORE MATERIAL / IMBALANCE ===
    int material_priority = 100;        // 1-100, lower = more sacrificial
    int imbalance_scale = 100;           // 30-150 (0.30-1.50)
    int knight_value_bias = 0;           // -50 to +50
    int bishop_value_bias = 0;           // -50 to +50
    int exchange_sensitivity = 100;     // 0-200
    
    // === EVAL LAYER WEIGHTS ===
    int w_pawn_structure = 100;         // 0-200
    int w_piece_activity = 100;         // 0-200
    int w_king_safety = 100;          // 0-200
    int w_initiative = 100;            // 0-200
    int w_imbalance = 100;             // 0-200
    
    // === KEY MICRO TERMS ===
    int outpost_bonus = 100;            // 0-200
    int bad_bishop_penalty = 100;       // 0-200
    int space_concept_weight = 100;      // 0-200
    int bishop_pair_bonus = 100;       // 0-200
    int rook_open_file_bonus = 100;   // 0-200
    int passed_pawn_bonus = 100;       // 0-200
    int pawn_shield_penalty = 100;     // 0-200
    
    // === KNOWLEDGE CONCEPTS ===
    int w_knowledge_concepts = 100;     // 0-200, master weight for all concepts
    
    // === SEARCH / HUMANISATION ===
    int candidate_margin_cp = 200;      // 0-400
    int candidate_moves_max = 10;       // 1-30
    bool human_enable = true;            // check
    int human_temperature = 100;         // 0-200
    int human_noise_cp = 0;             // 0-50
    int human_blunder_rate = 0;         // 0-1000
    int random_seed = 0;                // 0-2147483647
    
    // === DEBUG ===
    bool debug_trace_with_params = false;  // Include params in trace output
};

// Get reference to global params
Params& get_params();

// Set a param by name (returns true if successful)
bool set_param(const std::string& name, const std::string& value);

// Print all params (for debugging)
std::string dump_params();

} // namespace Evaluation

#endif // EVAL_PARAMS_HPP
