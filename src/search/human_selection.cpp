/**
 * Human Selection - Implementation
 * 
 * GUARDRAILS:
 * - HumanHardFloorCp: drop candidates worse than (best - floor)
 * - HumanOpeningSanity: penalize edge moves in opening
 * - HumanTopKOverride: restrict to top K moves
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

// Check if a move is an "edge" move in opening (Na3, Nh3, edge pawn pushes)
bool is_edge_move_opening(int move, void* board_ptr) {
    Board* board = static_cast<Board*>(board_ptr);
    int from = Bitboards::move_from(move);
    int to = Bitboards::move_to(move);
    int piece = board->piece_at(from);
    
    // Only check knights and pawns in opening
    if ((piece & 7) != 2 && (piece & 7) != 1) return false; // Not knight or pawn
    
    int from_file = from % 8;
    int from_rank = from / 8;
    
    // Edge knights: Na3 (a3=21, b3=22), Nh3 (g3=30, h3=31)
    if ((piece & 7) == 2) { // Knight
        // a3, b3, g3, h3 (rank 2 from white's perspective)
        if (from_rank == 2 && (from_file == 0 || from_file == 1 || from_file == 6 || from_file == 7)) {
            return true;
        }
        // a6, b6, g6, h6 (rank 5 from white's perspective)
        if (from_rank == 5 && (from_file == 0 || from_file == 1 || from_file == 6 || from_file == 7)) {
            return true;
        }
    }
    
    // Edge pawn pushes: a3, h3, a4, h4 (not center pawns)
    if ((piece & 7) == 1) { // Pawn
        // Only penalize truly edge pawn pushes, not a4/h4 which can be normal
        if ((from_rank == 1 || from_rank == 6) && (from_file == 0 || from_file == 7)) {
            return true;
        }
    }
    
    return false;
}

// Collect candidate moves with guardrails
std::vector<CandidateMove> collect_candidates(
    void* board_ptr,
    int candidate_margin_cp,
    int candidate_moves_max,
    int max_depth,
    int hard_floor_cp,
    int opening_sanity,
    int topk_override,
    int current_ply,
    bool debug_output
) {
    Board* board = static_cast<Board*>(board_ptr);
    std::vector<CandidateMove> candidates;
    
    // Generate all moves
    auto moves = board->generate_moves();
    
    // For each legal move, evaluate
    for (int move : moves) {
        // Make temporary copy using bitboard operations
        Board temp = *board;
        
        int from = Bitboards::move_from(move);
        int to = Bitboards::move_to(move);
        int flags = Bitboards::move_flags(move);
        int promo = Bitboards::move_promotion(move);
        int piece = board->piece_at(from);
        
        if (piece == 0) continue;
        
        // Remove from old square
        temp.remove_piece(from);
        // Add to new square
        temp.add_piece(to, piece & 7, piece >> 3);
        
        // Handle capture if any
        if (to != from && board->piece_at(to) != 0) {
            temp.remove_piece(to);
        }
        
        // Skip if king is in check (illegal)
        if (temp.is_in_check(temp.side_to_move)) {
            continue;
        }
        
        // Evaluate
        int score = Evaluation::evaluate(temp);
        
        candidates.emplace_back(move, score);
    }
    
    // Sort by score descending
    std::sort(candidates.begin(), candidates.end(), 
        [](const CandidateMove& a, const CandidateMove& b) {
            return a.score > b.score;
        });
    
    if (candidates.empty()) return candidates;
    
    int best_score = candidates[0].score;
    int dropped_by_floor = 0;
    int dropped_by_opening = 0;
    
    // === GUARDRAIL 1: Hard Floor ===
    // Drop candidates worse than (best_score - hard_floor_cp)
    int hard_floor = best_score - hard_floor_cp;
    std::vector<CandidateMove> filtered;
    for (const auto& c : candidates) {
        if (c.score >= hard_floor) {
            filtered.push_back(c);
        } else {
            dropped_by_floor++;
        }
    }
    candidates = filtered;
    
    // === GUARDRAIL 2: Opening Sanity ===
    // Penalize edge moves in opening (first 12 plies)
    bool is_opening = (current_ply < 12);
    if (is_opening && opening_sanity > 0) {
        for (auto& c : candidates) {
            if (is_edge_move_opening(c.move, board_ptr)) {
                // Penalize by reducing score
                int penalty = opening_sanity * 5; // Scale: 120 * 5 = 600cp penalty
                c.score -= penalty;
                dropped_by_opening++;
            }
        }
        // Re-sort after penalties
        std::sort(candidates.begin(), candidates.end(), 
            [](const CandidateMove& a, const CandidateMove& b) {
                return a.score > b.score;
            });
    }
    
    // === GUARDRAIL 3: TopK Override ===
    // Restrict to top K moves
    if (topk_override > 0 && topk_override < candidates.size()) {
        candidates.resize(topk_override);
    }
    
    // Filter by margin and max (original filter)
    if (!candidates.empty()) {
        int margin = candidate_margin_cp;
        int margin_floor = best_score - margin;
        
        filtered.clear();
        for (const auto& c : candidates) {
            if (c.score >= margin_floor) {
                filtered.push_back(c);
            }
        }
        
        // Also enforce candidate_moves_max
        if (filtered.size() > candidate_moves_max) {
            filtered.resize(candidate_moves_max);
        }
        candidates = filtered;
    }
    
    if (debug_output) {
        std::cerr << "HUMAN_PICK candidates=" << candidates.size() 
                  << " best=" << best_score 
                  << " floor=" << hard_floor 
                  << " droppedByFloor=" << dropped_by_floor
                  << " droppedByOpening=" << dropped_by_opening
                  << " isOpening=" << (is_opening ? "1" : "0") << std::endl;
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
        
        // Apply risk appetite
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
        // Print ALL candidates so chosen is always visible
        for (size_t i = 0; i < candidates.size(); i++) {
            const auto& c = candidates[i];
            std::cerr << "  " << Bitboards::move_to_uci(c.move) << " score=" << c.score 
                      << " prob=" << (c.probability * 100) << "%" << std::endl;
        }
    }
    
    // Sample using seeded random
    double r = seeded_random(random_seed + 12345);
    double cumulative = 0;
    int chosen = candidates[0].move;
    int chosen_score = candidates[0].score;
    
    for (const auto& c : candidates) {
        cumulative += c.probability;
        if (r <= cumulative) {
            chosen = c.move;
            chosen_score = c.score;
            break;
        }
    }
    
    if (debug_output) {
        std::cerr << "HUMAN_PICK chosen=" << Bitboards::move_to_uci(chosen) 
                  << " score=" << chosen_score << std::endl;
    }
    
    return chosen;
}

} // namespace HumanSelection
