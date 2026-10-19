#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Doubly Linked List Node representing a single line of text
typedef struct Node {
    char *line;         // Dynamically allocated string for line content
    struct Node *prev;  // Pointer to previous line
    struct Node *next;  // Pointer to next line
} Node;

// Main TextEditor state structure
typedef struct {
    Node *head;         // Pointer to the first line
    Node *tail;         // Pointer to the last line
    Node *cursor;       // Pointer to the current line
    int cursorLine;     // Line number of the cursor (1-indexed)
    int cursorPos;      // Column position of the cursor (0-indexed)
    char *clipboard;    // Internal clipboard buffer for copy/cut/paste
    char *currentFile;  // Currently opened filename (if any)
} TextEditor;

// Helper utility to duplicate strings safely in standard C99
static inline char* safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len + 1);
    }
    return dup;
}

// Create a new line node
static inline Node* createNode(const char *text) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Failed to allocate memory for Node");
        exit(EXIT_FAILURE);
    }
    newNode->line = safe_strdup(text ? text : "");
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

// Initialize the TextEditor structure with one empty line
static inline void initEditor(TextEditor *editor) {
    Node *initialLine = createNode("");
    editor->head = initialLine;
    editor->tail = initialLine;
    editor->cursor = initialLine;
    editor->cursorLine = 1;
    editor->cursorPos = 0;
    editor->clipboard = NULL;
    editor->currentFile = NULL;
}

// Free all lines in the editor
static inline void freeEditorLines(TextEditor *editor) {
    Node *curr = editor->head;
    while (curr) {
        Node *next = curr->next;
        free(curr->line);
        free(curr);
        curr = next;
    }
    editor->head = NULL;
    editor->tail = NULL;
    editor->cursor = NULL;
    editor->cursorLine = 0;
    editor->cursorPos = 0;
}

// Clean up entire editor resources
static inline void cleanupEditor(TextEditor *editor) {
    freeEditorLines(editor);
    if (editor->clipboard) {
        free(editor->clipboard);
        editor->clipboard = NULL;
    }
    if (editor->currentFile) {
        free(editor->currentFile);
        editor->currentFile = NULL;
    }
}

#endif // TEXT_EDITOR_H
