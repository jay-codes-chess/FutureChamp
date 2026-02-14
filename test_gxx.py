import subprocess

gxx = r'C:\ProgramData\mingw64\mingw64\bin\g++.exe'
result = subprocess.run([gxx, '--version'], capture_output=True)
print('stdout:', result.stdout.decode())
print('stderr:', result.stderr.decode())
print('returncode:', result.returncode)
