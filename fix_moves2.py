import re

# Read the file
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'r') as f:
    content = f.read()

# Replace all patterns that call generate_moves(board) returning vector
# with MoveList usage

# Pattern 1: auto all_moves = generate_moves(board);
content = re.sub(
    r'auto all_moves = generate_moves\(board\);',
    'MoveList all_moves; generate_moves(board, all_moves);',
    content
)

# Pattern 2: auto all_legal_moves = generate_moves(board);  
content = re.sub(
    r'auto all_legal_moves = generate_moves\(board\);',
    'MoveList all_legal_moves; generate_moves(board, all_legal_moves);',
    content
)

# Pattern 3: in generate_candidates - auto all_moves = generate_moves(board);
# Already handled above

# Write back
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'w') as f:
    f.write(content)

print("Done")
