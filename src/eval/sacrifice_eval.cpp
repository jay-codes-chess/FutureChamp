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

} // namespace Evaluation
