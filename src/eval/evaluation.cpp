/**
 * Human Chess Engine - Evaluation Function
 * Modular layered architecture
 */

#include "evaluation.hpp"
#include "material.hpp"
#include "pawn_structure.hpp"
#include "piece_activity.hpp"
#include "king_safety.hpp"
#include "imbalance.hpp"
#include "initiative.hpp"
#include "params.hpp"
#include "knowledge.hpp"
#include "../utils/board.hpp"
#include <iostream>
#include <sstream>

namespace Evaluation {

// Evaluation mode counters for diagnostics
static uint64_t g_eval_mode_fast = 0;
static uint64_t g_eval_mode_med = 0;
static uint64_t g_eval_mode_full = 0;

static StyleWeights current_weights;
static std::string current_style = "classical";
static bool debug_trace_enabled = false;

// Forward declarations for internal helpers
bool is_opening(const Board& board) {
    int total_material = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int type = board.piece_at(sq);
        if (type == PAWN) total_material += 100;
        else if (type == KNIGHT) total_material += 320;
        else if (type == BISHOP) total_material += 330;
        else if (type == ROOK) total_material += 500;
        else if (type == QUEEN) total_material += 900;
    }
    return total_material > 4000;
}

// ──────────────────────────────────────────────
//   Public API
// ──────────────────────────────────────────────

ScoreBreakdown evaluate_with_breakdown(const Board& board) {
    ScoreBreakdown bd;
    const auto& p = get_params();
    
    // Evaluate each layer
    bd.material = evaluate_material(board);
    bd.pawn_structure = evaluate_pawn_structure(board);
    bd.piece_activity = evaluate_piece_activity(board);
    bd.king_safety = evaluate_king_safety(board);
    bd.imbalance = evaluate_imbalance(board);
    bd.initiative = evaluate_initiative(board);
    bd.knowledge = evaluate_knowledge(board, p);
    
    // Individual master concept scores for trace
    bd.exchange_sac = eval_exchange_sac_compensation(board, p);
    bd.color_complex = eval_weak_color_complex(board, p);
    bd.pawn_lever = eval_pawn_lever_timing(board, p);
    bd.initiative_persist_raw = eval_initiative_persistence(board, p);  // Raw before scaling
    bd.initiative_persist = bd.initiative_persist_raw * p.concept_initiative_persist_weight / 100;  // Scaled
    
    // Apply params weights
    // Scale factor (100 = 1.0)
    float scale = p.w_imbalance / 100.0f;
    
    int score = 0;
    score += bd.material;  // Material is absolute
    score += static_cast<int>(bd.piece_activity * p.w_piece_activity / 100.0f);
    score += static_cast<int>(bd.pawn_structure * p.w_pawn_structure / 100.0f);
    score += static_cast<int>(bd.imbalance * scale);  // ImbalanceScale applies here
    score += static_cast<int>(bd.king_safety * p.w_king_safety / 100.0f);
    // Initiative with dominance multiplier
    int initiative_score = bd.initiative;
    if (p.initiative_dominance != 100) {
        initiative_score = initiative_score * p.initiative_dominance / 100;
    }
    score += static_cast<int>(initiative_score * p.w_initiative / 100.0f);
    score += bd.initiative_persist;  // Add scaled initiative persist concept
    score += bd.knowledge;  // Already weighted internally
    
    // Tempo
    if (board.side_to_move == WHITE) score += 10;
    else score -= 10;
    
    // **TradeBias "simplify when better"**: Encourage/discourage trading when ahead
    // Small effect (10-30cp) - this is a style nudge, not tactical
    int simplify_factor = 15;  // centipawns
    int material_balance = bd.material;  // positive = white ahead
    int trade_bias = get_params().trade_bias;
    
    if (trade_bias != 100 && material_balance > 100) {
        // White ahead - TradeBias > 100 encourages simplifying (trading)
        int simplify_bonus = (trade_bias - 100) * simplify_factor / 100;
        score += simplify_bonus;
    } else if (trade_bias != 100 && material_balance < -100) {
        // Black ahead - TradeBias > 100 encourages simplifying
        int simplify_bonus = (trade_bias - 100) * simplify_factor / 100;
        score -= simplify_bonus;
    }
    
    bd.total = score;
    
    return bd;
}

// Efficient version that takes a Board directly (for search)
int evaluate(const Board& board) {
    return evaluate_with_breakdown(board).total;
}

// **TIERED EVAL**: Evaluate with specified mode for speed control
int evaluate(const Board& board, EvalMode mode) {
    // Count calls by mode
    if (mode == EvalMode::FAST) g_eval_mode_fast++;
    else if (mode == EvalMode::MED) g_eval_mode_med++;
    else g_eval_mode_full++;
    
    const auto& p = get_params();
    
    // Always evaluate material (foundation)
    int material = evaluate_material(board);
    
    if (mode == EvalMode::FAST) {
        // FAST: Material + PST + basic king safety + simple pawn count
        ScoreBreakdown bd;
        bd.material = material;
        bd.pawn_structure = evaluate_pawn_structure(board);  // Basic pawn eval
        bd.king_safety = evaluate_king_safety(board);
        
        int score = 0;
        score += bd.material;
        score += static_cast<int>(bd.pawn_structure * p.w_pawn_structure / 100.0f);
        score += static_cast<int>(bd.king_safety * p.w_king_safety / 100.0f);
        
        // Tempo
        if (board.side_to_move == WHITE) score += 10;
        else score -= 10;
        
        return score;
    }
    
    if (mode == EvalMode::MED) {
        // MED: FAST + pawn structure + piece activity
        ScoreBreakdown bd;
        bd.material = material;
        bd.pawn_structure = evaluate_pawn_structure(board);
        bd.king_safety = evaluate_king_safety(board);
        bd.piece_activity = evaluate_piece_activity(board);
        
        int score = 0;
        score += bd.material;
        score += static_cast<int>(bd.pawn_structure * p.w_pawn_structure / 100.0f);
        score += static_cast<int>(bd.king_safety * p.w_king_safety / 100.0f);
        score += static_cast<int>(bd.piece_activity * p.w_piece_activity / 100.0f);
        
        // Tempo
        if (board.side_to_move == WHITE) score += 10;
        else score -= 10;
        
        return score;
    }
    
    // FULL: MED + all components
    return evaluate_with_breakdown(board).total;
}

// Get evaluation mode counters (for diagnostics)
void get_mode_counts(uint64_t& fast, uint64_t& med, uint64_t& full) {
    fast = g_eval_mode_fast;
    med = g_eval_mode_med;
    full = g_eval_mode_full;
}

// FEN string version (for UCI commands)
int evaluate(const std::string& fen) {
    Board board;
    if (!fen.empty()) board.set_from_fen(fen);
    
    // If debug trace is enabled, print breakdown for UCI "eval" command
    if (debug_trace_enabled) {
        return evaluate_at_root(board);
    }
    
    return evaluate(board);
}

Imbalances analyze_imbalances(const std::string& fen) {
    Board board;
    if (!fen.empty()) board.set_from_fen(fen);
    
    Imbalances imb{};
    imb.material_diff = evaluate_material(board);
    imb.white_king_safety = evaluate_king_safety(board);
    imb.black_king_safety = -evaluate_king_safety(board);  // Flip for black
    
    // Simple versions - would need full refactor for complete data
    imb.white_space = 0.0f;
    imb.black_space = 0.0f;
    
    return imb;
}

VerbalExplanation explain(int score, const std::string& fen) {
    VerbalExplanation exp;
    Imbalances imb = analyze_imbalances(fen);
    
    if (imb.material_diff > 120)
        exp.move_reasons.emplace_back("White has a clear material advantage");
    else if (imb.material_diff < -120)
        exp.move_reasons.emplace_back("Black has a clear material advantage");
    
    if (score > 40) exp.move_reasons.emplace_back("White has the better position overall");
    if (score < -40) exp.move_reasons.emplace_back("Black has the better position overall");
    
    return exp;
}

void initialize() { 
    set_style("classical"); 
    init_pawn_hash(16384);
}

void set_style(const std::string& style_name) {
    current_style = style_name;
    
    if (style_name == "classical") {
        current_weights = {
            1.0f, 0.3f, 0.8f, 0.1f, 0.4f, 1.0f, 0.2f, 0.4f
        };
    } else if (style_name == "attacking") {
        current_weights = {0.8f, 0.8f, 0.4f, 0.4f, 1.0f, 0.3f, 0.2f, 0.2f};
    } else if (style_name == "positional") {
        current_weights = {1.0f, 0.6f, 0.8f, 0.6f, 0.3f, 0.5f, 0.4f, 0.6f};
    } else {
        current_weights = {1.0f, 0.5f, 0.5f, 0.3f, 0.4f, 0.6f, 0.3f, 0.4f};
    }
}

std::string get_style_name() { return current_style; }

void set_debug_trace(bool enabled) { debug_trace_enabled = enabled; }
bool get_debug_trace() { return debug_trace_enabled; }

// Evaluate at root with trace output (for search to call at root)
int evaluate_at_root(const Board& board) {
    ScoreBreakdown bd = evaluate_with_breakdown(board);
    
    // Print trace if enabled
    if (debug_trace_enabled) {
        const auto& p = get_params();
        std::ostringstream oss;
        oss << "EVAL material=" << bd.material 
            << " pawns=" << bd.pawn_structure 
            << " activity=" << bd.piece_activity 
            << " king=" << bd.king_safety 
            << " imbalance=" << bd.imbalance 
            << " init=" << bd.initiative 
            << " knowledge=" << bd.knowledge
            << " exchange_sac=" << bd.exchange_sac
            << " color_complex=" << bd.color_complex
            << " pawn_lever=" << bd.pawn_lever
            << " init_persist=" << bd.initiative_persist
            << " init_persist_raw=" << bd.initiative_persist_raw
            << " total=" << bd.total;
        
        // Optionally include params in trace
        if (p.debug_trace_with_params) {
            oss << " | Personality=" << p.current_personality 
                << " AutoLoad=" << (p.personality_auto_load ? "1" : "0")
                << " W_pawn=" << p.w_pawn_structure 
                << " W_act=" << p.w_piece_activity 
                << " W_king=" << p.w_king_safety 
                << " W_init=" << p.w_initiative 
                << " W_imb=" << p.w_imbalance 
                << " W_know=" << p.w_knowledge_concepts
                << " C_outpost=" << p.concept_outpost_weight
                << " C_badbis=" << p.concept_bad_bishop_weight
                << " C_space=" << p.concept_space_weight
                << " C_exch=" << p.concept_exchange_sac_weight
                << " C_color=" << p.concept_color_complex_weight
                << " C_lever=" << p.concept_pawn_lever_weight
                << " C_init_persist=" << p.concept_initiative_persist_weight
                << " InitDom=" << p.initiative_dominance
                << " ImbScale=" << p.imbalance_scale;
        }
        
        std::cout << "info string " << oss.str() << std::endl;
    }
    
    return bd.total;
}

} // namespace Evaluation
