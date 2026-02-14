/**
 * UCI Protocol Implementation
 * Handles communication with chess GUIs
 */

#include "uci.hpp"
#include "../eval/evaluation.hpp"
#include "../eval/params.hpp"
#include "../search/search.hpp"
#include "../utils/board.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <mutex>
#include <chrono>
#include <atomic>

// UCI I/O logging - file only, not stdout/stderr
static std::ofstream uci_log;
static std::mutex log_mutex;
static bool logging_initialized = false;

static void init_logging() {
    if (!logging_initialized) {
        uci_log.open("uci_io.log", std::ios::app);
        logging_initialized = true;
    }
}

static void log_in(const std::string& line) {
    init_logging();
    std::lock_guard<std::mutex> lock(log_mutex);
    if (uci_log.is_open()) {
        uci_log << "<< " << line << std::endl;
        uci_log.flush();
    }
}

static void log_out(const std::string& line) {
    init_logging();
    std::lock_guard<std::mutex> lock(log_mutex);
    if (uci_log.is_open()) {
        uci_log << ">> " << line << std::endl;
        uci_log.flush();
    }
}

namespace UCI {

Options options;
static std::string current_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Forward declarations
void cmd_display();
void cmd_evaluate();

// Main UCI loop
void loop(int argc, char* argv[]) {
    std::string cmd;
    
    // Don't print welcome message in UCI mode - it breaks protocol
    // std::cout << "FutureChamp" << std::endl;
    // std::cout << "Type 'uci' to enter UCI mode, 'quit' to exit." << std::endl;
    
    while (std::getline(std::cin, cmd)) {
        log_in(cmd);  // Log every input line
        
        std::istringstream ss(cmd);
        std::string token;
        ss >> token;
        
        if (token == "uci") {
            cmd_uci();
        } else if (token == "isready") {
            cmd_is_ready();
        } else if (token == "quit") {
            break;
        } else if (token == "position") {
            std::vector<std::string> tokens;
            while (ss >> token) tokens.push_back(token);
            cmd_position(tokens);
        } else if (token == "go") {
            std::vector<std::string> tokens;
            while (ss >> token) tokens.push_back(token);
            cmd_go(tokens);
        } else if (token == "setoption") {
            std::vector<std::string> tokens;
            while (ss >> token) tokens.push_back(token);
            cmd_setoption(tokens);
        } else if (token == "stop") {
            cmd_stop();
        } else if (token == "d") {
            cmd_display();
        } else if (token == "eval") {
            cmd_evaluate();
        } else if (token == "ucinewgame") {
            // Reset for new game
            current_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        }
    }
}

// Display position for debugging
void cmd_display() {
    Board b;
    if (!current_position.empty()) {
        b.set_from_fen(current_position);
    } else {
        b.set_start_position();
    }
    std::cout << "FEN: " << b.get_fen() << std::endl;
    std::cout << "Side to move: " << (b.side_to_move == 0 ? "White" : "Black") << std::endl;
    auto moves = b.generate_moves();
    std::cout << "Legal moves: " << moves.size() << std::endl;
}

// Evaluate position
void cmd_evaluate() {
    int score = Evaluation::evaluate(current_position);
    std::cout << "Evaluation: " << score << " cp" << std::endl;
    
    auto exp = Evaluation::explain(score, current_position);
    std::cout << "Notes:" << std::endl;
    for (const auto& note : exp.move_reasons) {
        std::cout << "  - " << note << std::endl;
    }
    for (const auto& note : exp.imbalance_notes) {
        std::cout << "  - " << note << std::endl;
    }
}

// UCI protocol commands
void cmd_uci() {
    std::cout << "id name FutureChamp" << std::endl;
    std::cout << "id author Brendan & Jay" << std::endl;
    
    // UCI options
    std::cout << "option name PlayingStyle type combo default classical " <<
                 "var classical var attacking var tactical var positional var technical" << std::endl;
    std::cout << "option name SkillLevel type spin default 10 min 0 max 20" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 1024" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 32" << std::endl;
    std::cout << "option name UseMCTS type check default true" << std::endl;
    std::cout << "option name VerbalPV type check default false" << std::endl;
    std::cout << "option name ShowImbalances type check default false" << std::endl;
    std::cout << "option name DebugEvalTrace type check default false" << std::endl;
    std::cout << "option name DebugTraceWithParams type check default false" << std::endl;
    
    // === PERSONALITY ===
    std::cout << "option name Personality type combo default default "
              << "var default var petrosian var tal var capablanca var club1800" << std::endl;
    std::cout << "option name PersonalityAutoLoad type check default true" << std::endl;
    std::cout << "option name SavePersonality type string default \"\"" << std::endl;
    
    // === CORE MATERIAL / IMBALANCE ===
    std::cout << "option name MaterialPriority type spin default 100 min 1 max 100" << std::endl;
    std::cout << "option name ImbalanceScale type spin default 100 min 30 max 150" << std::endl;
    std::cout << "option name KnightValueBias type spin default 0 min -50 max 50" << std::endl;
    std::cout << "option name BishopValueBias type spin default 0 min -50 max 50" << std::endl;
    std::cout << "option name ExchangeSacrificeSensitivity type spin default 100 min 0 max 200" << std::endl;
    
    // === EVAL LAYER WEIGHTS ===
    std::cout << "option name W_PawnStructure type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name W_PieceActivity type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name W_KingSafety type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name W_Initiative type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name W_Imbalance type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name W_KnowledgeConcepts type spin default 100 min 0 max 200" << std::endl;
    
    // === KEY MICRO TERMS ===
    std::cout << "option name OutpostBonus type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name BishopPairBonus type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name RookOpenFileBonus type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name PassedPawnBonus type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name PawnShieldPenalty type spin default 100 min 0 max 200" << std::endl;
    
    // === KNOWLEDGE CONCEPT WEIGHTS ===
    std::cout << "option name ConceptOutpostWeight type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name ConceptBadBishopWeight type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name ConceptSpaceWeight type spin default 100 min 0 max 200" << std::endl;
    
    // === MASTER CONCEPTS ===
    std::cout << "option name ConceptExchangeSacWeight type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name ConceptColorComplexWeight type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name ConceptPawnLeverWeight type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name ConceptInitiativePersistWeight type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name InitiativeDominance type spin default 100 min 0 max 200" << std::endl;
    
    // === SEARCH / HUMANISATION ===
    std::cout << "option name CandidateMarginCp type spin default 200 min 0 max 400" << std::endl;
    std::cout << "option name CandidateMovesMax type spin default 10 min 1 max 30" << std::endl;
    std::cout << "option name HumanEnable type check default true" << std::endl;
    std::cout << "option name HumanSelect type check default false" << std::endl;
    std::cout << "option name HumanTemperature type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name HumanNoiseCp type spin default 0 min 0 max 50" << std::endl;
    std::cout << "option name HumanBlunderRate type spin default 0 min 0 max 1000" << std::endl;
    std::cout << "option name RandomSeed type spin default 0 min 0 max 2147483647" << std::endl;
    std::cout << "option name RiskAppetite type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name SacrificeBias type spin default 100 min 0 max 200" << std::endl;
    std::cout << "option name SimplicityBias type spin default 100 min 0 max 200" << std::endl;
    
    // === HUMAN GUARDRAILS ===
    std::cout << "option name HumanHardFloorCp type spin default 200 min 0 max 600" << std::endl;
    std::cout << "option name HumanOpeningSanity type spin default 120 min 0 max 200" << std::endl;
    std::cout << "option name HumanTopKOverride type spin default 0 min 0 max 10" << std::endl;
    
    // === DEBUG ===
    std::cout << "option name DebugHumanPick type check default false" << std::endl;
    std::cout << "option name DebugSearchTrace type check default false" << std::endl;
    
    std::cout << "uciok" << std::endl;
    std::cout.flush();
    log_out("uciok");
}

void cmd_is_ready() {
    std::cout << "readyok" << std::endl;
    std::cout.flush();
    log_out("readyok");
}

void cmd_position(const std::vector<std::string>& tokens) {
    std::string fen;
    size_t moves_idx = 0;
    
    // Handle different position commands
    if (tokens.size() > 0 && tokens[0] == "startpos") {
        // Standard starting position
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        // Check for "moves" keyword
        for (size_t i = 1; i < tokens.size(); i++) {
            if (tokens[i] == "moves") {
                moves_idx = i + 1;
                break;
            }
        }
    } else if (tokens.size() > 0 && tokens[0] == "fen") {
        // Custom FEN - assemble from tokens[1-6]
        std::string fen_parts[6] = {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-", "0", "1"};
        for (size_t i = 1; i < 7 && i < tokens.size(); i++) {
            if (tokens[i] == "moves") {
                moves_idx = i + 1;
                break;
            }
            fen_parts[i-1] = tokens[i];
        }
        fen = fen_parts[0] + " " + fen_parts[1] + " " + fen_parts[2] + 
              " " + fen_parts[3] + " " + fen_parts[4] + " " + fen_parts[5];
        
        // Check for "moves" keyword if not found above
        if (moves_idx == 0) {
            for (size_t i = 7; i < tokens.size(); i++) {
                if (tokens[i] == "moves") {
                    moves_idx = i + 1;
                    break;
                }
            }
        }
    } else {
        // Default to starting position
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }
    
    current_position = fen;
    
    // Apply moves if present using Search module's helper
    if (moves_idx > 0 && moves_idx < tokens.size()) {
        for (size_t i = moves_idx; i < tokens.size(); i++) {
            current_position = Search::apply_uci_move(current_position, tokens[i]);
        }
    }
}


void cmd_go(const std::vector<std::string>& tokens) {
    int depth = 20;  // Default max depth
    int movetime = -1;  // -1 means use time control
    int wtime = -1;
    int btime = -1;
    int winc = 0;
    int binc = 0;
    bool infinite = false;
    
    // Parse go parameters
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            depth = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            movetime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "wtime" && i + 1 < tokens.size()) {
            wtime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "btime" && i + 1 < tokens.size()) {
            btime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "winc" && i + 1 < tokens.size()) {
            winc = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "binc" && i + 1 < tokens.size()) {
            binc = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "infinite") {
            infinite = true;
        }
    }
    
    // Determine whose turn it is
    Board temp_board;
    temp_board.set_from_fen(current_position);
    bool is_white = (temp_board.side_to_move == 0);
    
    // Calculate move time if not set explicitly
    if (movetime == -1 && !infinite) {
        int my_time = is_white ? wtime : btime;
        int my_inc = is_white ? winc : binc;
        
        if (my_time > 0) {
            // **IMPROVED TIME MANAGEMENT: Spend appropriate time based on game phase**
            int base_moves_to_go = 40;
            
            // **OPENING (first 10 moves): Think more about opening principles**
            if (temp_board.fullmove_number <= 3) {
                // First 3 moves: use more time for important opening decisions
                movetime = my_time / 25;  // Use 1/25 of time
                if (movetime < 2000) movetime = 2000;   // At least 2 seconds
                if (movetime > 15000) movetime = 15000; // Max 15 seconds
            } 
            else if (temp_board.fullmove_number <= 10) {
                // Moves 4-10: early game, think about development and castling
                movetime = my_time / 30 + (my_inc * 2);  // More time with increment
                if (movetime < 1500) movetime = 1500;    // At least 1.5 seconds
            }
            else if (temp_board.fullmove_number <= 25) {
                // Middlegame: normal time usage
                movetime = (my_time / base_moves_to_go) + (my_inc * 3 / 2);
            }
            else {
                // Endgame: can use slightly less time per move
                movetime = (my_time / (base_moves_to_go + 10)) + my_inc;
            }
            
            // **Don't use more than 1/8th of remaining time on one move**
            int max_time = my_time / 8;
            if (movetime > max_time) movetime = max_time;
            
            // **Don't use less than 1/40th of time (except for very long time controls)**
            int min_time = my_time / 40;
            if (min_time < 100) min_time = 100;  // At least 0.1 seconds
            if (movetime < min_time) movetime = min_time;
            
            // **Absolute bounds for safety**
            if (movetime < 50) movetime = 50;       // Minimum 0.05 seconds
            if (movetime > 30000) movetime = 30000; // Maximum 30 seconds
            
        } else {
            movetime = 1000;  // Default 1 second if no time info (tournament-safe)
        }
    } else if (movetime == -1) {
        movetime = 10000;  // Infinite search with 10s limit
    }
    
    // Perform search
    auto result = Search::search(current_position, movetime, depth);
    
    // Convert move to UCI notation
    std::string best_move_uci = Bitboards::move_to_uci(result.best_move);
    
    // **MOVE VALIDATION: Ensure the returned move is legal**
    Board verify_board;
    verify_board.set_from_fen(current_position);
    
    // Simple validation: check if the move exists in legal moves
    bool move_is_valid = false;
    auto all_possible_moves = verify_board.generate_moves();
    
    // Convert best move to from/to squares for comparison
    int best_from = -1, best_to = -1;
    if (best_move_uci.length() >= 4) {
        best_from = (best_move_uci[1] - '1') * 8 + (best_move_uci[0] - 'a');
        best_to = (best_move_uci[3] - '1') * 8 + (best_move_uci[2] - 'a');
    }
    
    // Check all pseudo-legal moves
    for (int move : all_possible_moves) {
        int from = Bitboards::move_from(move);
        int to = Bitboards::move_to(move);
        
        if (from == best_from && to == best_to) {
            // Found matching move coordinates
            move_is_valid = true;
            
            // **Additional check: verify move flags match for special moves**
            int move_flags = Bitboards::move_flags(move);
            if (best_move_uci.length() == 5) {
                // Promotion move - check promotion type
                char promo_char = best_move_uci[4];
                int promo_type = 0;
                switch(promo_char) {
                    case 'n': promo_type = 0; break;
                    case 'b': promo_type = 1; break;
                    case 'r': promo_type = 2; break;
                    case 'q': promo_type = 3; break;
                    default: promo_type = 3; break;
                }
                if (move_flags != MOVE_PROMOTION || Bitboards::move_promotion(move) != promo_type) {
                    move_is_valid = false;
                }
            } else if (move_flags == MOVE_CASTLE) {
                // Castle move - verify it's actually a castle
                if (!(best_from == 4 && (best_to == 6 || best_to == 2)) &&  // White castle
                    !(best_from == 60 && (best_to == 62 || best_to == 58))) { // Black castle
                    move_is_valid = false;
                }
            }
            break;
        }
    }
    
    if (!move_is_valid) {
        // **FALLBACK: Find first legal move**
        std::cout << "info string WARNING: Invalid move " << best_move_uci 
                  << " generated, using fallback" << std::endl;
        
        for (int move : all_possible_moves) {
            // Try to make the move and see if it leaves king in check
            Board test_board = verify_board;
            int from = Bitboards::move_from(move);
            int to = Bitboards::move_to(move);
            int piece = test_board.piece_at(from);
            int color = test_board.color_at(from);
            
            // Skip if no piece or wrong color
            if (piece == NO_PIECE || color != verify_board.side_to_move) continue;
            
            // Remove captured piece if any
            int captured = test_board.piece_at(to);
            if (captured != NO_PIECE) {
                test_board.remove_piece(to);
            }
            
            // Move the piece
            test_board.remove_piece(from);
            test_board.add_piece(to, piece, color);
            test_board.side_to_move = 1 - color;
            
            // Check if our king is in check after the move
            int king_sq = -1;
            for (int sq = 0; sq < 64; sq++) {
                if (test_board.piece_at(sq) == KING && test_board.color_at(sq) == color) {
                    king_sq = sq;
                    break;
                }
            }
            
            if (king_sq != -1 && !Bitboards::is_square_attacked(test_board, king_sq, 1 - color)) {
                // Legal move found
                best_move_uci = Bitboards::move_to_uci(move);
                break;
            }
        }
        
        // If still no valid move (shouldn't happen), use first pseudo-legal move
        if (!all_possible_moves.empty()) {
            best_move_uci = Bitboards::move_to_uci(all_possible_moves[0]);
        }
    }
    
    // **CRITICAL: Output the bestmove command**
    std::string bestmove_line = "bestmove " + best_move_uci;
    std::cout << bestmove_line << std::endl;
    std::cout.flush();
    log_out(bestmove_line);
    
    // Output search diagnostics if enabled
    if (UCI::options.debug_search_trace) {
        // Calculate timing
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - Search::g_diag.searchStartTime).count();
        if (elapsedMs < 1) elapsedMs = 1;  // Avoid div0
        
        int nps = static_cast<int>((Search::g_diag.nodes * 1000) / elapsedMs);
        
        std::cout << "info string SPEED depth=" << result.depth 
                  << " timeMs=" << elapsedMs 
                  << " nodes=" << Search::g_diag.nodes 
                  << " qnodes=" << Search::g_diag.qnodes 
                  << " nps=" << nps << std::endl;
        
        int ttHitRate = 0;
        if (Search::g_diag.ttProbes > 0) {
            // Multiply by 10000 to get 2 decimal places (e.g., 0.69% -> 69)
            ttHitRate = static_cast<int>((10000.0 * Search::g_diag.ttHits) / Search::g_diag.ttProbes);
        }
        std::cout << "info string SEARCH_DIAG nodes=" << Search::g_diag.nodes << std::endl;
        std::cout << "info string SEARCH_DIAG qnodes=" << Search::g_diag.qnodes << std::endl;
        std::cout << "info string SEARCH_Q qEvasions=" << Search::g_diag.qEvasions << std::endl;
        std::cout << "info string SEARCH_Q qCapturesSearched=" << Search::g_diag.qCapturesSearched << std::endl;
        std::cout << "info string SEARCH_Q qCapturesSkippedSEE=" << Search::g_diag.qCapturesSkippedSEE << std::endl;
        std::cout << "info string SEARCH_Q qDeltaPruned=" << Search::g_diag.qDeltaPruned << std::endl;
        std::cout << "info string SEARCH_DIAG ttEntries=" << Search::g_diag.ttEntries << std::endl;
        std::cout << "info string SEARCH_DIAG ttProbes=" << Search::g_diag.ttProbes << std::endl;
        std::cout << "info string SEARCH_DIAG ttHits=" << Search::g_diag.ttHits << std::endl;
        std::cout << "info string SEARCH_DIAG ttHitRate=" << (ttHitRate / 100) << "." << (ttHitRate % 100) << "%" << std::endl;
        std::cout << "info string SEARCH_DIAG ttStores=" << Search::g_diag.ttStores << std::endl;
        std::cout << "info string SEARCH_DIAG ttCollisions=" << Search::g_diag.ttCollisions << std::endl;
        std::cout << "info string SEARCH_DIAG rootKeyNonZero=" << (Search::g_diag.rootKeyNonZero ? "1" : "0") << std::endl;
        std::cout << "info string SEARCH_DIAG betaCutoffs=" << Search::g_diag.betaCutoffs << std::endl;
        std::cout << "info string SEARCH_DIAG alphaImproves=" << Search::g_diag.alphaImproves << std::endl;
        
        // HOTPATH counters
        std::cout << "info string HOTPATH make=" << Search::g_diag.makeMoveCalls 
                  << " unmake=" << Search::g_diag.unmakeMoveCalls 
                  << " copies=" << Search::g_diag.boardCopies << std::endl;
        
        // COPIES attribution
        std::cout << "info string COPIES total=" << Search::g_diag.boardCopies
                  << " make_return=" << Search::g_diag.copies_make_return
                  << " clone=" << Search::g_diag.copies_board_clone
                  << " null=" << Search::g_diag.copies_nullmove
                  << " legality=" << Search::g_diag.copies_legality
                  << " q=" << Search::g_diag.copies_qsearch
                  << " pv=" << Search::g_diag.copies_pv
                  << " other=" << Search::g_diag.copies_other << std::endl;
        
        // PROFILE timing buckets
        std::cout << "info string PROFILE movegenMs=" << (Search::g_diag.t_movegen / 1000)
                  << " makeMs=" << (Search::g_diag.t_makeunmake / 1000)
                  << " evalMs=" << (Search::g_diag.t_eval / 1000)
                  << " legalityMs=" << (Search::g_diag.t_legality / 1000) << std::endl;
        
        std::cout.flush();
    }
}



void cmd_setoption(const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) return;
    
    std::string name, value;
    bool in_name = false, in_value = false;
    
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "name") {
            in_name = true; in_value = false;
        } else if (tokens[i] == "value") {
            in_name = false; in_value = true;
        } else if (in_name) {
            // Collect name tokens until we hit "value" or a standalone true/false
            // For check options without "value" keyword, last token might be true/false
            if (i == tokens.size() - 1 && (tokens[i] == "true" || tokens[i] == "false")) {
                // This is the value for a check option without "value" keyword
                value = tokens[i];
            } else {
                name += tokens[i] + " ";
            }
        } else if (in_value) {
            value += tokens[i] + " ";
        }
    }
    
    if (!name.empty() && name.back() == ' ') name.pop_back();
    if (!value.empty() && value.back() == ' ') value.pop_back();
    
    if (name == "PlayingStyle") {
        Evaluation::set_style(value);
        options.playing_style = value;
    } else if (name == "SkillLevel") {
        options.skill_level = std::stoi(value);
    } else if (name == "Hash") {
        options.hash_size = std::stoi(value);
    } else if (name == "Threads") {
        options.threads = std::stoi(value);
        Search::set_threads(options.threads);
    } else if (name == "UseMCTS") {
        options.use_mcts = (value == "true");
        Search::set_use_mcts(options.use_mcts);
    } else if (name == "VerbalPV") {
        options.verbal_pv = (value == "true");
    } else if (name == "ShowImbalances") {
        options.show_imbalances = (value == "true");
    } else if (name == "DebugEvalTrace") {
        options.debug_eval_trace = (value == "true");
        Evaluation::set_debug_trace(options.debug_eval_trace);
    } else if (name == "DebugSearchTrace") {
        options.debug_search_trace = (value == "true");
    } else if (name == "DebugTraceWithParams") {
        Evaluation::set_param("DebugTraceWithParams", value);
    } else if (name == "PersonalityAutoLoad") {
        Evaluation::get_params().personality_auto_load = (value == "true");
    } else if (name == "Personality") {
        if (Evaluation::get_params().personality_auto_load) {
            if (Evaluation::load_personality(value, true)) {
                // Summary already printed in load_personality
            } else {
                std::cout << "info string Failed to load personality: " << value << std::endl;
            }
        }
    } else if (name == "SavePersonality") {
        if (!value.empty() && value != "\"\"") {
            // Remove quotes if present
            size_t start = value.find_first_not_of('"');
            size_t end = value.find_last_not_of('"');
            if (start != std::string::npos && end != std::string::npos) {
                value = value.substr(start, end - start + 1);
            }
            if (Evaluation::save_personality(value)) {
                std::cout << "info string Saved personality: " << value << std::endl;
            } else {
                std::cout << "info string Failed to save personality: " << value << std::endl;
            }
        }
    }
    // Pass all other params to Evaluation params system
    else if (Evaluation::set_param(name, value)) {
        // Param was set successfully
    }
}

void cmd_stop() {
    Search::stop();
}

void cmd_quit() {}

} // namespace UCI
