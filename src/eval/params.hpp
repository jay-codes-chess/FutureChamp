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
    int bishop_pair_bonus = 100;       // 0-200
    int rook_open_file_bonus = 100;   // 0-200
    int passed_pawn_bonus = 100;       // 0-200
    int pawn_shield_penalty = 100;     // 0-200
    
    // === KNOWLEDGE CONCEPT WEIGHTS ===
    int w_knowledge_concepts = 100;     // 0-200, master gate for all concepts
    int concept_outpost_weight = 100;   // 0-200
    int concept_bad_bishop_weight = 100; // 0-200
    int concept_space_weight = 100;      // 0-200
    
    // === MASTER CONCEPTS ===
    int concept_exchange_sac_weight = 100;    // 0-200
    int concept_color_complex_weight = 100;  // 0-200
    int concept_pawn_lever_weight = 100;      // 0-200
    int concept_initiative_persist_weight = 100; // 0-200
    int initiative_dominance = 100;           // 0-200 - amplifies initiative term
    
    // === SEARCH / HUMANISATION ===
    int candidate_margin_cp = 200;      // 0-400
    int candidate_moves_max = 10;       // 1-30
    bool human_enable = true;            // check
    bool human_select = true;            // check - use human selection at root
    int human_temperature = 100;         // 0-200
    int human_noise_cp = 0;             // 0-50
    int human_blunder_rate = 0;         // 0-1000
    int random_seed = 0;                // 0-2147483647
    int risk_appetite = 100;            // 0-200 - Tal high, Petrosian low
    int sacrifice_bias = 100;            // 0-200 - ties to MaterialPriority
    int simplicity_bias = 100;           // 0-200 - Capa higher
    int trade_bias = 100;               // 0-200 - willingness to trade pieces
    
    // === HUMAN GUARDRAILS ===
    int human_hard_floor_cp = 200;       // 0-600 - drop candidates worse than best-foor
    int human_opening_sanity = 120;     // 0-200 - penalize edge moves in opening
    int human_topk_override = 0;        // 1-10 - restrict to top K moves
    
    // === DEBUG ===
    bool debug_trace_with_params = false;  // Include params in trace output
    bool debug_human_pick = false;        // Debug human move selection
    
    // === PERSONALITY ===
    std::string current_personality = "default";  // Current loaded personality
    bool personality_auto_load = true;  // Auto-load personality from JSON
};

// Get reference to global params
Params& get_params();

// Set executable path for relative file resolution
void set_exe_path(const std::string& path);
std::string get_exe_path();
std::string get_file_path(const std::string& relative_path);

// Load personality from name (tries .txt then .json)
bool load_personality(const std::string& name, bool verbose = true);

// Load personality from explicit file path
bool load_personality_file(const std::string& filepath, bool verbose = true);

// Save current params to JSON file
bool save_personality(const std::string& name);

// Set a param by name (returns true if successful)
bool set_param(const std::string& name, const std::string& value);

// Print all params (for debugging)
std::string dump_params();

} // namespace Evaluation

#endif // EVAL_PARAMS_HPP
