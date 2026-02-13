/**
 * Human Chess Engine
 * 
 * A human-like chess engine that thinks like a coach, not a calculator.
 * Built with Silman-style evaluation, strategic thinking, and teaching focus.
 * 
 * Author: Brendan & Jay
 * License: MIT
 */

#include <iostream>
#include <string>
#include <chrono>
#include "uci/uci.hpp"
#include "search/search.hpp"
#include "evaluation/evaluation.hpp"

int main(int argc, char* argv[]) {
    std::cout << "Human Chess Engine " << std::endl;
    std::cout << "A chess engine that thinks like a coach." << std::endl;
    std::cout << std::endl;
    
    // Initialize engine components
    Evaluation::initialize();
    Search::initialize();
    
    // Run UCI loop
    UCI::loop(argc, argv);
    
    return 0;
}
