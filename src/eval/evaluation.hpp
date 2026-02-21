/**
 * Evaluation Function - Silman-Based Human-Like Assessment
 * 
 * Modular layered architecture:
 * - Material
 * - Pawn Structure
 * - Piece Activity
 * - King Safety
 * - Imbalance
 * - Initiative
 * 
 * Reference: Jeremy Silman "How to Reassess Your Chess"
 */

#ifndef EVALUATION_HPP
#define EVALUATION_HPP

#include <string>
#include <vector>

// Forward declaration
struct Board;

namespace Evaluation {

// Forward declarations for layer functions
int evaluate_material(const Board& board);
int evaluate_pawn_structure(const Board& board);
int evaluate_piece_activity(const Board& board);
int evaluate_king_safety(const Board& board);
int evaluate_imbalance(const Board& board);
int evaluate_initiative(const Board& board);

// Check if position is in opening phase
bool is_opening(const Board& board);

// Score breakdown for debugging/trace
struct ScoreBreakdown {
    int material = 0;
    int pawn_structure = 0;
    int piece_activity = 0;
    int king_safety = 0;
    int imbalance = 0;
    int initiative = 0;
    int knowledge = 0;
    // Master concepts (part of knowledge)
    int exchange_sac = 0;
    int color_complex = 0;
    int pawn_lever = 0;
    int initiative_persist = 0;
    int initiative_persist_raw = 0;  // Before personality scaling
    // Aggressive attack evaluation
    int king_tropism = 0;
    int pawn_storm = 0;
    int line_opening = 0;
    int aggressive_initiative = 0;
    int attack_momentum = 0;
    int sacrifice_justification = 0;
    int total = 0;
};

// Evaluation weights for different styles
struct StyleWeights {
    float material = 1.0f;
    float piece_activity = 0.5f;
    float pawn_structure = 0.5f;
    float space = 0.3f;
    float initiative = 0.4f;
    float king_safety = 0.6f;
    float development = 0.3f;
    float prophylaxis = 0.4f;
};

// Imbalance analysis structure
struct Imbalances {
    int material_diff = 0;
    bool white_has_better_minor = false;
    bool black_has_better_minor = false;
    int white_weak_pawns = 0;
    int black_weak_pawns = 0;
    bool white_has_passed_pawn = false;
    bool black_has_passed_pawn = false;
    bool white_has_isolated_pawn = false;
    bool black_has_isolated_pawn = false;
    float white_space = 0.0f;
    float black_space = 0.0f;
    bool white_has_initiative = false;
    bool black_has_initiative = false;
    int white_development_score = 0;
    int black_development_score = 0;
    int white_king_safety = 0;
    int black_king_safety = 0;
};

// Verbal explanation for PV
struct VerbalExplanation {
    std::vector<std::string> move_reasons;
    std::vector<std::string> imbalance_notes;
};

// Initialize evaluation tables
void initialize();

// Get evaluation for position (centipawns from White's perspective)
int evaluate(const std::string& fen = "");

// Efficient version that takes a Board directly (for use in search)
int evaluate(const Board& board);

// Evaluate with full breakdown (for debugging)
ScoreBreakdown evaluate_with_breakdown(const Board& board);

// Analyze imbalances
Imbalances analyze_imbalances(const std::string& fen = "");

// Get verbal explanation
VerbalExplanation explain(int score, const std::string& fen = "");

// Set style weights
void set_style(const std::string& style_name);

// Get current style name
std::string get_style_name();

// Debug trace control
void set_debug_trace(bool enabled);
bool get_debug_trace();

// Evaluate at root with trace output (called from search)
int evaluate_at_root(const Board& board);

} // namespace Evaluation

#endif // EVALUATION_HPP
