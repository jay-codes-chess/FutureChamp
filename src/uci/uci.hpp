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
