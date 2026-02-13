/**
 * Board Representation Implementation
 * Enhanced with castling and promotion support
 */

#include "board.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

void Board::set_start_position() {
    clear();
    
    // White pieces
    add_piece(0, ROOK, WHITE);
    add_piece(1, KNIGHT, WHITE);
    add_piece(2, BISHOP, WHITE);
    add_piece(3, QUEEN, WHITE);
    add_piece(4, KING, WHITE);
    add_piece(5, BISHOP, WHITE);
    add_piece(6, KNIGHT, WHITE);
    add_piece(7, ROOK, WHITE);
    for (int i = 8; i < 16; i++) add_piece(i, PAWN, WHITE);
    
    // Black pieces
    add_piece(56, ROOK, BLACK);
    add_piece(57, KNIGHT, BLACK);
    add_piece(58, BISHOP, BLACK);
    add_piece(59, QUEEN, BLACK);
    add_piece(60, KING, BLACK);
    add_piece(61, BISHOP, BLACK);
    add_piece(62, KNIGHT, BLACK);
    add_piece(63, ROOK, BLACK);
    for (int i = 48; i < 56; i++) add_piece(i, PAWN, BLACK);
    
    side_to_move = WHITE;
    castling[WHITE][0] = castling[WHITE][1] = true;
    castling[BLACK][0] = castling[BLACK][1] = true;
    en_passant_square = -1;
    fullmove_number = 1;
    halfmove_clock = 0;
    
    compute_hash();
}

bool Board::set_from_fen(const std::string& fen) {
    clear();
    
    std::istringstream ss(fen);
    std::string board_str, side_str, castling_str, ep_str;
    ss >> board_str >> side_str >> castling_str >> ep_str;
    
    // Parse board - FIXED: Start from a8 (square 56)
    int sq = 56;  // a8 square
    for (char c : board_str) {
        if (c == '/') {
            sq -= 16;  // Move down one rank: -8 for rank change, -8 for going to beginning of next rank
            if (sq < 0) break;  // Safety check
        } else if (c >= '1' && c <= '8') {
            sq += (c - '0');
        } else {
            int piece_type = NO_PIECE;
            bool is_white = !std::islower(c);
            char pc = std::tolower(c);
            switch (pc) {
                case 'p': piece_type = PAWN; break;
                case 'n': piece_type = KNIGHT; break;
                case 'b': piece_type = BISHOP; break;
                case 'r': piece_type = ROOK; break;
                case 'q': piece_type = QUEEN; break;
                case 'k': piece_type = KING; break;
                default: continue;
            }
            if (sq >= 0 && sq < 64) {
                add_piece(sq, piece_type, is_white ? WHITE : BLACK);
            }
            sq++;
        }
    }


    
    // Side to move
    side_to_move = (side_str == "w") ? WHITE : BLACK;
    
    // Castling
    castling[WHITE][0] = castling_str.find('K') != std::string::npos;
    castling[WHITE][1] = castling_str.find('Q') != std::string::npos;
    castling[BLACK][0] = castling_str.find('k') != std::string::npos;
    castling[BLACK][1] = castling_str.find('q') != std::string::npos;
    
    // En passant
    if (ep_str == "-") {
        en_passant_square = -1;
    } else {
        int file = ep_str[0] - 'a';
        int rank = ep_str[1] - '1';
        en_passant_square = file + rank * 8;
    }
    
    // Move counters
    std::string move_str;
    ss >> move_str;
    if (!move_str.empty()) {
        halfmove_clock = std::stoi(move_str);
    }
    ss >> move_str;
    if (!move_str.empty()) {
        fullmove_number = std::stoi(move_str);
    }
    
    compute_hash();
    return true;
}

std::string Board::get_fen() const {
    std::ostringstream ss;
    
    // Board
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            int sq = file + rank * 8;
            int p = piece_at(sq);
            int c = color_at(sq);
            
            if (p == NO_PIECE) {
                empty++;
            } else {
                if (empty > 0) {
                    ss << empty;
                    empty = 0;
                }
                char ch = " PNBRQK"[p];  // Start with uppercase
                if (c == BLACK) ch = std::tolower(ch);
                ss << ch;
            }
        }
        if (empty > 0) ss << empty;
        if (rank > 0) ss << '/';
    }
    
    ss << ' ';
    ss << (side_to_move == WHITE ? 'w' : 'b');
    ss << ' ';
    
    // Castling
    std::string cast;
    if (castling[WHITE][0]) cast += 'K';
    if (castling[WHITE][1]) cast += 'Q';
    if (castling[BLACK][0]) cast += 'k';
    if (castling[BLACK][1]) cast += 'q';
    ss << (cast.empty() ? "-" : cast);
    ss << ' ';
    
    // En passant
    if (en_passant_square == -1) {
        ss << '-';
    } else {
        ss << char('a' + Bitboards::file_of(en_passant_square));
        ss << char('1' + Bitboards::rank_of(en_passant_square));
    }
    
    ss << ' ' << halfmove_clock << ' ' << fullmove_number;
    
    return ss.str();
}

void Board::clear() {
    for (int i = 0; i < 7; i++) pieces[i] = 0;
    colors[WHITE] = colors[BLACK] = 0;
}

void Board::add_piece(int square, int piece_type, int color) {
    if (piece_type == NO_PIECE) return;
    
    Bitboards::set(pieces[piece_type], square);
    Bitboards::set(colors[color], square);
}

void Board::remove_piece(int square) {
    for (int pt = PAWN; pt <= KING; pt++) {
        if (Bitboards::test(pieces[pt], square)) {
            pieces[pt] &= ~(1ULL << square);
        }
    }
    colors[WHITE] &= ~(1ULL << square);
    colors[BLACK] &= ~(1ULL << square);
}

void Board::move_piece(int from, int to) {
    int pt = piece_at(from);
    int c = color_at(from);
    
    remove_piece(from);
    add_piece(to, pt, c);
}

int Board::piece_at(int square) const {
    if (square < 0 || square >= 64) return NO_PIECE;
    for (int pt = PAWN; pt <= KING; pt++) {
        if (Bitboards::test(pieces[pt], square)) return pt;
    }
    return NO_PIECE;
}

int Board::color_at(int square) const {
    if (square < 0 || square >= 64) return -1;
    if (Bitboards::test(colors[WHITE], square)) return WHITE;
    if (Bitboards::test(colors[BLACK], square)) return BLACK;
    return -1;
}

bool Board::is_empty(int square) const {
    return piece_at(square) == NO_PIECE;
}

uint64_t Board::pieces_of_color(int color) const {
    return colors[color];
}

uint64_t Board::all_pieces() const {
    return colors[WHITE] | colors[BLACK];
}



void Board::compute_hash() {
    hash = 0;
    
    // Initialize random piece keys properly
    static bool initialized = false;
    static uint64_t piece_keys[2][7][64];  // [color][piece][square]
    
    if (!initialized) {
        // Simple but deterministic pseudo-random numbers
        uint64_t seed = 0x123456789ABCDEFULL;
        for (int color = 0; color < 2; color++) {
            for (int piece = 0; piece < 7; piece++) {
                for (int sq = 0; sq < 64; sq++) {
                    // Simple LCG for random numbers
                    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
                    piece_keys[color][piece][sq] = seed;
                }
            }
        }
        initialized = true;
    }
    
    // Hash piece positions
    for (int sq = 0; sq < 64; sq++) {
        int p = piece_at(sq);
        if (p != NO_PIECE) {
            int c = color_at(sq);
            hash ^= piece_keys[c][p][sq];
        }
    }
    
    // Include side to move
    if (side_to_move == BLACK) {
        hash ^= 0xF0F0F0F0F0F0F0F0ULL;
    }
    
    // Include castling rights
    if (castling[WHITE][0]) hash ^= 0x1111111111111111ULL;
    if (castling[WHITE][1]) hash ^= 0x2222222222222222ULL;
    if (castling[BLACK][0]) hash ^= 0x4444444444444444ULL;
    if (castling[BLACK][1]) hash ^= 0x8888888888888888ULL;
    
    // Include en passant square
    if (en_passant_square != -1) {
        hash ^= (en_passant_square * 31415926535ULL);
    }
}


bool Board::is_in_check(int color) const {
    int king_sq = -1;
    for (int sq = 0; sq < 64; sq++) {
        if (piece_at(sq) == KING && color_at(sq) == color) {
            king_sq = sq;
            break;
        }
    }
    if (king_sq == -1) return false;
    return Bitboards::is_square_attacked(*this, king_sq, 1 - color);
}


std::vector<int> Board::generate_moves() const {
    std::vector<int> moves;
    moves.reserve(128); // Pre-allocate to reduce reallocations
    
    uint64_t our_pieces = pieces_of_color(side_to_move);
    uint64_t enemy_pieces = pieces_of_color(1 - side_to_move);
    uint64_t all = all_pieces();
    
    // 1. Pawn moves
    uint64_t pawns = pieces[PAWN] & colors[side_to_move];
    while (pawns) {
        int sq = Bitboards::pop_lsb(pawns);
        int rank = Bitboards::rank_of(sq);
        int file = Bitboards::file_of(sq);
        
        // Forward direction
        int forward_dir = (side_to_move == WHITE) ? 8 : -8;
        
        // Single push
        int forward = sq + forward_dir;
        if (forward >= 0 && forward < 64 && is_empty(forward)) {
            int to_rank = Bitboards::rank_of(forward);
            
            // Promotion?
            if ((side_to_move == WHITE && to_rank == 7) || 
                (side_to_move == BLACK && to_rank == 0)) {
                // Generate all promotion options
                moves.push_back(Bitboards::make_move(sq, forward, MOVE_PROMOTION, 0)); // Knight
                moves.push_back(Bitboards::make_move(sq, forward, MOVE_PROMOTION, 1)); // Bishop
                moves.push_back(Bitboards::make_move(sq, forward, MOVE_PROMOTION, 2)); // Rook
                moves.push_back(Bitboards::make_move(sq, forward, MOVE_PROMOTION, 3)); // Queen
            } else {
                moves.push_back(Bitboards::make_move(sq, forward));
                
                // Double push from starting rank
                int start_rank = (side_to_move == WHITE) ? 1 : 6;
                if (rank == start_rank) {
                    int double_forward = forward + forward_dir;
                    if (double_forward >= 0 && double_forward < 64 && is_empty(double_forward)) {
                        moves.push_back(Bitboards::make_move(sq, double_forward));
                    }
                }
            }
        }
        
        // Captures - precompute target squares
        int capture_targets[2] = {-1, -1};
        int capture_count = 0;
        
        // Left capture (relative to pawn direction)
        if (file > 0) {
            int left_cap = sq + forward_dir - 1;
            if (left_cap >= 0 && left_cap < 64) {
                capture_targets[capture_count++] = left_cap;
            }
        }
        
        // Right capture (relative to pawn direction)
        if (file < 7) {
            int right_cap = sq + forward_dir + 1;
            if (right_cap >= 0 && right_cap < 64) {
                capture_targets[capture_count++] = right_cap;
            }
        }
        
        for (int i = 0; i < capture_count; i++) {
            int cap = capture_targets[i];
            int to_rank = Bitboards::rank_of(cap);
            
            // Normal capture
            if (Bitboards::test(enemy_pieces, cap)) {
                if ((side_to_move == WHITE && to_rank == 7) || 
                    (side_to_move == BLACK && to_rank == 0)) {
                    // Promotion capture
                    moves.push_back(Bitboards::make_move(sq, cap, MOVE_PROMOTION, 0)); // Knight
                    moves.push_back(Bitboards::make_move(sq, cap, MOVE_PROMOTION, 1)); // Bishop
                    moves.push_back(Bitboards::make_move(sq, cap, MOVE_PROMOTION, 2)); // Rook
                    moves.push_back(Bitboards::make_move(sq, cap, MOVE_PROMOTION, 3)); // Queen
                } else {
                    moves.push_back(Bitboards::make_move(sq, cap));
                }
            }
            // En passant
            else if (cap == en_passant_square) {
                moves.push_back(Bitboards::make_move(sq, cap, MOVE_EN_PASSANT));
            }
        }
    }
    
    // 2. Knight moves
    uint64_t knights = pieces[KNIGHT] & colors[side_to_move];
    while (knights) {
        int sq = Bitboards::pop_lsb(knights);
        uint64_t attacks = Bitboards::knight_attacks(sq);
        
        // Filter out squares occupied by our pieces
        attacks &= ~our_pieces;
        
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back(Bitboards::make_move(sq, to));
        }
    }
    
    // 3. Bishop moves
    uint64_t bishops = pieces[BISHOP] & colors[side_to_move];
    while (bishops) {
        int sq = Bitboards::pop_lsb(bishops);
        uint64_t attacks = Bitboards::bishop_attacks(sq, all);
        
        // Filter out squares occupied by our pieces
        attacks &= ~our_pieces;
        
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back(Bitboards::make_move(sq, to));
        }
    }
    
    // 4. Rook moves
    uint64_t rooks = pieces[ROOK] & colors[side_to_move];
    while (rooks) {
        int sq = Bitboards::pop_lsb(rooks);
        uint64_t attacks = Bitboards::rook_attacks(sq, all);
        
        // Filter out squares occupied by our pieces
        attacks &= ~our_pieces;
        
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back(Bitboards::make_move(sq, to));
        }
    }
    
    // 5. Queen moves
    uint64_t queens = pieces[QUEEN] & colors[side_to_move];
    while (queens) {
        int sq = Bitboards::pop_lsb(queens);
        uint64_t attacks = Bitboards::queen_attacks(sq, all);
        
        // Filter out squares occupied by our pieces
        attacks &= ~our_pieces;
        
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back(Bitboards::make_move(sq, to));
        }
    }
    
    // 6. King moves - WITH OPTIMIZED CASTLING
    uint64_t kings = pieces[KING] & colors[side_to_move];
    while (kings) {
        int sq = Bitboards::pop_lsb(kings);
        uint64_t attacks = Bitboards::king_attacks(sq);
        
        // Filter out squares occupied by our pieces
        attacks &= ~our_pieces;
        
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            moves.push_back(Bitboards::make_move(sq, to));
        }
        
        // Castling - only if not in check
        if (!is_in_check(side_to_move)) {
            // Precomputed castling data
            static const struct {
                int king_from;
                int king_to;
                int rook_from;
                int rook_to;
                uint64_t path_mask;
                uint64_t check_squares_mask;
            } CASTLING_DATA[2][2] = {  // [color][kingside/queenside]
                {   // WHITE
                    {4, 6, 7, 5, 0x60ULL, 0x60ULL},          // Kingside: f1,g1
                    {4, 2, 0, 3, 0xEULL, 0xCULL}             // Queenside: b1,c1,d1 (check only c1,d1)
                },
                {   // BLACK
                    {60, 62, 63, 61, 0x6000000000000000ULL, 0x6000000000000000ULL},  // Kingside: f8,g8
                    {60, 58, 56, 59, 0xE00000000000000ULL, 0xC00000000000000ULL}     // Queenside: b8,c8,d8 (check only c8,d8)
                }
            };
            
            // Kingside castling
            if (castling[side_to_move][0]) {
                const auto& data = CASTLING_DATA[side_to_move][0];
                
                // Check squares between king and rook are empty using bitmask
                bool path_clear = (all & data.path_mask) == 0;
                
                // Check king doesn't pass through or land in check
                if (path_clear) {
                    bool safe = true;
                    
                    // Check only the squares the king moves through
                    uint64_t check_mask = data.check_squares_mask;
                    while (check_mask) {
                        int check_sq = Bitboards::pop_lsb(check_mask);
                        if (Bitboards::is_square_attacked(*this, check_sq, 1 - side_to_move)) {
                            safe = false;
                            break;
                        }
                    }
                    
                    if (safe) {
                        moves.push_back(Bitboards::make_move(data.king_from, data.king_to, MOVE_CASTLE));
                    }
                }
            }
            
            // Queenside castling
            if (castling[side_to_move][1]) {
                const auto& data = CASTLING_DATA[side_to_move][1];
                
                // Check squares between king and rook are empty
                bool path_clear = (all & data.path_mask) == 0;
                
                // Check king doesn't pass through or land in check
                if (path_clear) {
                    bool safe = true;
                    
                    // Check only the squares the king moves through
                    uint64_t check_mask = data.check_squares_mask;
                    while (check_mask) {
                        int check_sq = Bitboards::pop_lsb(check_mask);
                        if (Bitboards::is_square_attacked(*this, check_sq, 1 - side_to_move)) {
                            safe = false;
                            break;
                        }
                    }
                    
                    if (safe) {
                        moves.push_back(Bitboards::make_move(data.king_from, data.king_to, MOVE_CASTLE));
                    }
                }
            }
        }
    }
    
    return moves;
}



namespace Bitboards {

uint64_t knight_attacks(int square) {
   
   
   static const uint64_t table[64] = {
    0x0000000000020400ULL, 0x0000000000050800ULL, 0x00000000000a1100ULL, 0x0000000000142200ULL, 0x0000000000284400ULL, 0x0000000000508800ULL, 0x0000000000a01000ULL, 0x0000000000402000ULL, 
    0x0000000002040004ULL, 0x0000000005080008ULL, 0x000000000a110011ULL, 0x0000000014220022ULL, 0x0000000028440044ULL, 0x0000000050880088ULL, 0x00000000a0100010ULL, 0x0000000040200020ULL, 
    0x0000000204000402ULL, 0x0000000508000805ULL, 0x0000000a1100110aULL, 0x0000001422002214ULL, 0x0000002844004428ULL, 0x0000005088008850ULL, 0x000000a0100010a0ULL, 0x0000004020002040ULL, 
    0x0000020400040200ULL, 0x0000050800080500ULL, 0x00000a1100110a00ULL, 0x0000142200221400ULL, 0x0000284400442800ULL, 0x0000508800885000ULL, 0x0000a0100010a000ULL, 0x0000402000204000ULL, 
    0x0002040004020000ULL, 0x0005080008050000ULL, 0x000a1100110a0000ULL, 0x0014220022140000ULL, 0x0028440044280000ULL, 0x0050880088500000ULL, 0x00a0100010a00000ULL, 0x0040200020400000ULL, 
    0x0204000402000000ULL, 0x0508000805000000ULL, 0x0a1100110a000000ULL, 0x1422002214000000ULL, 0x2844004428000000ULL, 0x5088008850000000ULL, 0xa0100010a0000000ULL, 0x4020002040000000ULL, 
    0x0400040200000000ULL, 0x0800080500000000ULL, 0x1100110a00000000ULL, 0x2200221400000000ULL, 0x4400442800000000ULL, 0x8800885000000000ULL, 0x100010a000000000ULL, 0x2000204000000000ULL, 
    0x0004020000000000ULL, 0x0008050000000000ULL, 0x00110a0000000000ULL, 0x0022140000000000ULL, 0x0044280000000000ULL, 0x0088500000000000ULL, 0x0010a00000000000ULL, 0x0020400000000000ULL
};
   
   
   
    return table[square];
}

uint64_t king_attacks(int square) {
    static const uint64_t table[64] = {
        0x0000000000000302ULL, 0x0000000000000507ULL, 0x0000000000000A0EULL, 0x000000000000141CULL,
        0x0000000000002838ULL, 0x0000000000005070ULL, 0x000000000000A0E0ULL, 0x00000000000040C0ULL,
        0x0000000000030203ULL, 0x0000000000070507ULL, 0x00000000000E0A0EULL, 0x00000000001C141CULL,
        0x0000000000382838ULL, 0x0000000000705070ULL, 0x0000000000E0A0E0ULL, 0x0000000000C040C0ULL,
        0x0000000003020300ULL, 0x0000000007050700ULL, 0x000000000E0A0E00ULL, 0x000000001C141C00ULL,
        0x0000000038283800ULL, 0x0000000070507000ULL, 0x00000000E0A0E000ULL, 0x00000000C040C000ULL,
        0x0000000302030000ULL, 0x0000000705070000ULL, 0x0000000E0A0E0000ULL, 0x0000001C141C0000ULL,
        0x0000003828380000ULL, 0x0000007050700000ULL, 0x000000E0A0E00000ULL, 0x000000C040C00000ULL,
        0x0000030203000000ULL, 0x0000070507000000ULL, 0x00000E0A0E000000ULL, 0x00001C141C000000ULL,
        0x0000382838000000ULL, 0x0000705070000000ULL, 0x0000E0A0E0000000ULL, 0x0000C040C0000000ULL,
        0x0003020300000000ULL, 0x0007050700000000ULL, 0x000E0A0E00000000ULL, 0x001C141C00000000ULL,
        0x0038283800000000ULL, 0x0070507000000000ULL, 0x00E0A0E000000000ULL, 0x00C040C000000000ULL,
        0x0302030000000000ULL, 0x0705070000000000ULL, 0x0E0A0E0000000000ULL, 0x1C141C0000000000ULL,
        0x3828380000000000ULL, 0x7050700000000000ULL, 0xE0A0E00000000000ULL, 0xC040C00000000000ULL,
        0x0203000000000000ULL, 0x0507000000000000ULL, 0x0A0E000000000000ULL, 0x141C000000000000ULL,
        0x2838000000000000ULL, 0x5070000000000000ULL, 0xA0E0000000000000ULL, 0x40C0000000000000ULL
    };
    return table[square];
}


uint64_t bishop_attacks(int square, uint64_t blockers) {
    uint64_t attacks = 0;
    
    // Directions: up-right, up-left, down-right, down-left
    int dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];
        
        int x = Bitboards::file_of(square);
        int y = Bitboards::rank_of(square);
        
        for (int step = 1; step < 8; step++) {
            int nx = x + dx * step;
            int ny = y + dy * step;
            
            if (nx < 0 || nx >= 8 || ny < 0 || ny >= 8) break;
            
            int nsq = nx + ny * 8;
            Bitboards::set(attacks, nsq);
            
            if (blockers & (1ULL << nsq)) {
                break; // Blocked by a piece
            }
        }
    }
    
    return attacks;
}



uint64_t rook_attacks(int square, uint64_t blockers) {
    uint64_t attacks = 0;
    int file = file_of(square);
    int rank = rank_of(square);
    
    // North
    for (int r = rank + 1; r < 8; r++) {
        int sq = file + r * 8;
        set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    // South
    for (int r = rank - 1; r >= 0; r--) {
        int sq = file + r * 8;
        set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    // East
    for (int f = file + 1; f < 8; f++) {
        int sq = f + rank * 8;
        set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    // West
    for (int f = file - 1; f >= 0; f--) {
        int sq = f + rank * 8;
        set(attacks, sq);
        if (blockers & (1ULL << sq)) break;
    }
    
    return attacks;
}



uint64_t queen_attacks(int square, uint64_t blockers) {
    return bishop_attacks(square, blockers) | rook_attacks(square, blockers);
}

uint64_t pawn_attacks(int square, int color) {
    uint64_t attacks = 0;
    int file = file_of(square);
    int rank = rank_of(square);
    
    int forward = (color == WHITE) ? 1 : -1;
    
    if (file > 0 && rank + forward >= 0 && rank + forward < 8) {
        Bitboards::set(attacks, square + forward * 8 - 1);
    }
    if (file < 7 && rank + forward >= 0 && rank + forward < 8) {
        Bitboards::set(attacks, square + forward * 8 + 1);
    }
    
    return attacks;
}

bool is_square_attacked(const Board& board, int square, int color) {
    uint64_t enemy_pawns = board.pieces[PAWN] & board.colors[color];
    uint64_t enemy_knights = board.pieces[KNIGHT] & board.colors[color];
    uint64_t enemy_bishops = board.pieces[BISHOP] & board.colors[color];
    uint64_t enemy_rooks = board.pieces[ROOK] & board.colors[color];
    uint64_t enemy_queens = board.pieces[QUEEN] & board.colors[color];
    uint64_t enemy_king = board.pieces[KING] & board.colors[color];
    
    // Pawn attacks
    if (Bitboards::pawn_attacks(square, 1 - color) & enemy_pawns) return true;
    
    // Knight attacks
    if (Bitboards::knight_attacks(square) & enemy_knights) return true;
    
    // King attacks
    if (Bitboards::king_attacks(square) & enemy_king) return true;
    
    // Sliding attacks
    uint64_t all = board.all_pieces();
    if (Bitboards::bishop_attacks(square, all) & (enemy_bishops | enemy_queens)) return true;
    if (Bitboards::rook_attacks(square, all) & (enemy_rooks | enemy_queens)) return true;
    
    return false;
}

uint64_t all_attacks(const Board& board, int color) {
    uint64_t attacks = 0;
    
    // Pawn attacks
    uint64_t pawns = board.pieces[PAWN] & board.colors[color];
    while (pawns) {
        int sq = Bitboards::pop_lsb(pawns);
        attacks |= Bitboards::pawn_attacks(sq, color);
    }
    
    // Knight attacks
    uint64_t knights = board.pieces[KNIGHT] & board.colors[color];
    while (knights) {
        int sq = Bitboards::pop_lsb(knights);
        attacks |= Bitboards::knight_attacks(sq);
    }
    
    // King attacks
    uint64_t king = board.pieces[KING] & board.colors[color];
    while (king) {
        int sq = Bitboards::pop_lsb(king);
        attacks |= Bitboards::king_attacks(sq);
    }
    
    // Sliding attacks
    uint64_t all = board.all_pieces();
    uint64_t bishops = board.pieces[BISHOP] & board.colors[color];
    while (bishops) {
        int sq = Bitboards::pop_lsb(bishops);
        attacks |= Bitboards::bishop_attacks(sq, all);
    }
    
    uint64_t rooks = board.pieces[ROOK] & board.colors[color];
    while (rooks) {
        int sq = Bitboards::pop_lsb(rooks);
        attacks |= Bitboards::rook_attacks(sq, all);
    }
    
    uint64_t queens = board.pieces[QUEEN] & board.colors[color];
    while (queens) {
        int sq = Bitboards::pop_lsb(queens);
        attacks |= Bitboards::queen_attacks(sq, all);
    }
    
    return attacks;
}

// Convert move to UCI notation
std::string move_to_uci(int move_value) {
    int from = move_from(move_value);
    int to = move_to(move_value);
    int flags = move_flags(move_value);
    int promo = move_promotion(move_value);
    
    char from_file = 'a' + file_of(from);
    char from_rank = '1' + rank_of(from);
    char to_file = 'a' + file_of(to);
    char to_rank = '1' + rank_of(to);
    
    std::string result;
    result += from_file;
    result += from_rank;
    result += to_file;
    result += to_rank;
    
    // Add promotion piece
    if (flags == MOVE_PROMOTION) {
        const char promo_chars[] = "nbrq";
        result += promo_chars[promo];
    }
    
    return result;
}

// Convert UCI to move (will need board context for full validation)
int uci_to_move(const std::string& uci) {
    if (uci.length() < 4) return 0;
    
    int from_file = uci[0] - 'a';
    int from_rank = uci[1] - '1';
    int to_file = uci[2] - 'a';
    int to_rank = uci[3] - '1';
    
    int from_sq = (from_rank << 3) | from_file;
    int to_sq = (to_rank << 3) | to_file;
    
    // Check for promotion
    if (uci.length() == 5) {
        char promo_char = uci[4];
        int promo = 0;
        switch (promo_char) {
            case 'n': promo = 0; break; // Knight
            case 'b': promo = 1; break; // Bishop
            case 'r': promo = 2; break; // Rook
            case 'q': promo = 3; break; // Queen
            default: promo = 3; break;  // Default to queen
        }
        return make_move(from_sq, to_sq, MOVE_PROMOTION, promo);
    }
    
    return make_move(from_sq, to_sq);
}

} // namespace Bitboards
