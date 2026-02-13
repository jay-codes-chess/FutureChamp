/**
 * Human Chess Engine
 * 
 * A human-like chess engine that thinks like a coach, not a calculator.
 * Built with Silman-style evaluation, strategic thinking, and teaching focus.
 * 
 * Author: Brendan & Jay
 * License: MIT
 */

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include "uci/uci.hpp"
#include "search/search.hpp"
#include "eval/evaluation.hpp"
#include "eval/params.hpp"
#include "utils/board.hpp"

// Process a single FEN and return eval breakdown
std::string process_fen(const std::string& fen) {
    Board board;
    board.set_from_fen(fen);
    
    Evaluation::ScoreBreakdown bd = Evaluation::evaluate_with_breakdown(board);
    
    std::ostringstream oss;
    oss << "total=" << bd.total 
        << " material=" << bd.material 
        << " pawns=" << bd.pawn_structure 
        << " activity=" << bd.piece_activity 
        << " king=" << bd.king_safety 
        << " imbalance=" << bd.imbalance 
        << " init=" << bd.initiative 
        << " knowledge=" << bd.knowledge;
    
    return oss.str();
}

// Run eval file test mode
void run_evalfile_mode(const std::string& filename, const std::string& personality) {
    // Load personality if specified
    if (!personality.empty()) {
        if (Evaluation::load_personality(personality)) {
            std::cout << "Loaded personality: " << personality << std::endl;
        } else {
            std::cout << "Failed to load personality: " << personality << std::endl;
        }
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Parse: description|FEN
        size_t pipe_pos = line.find('|');
        if (pipe_pos == std::string::npos) continue;
        
        std::string desc = line.substr(0, pipe_pos);
        std::string fen = line.substr(pipe_pos + 1);
        
        std::string result = process_fen(fen);
        std::cout << desc << " | " << result << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "FutureChamp" << std::endl;
    std::cout << "A chess engine that thinks like a coach." << std::endl;
    std::cout << std::endl;
    
    // Check for eval file mode
    std::string evalfile;
    std::string personality;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--evalfile" && i + 1 < argc) {
            evalfile = argv[++i];
        } else if (arg == "--personality" && i + 1 < argc) {
            personality = argv[++i];
        }
    }
    
    if (!evalfile.empty()) {
        run_evalfile_mode(evalfile, personality);
        return 0;
    }
    
    // Initialize engine components
    Evaluation::initialize();
    Search::initialize();
    
    // Run UCI loop
    UCI::loop(argc, argv);
    
    return 0;
}
