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
