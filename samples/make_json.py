import os
import json

def collect_aki_files():
    current_dir = os.getcwd()
    aki_files = []

    for file in os.listdir(current_dir):
        if file.endswith('.aki'):
            absolute_path = os.path.abspath(os.path.join(current_dir, file))
            aki_entry = {
                "path": absolute_path,
                "commands": []
            }
            aki_files.append(aki_entry)

    with open('aki_files.json', 'w') as f:
        json.dump(aki_files, f, indent=2)

if __name__ == "__main__":
    collect_aki_files()
