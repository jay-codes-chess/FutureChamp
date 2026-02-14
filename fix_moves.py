import re

# Read the file
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'r') as f:
    content = f.read()

# Pattern 1: board.generate_moves() -> MoveList xxx; generate_moves(board, xxx)
def replace1(match):
    var_name = match.group(1)
    return f'MoveList {var_name}; generate_moves(board, {var_name});'

content = re.sub(
    r'(\w+)\s*=\s*board\.generate_moves\(\);',
    replace1,
    content
)

# Pattern 2: auto xxx = generate_moves(board); -> MoveList xxx; generate_moves(board, xxx);
def replace2(match):
    var_name = match.group(1)
    return f'MoveList {var_name}; generate_moves(board, {var_name});'

content = re.sub(
    r'auto\s+(\w+)\s*=\s*generate_moves\(board\);',
    replace2,
    content
)

# Write back
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'w') as f:
    f.write(content)

print("Done")
