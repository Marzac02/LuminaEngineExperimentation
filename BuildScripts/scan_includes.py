import os
import re
from collections import Counter

include_regex = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]')

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
TARGET_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "..", "Lumina", "Engine"))

def scan_directory(root):
    includes = Counter()

    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if not fname.endswith((".cpp", ".c", ".h", ".hpp", ".inl", ".ixx")):
                continue

            path = os.path.join(dirpath, fname)

            try:
                with open(path, "r", encoding="utf-8", errors="ignore") as f:
                    for line in f:
                        m = include_regex.match(line)
                        if m:
                            includes[m.group(1)] += 1
            except Exception as e:
                print(f"[WARN] Could not read file {path}: {e}")

    return includes


def main():
    print(f"Scanning: {TARGET_DIR}\n")
    includes = scan_directory(TARGET_DIR)

    print("\nMost common includes:\n")
    for inc, count in includes.most_common():
        if(count > 2):
            print(f"{count:5}  {inc}")

    input("\nPress ENTER to exit...")


if __name__ == "__main__":
    main()
