#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "text_editor.h"
#include "redo_undo.h"

// Open a file and populate the doubly linked list
static inline void openFile(TextEditor *editor, DynamicArrayStack *undoStack,
                            DynamicArrayStack *redoStack, const char *filename) {
    if (!filename || strlen(filename) == 0) {
        printf("[Error] Please specify a filename to open.\n");
        return;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("[Error] File '%s' not found.\n", filename);
        return;
    }

    freeEditorLines(editor);
    clearStack(undoStack);
    clearStack(redoStack);

    char buffer[4096];
    int lineCount = 0;
    long charCount = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        charCount += (long)len;
        // Strip trailing CR/LF
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[--len] = '\0';
        }
        Node *newNode = createNode(buffer);
        if (!editor->head) {
            editor->head = newNode;
            editor->tail = newNode;
        } else {
            editor->tail->next = newNode;
            newNode->prev = editor->tail;
            editor->tail = newNode;
        }
        lineCount++;
    }

    fclose(fp);

    if (!editor->head) {
        Node *emptyNode = createNode("");
        editor->head = emptyNode;
        editor->tail = emptyNode;
        lineCount = 1;
    }

    editor->cursor = editor->head;
    editor->cursorLine = 1;
    editor->cursorPos = 0;

    if (editor->currentFile) free(editor->currentFile);
    editor->currentFile = safe_strdup(filename);

    printf("[OK] File '%s' opened successfully.\n", filename);
    printf("Loaded %d lines, %ld characters.\n", lineCount, charCount);
}

// Save current editor content to a file
static inline void saveFile(TextEditor *editor, const char *filename) {
    const char *target = (filename && strlen(filename) > 0) ? filename : editor->currentFile;
    if (!target || strlen(target) == 0) {
        printf("[Error] No filename specified. Usage: save <filename>\n");
        return;
    }

    FILE *fp = fopen(target, "w");
    if (!fp) {
        printf("[Error] Unable to write to file '%s'.\n", target);
        return;
    }

    int lineCount = 0;
    long charCount = 0;
    Node *curr = editor->head;
    while (curr) {
        size_t len = strlen(curr->line);
        fputs(curr->line, fp);
        charCount += (long)len;
        if (curr->next) {
            fputc('\n', fp);
            charCount++;
        }
        lineCount++;
        curr = curr->next;
    }

    fclose(fp);

    if (filename && strlen(filename) > 0) {
        if (editor->currentFile) free(editor->currentFile);
        editor->currentFile = safe_strdup(filename);
    }

    printf("[OK] File saved successfully as '%s'\n", target);
    printf("Saved %d lines, %ld characters.\n", lineCount, charCount);
}

// Close the current file and reset the editor buffer
static inline void closeFile(TextEditor *editor, DynamicArrayStack *undoStack,
                             DynamicArrayStack *redoStack) {
    freeEditorLines(editor);
    clearStack(undoStack);
    clearStack(redoStack);

    if (editor->currentFile) {
        free(editor->currentFile);
        editor->currentFile = NULL;
    }

    Node *initialLine = createNode("");
    editor->head = initialLine;
    editor->tail = initialLine;
    editor->cursor = initialLine;
    editor->cursorLine = 1;
    editor->cursorPos = 0;

    printf("[OK] File closed successfully.\n");
}

#endif // FILE_OPS_H
