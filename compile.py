import subprocess
import os

os.chdir(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay')

gxx = r'C:\ProgramData\mingw64\mingw64\bin\g++.exe'
src = [
    'src/main.cpp',
    'src/eval/evaluation.cpp',
    'src/eval/imbalance.cpp',
    'src/eval/initiative.cpp',
    'src/eval/king_safety.cpp',
    'src/eval/knowledge.cpp',
    'src/eval/material.cpp',
    'src/eval/params.cpp',
    'src/eval/pawn_structure.cpp',
    'src/eval/piece_activity.cpp',
    'src/search/human_selection.cpp',
    'src/search/search.cpp',
    'src/uci/uci.cpp',
    'src/utils/board.cpp'
]

cmd = [gxx, '-O2', '-DNDEBUG', '-std=c++17', '-static', '-o', 'FutureChamp.exe'] + src + ['-lws2_32']

print('Compiling:', ' '.join(cmd))
result = subprocess.run(cmd, capture_output=True, text=True)
print('stdout:', result.stdout)
print('stderr:', result.stderr)
print('returncode:', result.returncode)
