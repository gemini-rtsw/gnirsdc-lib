import os

# Dynamically set the path for the shared libraries (.so files)
so_file_path = os.path.join(os.path.dirname(__file__), 'your_so_files')

os.environ['LD_LIBRARY_PATH'] = f"{so_file_path}:{os.environ.get('LD_LIBRARY_PATH', '')}"

