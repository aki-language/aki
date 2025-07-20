import os
import subprocess

def format_files(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cc', '.h')):
                file_path = os.path.join(root, file)
                print(f"Formatting {file_path}")
                subprocess.run(['clang-format', '-style=google', '-i', file_path])

if __name__ == '__main__':
    current_dir = os.getcwd()
    format_files(current_dir)
    print("Done formatting files")
