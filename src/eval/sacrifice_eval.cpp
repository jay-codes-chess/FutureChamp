/**
 * Sacrifice Justification Evaluation (Tal-mode)
 * Reduces penalty for material down when strong attack momentum exists
 */

#include "sacrifice_eval.hpp"

namespace Evaluation {

// Compute game phase (0-24 scale, 0=endgame, 24=opening)
static int get_game_phase(const Board& board) {
    int phase = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int p = board.piece_at(sq);
        switch (p) {
            case QUEEN:  phase += 4; break;
            case ROOK:   phase += 2; break;
            case BISHOP: 
            case KNIGHT: phase += 1; break;
            default: break;
        }
    }
    return phase;  // 0-24
}

int evaluate_sacrifice_justification(
    const Board& board,
    int material_score,
    int attack_momentum_score)
{
    // Only apply if attack momentum magnitude is meaningful
    if (std::abs(attack_momentum_score) < 10) {
        return 0;
    }
    
    // Compute middlegame factor (1.0 = full middlegame, 0.0 = endgame)
    int phase = get_game_phase(board);
    float mg_factor = 1.0f;
    if (phase >= 16) {
        mg_factor = 1.0f;  // Opening/middlegame
    } else if (phase <= 8) {
        mg_factor = 0.0f;  // Endgame
    } else {
        mg_factor = static_cast<float>(phase - 8) / 8.0f;
    }
    
    // Don't apply in endgame
    if (mg_factor <= 0.35f) {
        return 0;
    }
    
    int bonus = 0;
    
    // White is attacking but down material
    if (material_score < 0 && attack_momentum_score > 0) {
        int pressure = attack_momentum_score;
        bonus = std::min(pressure, 30);
    }
    // Black is attacking but down material
    else if (material_score > 0 && attack_momentum_score < 0) {
        int pressure = -attack_momentum_score;
        bonus = -std::min(pressure, 30);
    }
    
    // Scale by middlegame factor
    bonus = static_cast<int>(bonus * mg_factor);
    
    return bonus;
}

// Detect if a side is down an exchange (rook vs minor)
static bool is_down_exchange(const Board& board, int color) {
    int our_rooks = 0, enemy_rooks = 0;
    int our_minors = 0, enemy_minors = 0;
    
    for (int sq = 0; sq < 64; ++sq) {
        int p = board.piece_at(sq);
        int c = board.color_at(sq);
        if (c == color) {
            if (p == ROOK) our_rooks++;
            else if (p == BISHOP || p == KNIGHT) our_minors++;
        } else if (c >= 0 && c != color) {
            if (p == ROOK) enemy_rooks++;
            else if (p == BISHOP || p == KNIGHT) enemy_minors++;
        }
    }
    
    // Exchange down: we have fewer rooks but more minors
    return (our_rooks < enemy_rooks && our_minors > enemy_minors);
}

int evaluate_exchange_sac_tolerance(
    const Board& board,
    int attack_momentum_score,
    int line_opening_score,
    int king_tropism_score)
{
    // Only apply if significant attack momentum
    if (std::abs(attack_momentum_score) < 15) {
        return 0;
    }
    
    // Check if there's line opening or king tropism indicating real attack
    int attack_signal = std::abs(line_opening_score) + std::abs(king_tropism_score);
    if (attack_signal < 10) {
        return 0;
    }
    
    // Compute middlegame factor
    int phase = get_game_phase(board);
    float mg_factor = 1.0f;
    if (phase >= 16) {
        mg_factor = 1.0f;
    } else if (phase <= 8) {
        mg_factor = 0.0f;
    } else {
        mg_factor = static_cast<float>(phase - 8) / 8.0f;
    }
    
    // Don't apply in endgame
    if (mg_factor <= 0.35f) {
        return 0;
    }
    
    // Don't apply if queens are off the board
    bool white_queen = false, black_queen = false;
    for (int sq = 0; sq < 64; ++sq) {
        if (board.piece_at(sq) == QUEEN) {
            if (board.color_at(sq) == WHITE) white_queen = true;
            else black_queen = true;
        }
    }
    if (!white_queen || !black_queen) {
        return 0;
    }
    
    // Check each side's exchange status and compute tolerance
    int tolerance = 0;
    
    // White's tolerance (if white is down exchange but attacking)
    bool white_down_exchange = is_down_exchange(board, WHITE);
    if (white_down_exchange && attack_momentum_score > 0) {
        int pressure = attack_momentum_score;
        tolerance += std::min(60, (pressure - 10) * 2);
    }
    
    // Black's tolerance (if black is down exchange but attacking)
    bool black_down_exchange = is_down_exchange(board, BLACK);
    if (black_down_exchange && attack_momentum_score < 0) {
        int pressure = -attack_momentum_score;
        tolerance -= std::min(60, (pressure - 10) * 2);
    }
    
    // Scale by middlegame factor
    tolerance = static_cast<int>(tolerance * mg_factor);
    
    // Cap at ±60
    if (tolerance > 60) tolerance = 60;
    if (tolerance < -60) tolerance = -60;
    
    return tolerance;
}

} // namespace Evaluation
