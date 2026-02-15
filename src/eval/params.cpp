/**
 * Parameters Registry Implementation
 */

#include "params.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace Evaluation {

// Store executable directory for relative path resolution
static std::string g_exe_path;

void set_exe_path(const std::string& path) {
    g_exe_path = path;
}

std::string get_exe_path() {
    return g_exe_path;
}

// Global params instance
static Params global_params;

Params& get_params() {
    return global_params;
}

// Get full path for a file (relative to exe or absolute)
std::string get_file_path(const std::string& relative_path) {
    if (!g_exe_path.empty()) {
        // Try exe directory first
        std::string exe_dir = g_exe_path;
        size_t last_slash = exe_dir.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            exe_dir = exe_dir.substr(0, last_slash);
        }
        std::string full_path = exe_dir + "/" + relative_path;
        std::ifstream test(full_path);
        if (test.is_open()) {
            test.close();
            return full_path;
        }
    }
    // Fall back to current working directory
    return relative_path;
}

// Clamp value to min/max range
int clamp_value(int value, int min_val, int max_val) {
    return std::max(min_val, std::min(max_val, value));
}

// Load personality from text file (Rodent-style format)
bool load_personality_text(const std::string& filepath, bool verbose) {
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    int applied = 0, ignored = 0;
    std::string personality_name;
    
    // Parse line-by-line: key = value
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;  // Empty line
        if (line[start] == '#' || line[start] == '/') continue;  // Comment
        
        // Find '='
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;
        
        std::string key = line.substr(start, eq_pos - start);
        std::string value = line.substr(eq_pos + 1);
        
        // Trim whitespace from key and value
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(value.begin());
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
        
        if (key.empty() || value.empty()) continue;
        
        // Special handling for Name field
        if (key == "Name") {
            personality_name = value;
            continue;
        }
        
        // Use the same set_param function as UCI to ensure consistent behavior
        if (set_param(key, value)) {
            applied++;
        } else {
            ignored++;
            if (verbose) {
                std::cout << "info string Warning: Unknown personality key: " << key << std::endl;
            }
        }
    }
    
    // Set the personality name
    if (!personality_name.empty()) {
        global_params.current_personality = personality_name;
    }
    
    if (verbose) {
        std::string display_name = personality_name.empty() ? filepath : personality_name;
        std::cout << "info string Loaded personality: " << display_name 
                  << " (" << applied << " options applied, " << ignored << " ignored)" << std::endl;
    }
    
    return true;
}

// Load personality from explicit file path
bool load_personality_file(const std::string& filepath, bool verbose) {
    // Check file extension
    if (filepath.size() >= 5 && filepath.substr(filepath.size() - 5) == ".json") {
        // JSON file - extract name from filepath
        size_t slash_pos = filepath.find_last_of("/\\");
        std::string name = (slash_pos != std::string::npos) ? filepath.substr(slash_pos + 1) : filepath;
        if (name.size() >= 5) name = name.substr(0, name.size() - 5);  // remove .json
        
        // Use existing JSON loader
        return load_personality(name, verbose);
    }
    
    // Text file
    return load_personality_text(filepath, verbose);
}

// Load personality from JSON or text file
bool load_personality(const std::string& name, bool verbose) {
    // First try: text file (.txt)
    std::string txt_path = get_file_path("./personalities/" + name + ".txt");
    if (load_personality_text(txt_path, verbose)) {
        global_params.current_personality = name;
        return true;
    }
    
    // Try alternative path for text
    txt_path = get_file_path("personalities/" + name + ".txt");
    if (load_personality_text(txt_path, verbose)) {
        global_params.current_personality = name;
        return true;
    }
    
    // Fallback: JSON file
    std::string filename = get_file_path("./personalities/" + name + ".json");
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        // Try alternative path
        filename = get_file_path("personalities/" + name + ".json");
        file.open(filename);
    }
    
    if (!file.is_open()) {
        std::cerr << "Failed to open personality file: " << filename << std::endl;
        return false;
    }
    
    int applied = 0, ignored = 0;
    
    // Simple JSON parsing (key: value pairs)
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and braces
        if (line.empty() || line == "{" || line == "}") continue;
        
        // Parse "key": value
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;
        
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \""));
        key.erase(key.find_last_not_of(" \"") + 1);
        value.erase(0, value.find_first_not_of(" \""));
        value.erase(value.find_last_not_of(" \"") + 1);
        
        // Try to set param (returns false if unknown key)
        if (set_param(key, value)) {
            applied++;
        } else {
            ignored++;
            if (verbose) {
                std::cout << "info string Warning: Unknown personality key: " << key << std::endl;
            }
        }
    }
    
    global_params.current_personality = name;
    
    if (verbose) {
        std::cout << "info string Loaded personality=" << name 
                  << " (" << applied << " options applied, " << ignored << " ignored)" << std::endl;
    }
    
    return true;
}

// Save current params to JSON file
bool save_personality(const std::string& name) {
    std::string filename = "./personalities/" + name + ".json";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to write personality file: " << filename << std::endl;
        return false;
    }
    
    const auto& p = global_params;
    
    file << "{\n";
    file << "  \"MaterialPriority\": " << p.material_priority << ",\n";
    file << "  \"ImbalanceScale\": " << p.imbalance_scale << ",\n";
    file << "  \"W_PawnStructure\": " << p.w_pawn_structure << ",\n";
    file << "  \"W_PieceActivity\": " << p.w_piece_activity << ",\n";
    file << "  \"W_KingSafety\": " << p.w_king_safety << ",\n";
    file << "  \"W_Initiative\": " << p.w_initiative << ",\n";
    file << "  \"W_Imbalance\": " << p.w_imbalance << ",\n";
    file << "  \"W_KnowledgeConcepts\": " << p.w_knowledge_concepts << ",\n";
    file << "  \"OutpostBonus\": " << p.outpost_bonus << ",\n";
    file << "  \"BishopPairBonus\": " << p.bishop_pair_bonus << ",\n";
    file << "  \"RookOpenFileBonus\": " << p.rook_open_file_bonus << ",\n";
    file << "  \"PassedPawnBonus\": " << p.passed_pawn_bonus << ",\n";
    file << "  \"PawnShieldPenalty\": " << p.pawn_shield_penalty << ",\n";
    file << "  \"ConceptOutpostWeight\": " << p.concept_outpost_weight << ",\n";
    file << "  \"ConceptBadBishopWeight\": " << p.concept_bad_bishop_weight << ",\n";
    file << "  \"ConceptSpaceWeight\": " << p.concept_space_weight << ",\n";
    file << "  \"HumanEnable\": " << (p.human_enable ? "true" : "false") << ",\n";
    file << "  \"HumanTemperature\": " << p.human_temperature << ",\n";
    file << "  \"HumanNoiseCp\": " << p.human_noise_cp << ",\n";
    file << "  \"HumanBlunderRate\": " << p.human_blunder_rate << ",\n";
    file << "  \"CandidateMarginCp\": " << p.candidate_margin_cp << ",\n";
    file << "  \"CandidateMovesMax\": " << p.candidate_moves_max << ",\n";
    file << "  \"RandomSeed\": " << p.random_seed << "\n";
    file << "}\n";
    
    return true;
}

bool set_param(const std::string& name, const std::string& value) {
    try {
        // Core Material / Imbalance
        if (name == "MaterialPriority") {
            global_params.material_priority = std::stoi(value);
        } else if (name == "ImbalanceScale") {
            global_params.imbalance_scale = std::stoi(value);
        } else if (name == "KnightValueBias") {
            global_params.knight_value_bias = std::stoi(value);
        } else if (name == "BishopValueBias") {
            global_params.bishop_value_bias = std::stoi(value);
        } else if (name == "ExchangeSacrificeSensitivity") {
            global_params.exchange_sensitivity = std::stoi(value);
        }
        // Eval Layer Weights
        else if (name == "W_PawnStructure") {
            global_params.w_pawn_structure = std::stoi(value);
        } else if (name == "W_PieceActivity") {
            global_params.w_piece_activity = std::stoi(value);
        } else if (name == "W_KingSafety") {
            global_params.w_king_safety = std::stoi(value);
        } else if (name == "W_Initiative") {
            global_params.w_initiative = std::stoi(value);
        } else if (name == "W_Imbalance") {
            global_params.w_imbalance = std::stoi(value);
        }
        // Key Micro Terms
        else if (name == "OutpostBonus") {
            global_params.outpost_bonus = std::stoi(value);
        } else if (name == "BishopPairBonus") {
            global_params.bishop_pair_bonus = std::stoi(value);
        } else if (name == "RookOpenFileBonus") {
            global_params.rook_open_file_bonus = std::stoi(value);
        } else if (name == "PassedPawnBonus") {
            global_params.passed_pawn_bonus = std::stoi(value);
        } else if (name == "PawnShieldPenalty") {
            global_params.pawn_shield_penalty = std::stoi(value);
        }
        // Knowledge Concept Weights
        else if (name == "W_KnowledgeConcepts") {
            global_params.w_knowledge_concepts = std::stoi(value);
        } else if (name == "ConceptOutpostWeight") {
            global_params.concept_outpost_weight = std::stoi(value);
        } else if (name == "ConceptBadBishopWeight") {
            global_params.concept_bad_bishop_weight = std::stoi(value);
        } else if (name == "ConceptSpaceWeight") {
            global_params.concept_space_weight = std::stoi(value);
        }
        // Master Concepts
        else if (name == "ConceptExchangeSacWeight") {
            global_params.concept_exchange_sac_weight = std::stoi(value);
        } else if (name == "ConceptColorComplexWeight") {
            global_params.concept_color_complex_weight = std::stoi(value);
        } else if (name == "ConceptPawnLeverWeight") {
            global_params.concept_pawn_lever_weight = std::stoi(value);
        } else if (name == "ConceptInitiativePersistWeight") {
            global_params.concept_initiative_persist_weight = std::stoi(value);
        } else if (name == "InitiativeDominance") {
            global_params.initiative_dominance = std::stoi(value);
        }
        // Search / Humanisation
        else if (name == "CandidateMarginCp") {
            global_params.candidate_margin_cp = std::stoi(value);
        } else if (name == "CandidateMovesMax") {
            global_params.candidate_moves_max = std::stoi(value);
        } else if (name == "HumanEnable") {
            global_params.human_enable = (value == "true");
        } else if (name == "HumanSelect") {
            global_params.human_select = (value == "true");
        } else if (name == "HumanTemperature") {
            global_params.human_temperature = std::stoi(value);
        } else if (name == "HumanNoiseCp") {
            global_params.human_noise_cp = std::stoi(value);
        } else if (name == "HumanBlunderRate") {
            global_params.human_blunder_rate = std::stoi(value);
        } else if (name == "RandomSeed") {
            global_params.random_seed = std::stoi(value);
        } else if (name == "RiskAppetite") {
            global_params.risk_appetite = std::stoi(value);
        } else if (name == "SacrificeBias") {
            global_params.sacrifice_bias = std::stoi(value);
        } else if (name == "SimplicityBias") {
            global_params.simplicity_bias = std::stoi(value);
        } else if (name == "TradeBias") {
            global_params.trade_bias = std::stoi(value);
        } else if (name == "HumanHardFloorCp") {
            global_params.human_hard_floor_cp = std::stoi(value);
        } else if (name == "HumanOpeningSanity") {
            global_params.human_opening_sanity = std::stoi(value);
        } else if (name == "HumanTopKOverride") {
            global_params.human_topk_override = std::stoi(value);
        }
        // Debug
        else if (name == "DebugTraceWithParams") {
            global_params.debug_trace_with_params = (value == "true");
        } else if (name == "DebugHumanPick") {
            global_params.debug_human_pick = (value == "true");
        }
        else {
            return false;  // Unknown param
        }
        return true;
    } catch (...) {
        return false;  // Parse error
    }
}

std::string dump_params() {
    std::ostringstream oss;
    const auto& p = global_params;
    
    oss << "=== EVAL PARAMS ===" << std::endl;
    oss << "MaterialPriority=" << p.material_priority << std::endl;
    oss << "ImbalanceScale=" << p.imbalance_scale << std::endl;
    oss << "KnightValueBias=" << p.knight_value_bias << std::endl;
    oss << "BishopValueBias=" << p.bishop_value_bias << std::endl;
    oss << "ExchangeSacrificeSensitivity=" << p.exchange_sensitivity << std::endl;
    oss << "W_PawnStructure=" << p.w_pawn_structure << std::endl;
    oss << "W_PieceActivity=" << p.w_piece_activity << std::endl;
    oss << "W_KingSafety=" << p.w_king_safety << std::endl;
    oss << "W_Initiative=" << p.w_initiative << std::endl;
    oss << "W_Imbalance=" << p.w_imbalance << std::endl;
    oss << "OutpostBonus=" << p.outpost_bonus << std::endl;
    oss << "BishopPairBonus=" << p.bishop_pair_bonus << std::endl;
    oss << "RookOpenFileBonus=" << p.rook_open_file_bonus << std::endl;
    oss << "PassedPawnBonus=" << p.passed_pawn_bonus << std::endl;
    oss << "PawnShieldPenalty=" << p.pawn_shield_penalty << std::endl;
    oss << "W_KnowledgeConcepts=" << p.w_knowledge_concepts << std::endl;
    oss << "ConceptOutpostWeight=" << p.concept_outpost_weight << std::endl;
    oss << "ConceptBadBishopWeight=" << p.concept_bad_bishop_weight << std::endl;
    oss << "ConceptSpaceWeight=" << p.concept_space_weight << std::endl;
    oss << "CandidateMarginCp=" << p.candidate_margin_cp << std::endl;
    oss << "CandidateMovesMax=" << p.candidate_moves_max << std::endl;
    oss << "HumanEnable=" << (p.human_enable ? "true" : "false") << std::endl;
    oss << "HumanTemperature=" << p.human_temperature << std::endl;
    oss << "HumanNoiseCp=" << p.human_noise_cp << std::endl;
    oss << "HumanBlunderRate=" << p.human_blunder_rate << std::endl;
    oss << "RandomSeed=" << p.random_seed << std::endl;
    oss << "DebugTraceWithParams=" << (p.debug_trace_with_params ? "true" : "false") << std::endl;
    
    return oss.str();
}

} // namespace Evaluation
