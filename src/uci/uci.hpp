/**
 * UCI Protocol Interface
 * Handles communication with chess GUIs
 */

#ifndef UCI_HPP
#define UCI_HPP

#include <string>
#include <vector>
#include <sstream>

namespace UCI {

struct Options {
    // Style options
    std::string playing_style = "classical";  // classical, attacking, tactical, positional, technical
    int skill_level = 10;  // 0-20
    
    // Search options
    int hash_size = 64;  // MB
    int threads = 1;
    bool use_mcts = true;
    
    // Teaching options
    bool verbal_pv = false;
    bool show_imbalances = false;
    
    // Debug options
    bool debug_eval_trace = false;
    bool debug_search_trace = false;
    
    // **EVAL TIERING**
    bool eval_tiering = true;
    int eval_fast_depth_threshold = 3;
    std::string eval_qsearch_mode = "MED";  // "FAST" or "MED"
    
    // **SEARCH PRUNING**
    bool debug_pruning_trace = false;
    bool lmr_enable = true;
    int lmr_move_index = 4;
    int lmr_depth_min = 3;
    int lmr_base_reduction = 1;
    bool null_move_enable = true;
    int null_move_r = 2;
    bool futility_enable = true;
    int futility_margin1 = 120;
    int futility_margin2 = 240;
    bool see_prune_enable = true;
    int see_prune_threshold = -100;
    bool check_ext_enable = true;
    int check_ext_depth_min = 3;
    
    // IID (Internal Iterative Deepening)
    bool iid_enable = true;
    int iid_depth_min = 5;
    int iid_reduction = 2;
    
    // PVS (Principal Variation Search)
    bool pvs_enable = true;
    
    // LMP (Late Move Pruning)
    bool lmp_enable = true;
    int lmp_move_count = 6;
    
    // Razoring
    bool razor_enable = true;
    
    // Null Move Verification
    bool null_move_verify = true;
    
    // Eval Cache
    bool eval_cache_enable = true;
    int eval_cache_mb = 16;
    bool debug_eval_cache = false;
    
    // QSearch Checks
    bool qsearch_checks_enable = true;
    int qsearch_checks_plies = 2;
    int qsearch_check_see_threshold = -50;
    
    // Singular Extensions
    bool singular_ext_enable = true;
    int singular_ext_depth_min = 6;
    int singular_ext_margin_cp = 60;
    int singular_ext_verification_reduction = 2;
    
    // Time management
    int move_overhead = 30;
    int min_think_ms = 20;
    int max_think_ms = 0;
    int time_safety = 90;
    
    // Draw rules
    int contempt = 0;
    
    // King danger eval
    int w_king_danger = 100;
    int king_danger_ring_bonus = 100;
    int king_danger_shield_penalty = 100;
    
    // PST (Piece-Square Tables)
    int w_pst = 100;
    int pst_center_bias = 120;
    int pst_knight_edge_penalty = 130;
    bool pst_opening_only = false;
    bool debug_pst_trace = false;
};

extern Options options;

// Main UCI loop
void loop(int argc, char* argv[]);

// Parse UCI commands
void parse_command(const std::string& cmd);

// UCI protocol commands
void cmd_uci();
void cmd_is_ready();
void cmd_position(const std::vector<std::string>& tokens);
void cmd_go(const std::vector<std::string>& tokens);
void cmd_setoption(const std::vector<std::string>& tokens);
void cmd_stop();
void cmd_quit();

} // namespace UCI

#endif // UCI_HPP
