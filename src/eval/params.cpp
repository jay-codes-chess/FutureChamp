/**
 * Parameters Registry Implementation
 */

#include "params.hpp"
#include <sstream>
#include <iomanip>

namespace Evaluation {

// Global params instance
static Params global_params;

Params& get_params() {
    return global_params;
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
        } else if (name == "BadBishopPenalty") {
            global_params.bad_bishop_penalty = std::stoi(value);
        } else if (name == "SpaceConceptWeight") {
            global_params.space_concept_weight = std::stoi(value);
        } else if (name == "W_KnowledgeConcepts") {
            global_params.w_knowledge_concepts = std::stoi(value);
        } else if (name == "BishopPairBonus") {
            global_params.bishop_pair_bonus = std::stoi(value);
        } else if (name == "RookOpenFileBonus") {
            global_params.rook_open_file_bonus = std::stoi(value);
        } else if (name == "PassedPawnBonus") {
            global_params.passed_pawn_bonus = std::stoi(value);
        } else if (name == "PawnShieldPenalty") {
            global_params.pawn_shield_penalty = std::stoi(value);
        }
        // Search / Humanisation
        else if (name == "CandidateMarginCp") {
            global_params.candidate_margin_cp = std::stoi(value);
        } else if (name == "CandidateMovesMax") {
            global_params.candidate_moves_max = std::stoi(value);
        } else if (name == "HumanEnable") {
            global_params.human_enable = (value == "true");
        } else if (name == "HumanTemperature") {
            global_params.human_temperature = std::stoi(value);
        } else if (name == "HumanNoiseCp") {
            global_params.human_noise_cp = std::stoi(value);
        } else if (name == "HumanBlunderRate") {
            global_params.human_blunder_rate = std::stoi(value);
        } else if (name == "RandomSeed") {
            global_params.random_seed = std::stoi(value);
        }
        // Debug
        else if (name == "DebugTraceWithParams") {
            global_params.debug_trace_with_params = (value == "true");
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
    oss << "BadBishopPenalty=" << p.bad_bishop_penalty << std::endl;
    oss << "SpaceConceptWeight=" << p.space_concept_weight << std::endl;
    oss << "W_KnowledgeConcepts=" << p.w_knowledge_concepts << std::endl;
    oss << "BishopPairBonus=" << p.bishop_pair_bonus << std::endl;
    oss << "RookOpenFileBonus=" << p.rook_open_file_bonus << std::endl;
    oss << "PassedPawnBonus=" << p.passed_pawn_bonus << std::endl;
    oss << "PawnShieldPenalty=" << p.pawn_shield_penalty << std::endl;
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
