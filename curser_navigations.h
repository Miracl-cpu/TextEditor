#ifndef CURSER_NAVIGATIONS_H
#define CURSER_NAVIGATIONS_H

#include "text_editor.h"

// Position the cursor precisely at (targetLine, targetPos)
static inline bool setCursorPosition(TextEditor *editor, int targetLine, int targetPos) {
    if (!editor->head || targetLine < 1) return false;
    Node *curr = editor->head;
    int lineNum = 1;
    while (curr && lineNum < targetLine) {
        curr = curr->next;
        lineNum++;
    }
    if (!curr) return false;
    editor->cursor = curr;
    editor->cursorLine = lineNum;
    int len = (int)strlen(curr->line);
    if (targetPos < 0) targetPos = 0;
    if (targetPos > len) targetPos = len;
    editor->cursorPos = targetPos;
    return true;
}

// Move cursor one line up
static inline void moveCursorUp(TextEditor *editor) {
    if (editor->cursor && editor->cursor->prev) {
        editor->cursor = editor->cursor->prev;
        editor->cursorLine--;
        int len = (int)strlen(editor->cursor->line);
        if (editor->cursorPos > len) {
            editor->cursorPos = len;
        }
        printf("[OK] Cursor moved up to Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
    } else {
        printf("[Error] Already at the first line.\n");
    }
}

// Move cursor one line down
static inline void moveCursorDown(TextEditor *editor) {
    if (editor->cursor && editor->cursor->next) {
        editor->cursor = editor->cursor->next;
        editor->cursorLine++;
        int len = (int)strlen(editor->cursor->line);
        if (editor->cursorPos > len) {
            editor->cursorPos = len;
        }
        printf("[OK] Cursor moved down to Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
    } else {
        printf("[Error] Already at the last line.\n");
    }
}

// Move cursor one character left (wraps to previous line if at column 0)
static inline void moveCursorLeft(TextEditor *editor) {
    if (!editor->cursor) return;
    if (editor->cursorPos > 0) {
        editor->cursorPos--;
        printf("[OK] Cursor moved left to Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
    } else if (editor->cursor->prev) {
        editor->cursor = editor->cursor->prev;
        editor->cursorLine--;
        editor->cursorPos = (int)strlen(editor->cursor->line);
        printf("[OK] Cursor moved left to previous line: Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
    } else {
        printf("[Error] Already at the beginning of the file.\n");
    }
}

// Move cursor one character right (wraps to next line if at end of line)
static inline void moveCursorRight(TextEditor *editor) {
    if (!editor->cursor) return;
    int len = (int)strlen(editor->cursor->line);
    if (editor->cursorPos < len) {
        editor->cursorPos++;
        printf("[OK] Cursor moved right to Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
    } else if (editor->cursor->next) {
        editor->cursor = editor->cursor->next;
        editor->cursorLine++;
        editor->cursorPos = 0;
        printf("[OK] Cursor moved right to next line: Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
    } else {
        printf("[Error] Already at the end of the file.\n");
    }
}

// Jump to the very start of the file (Line 1, Column 0)
static inline void jumpToStartOfFile(TextEditor *editor) {
    if (!editor->head) return;
    editor->cursor = editor->head;
    editor->cursorLine = 1;
    editor->cursorPos = 0;
    printf("[OK] Jumped to start of file (Line 1, Column 0).\n");
}

// Jump to the very end of the file (Last line, last column)
static inline void jumpToEndOfFile(TextEditor *editor) {
    if (!editor->tail) return;
    Node *curr = editor->head;
    int count = 1;
    while (curr && curr->next) {
        curr = curr->next;
        count++;
    }
    editor->cursor = editor->tail;
    editor->cursorLine = count;
    editor->cursorPos = (int)strlen(editor->cursor->line);
    printf("[OK] Jumped to end of file (Line %d, Column %d).\n", editor->cursorLine, editor->cursorPos);
}

// Jump to the beginning of the current line (Column 0)
static inline void jumpToStartOfLine(TextEditor *editor) {
    if (!editor->cursor) return;
    editor->cursorPos = 0;
    printf("[OK] Jumped to start of Line %d (Column 0).\n", editor->cursorLine);
}

// Jump to the end of the current line
static inline void jumpToEndOfLine(TextEditor *editor) {
    if (!editor->cursor) return;
    editor->cursorPos = (int)strlen(editor->cursor->line);
    printf("[OK] Jumped to end of Line %d (Column %d).\n", editor->cursorLine, editor->cursorPos);
}

#endif // CURSER_NAVIGATIONS_H
