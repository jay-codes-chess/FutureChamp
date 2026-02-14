#include "movegen.hpp"

void generate_moves(const Board& board, MoveList& list) {
    list.count = 0;
    
    int side = board.side_to_move;
    uint64_t our_pieces = board.pieces_of_color(side);
    uint64_t enemy_pieces = board.pieces_of_color(1 - side);
    uint64_t all = board.all_pieces();
    
    // 1. Pawn moves
    uint64_t pawns = board.pieces[PAWN] & board.colors[side];
    while (pawns) {
        int sq = Bitboards::pop_lsb(pawns);
        int rank = Bitboards::rank_of(sq);
        int file = Bitboards::file_of(sq);
        
        int forward_dir = (side == WHITE) ? 8 : -8;
        
        // Single push
        int forward = sq + forward_dir;
        if (forward >= 0 && forward < 64 && board.is_empty(forward)) {
            int to_rank = Bitboards::rank_of(forward);
            
            if ((side == WHITE && to_rank == 7) || (side == BLACK && to_rank == 0)) {
                list.add(Bitboards::make_move(sq, forward, MOVE_PROMOTION, QUEEN));
                list.add(Bitboards::make_move(sq, forward, MOVE_PROMOTION, ROOK));
                list.add(Bitboards::make_move(sq, forward, MOVE_PROMOTION, BISHOP));
                list.add(Bitboards::make_move(sq, forward, MOVE_PROMOTION, KNIGHT));
            } else {
                list.add(Bitboards::make_move(sq, forward));
            }
            
            // Double push from starting rank
            int start_rank = (side == WHITE) ? 1 : 6;
            if (rank == start_rank) {
                int double_forward = sq + 2 * forward_dir;
                if (board.is_empty(double_forward)) {
                    list.add(Bitboards::make_move(sq, double_forward));
                }
            }
        }
        
        // Captures
        int left = sq + forward_dir - 1;
        int right = sq + forward_dir + 1;
        
        if (left >= 0 && left < 64 && Bitboards::file_of(left) == file - 1) {
            if (enemy_pieces & (1ULL << left)) {
                int to_rank = Bitboards::rank_of(left);
                if ((side == WHITE && to_rank == 7) || (side == BLACK && to_rank == 0)) {
                    list.add(Bitboards::make_move(sq, left, MOVE_PROMOTION, QUEEN));
                    list.add(Bitboards::make_move(sq, left, MOVE_PROMOTION, ROOK));
                    list.add(Bitboards::make_move(sq, left, MOVE_PROMOTION, BISHOP));
                    list.add(Bitboards::make_move(sq, left, MOVE_PROMOTION, KNIGHT));
                } else {
                    list.add(Bitboards::make_move(sq, left));
                }
            }
            if (left == board.en_passant_square) {
                list.add(Bitboards::make_move(sq, left, MOVE_EN_PASSANT));
            }
        }
        
        if (right >= 0 && right < 64 && Bitboards::file_of(right) == file + 1) {
            if (enemy_pieces & (1ULL << right)) {
                int to_rank = Bitboards::rank_of(right);
                if ((side == WHITE && to_rank == 7) || (side == BLACK && to_rank == 0)) {
                    list.add(Bitboards::make_move(sq, right, MOVE_PROMOTION, QUEEN));
                    list.add(Bitboards::make_move(sq, right, MOVE_PROMOTION, ROOK));
                    list.add(Bitboards::make_move(sq, right, MOVE_PROMOTION, BISHOP));
                    list.add(Bitboards::make_move(sq, right, MOVE_PROMOTION, KNIGHT));
                } else {
                    list.add(Bitboards::make_move(sq, right));
                }
            }
            if (right == board.en_passant_square) {
                list.add(Bitboards::make_move(sq, right, MOVE_EN_PASSANT));
            }
        }
    }
    
    // 2. Knight moves
    uint64_t knights = board.pieces[KNIGHT] & board.colors[side];
    while (knights) {
        int sq = Bitboards::pop_lsb(knights);
        uint64_t attacks = Bitboards::knight_attacks(sq) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            list.add(Bitboards::make_move(sq, to));
        }
    }
    
    // 3. Bishop moves
    uint64_t bishops = board.pieces[BISHOP] & board.colors[side];
    while (bishops) {
        int sq = Bitboards::pop_lsb(bishops);
        uint64_t attacks = Bitboards::bishop_attacks(sq, all) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            list.add(Bitboards::make_move(sq, to));
        }
    }
    
    // 4. Rook moves
    uint64_t rooks = board.pieces[ROOK] & board.colors[side];
    while (rooks) {
        int sq = Bitboards::pop_lsb(rooks);
        uint64_t attacks = Bitboards::rook_attacks(sq, all) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            list.add(Bitboards::make_move(sq, to));
        }
    }
    
    // 5. Queen moves
    uint64_t queens = board.pieces[QUEEN] & board.colors[side];
    while (queens) {
        int sq = Bitboards::pop_lsb(queens);
        uint64_t attacks = Bitboards::queen_attacks(sq, all) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            list.add(Bitboards::make_move(sq, to));
        }
    }
    
    // 6. King moves
    uint64_t kings = board.pieces[KING] & board.colors[side];
    while (kings) {
        int sq = Bitboards::pop_lsb(kings);
        uint64_t attacks = Bitboards::king_attacks(sq) & ~our_pieces;
        while (attacks) {
            int to = Bitboards::pop_lsb(attacks);
            list.add(Bitboards::make_move(sq, to));
        }
        
        // Castling
        if (!board.is_in_check(side)) {
            int kingside_sq = (side == WHITE) ? 6 : 62;
            int queenside_sq = (side == WHITE) ? 2 : 58;
            
            // Kingside castling
            if (board.castling[side][0]) {
                bool path_clear = true;
                for (int s = sq + 1; s < kingside_sq; s++) {
                    if (!board.is_empty(s)) { path_clear = false; break; }
                }
                if (path_clear && !Bitboards::is_square_attacked(board, sq + 1, 1 - side) &&
                    !Bitboards::is_square_attacked(board, kingside_sq, 1 - side)) {
                    list.add(Bitboards::make_move(sq, kingside_sq, MOVE_CASTLE));
                }
            }
            
            // Queenside castling
            if (board.castling[side][1]) {
                bool path_clear = true;
                for (int s = sq - 1; s > queenside_sq; s--) {
                    if (!board.is_empty(s)) { path_clear = false; break; }
                }
                if (path_clear && !Bitboards::is_square_attacked(board, sq - 1, 1 - side) &&
                    !Bitboards::is_square_attacked(board, queenside_sq, 1 - side)) {
                    list.add(Bitboards::make_move(sq, queenside_sq, MOVE_CASTLE));
                }
            }
        }
    }
}
