#ifndef DISPLAY_SEARCH_H
#define DISPLAY_SEARCH_H

#include "text_editor.h"
#include "redo_undo.h"

// Display the current state of the text editor inside a formatted box
static inline void displayText(const TextEditor *editor) {
    printf("+---------------------------------------------------------+\n");
    Node *curr = editor->head;
    int lineNum = 1;
    while (curr) {
        char formatted[256];
        snprintf(formatted, sizeof(formatted), "Line %d: %s", lineNum, curr->line);
        printf("| %-55.55s |\n", formatted);
        curr = curr->next;
        lineNum++;
    }
    char cursorInfo[128];
    snprintf(cursorInfo, sizeof(cursorInfo), "Cursor position: Line %d, Column %d",
             editor->cursorLine, editor->cursorPos);
    printf("| %-55.55s |\n", cursorInfo);
    printf("+---------------------------------------------------------+\n");
}

// Display the current cursor coordinates
static inline void showCursorPosition(const TextEditor *editor) {
    printf("Cursor position: Line %d, Column %d\n", editor->cursorLine, editor->cursorPos);
}

// Search for all occurrences of a substring across all lines
static inline void findText(const TextEditor *editor, const char *pattern) {
    if (!pattern || strlen(pattern) == 0) {
        printf("[Error] Search pattern cannot be empty.\n");
        return;
    }

    Node *curr = editor->head;
    int lineNum = 1;
    int matches = 0;
    size_t patLen = strlen(pattern);

    while (curr) {
        const char *p = curr->line;
        while ((p = strstr(p, pattern)) != NULL) {
            int col = (int)(p - curr->line);
            printf("Found \"%s\" at Line %d, Column %d\n", pattern, lineNum, col);
            matches++;
            p += patLen;
        }
        curr = curr->next;
        lineNum++;
    }

    if (matches > 0) {
        printf("[OK] Search completed. %d %s found.\n", matches, (matches == 1) ? "match" : "matches");
    } else {
        printf("No matches found for \"%s\".\n", pattern);
    }
}

// Replace all occurrences of `oldText` with `newText` across the editor
static inline void replaceText(TextEditor *editor, const char *oldText, const char *newText) {
    if (!oldText || strlen(oldText) == 0 || !newText) {
        printf("[Error] Invalid arguments for replace.\n");
        return;
    }

    printf("Replacing \"%s\" with \"%s\"...\n", oldText, newText);

    size_t oldLen = strlen(oldText);
    size_t newLen = strlen(newText);
    int totalReplaced = 0;

    Node *curr = editor->head;
    while (curr) {
        int countOnLine = 0;
        const char *tmp = curr->line;
        while ((tmp = strstr(tmp, oldText)) != NULL) {
            countOnLine++;
            tmp += oldLen;
        }

        if (countOnLine > 0) {
            size_t newLineLen = strlen(curr->line) + countOnLine * (newLen - oldLen);
            char *updatedLine = (char *)malloc(newLineLen + 1);
            if (!updatedLine) {
                perror("Memory allocation failed in replaceText");
                exit(EXIT_FAILURE);
            }

            char *dst = updatedLine;
            const char *src = curr->line;
            const char *match;
            while ((match = strstr(src, oldText)) != NULL) {
                size_t prefixLen = (size_t)(match - src);
                memcpy(dst, src, prefixLen);
                dst += prefixLen;
                memcpy(dst, newText, newLen);
                dst += newLen;
                src = match + oldLen;
                totalReplaced++;
            }
            strcpy(dst, src);

            free(curr->line);
            curr->line = updatedLine;

            if (curr == editor->cursor) {
                int maxCol = (int)strlen(curr->line);
                if (editor->cursorPos > maxCol) {
                    editor->cursorPos = maxCol;
                }
            }
        }
        curr = curr->next;
    }

    printf("[OK] Replaced %d %s.\n", totalReplaced, (totalReplaced == 1) ? "occurrence" : "occurrences");
}

#endif // DISPLAY_SEARCH_H
