# Shadows the pyperf venv's psutil, whose C extension is compiled against one
# object layout and crashes an interpreter with another.  pyperf imports
# psutil only for metadata and treats ImportError as "not installed".
raise ImportError("psutil is shadowed for measurement")
