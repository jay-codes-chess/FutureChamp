import re

# Read the file
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'r') as f:
    content = f.read()

# Pattern: auto xxx = board.generate_moves(); -> MoveList xxx; generate_moves(board, xxx);
# Also: return board.generate_moves();

replacements = [
    # return statement
    (r'return board\.generate_moves\(\);', r'MoveList _m; generate_moves(board, _m); return _m;'),
    # auto legal_moves = board.generate_moves();
    (r'auto legal_moves = board\.generate_moves\(\);', r'MoveList legal_moves; generate_moves(board, legal_moves);'),
    # auto all_moves = board.generate_moves();
    (r'auto all_moves = board\.generate_moves\(\);', r'MoveList all_moves; generate_moves(board, all_moves);'),
    # auto moves = board.generate_moves();
    (r'auto moves = board\.generate_moves\(\);', r'MoveList moves; generate_moves(board, moves);'),
]

for pattern, replacement in replacements:
    content = re.sub(pattern, replacement, content)

# Write back
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'w') as f:
    f.write(content)

print("Done")
