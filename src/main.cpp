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
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include "uci/uci.hpp"
#include "search/search.hpp"
#include "eval/evaluation.hpp"
#include "eval/params.hpp"
#include "utils/board.hpp"

// Process a single FEN and return eval breakdown (returns full breakdown)
Evaluation::ScoreBreakdown eval_fen(const std::string& fen) {
    Board board;
    board.set_from_fen(fen);
    return Evaluation::evaluate_with_breakdown(board);
}

// Process a single FEN and return eval breakdown as string
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
        if (Evaluation::load_personality(personality, true)) {
            // Summary already printed
        } else {
            std::cout << "Failed to load personality: " << personality << std::endl;
        }
    }
    
    // Resolve file path
    std::string resolved_path = Evaluation::get_file_path(filename);
    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        // Try alternative path
        resolved_path = filename;
        file.open(resolved_path);
    }
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

// Parse comma-separated personality list
std::vector<std::string> parse_personality_list(const std::string& list) {
    std::vector<std::string> result;
    std::string current;
    for (char c : list) {
        if (c == ',') {
            if (!current.empty()) result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) result.push_back(current);
    return result;
}

// Run comparison mode
void run_compare_mode(const std::string& personalities_list, const std::string& evalfile) {
    std::vector<std::string> personalities = parse_personality_list(personalities_list);
    
    if (personalities.size() < 2) {
        std::cerr << "Error: --compare requires at least 2 personalities (comma-separated)" << std::endl;
        return;
    }
    
    std::cout << "=== Comparing personalities: " << personalities_list << " ===" << std::endl;
    std::cout << "info string Compare: " << personalities[0] << "=personalities/" << personalities[0] << ".json, " << personalities[1] << "=personalities/" << personalities[1] << ".json" << std::endl;
    
    // Print key initiative params for each personality
    for (const auto& p : personalities) {
        Evaluation::load_personality(p, false);
        const auto& params = Evaluation::get_params();
        std::cout << "info string " << p << ": W_Init=" << params.w_initiative 
                  << " InitPersist=" << params.concept_initiative_persist_weight
                  << " InitDom=" << params.initiative_dominance << std::endl;
    }
    std::cout << std::endl;
    
    // Load all FENs
    std::vector<std::pair<std::string, std::string>> fens; // (description, fen)
    
    std::string resolved_path = Evaluation::get_file_path(evalfile);
    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        resolved_path = evalfile;
        file.open(resolved_path);
    }
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << evalfile << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pipe_pos = line.find('|');
        if (pipe_pos == std::string::npos) continue;
        fens.push_back({line.substr(0, pipe_pos), line.substr(pipe_pos + 1)});
    }
    
    // Print header
    std::cout << "FEN";
    for (const auto& p : personalities) {
        std::cout << " | " << p << "_total | " << p << "_exch | " << p << "_init | " << p << "_init_raw";
    }
    std::cout << " | delta" << std::endl;
    
    std::cout << "---";
    for (size_t i = 0; i < personalities.size(); ++i) {
        std::cout << " | ---total--- | ---exch--- | ---init--- | ---init_raw---";
    }
    std::cout << " | ------" << std::endl;
    
    // Evaluate each FEN with each personality
    for (const auto& fen_pair : fens) {
        std::cout << fen_pair.first;
        
        std::vector<int> totals;
        
        for (const auto& p : personalities) {
            // Load personality
            Evaluation::load_personality(p, false);
            
            // Evaluate
            Evaluation::ScoreBreakdown bd = eval_fen(fen_pair.second);
            
            int total = bd.total;
            bool clamped = false;
            
            // Warn about suspicious eval magnitudes
            if (abs(total) > 5000) {
                std::cout << "[WARN: eval=" << total << "]";
                clamped = true;
            }
            
            std::cout << " | " << total 
                      << " | " << bd.exchange_sac 
                      << " | " << bd.initiative_persist
                      << " | " << bd.initiative_persist_raw;
            
            totals.push_back(total);
        }
        
        // Calculate delta: tal_total - petrosian_total (second personality - first)
        int delta = totals.back() - totals[0];
        std::cout << " | " << delta << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Comparison complete ===" << std::endl;
}

// Load expectations from JSON file
std::map<std::string, std::map<std::string, int>> load_expectations(const std::string& filename) {
    std::map<std::string, std::map<std::string, int>> expectations;
    
    std::string resolved_path = Evaluation::get_file_path(filename);
    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        resolved_path = filename;
        file.open(resolved_path);
    }
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open expectations file: " << filename << std::endl;
        return expectations;
    }
    
    // Simple JSON parsing - look for pattern: "fen_id": { "xxx_should_be_higher_by": N }
    std::string current_fen;
    std::string current_type;
    
    std::string line;
    while (std::getline(file, line)) {
        // Find fen keys (things with quotes followed by colon)
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        
        std::string before = line.substr(0, colon);
        std::string after = line.substr(colon + 1);
        
        // Trim
        before.erase(0, before.find_first_not_of(" \""));
        before.erase(before.find_last_not_of(" \"") + 1);
        after.erase(0, after.find_first_not_of(" \""));
        after.erase(after.find_last_not_of(" ,}") + 1);
        
        // Check if this is a parent key (value is {)
        if (after == "{") {
            // This is a fen key
            current_fen = before;
            current_type = "";
        }
        // Check for expectation keys
        else if (before.find("should_be_higher_by") != std::string::npos) {
            // Use the FULL key as the type (e.g., "tal_init_should_be_higher_by")
            // Replace spaces with underscores for valid key
            std::string type = before;
            std::replace(type.begin(), type.end(), ' ', '_');
            
            // Parse value
            try {
                int val = std::stoi(after);
                if (!current_fen.empty() && !type.empty()) {
                    expectations[current_fen][type] = val;
                }
            } catch (...) {}
        }
    }
    
    return expectations;
}

// Run expectations check
void run_expectations_mode(const std::string& personalities_list, const std::string& evalfile, const std::string& expectations_file) {
    std::vector<std::string> personalities = parse_personality_list(personalities_list);
    
    if (personalities.size() < 2) {
        std::cerr << "Error: --expectations requires at least 2 personalities" << std::endl;
        return;
    }
    
    std::cout << "=== Running expectations check ===" << std::endl;
    
    // Load expectations
    auto expectations = load_expectations(expectations_file);
    
    // Load FENs
    std::vector<std::pair<std::string, std::string>> fens;
    std::string resolved_path = Evaluation::get_file_path(evalfile);
    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        resolved_path = evalfile;
        file.open(resolved_path);
    }
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << evalfile << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pipe_pos = line.find('|');
        if (pipe_pos == std::string::npos) continue;
        fens.push_back({line.substr(0, pipe_pos), line.substr(pipe_pos + 1)});
    }
    
    int passed = 0, failed = 0;
    
    // Check each expectation
    for (const auto& fen_pair : fens) {
        std::string fen_id = fen_pair.first;
        
        if (expectations.find(fen_id) == expectations.end()) continue;
        
        // Evaluate with first personality (petrosian)
        Evaluation::load_personality(personalities[0], false);
        int petrosian_score = eval_fen(fen_pair.second).total;
        
        // Evaluate with second personality (tal)  
        Evaluation::load_personality(personalities[1], false);
        int tal_score = eval_fen(fen_pair.second).total;
        
        // Clamp suspicious scores
        if (abs(petrosian_score) > 5000) petrosian_score = (petrosian_score > 0) ? 5000 : -5000;
        if (abs(tal_score) > 5000) tal_score = (tal_score > 0) ? 5000 : -5000;
        
            // delta = tal - petrosian (positive = tal higher)
        int delta = tal_score - petrosian_score;
        
        // Also calculate initiative_persist component delta
        int petrosian_init = 0, tal_init = 0;
        
        // Re-evaluate to get initiative component
        Evaluation::load_personality("petrosian", false);
        Evaluation::ScoreBreakdown bd_p = eval_fen(fen_pair.second);
        petrosian_init = bd_p.initiative_persist;
        
        Evaluation::load_personality("tal", false);
        Evaluation::ScoreBreakdown bd_t = eval_fen(fen_pair.second);
        tal_init = bd_t.initiative_persist;
        
        int init_delta = tal_init - petrosian_init;
        
        auto& exp = expectations[fen_id];
        
        // Check each expectation
        for (const auto& e : exp) {
            std::string type = e.first;
            int expected_delta = e.second;
            
            bool test_passed = false;
            std::string note;
            
            // delta = tal - petrosian
            if (type == "tal_higher") {
                // Tal should be higher in TOTAL score
                if (delta >= expected_delta) {
                    test_passed = true;
                    note = "Tal correctly higher by " + std::to_string(delta);
                } else {
                    note = "FAILED: Tal should be higher by " + std::to_string(expected_delta) + ", was " + std::to_string(delta);
                }
            } else if (type == "petrosian_higher") {
                // Petrosian should be higher in TOTAL score
                if (delta <= -expected_delta) {
                    test_passed = true;
                    note = "Petrosian correctly higher by " + std::to_string(-delta);
                } else {
                    note = "FAILED: Petrosian should be higher by " + std::to_string(expected_delta) + ", was " + std::to_string(-delta);
                }
            } else if (type == "tal_init_should_be_higher_by") {
                // Tal should have HIGHER init_persist component
                if (init_delta >= expected_delta) {
                    test_passed = true;
                    note = "Tal init_persist correctly higher by " + std::to_string(init_delta);
                } else {
                    note = "FAILED: Tal init_persist should be higher by " + std::to_string(expected_delta) + ", was " + std::to_string(init_delta);
                }
            } else if (type == "petrosian_init_should_be_higher_by") {
                // Petrosian should have HIGHER init_persist component
                if (init_delta <= -expected_delta) {
                    test_passed = true;
                    note = "Petrosian init_persist correctly higher by " + std::to_string(-init_delta);
                } else {
                    note = "FAILED: Petrosian init_persist should be higher by " + std::to_string(expected_delta) + ", was " + std::to_string(-init_delta);
                }
            }
            
            if (test_passed) {
                passed++;
                std::cout << "[PASS] " << fen_id << ": " << note << std::endl;
            } else {
                failed++;
                std::cout << "[FAIL] " << fen_id << ": " << note << std::endl;
            }
        }
    }
    
    std::cout << std::endl;
    std::cout << "=== Results: " << passed << " passed, " << failed << " failed ===" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "FutureChamp" << std::endl;
    std::cout << "A chess engine that thinks like a coach." << std::endl;
    std::cout << std::endl;
    
    // Set executable path for relative file resolution
    if (argc > 0) {
        Evaluation::set_exe_path(argv[0]);
    }
    
    // Check for modes
    std::string evalfile;
    std::string personality;
    std::string compare;
    std::string expectations;
    std::string personalities = "petrosian,tal";  // default
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--evalfile" && i + 1 < argc) {
            evalfile = argv[++i];
        } else if (arg == "--personality" && i + 1 < argc) {
            personality = argv[++i];
        } else if (arg == "--compare" && i + 1 < argc) {
            compare = argv[++i];
        } else if (arg == "--expectations" && i + 1 < argc) {
            expectations = argv[++i];
        } else if (arg == "--personalities" && i + 1 < argc) {
            personalities = argv[++i];
        } else if (arg == "perft" || arg == "--perft") {
            // Perft mode: FutureChamp.exe perft <depth> [fen]
            int depth = 4;
            std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            if (i + 1 < argc) {
                depth = std::stoi(argv[++i]);
            }
            if (i + 1 < argc && argv[i+1][0] != '-') {
                fen = argv[++i];
            }
            Board b;
            b.set_from_fen(fen);
            Search::perft(b, depth);
            return 0;
        }
    }
    
    if (!compare.empty() && !evalfile.empty()) {
        run_compare_mode(personalities, evalfile);
        return 0;
    }
    
    if (!expectations.empty() && !evalfile.empty()) {
        run_expectations_mode(personalities, evalfile, expectations);
        return 0;
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
