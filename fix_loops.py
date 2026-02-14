import re

# Read the file
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'r') as f:
    content = f.read()

# Replace range-based for loops with index-based
# Pattern: for (int move : moves) -> for (int i = 0; i < moves.count; i++) { int move = moves.moves[i];
# Similarly for legal_moves, all_moves, all_legal_moves

# Need to handle different variable names
patterns = [
    (r'for \(int move : moves\)', 'for (int i = 0; i < moves.count; i++) { int move = moves.moves[i];'),
    (r'for \(int move : all_moves\)', 'for (int i = 0; i < all_moves.count; i++) { int move = all_moves.moves[i];'),
    (r'for \(int legal_move : legal_moves\)', 'for (int i = 0; i < legal_moves.count; i++) { int legal_move = legal_moves.moves[i];'),
    (r'for \(int legal_move : all_moves\)', 'for (int i = 0; i < all_moves.count; i++) { int legal_move = all_moves.moves[i];'),
    (r'for \(int legal_move : all_legal_moves\)', 'for (int i = 0; i < all_legal_moves.count; i++) { int legal_move = all_legal_moves.moves[i];'),
]

for pattern, replacement in patterns:
    content = re.sub(pattern, replacement, content)

# Now add closing brace for each loop - need to find where the loop body ends
# Actually, the simplest approach is to add the closing brace after each for loop
# Let's do this more carefully - replace the whole for loop structure

# Actually, let's be more surgical - add "} " before each "for (int..." that was converted
# This is tricky. Let me try a different approach - wrap the body in braces

# The issue is the original code relies on the vector's begin/end
# Let me add begin/end methods to MoveList instead
# That's cleaner!

# Write current state first to see what's there
with open(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay\src\search\search.cpp', 'w') as f:
    f.write(content)

print("Done - need to add begin/end to MoveList")
