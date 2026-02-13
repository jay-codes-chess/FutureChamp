/**
 * Board Representation using Bitboards
 * 
 * Efficient board representation for fast move generation
 * and attack detection.
 * 
 * Square numbering:
 *  56 57 58 59 60 61 62 63
 *  48 49 50 51 52 53 54 55
 *  40 41 42 43 44 45 46 47
 *  32 33 34 35 36 37 38 39
 *  24 25 26 27 28 29 30 31
 *  16 17 18 19 20 21 22 23
 *   8  9 10 11 12 13 14 15
 *   0  1  2  3  4  5  6  7
 * 
 * ENHANCED MOVE ENCODING (16 bits):
 *   Bits 0-5:   To square (0-63)
 *   Bits 6-11:  From square (0-63)
 *   Bits 12-13: Promotion piece (0=none, 1=N, 2=B, 3=R, 4=Q)
 *   Bits 14-15: Special flags (0=normal, 1=castling, 2=en passant, 3=promotion)
 */

#ifndef BOARD_HPP
#define BOARD_HPP

#include <cstdint>
#include <string>
#include <vector>

inline uint64_t attack_bb[2]; // [color] - precomputed attacks


// Piece types
enum PieceType {
    NO_PIECE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

// Colors
enum Color {
    WHITE = 0,
    BLACK = 1,
    BOTH = 2
};

// Move flags
enum MoveFlags {
    MOVE_NORMAL = 0,
    MOVE_CASTLE = 1,
    MOVE_EN_PASSANT = 2,
    MOVE_PROMOTION = 3
};

struct Board {
    // Bitboards for each piece type
    uint64_t pieces[7];  // [piece_type]
    
    // Bitboards for each color
    uint64_t colors[2];   // [color]
    
    // Side to move
    int side_to_move;
    
    // Castling rights
    bool castling[2][2];  // [color][kingside/queenside]
    
    // En passant square
    int en_passant_square;
    
    // Move counters
    int fullmove_number;
    int halfmove_clock;
    
    // Hash for transposition table
    uint64_t hash;
    
    Board() {
        reset();
    }
    
    void reset() {
        for (int i = 0; i < 7; i++) pieces[i] = 0;
        colors[WHITE] = colors[BLACK] = 0;
        side_to_move = WHITE;
        castling[WHITE][0] = castling[WHITE][1] = true;
        castling[BLACK][0] = castling[BLACK][1] = true;
        en_passant_square = -1;
        fullmove_number = 1;
        halfmove_clock = 0;
        hash = 0;
    }
    
    // Set up starting position
    void set_start_position();
    
    // Set position from FEN
    bool set_from_fen(const std::string& fen);
    
    // Get FEN string
    std::string get_fen() const;
    
    // Clear board
    void clear();
    
    // Add piece
    void add_piece(int square, int piece_type, int color);
    
    // Remove piece
    void remove_piece(int square);
    
    // Move piece
    void move_piece(int from, int to);
    
    // Get piece at square
    int piece_at(int square) const;
    
    // Color of piece at square
    int color_at(int square) const;
    
    // Is square empty?
    bool is_empty(int square) const;
    
    // Get all pieces of a color
    uint64_t pieces_of_color(int color) const;
    
    // Get all pieces
    uint64_t all_pieces() const;
    
    // Generate hash
    void compute_hash();
    
    // Is side in check?
    bool is_in_check(int color) const;
    
    // Generate pseudo-legal moves (for evaluation)
    std::vector<int> generate_moves() const;
};

// Bitboard operations
namespace Bitboards {
    
    // Population count
    inline int popcount(uint64_t bb) {
        return __builtin_popcountll(bb);
    }
    
    // Get least significant bit
    inline int lsb(uint64_t bb) {
        return __builtin_ctzll(bb);
    }
    
    // Get most significant bit
    inline int msb(uint64_t bb) {
        return 63 - __builtin_clzll(bb);
    }
    
    // Clear least significant bit and return index
    inline int pop_lsb(uint64_t& bb) {
        int idx = lsb(bb);
        bb &= bb - 1;  // Clear LSB
        return idx;
    }
    
    // Test if bit is set
    inline bool test(uint64_t bb, int square) {
        return bb & (1ULL << square);
    }
    
    // Set bit
    inline void set(uint64_t& bb, int square) {
        bb |= (1ULL << square);
    }
    
    // Clear bit
    inline void clear(uint64_t& bb, int square) {
        bb &= ~(1ULL << square);
    }
    
    // Rank of square
    inline int rank_of(int square) {
        return square >> 3;
    }
    
    // File of square
    inline int file_of(int square) {
        return square & 7;
    }
    
    // Square from file and rank
    inline int square(int file, int rank) {
        return (rank << 3) | file;
    }
    
    // Color of square
    inline int color_of(int square) {
        return (file_of(square) + rank_of(square)) % 2;
    }
    
    // Mirror square
    inline int mirror(int square) {
        return 63 - square;
    }
    
    // Directions for sliding pieces
    constexpr int DIRECTIONS[8] = {0, 1, -1, 8, -8, 9, -9, 7};
    
    // Knight moves offsets
    constexpr int KNIGHT_OFFSETS[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    
    // King moves offsets
    constexpr int KING_OFFSETS[8] = {-9, -8, -7, -1, 1, 7, 8, 9};
    
    // Pawn attack directions
    constexpr int PAWN_ATTACK[2][2] = {{7, 9}, {-7, -9}};
    
    // Pawn move directions
    constexpr int PAWN_MOVE[2] = {8, -8};
    
    // Generate knight moves
    uint64_t knight_attacks(int square);
    
    // Generate king moves
    uint64_t king_attacks(int square);
    
    // Generate sliding attacks (bishop/rook/queen)
    uint64_t bishop_attacks(int square, uint64_t blockers);
    uint64_t rook_attacks(int square, uint64_t blockers);
    uint64_t queen_attacks(int square, uint64_t blockers);
    
    // Generate pawn attacks
    uint64_t pawn_attacks(int square, int color);
    
    // Generate pawn moves (non-captures)
    uint64_t pawn_moves(int square, int color, uint64_t all_pieces);
    
    // Is square attacked by color?
    bool is_square_attacked(const Board& board, int square, int color);
    
    // All attacks by color
    uint64_t all_attacks(const Board& board, int color);
    
    // Move encoding/decoding helpers
    inline int make_move(int from, int to, int flags = 0, int promo = 0) {
        return to | (from << 6) | (promo << 12) | (flags << 14);
    }
    
    inline int move_from(int move) { return (move >> 6) & 63; }
    inline int move_to(int move) { return move & 63; }
    inline int move_promotion(int move) { return (move >> 12) & 3; }
    inline int move_flags(int move) { return (move >> 14) & 3; }
    
    inline bool is_promotion(int move) { return move_flags(move) == MOVE_PROMOTION; }
    inline bool is_castle(int move) { return move_flags(move) == MOVE_CASTLE; }
    inline bool is_en_passant(int move) { return move_flags(move) == MOVE_EN_PASSANT; }
    
    // Convert move to UCI notation
    std::string move_to_uci(int move_value);
    
    // Convert UCI to move
    int uci_to_move(const std::string& uci);
}

#endif // BOARD_HPP
