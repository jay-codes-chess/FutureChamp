/**
 * Human Selection - Implementation
 * 
 * Simplified deterministic version for initial testing
 */

#include "human_selection.hpp"
#include "search.hpp"
#include "../eval/evaluation.hpp"
#include "../utils/board.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace HumanSelection {

// Seeded random [0, 1)
double seeded_random(int seed) {
    static const unsigned long long a = 1103515245ULL;
    static const unsigned long long c = 12345ULL;
    static const unsigned long long m = 2147483647ULL;

    static unsigned long long state = 1;
    if (seed != 0) {
        state = seed;
    }
    
    state = (a * state + c) % m;
    return static_cast<double>(state) / static_cast<double>(m);
}

// Simple variance score (higher = more tactical)
int calculate_variance_score(void* board_ptr, int move) {
    int variance = 0;
    
    int to = Bitboards::move_to(move);
    int flags = Bitboards::move_flags(move);
    int promo = Bitboards::move_promotion(move);
    
    // Capture?
    if ((move >> 6 & 63) != (move & 63)) {
        // This is a simplification - we'd need to check actual board
        variance += 30;
    }
    
    // Promotion
    if (promo != 0) {
        variance += 40;
    }
    
    // Castling
    if (flags == MOVE_CASTLE) {
        variance += 20;
    }
    
    return variance;
}

// Collect candidate moves - simplified evaluation version
std::vector<CandidateMove> collect_candidates(
    void* board_ptr,
    int candidate_margin_cp,
    int candidate_moves_max,
    int max_depth
) {
    Board* board = static_cast<Board*>(board_ptr);
    std::vector<CandidateMove> candidates;
    
    // Generate all moves
    auto moves = board->generate_moves();
    
    // For each legal move, evaluate it using simple material + position
    for (int move : moves) {
        // Quick check: make a copy and see if king is in check
        Board temp = *board;
        
        // Try move using bitboard functions
        int from = Bitboards::move_from(move);
        int to = Bitboards::move_to(move);
        int piece = board->piece_at(from);
        
        // Skip if no piece on from square
        if (piece == 0) continue;
        
        // Make move on temp board
        temp.remove_piece(from);
        temp.add_piece(to, piece & 7, piece >> 3);
        
        // Skip if king is in check (likely illegal)
        if (temp.is_in_check(temp.side_to_move)) {
            continue;
        }
        
        // Simple evaluation (material + basic position)
        int score = Evaluation::evaluate(temp);
        candidates.emplace_back(move, score);
    }
    
    // Sort by score descending
    std::sort(candidates.begin(), candidates.end(), 
        [](const CandidateMove& a, const CandidateMove& b) {
            return a.score > b.score;
        });
    
    // Filter by margin and max
    if (!candidates.empty()) {
        int best_score = candidates[0].score;
        int margin = candidate_margin_cp;
        int hard_floor = best_score - 400;
        
        std::vector<CandidateMove> filtered;
        int count = 0;
        for (const auto& c : candidates) {
            if (count >= candidate_moves_max) break;
            if (c.score < hard_floor) break;
            if (best_score - c.score <= margin || c.score == best_score) {
                filtered.push_back(c);
                count++;
            }
        }
        candidates = filtered;
    }
    
    return candidates;
}

// Apply human selection and pick a move
int pick_human_move(
    void* board_ptr,
    std::vector<CandidateMove>& candidates,
    int best_score,
    int human_temperature,
    int human_noise_cp,
    int risk_appetite,
    int sacrifice_bias,
    int simplicity_bias,
    int random_seed,
    bool debug_output
) {
    if (candidates.empty()) {
        return 0;
    }
    
    if (candidates.size() == 1) {
        return candidates[0].move;
    }
    
    // Temperature: higher = more random
    double temperature = human_temperature / 100.0;
    
    // Calculate base weights using softmax
    double total_weight = 0;
    
    for (auto& c : candidates) {
        // Base weight from score difference
        double score_diff = (c.score - best_score) / 100.0;
        
        // Apply temperature
        double weight = std::exp(score_diff / (temperature + 0.01));
        
        // Apply noise
        if (human_noise_cp > 0) {
            double noise = (seeded_random(random_seed + c.move) - 0.5) * 2.0 * human_noise_cp / 100.0;
            weight *= std::exp(noise);
        }
        
        // Apply risk appetite (prefer tactical moves when high)
        if (risk_appetite > 100) {
            double risk_boost = (risk_appetite - 100) / 100.0;
            if (c.score < best_score) {
                weight *= (1.0 + risk_boost * 0.3);
            }
        } else if (risk_appetite < 100) {
            double risk_penalty = (100 - risk_appetite) / 100.0;
            if (c.score < best_score) {
                weight *= (1.0 - risk_penalty * 0.5);
            }
        }
        
        // Apply simplicity bias
        if (simplicity_bias > 100) {
            double simplicity_boost = (simplicity_bias - 100) / 100.0;
            if (c.score < best_score - 50) {
                weight *= (1.0 - simplicity_boost * 0.3);
            }
        }
        
        c.weight = weight;
        total_weight += weight;
    }
    
    // Normalize to probabilities
    for (auto& c : candidates) {
        c.probability = c.weight / total_weight;
    }
    
    if (debug_output) {
        std::cerr << "HUMAN_PICK candidates=" << candidates.size() 
                  << " best=" << best_score 
                  << " temp=" << human_temperature 
                  << " margin=" << (best_score - candidates.back().score) << std::endl;
        for (size_t i = 0; i < std::min(candidates.size(), size_t(5)); i++) {
            const auto& c = candidates[i];
            std::cerr << "  " << Bitboards::move_to_uci(c.move) << " score=" << c.score 
                      << " prob=" << (c.probability * 100) << "%" << std::endl;
        }
    }
    
    // Sample using seeded random
    double r = seeded_random(random_seed + 12345);
    double cumulative = 0;
    
    for (const auto& c : candidates) {
        cumulative += c.probability;
        if (r <= cumulative) {
            if (debug_output) {
                std::cerr << "HUMAN_PICK chosen=" << Bitboards::move_to_uci(c.move) 
                          << " score=" << c.score << std::endl;
            }
            return c.move;
        }
    }
    
    // Fallback to best move
    return candidates[0].move;
}

} // namespace HumanSelection
