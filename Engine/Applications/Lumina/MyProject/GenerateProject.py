import os
import subprocess
import time

try:
    result = subprocess.run(["Tools/Premake5.exe", "vs2022"], check=True)   
    print("Solution generated successfully!")
        
except subprocess.CalledProcessError as e:
    print(f"Error generating solution: {e}")