import subprocess
import sys
import os
from pathlib import Path

def main():
    # Get LUMINA_DIR from environment
    lumina_dir = os.getenv('LUMINA_DIR')
    if not lumina_dir:
        print("ERROR: LUMINA_DIR environment variable not set")
        return 1
    
    reflector_exe = Path(lumina_dir) / "Binaries" / "Shipping-windows-x86_64" / "Reflector.exe"
    
    if len(sys.argv) < 2:
        print("Usage: RunReflector.py <solution_path>")
        return 1
    
    solution_path = sys.argv[1]
    
    print("=" * 60)
    print("Running Reflector")
    print("=" * 60)
    print(f"Executable: {reflector_exe}")
    print(f"Solution:   {solution_path}")
    print(f"Working Dir: {os.getcwd()}")
    print("=" * 60)
    print()
    
    if not reflector_exe.exists():
        print(f"ERROR: Reflector executable not found: {reflector_exe}")
        return 1
    
    if not os.path.exists(solution_path):
        print(f"ERROR: Solution file not found: {solution_path}")
        return 1
    
    # Run reflector and stream output in real-time
    process = subprocess.Popen(
        [str(reflector_exe), solution_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )
    
    # Print output as it comes
    for line in process.stdout:
        print(line, end='', flush=True)
    
    process.wait()
    
    print()
    print("=" * 60)
    if process.returncode == 0:
        print("Reflector completed successfully")
    else:
        print(f"Reflector FAILED with code {process.returncode}")
    print("=" * 60)
    
    return process.returncode

if __name__ == "__main__":
    sys.exit(main())