import subprocess
import os
import sys

os.chdir(r'C:\Users\Jay\.openclaw\workspace\engine_temp\source code for Jay')

gxx = r'C:\ProgramData\mingw64\mingw64\bin\g++.exe'

# Test compile a simple program
test_src = 'test.cpp'
test_exe = 'test.exe'

cmd = [gxx, '-o', test_exe, test_src]

print('Testing simple compile...', file=sys.stderr)
result = subprocess.run(cmd, capture_output=True)
print('stdout:', result.stdout, file=sys.stderr)
print('stderr:', result.stderr, file=sys.stderr)
print('returncode:', result.returncode, file=sys.stderr)
