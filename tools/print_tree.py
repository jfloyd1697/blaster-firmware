# dump_folder_structure.py
import os
import fnmatch


def matches_exclude(path, exclude_patterns):
    """
    Returns True if the given path matches any of the exclude patterns.
    """
    return any(fnmatch.fnmatch(path, pattern) for pattern in exclude_patterns)


def print_tree(root_dir, prefix="", exclude_patterns=None):
    """
    Recursively prints folder structure like a tree, excluding paths matching patterns.

    :param root_dir: Root directory to scan
    :param prefix: Internal prefix for tree formatting
    :param exclude_patterns: List of wildcard patterns to exclude
    """
    if exclude_patterns is None:
        exclude_patterns = []

    entries = sorted(os.listdir(root_dir))
    # Filter out entries matching any exclude pattern
    entries = [e for e in entries if not matches_exclude(e, exclude_patterns)]

    for i, entry in enumerate(entries):
        path = os.path.join(root_dir, entry)
        connector = "└── " if i == len(entries) - 1 else "├── "
        print(f"{prefix}{connector}{entry}")
        if os.path.isdir(path):
            extension = "    " if i == len(entries) - 1 else "│   "
            print_tree(path, prefix + extension, exclude_patterns)


if __name__ == "__main__":
    folder_to_dump = ".."  # Change to your target folder
    # Exclude folders like build, .git, and __pycache__
    excluded_patterns = [
        "__pycache__",
        ".git",
        ".idea",
        ".venv",
        "build",
        "cmake-build-debug",
        "cmake-build-release",
        "scripts",
        "*.wav",
    ]
    print()
    print_tree(folder_to_dump, exclude_patterns=excluded_patterns)
