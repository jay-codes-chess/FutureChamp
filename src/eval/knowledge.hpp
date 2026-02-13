/**
 * Knowledge Registry
 * Strategic concepts extracted from chess books → hard-coded heuristics
 */

#ifndef EVAL_KNOWLEDGE_HPP
#define EVAL_KNOWLEDGE_HPP

#include "../utils/board.hpp"
#include "params.hpp"
#include <string>
#include <vector>

namespace Evaluation {

// Strategic concept with evaluator function
struct StrategicConcept {
    std::string name;
    int weight;  // Base weight (100 = 1.0)
    int (*evaluate)(const Board&, const Params&);
};

// Knowledge evaluation - sums all active concepts
int evaluate_knowledge(const Board& board, const Params& params);

// Individual concept evaluators
int eval_knight_outpost(const Board& board, const Params& params);
int eval_bad_bishop(const Board& board, const Params& params);
int eval_knight_vs_bad_bishop(const Board& board, const Params& params);
int eval_rook_on_7th(const Board& board, const Params& params);
int eval_space_advantage(const Board& board, const Params& params);

} // namespace Evaluation

#endif // EVAL_KNOWLEDGE_HPP
