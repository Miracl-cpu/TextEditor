# Console-Based Text Editor in C (v1.0)

## Project Abstract
This project implements a console-based text editor written in C that provides essential text editing capabilities with advanced features like multi-level undo/redo functionality. The editor uses a **doubly linked list** data structure to manage lines of text and **dynamic array stacks** for implementing undo/redo operations.

### Key Features
- Text insertion and deletion at cursor position
- Multi-level undo/redo operations using stack-based implementation
- Cursor navigation (`up`, `down`, `left`, `right`, `home`, `end`, `top`, `bottom`)
- File operations (`open`, `save`, `close`)
- Text search and replace functionality (`find`, `replace`)
- Clipboard operations (`copy`, `cut`, `paste`)
- Real-time cursor position tracking
- Interactive command-line interface

---

## Project Structure
```text
TextEditor/
├── text_editor.c           # Main controller and interactive CLI loop
├── text_editor.h           # Core data structure definitions (Node, TextEditor)
├── file_ops.h              # File operation implementations (open, save, close)
├── text_ops.h              # Text manipulation (insert, delete, copy, cut, paste)
├── curser_navigations.h    # Cursor movement and boundary jumps
├── cursor_nav.h            # Header alias for cursor navigation
├── display_search.h        # Display formatting, find, and replace
├── redo_undo.h             # DynamicArrayStack and Action structures
├── sample.txt              # Sample text file for testing file I/O
├── Makefile                # Build configuration
└── README.md               # Quick start guide and documentation
```

---

## Compilation and Execution

### Using GCC Directly
```bash
gcc -o text_editor text_editor.c -std=c99 -Wall -Wextra
./text_editor
```

### Using Make
```bash
make
./text_editor
```
