/**
 * Evaluation Function - Silman-Based Human-Like Assessment
 * 
 * Evaluates positions using human concepts:
 * - Material (but not king safety)
 * - Piece activity and harmony
 * - Pawn structure (weaknesses, passed pawns)
 * - Space advantages
 * - Initiative and dynamics
 * - Strategic imbalances
 * 
 * Reference: Jeremy Silman "How to Reassess Your Chess"
 */

#ifndef EVALUATION_HPP
#define EVALUATION_HPP

#include <string>
#include <vector>

// Forward declaration: Tell the compiler Board exists without including board.hpp
struct Board;

namespace Evaluation {

// Piece values (centipawns)
constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;
constexpr int KING_VALUE = 0;  // King safety handled separately

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
    // Material
    int material_diff = 0;
    
    // Minor piece evaluation
    bool white_has_better_minor = false;
    bool black_has_better_minor = false;
    
    // Pawn structure
    int white_weak_pawns = 0;
    int black_weak_pawns = 0;
    bool white_has_passed_pawn = false;
    bool black_has_passed_pawn = false;
    bool white_has_isolated_pawn = false;
    bool black_has_isolated_pawn = false;
    
    // Space
    float white_space = 0.0f;
    float black_space = 0.0f;
    
    // Initiative
    bool white_has_initiative = false;
    bool black_has_initiative = false;
    
    // Development (early game)
    int white_development_score = 0;
    int black_development_score = 0;
    
    // King safety
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

bool is_opening(const Board& board);

// Analyze imbalances
Imbalances analyze_imbalances(const std::string& fen = "");

// Get verbal explanation
VerbalExplanation explain(int score, const std::string& fen = "");

// Set style weights
void set_style(const std::string& style_name);

// Get current style name
std::string get_style_name();

} // namespace Evaluation

#endif // EVALUATION_HPP
