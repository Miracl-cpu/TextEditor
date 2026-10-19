#ifndef TEXT_OPS_H
#define TEXT_OPS_H

#include "text_editor.h"
#include "redo_undo.h"
#include "curser_navigations.h"

// Low-level insert that supports single or multi-line strings (containing '\n')
static inline void rawInsertText(TextEditor *editor, const char *text) {
    if (!editor->cursor || !text) return;

    const char *p = text;
    while (*p != '\0') {
        const char *nl = strchr(p, '\n');
        if (!nl) {
            // Insert remaining segment into current line at cursorPos
            size_t segLen = strlen(p);
            size_t curLen = strlen(editor->cursor->line);
            char *newLine = (char *)malloc(curLen + segLen + 1);
            if (!newLine) {
                perror("Memory allocation failed in rawInsertText");
                exit(EXIT_FAILURE);
            }
            memcpy(newLine, editor->cursor->line, editor->cursorPos);
            memcpy(newLine + editor->cursorPos, p, segLen);
            memcpy(newLine + editor->cursorPos + segLen,
                   editor->cursor->line + editor->cursorPos,
                   curLen - editor->cursorPos + 1);

            free(editor->cursor->line);
            editor->cursor->line = newLine;
            editor->cursorPos += (int)segLen;
            break;
        } else {
            // Insert segment before '\n', then split line into a new Node
            size_t segLen = (size_t)(nl - p);

            char *leftPart = (char *)malloc(editor->cursorPos + segLen + 1);
            if (!leftPart) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            memcpy(leftPart, editor->cursor->line, editor->cursorPos);
            memcpy(leftPart + editor->cursorPos, p, segLen);
            leftPart[editor->cursorPos + segLen] = '\0';

            char *rightPart = safe_strdup(editor->cursor->line + editor->cursorPos);

            free(editor->cursor->line);
            editor->cursor->line = leftPart;

            Node *newNode = createNode(rightPart);
            free(rightPart);

            newNode->next = editor->cursor->next;
            newNode->prev = editor->cursor;
            if (editor->cursor->next) {
                editor->cursor->next->prev = newNode;
            } else {
                editor->tail = newNode;
            }
            editor->cursor->next = newNode;

            editor->cursor = newNode;
            editor->cursorLine++;
            editor->cursorPos = 0;

            p = nl + 1;
        }
    }
}

// Low-level delete backward from current cursor position (or delete between two positions)
// Deletes `numChars` characters backward from the cursor (like backspace/delete from cursor)
static inline char* rawDeleteBackward(TextEditor *editor, int numChars) {
    if (!editor->cursor || numChars <= 0) return NULL;

    // First verify if enough characters exist backward across lines, or on current line
    int availBefore = editor->cursorPos;
    Node *scan = editor->cursor->prev;
    while (scan && availBefore < numChars) {
        availBefore += (int)strlen(scan->line) + 1; // +1 for newline
        scan = scan->prev;
    }

    int availAfter = (int)strlen(editor->cursor->line) - editor->cursorPos;

    // Following the specification interactive session:
    // After inserting "Programming is fun!" (length 19, cursor at Col 19),
    // `delete 5` deletes the last 5 characters (" fun!") leaving "Programming is" and Cursor at Col 14.
    // If cursor is at 0 or doesn't have enough chars before it, we also check forward on the line.
    if (availBefore >= numChars) {
        char *deleted = (char *)malloc(numChars + 1);
        deleted[numChars] = '\0';
        int rem = numChars;
        int writeIdx = numChars;

        while (rem > 0) {
            if (editor->cursorPos >= rem) {
                int startPos = editor->cursorPos - rem;
                writeIdx -= rem;
                memcpy(deleted + writeIdx, editor->cursor->line + startPos, rem);
                size_t curLen = strlen(editor->cursor->line);
                memmove(editor->cursor->line + startPos,
                        editor->cursor->line + editor->cursorPos,
                        curLen - editor->cursorPos + 1);
                editor->cursorPos = startPos;
                rem = 0;
            } else {
                // Consume all characters before cursor on current line, then merge with prev line
                if (editor->cursorPos > 0) {
                    writeIdx -= editor->cursorPos;
                    memcpy(deleted + writeIdx, editor->cursor->line, editor->cursorPos);
                    rem -= editor->cursorPos;
                    size_t curLen = strlen(editor->cursor->line);
                    memmove(editor->cursor->line,
                            editor->cursor->line + editor->cursorPos,
                            curLen - editor->cursorPos + 1);
                    editor->cursorPos = 0;
                }
                if (rem > 0 && editor->cursor->prev) {
                    // Merge current line into previous line (deleting newline)
                    writeIdx -= 1;
                    deleted[writeIdx] = '\n';
                    rem--;

                    Node *prevNode = editor->cursor->prev;
                    Node *currNode = editor->cursor;
                    size_t prevLen = strlen(prevNode->line);
                    size_t currLen = strlen(currNode->line);

                    char *merged = (char *)malloc(prevLen + currLen + 1);
                    memcpy(merged, prevNode->line, prevLen);
                    memcpy(merged + prevLen, currNode->line, currLen + 1);
                    free(prevNode->line);
                    prevNode->line = merged;

                    prevNode->next = currNode->next;
                    if (currNode->next) {
                        currNode->next->prev = prevNode;
                    } else {
                        editor->tail = prevNode;
                    }
                    free(currNode->line);
                    free(currNode);

                    editor->cursor = prevNode;
                    editor->cursorLine--;
                    editor->cursorPos = (int)prevLen;
                }
            }
        }
        return deleted;
    } else if (availAfter >= numChars) {
        // Delete forward from cursor on current line
        char *deleted = (char *)malloc(numChars + 1);
        memcpy(deleted, editor->cursor->line + editor->cursorPos, numChars);
        deleted[numChars] = '\0';
        size_t curLen = strlen(editor->cursor->line);
        memmove(editor->cursor->line + editor->cursorPos,
                editor->cursor->line + editor->cursorPos + numChars,
                curLen - (editor->cursorPos + numChars) + 1);
        return deleted;
    }

    return NULL;
}

// Insert text at the current cursor position and record on undo stack
static inline void insertText(TextEditor *editor, DynamicArrayStack *undoStack,
                              DynamicArrayStack *redoStack, const char *text) {
    if (!text) return;

    // Check if current line is completely empty and not line 1, or if user inserted on a new line
    int startLine = editor->cursorLine;
    int startPos = editor->cursorPos;

    rawInsertText(editor, text);

    int endLine = editor->cursorLine;
    int endPos = editor->cursorPos;

    pushAction(undoStack, "insert", text, startLine, startPos, endLine, endPos);
    clearStack(redoStack);

    printf("[OK] Text inserted successfully.\n");
}

// Delete specified number of characters and record on undo stack
static inline void deleteCharacters(TextEditor *editor, DynamicArrayStack *undoStack,
                                    DynamicArrayStack *redoStack, int numChars) {
    if (numChars <= 0) {
        printf("[Error] Number of characters to delete must be positive.\n");
        return;
    }

    int lineLen = (int)strlen(editor->cursor->line);
    int availBefore = editor->cursorPos;
    Node *scan = editor->cursor->prev;
    while (scan) {
        availBefore += (int)strlen(scan->line) + 1;
        scan = scan->prev;
    }
    int availAfter = lineLen - editor->cursorPos;
    int maxAvailable = (availBefore > availAfter) ? availBefore : availAfter;
    if (lineLen > 0 && maxAvailable > lineLen && availBefore <= lineLen) {
        maxAvailable = lineLen;
    }

    if (numChars > availBefore && numChars > availAfter) {
        int reportAvail = (editor->cursorPos > 0) ? editor->cursorPos : lineLen;
        printf("[Error] Cannot delete %d characters. Only %d characters available.\n",
               numChars, reportAvail);
        return;
    }

    int beforeLine = editor->cursorLine;
    int beforePos = editor->cursorPos;

    char *deletedText = rawDeleteBackward(editor, numChars);
    if (!deletedText) {
        printf("[Error] Cannot delete %d characters. Only %d characters available.\n",
               numChars, lineLen);
        return;
    }

    int afterLine = editor->cursorLine;
    int afterPos = editor->cursorPos;

    // Record the deletion so undo can re-insert `deletedText` at (afterLine, afterPos)
    pushAction(undoStack, "delete", deletedText, afterLine, afterPos, beforeLine, beforePos);
    clearStack(redoStack);
    free(deletedText);

    printf("[OK] Deleted %d characters successfully.\n", numChars);
}

// Copy `numChars` from current cursor (or entire current line if numChars <= 0) to clipboard
static inline void copyText(TextEditor *editor, int numChars) {
    if (!editor->cursor) return;
    const char *line = editor->cursor->line;
    int len = (int)strlen(line);

    if (editor->clipboard) {
        free(editor->clipboard);
        editor->clipboard = NULL;
    }

    if (numChars <= 0) {
        // Copy entire current line
        editor->clipboard = safe_strdup(line);
        printf("[OK] Copied line (%d characters) to clipboard: \"%s\"\n", len, editor->clipboard);
    } else {
        int start = editor->cursorPos;
        if (start + numChars > len) {
            start = (len - numChars >= 0) ? (len - numChars) : 0;
        }
        int actual = (start + numChars <= len) ? numChars : (len - start);
        editor->clipboard = (char *)malloc(actual + 1);
        memcpy(editor->clipboard, line + start, actual);
        editor->clipboard[actual] = '\0';
        printf("[OK] Copied %d characters to clipboard: \"%s\"\n", actual, editor->clipboard);
    }
}

// Cut text into clipboard and record deletion in undo stack
static inline void cutText(TextEditor *editor, DynamicArrayStack *undoStack,
                           DynamicArrayStack *redoStack, int numChars) {
    if (!editor->cursor) return;
    int len = (int)strlen(editor->cursor->line);
    if (len == 0) {
        printf("[Error] Current line is empty, nothing to cut.\n");
        return;
    }
    if (numChars <= 0 || numChars > len) {
        numChars = len;
        editor->cursorPos = len;
    }
    copyText(editor, numChars);
    deleteCharacters(editor, undoStack, redoStack, numChars);
    printf("[OK] Cut operation completed.\n");
}

// Paste text from clipboard at cursor position
static inline void pasteText(TextEditor *editor, DynamicArrayStack *undoStack,
                             DynamicArrayStack *redoStack) {
    if (!editor->clipboard || strlen(editor->clipboard) == 0) {
        printf("[Error] Clipboard is empty.\n");
        return;
    }
    insertText(editor, undoStack, redoStack, editor->clipboard);
    printf("Pasted \"%s\" from clipboard.\n", editor->clipboard);
}

// Delete the entire current line
static inline void deleteLine(TextEditor *editor, DynamicArrayStack *undoStack,
                              DynamicArrayStack *redoStack) {
    if (!editor->cursor) return;

    int lineNum = editor->cursorLine;
    char *savedContent = safe_strdup(editor->cursor->line);

    if (editor->head == editor->tail) {
        // Only one line in editor: clear its text
        pushAction(undoStack, "delete", savedContent, 1, 0, 1, (int)strlen(savedContent));
        clearStack(redoStack);
        free(editor->cursor->line);
        editor->cursor->line = safe_strdup("");
        editor->cursorPos = 0;
    } else {
        Node *toDelete = editor->cursor;
        if (toDelete->prev) {
            toDelete->prev->next = toDelete->next;
        } else {
            editor->head = toDelete->next;
        }
        if (toDelete->next) {
            toDelete->next->prev = toDelete->prev;
        } else {
            editor->tail = toDelete->prev;
        }

        if (toDelete->prev) {
            editor->cursor = toDelete->prev;
            editor->cursorLine--;
            editor->cursorPos = (int)strlen(editor->cursor->line);
            char *withNewline = (char *)malloc(strlen(savedContent) + 2);
            withNewline[0] = '\n';
            strcpy(withNewline + 1, savedContent);
            pushAction(undoStack, "delete", withNewline,
                       editor->cursorLine, editor->cursorPos,
                       lineNum, (int)strlen(savedContent));
            free(withNewline);
        } else {
            editor->cursor = toDelete->next;
            editor->cursorLine = 1;
            editor->cursorPos = 0;
            char *withNewline = (char *)malloc(strlen(savedContent) + 2);
            strcpy(withNewline, savedContent);
            strcat(withNewline, "\n");
            pushAction(undoStack, "delete", withNewline, 1, 0, 2, 0);
            free(withNewline);
        }
        clearStack(redoStack);
        free(toDelete->line);
        free(toDelete);
    }

    free(savedContent);
    printf("[OK] Line %d deleted successfully.\n", lineNum);
}

// Undo the last operation
static inline void performUndo(TextEditor *editor, DynamicArrayStack *undoStack,
                               DynamicArrayStack *redoStack) {
    if (isStackEmpty(undoStack)) {
        printf("[Error] Nothing to undo.\n");
        return;
    }

    Action act = popAction(undoStack);

    if (strcmp(act.operation, "insert") == 0) {
        // Move cursor to end of inserted text and delete backward
        setCursorPosition(editor, act.endLine, act.endPos);
        int len = (int)strlen(act.text);
        char *del = rawDeleteBackward(editor, len);
        if (del) free(del);
        setCursorPosition(editor, act.cursorLine, act.cursorPos);
    } else if (strcmp(act.operation, "delete") == 0) {
        // Move cursor to deletion start point and re-insert the text
        setCursorPosition(editor, act.cursorLine, act.cursorPos);
        rawInsertText(editor, act.text);
        setCursorPosition(editor, act.endLine, act.endPos);
    }

    pushAction(redoStack, act.operation, act.text,
               act.cursorLine, act.cursorPos, act.endLine, act.endPos);
    freeAction(&act);

    printf("[OK] Undo operation successful.\n");
}

// Redo the last undone operation
static inline void performRedo(TextEditor *editor, DynamicArrayStack *undoStack,
                               DynamicArrayStack *redoStack) {
    if (isStackEmpty(redoStack)) {
        printf("[Error] Nothing to redo.\n");
        return;
    }

    Action act = popAction(redoStack);

    if (strcmp(act.operation, "insert") == 0) {
        setCursorPosition(editor, act.cursorLine, act.cursorPos);
        rawInsertText(editor, act.text);
        setCursorPosition(editor, act.endLine, act.endPos);
    } else if (strcmp(act.operation, "delete") == 0) {
        setCursorPosition(editor, act.endLine, act.endPos);
        int len = (int)strlen(act.text);
        char *del = rawDeleteBackward(editor, len);
        if (del) free(del);
        setCursorPosition(editor, act.cursorLine, act.cursorPos);
    }

    pushAction(undoStack, act.operation, act.text,
               act.cursorLine, act.cursorPos, act.endLine, act.endPos);
    freeAction(&act);

    printf("[OK] Redo operation successful.\n");
}

#endif // TEXT_OPS_H
