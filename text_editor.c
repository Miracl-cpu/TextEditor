#include "text_editor.h"
#include "redo_undo.h"
#include "curser_navigations.h"
#include "text_ops.h"
#include "file_ops.h"
#include "display_search.h"

// Print the main banner and command menu
void printMenu(void) {
    printf("===========================================================\n");
    printf("                    TEXT EDITOR v1.0\n");
    printf("===========================================================\n");
    printf("Text Editor Commands:\n");
    printf("1. insert <text>          - Insert text at the current cursor position\n");
    printf("2. newline [text]         - Insert a new line (optionally with text)\n");
    printf("3. delete <num_chars>     - Delete a number of characters from cursor\n");
    printf("4. deleteline             - Delete the entire current line\n");
    printf("5. undo                   - Undo the last operation\n");
    printf("6. redo                   - Redo the last undone operation\n");
    printf("7. up / down / left / right - Move cursor in specified direction\n");
    printf("8. home / end             - Jump to start / end of current line\n");
    printf("9. top / bottom           - Jump to start / end of file\n");
    printf("10. cursor                - Show current cursor coordinates\n");
    printf("11. copy [n] / cut [n] / paste - Clipboard operations\n");
    printf("12. open <file> / save [file] / close - File operations\n");
    printf("13. find \"<pattern>\"      - Search for text pattern\n");
    printf("14. replace \"<old>\" \"<new>\" - Replace text pattern\n");
    printf("15. print                 - Print the current state of the text editor\n");
    printf("16. help                  - Show this command menu\n");
    printf("17. exit                  - Exit the text editor\n");
    printf("You can enter a menu number or a command. For grouped options, enter the command.\n");
}

// Helper to parse quoted or unquoted arguments (used for find & replace)
static const char* extractToken(const char *input, char *outBuf, size_t bufSize) {
    while (*input && isspace((unsigned char)*input)) input++;
    if (*input == '\0') {
        outBuf[0] = '\0';
        return input;
    }

    size_t idx = 0;
    if (*input == '"') {
        input++; // skip opening quote
        while (*input && *input != '"' && idx + 1 < bufSize) {
            outBuf[idx++] = *input++;
        }
        if (*input == '"') input++; // skip closing quote
    } else {
        while (*input && !isspace((unsigned char)*input) && idx + 1 < bufSize) {
            outBuf[idx++] = *input++;
        }
    }
    outBuf[idx] = '\0';
    return input;
}

// Handle a single command line entered by the user
bool handleCommand(TextEditor *editor, DynamicArrayStack *undoStack,
                   DynamicArrayStack *redoStack, char *inputLine) {
    // Strip trailing newline/carriage return
    size_t len = strlen(inputLine);
    while (len > 0 && (inputLine[len - 1] == '\n' || inputLine[len - 1] == '\r')) {
        inputLine[--len] = '\0';
    }

    // Skip leading whitespace
    char *p = inputLine;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0') return true;

    // Extract command word
    char cmd[64];
    size_t i = 0;
    while (*p && !isspace((unsigned char)*p) && i + 1 < sizeof(cmd)) {
        cmd[i++] = *p++;
    }
    cmd[i] = '\0';

    // Advance `p` past spaces after command word (preserving trailing spaces for insert if needed)
    if (*p == ' ') p++;

    const char *menuCommand = NULL;
    if (strcmp(cmd, "1") == 0) menuCommand = "insert";
    else if (strcmp(cmd, "2") == 0) menuCommand = "newline";
    else if (strcmp(cmd, "3") == 0) menuCommand = "delete";
    else if (strcmp(cmd, "4") == 0) menuCommand = "deleteline";
    else if (strcmp(cmd, "5") == 0) menuCommand = "undo";
    else if (strcmp(cmd, "6") == 0) menuCommand = "redo";
    else if (strcmp(cmd, "10") == 0) menuCommand = "cursor";
    else if (strcmp(cmd, "13") == 0) menuCommand = "find";
    else if (strcmp(cmd, "14") == 0) menuCommand = "replace";
    else if (strcmp(cmd, "15") == 0) menuCommand = "print";
    else if (strcmp(cmd, "16") == 0) menuCommand = "help";
    else if (strcmp(cmd, "17") == 0) menuCommand = "exit";

    if (menuCommand) {
        strcpy(cmd, menuCommand);
    } else if (strcmp(cmd, "7") == 0 || strcmp(cmd, "8") == 0 ||
               strcmp(cmd, "9") == 0 || strcmp(cmd, "11") == 0 ||
               strcmp(cmd, "12") == 0) {
        char selectionLine[2048];
        if (*p != '\0') {
            snprintf(selectionLine, sizeof(selectionLine), "%s", p);
        } else {
            const char *choices = strcmp(cmd, "7") == 0 ? "up/down/left/right" :
                                  strcmp(cmd, "8") == 0 ? "home/end" :
                                  strcmp(cmd, "9") == 0 ? "top/bottom" :
                                  strcmp(cmd, "11") == 0 ? "copy/cut/paste" :
                                  "open/save/close";
            printf("Choose command (%s): ", choices);
            fflush(stdout);
            if (!fgets(selectionLine, sizeof(selectionLine), stdin)) return false;
        }
        return handleCommand(editor, undoStack, redoStack, selectionLine);
    }

    if (strcmp(cmd, "insert") == 0) {
        if (*p == '\0') {
            char nextLine[2048];
            printf("Text to insert: ");
            fflush(stdout);
            if (fgets(nextLine, sizeof(nextLine), stdin)) {
                size_t nlen = strlen(nextLine);
                while (nlen > 0 && (nextLine[nlen - 1] == '\n' || nextLine[nlen - 1] == '\r')) {
                    nextLine[--nlen] = '\0';
                }
                if (strlen(editor->cursor->line) > 0 && editor->cursorPos == (int)strlen(editor->cursor->line)) {
                    char *combined = (char *)malloc(nlen + 2);
                    combined[0] = '\n';
                    strcpy(combined + 1, nextLine);
                    insertText(editor, undoStack, redoStack, combined);
                    free(combined);
                } else {
                    insertText(editor, undoStack, redoStack, nextLine);
                }
            }
        } else {
            insertText(editor, undoStack, redoStack, p);
        }
    } else if (strcmp(cmd, "newline") == 0) {
        size_t plen = strlen(p);
        char *combined = (char *)malloc(plen + 2);
        combined[0] = '\n';
        strcpy(combined + 1, p);
        insertText(editor, undoStack, redoStack, combined);
        free(combined);
    } else if (strcmp(cmd, "delete") == 0) {
        if (*p == '\0') {
            printf("[Error] Please specify number of characters to delete. Usage: delete <num_chars>\n");
        } else {
            int numChars = atoi(p);
            deleteCharacters(editor, undoStack, redoStack, numChars);
        }
    } else if (strcmp(cmd, "deleteline") == 0) {
        deleteLine(editor, undoStack, redoStack);
    } else if (strcmp(cmd, "undo") == 0) {
        performUndo(editor, undoStack, redoStack);
    } else if (strcmp(cmd, "redo") == 0) {
        performRedo(editor, undoStack, redoStack);
    } else if (strcmp(cmd, "print") == 0) {
        displayText(editor);
    } else if (strcmp(cmd, "cursor") == 0) {
        showCursorPosition(editor);
    } else if (strcmp(cmd, "up") == 0) {
        moveCursorUp(editor);
    } else if (strcmp(cmd, "down") == 0) {
        moveCursorDown(editor);
    } else if (strcmp(cmd, "left") == 0) {
        moveCursorLeft(editor);
    } else if (strcmp(cmd, "right") == 0) {
        moveCursorRight(editor);
    } else if (strcmp(cmd, "home") == 0) {
        jumpToStartOfLine(editor);
    } else if (strcmp(cmd, "end") == 0) {
        jumpToEndOfLine(editor);
    } else if (strcmp(cmd, "top") == 0) {
        jumpToStartOfFile(editor);
    } else if (strcmp(cmd, "bottom") == 0) {
        jumpToEndOfFile(editor);
    } else if (strcmp(cmd, "copy") == 0) {
        int n = (*p != '\0') ? atoi(p) : 0;
        copyText(editor, n);
    } else if (strcmp(cmd, "cut") == 0) {
        int n = (*p != '\0') ? atoi(p) : 0;
        cutText(editor, undoStack, redoStack, n);
    } else if (strcmp(cmd, "paste") == 0) {
        pasteText(editor, undoStack, redoStack);
    } else if (strcmp(cmd, "open") == 0) {
        char filename[512];
        extractToken(p, filename, sizeof(filename));
        openFile(editor, undoStack, redoStack, filename);
    } else if (strcmp(cmd, "save") == 0) {
        char filename[512];
        extractToken(p, filename, sizeof(filename));
        saveFile(editor, filename);
    } else if (strcmp(cmd, "close") == 0) {
        closeFile(editor, undoStack, redoStack);
    } else if (strcmp(cmd, "find") == 0) {
        char pattern[512];
        extractToken(p, pattern, sizeof(pattern));
        findText(editor, pattern);
    } else if (strcmp(cmd, "replace") == 0) {
        char oldPat[512], newPat[512];
        const char *rest = extractToken(p, oldPat, sizeof(oldPat));
        extractToken(rest, newPat, sizeof(newPat));
        replaceText(editor, oldPat, newPat);
    } else if (strcmp(cmd, "help") == 0) {
        printMenu();
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        printf("Goodbye! Thank you for using Text Editor v1.0.\n");
        return false;
    } else {
        printf("[Error] Unknown command '%s'.\n", cmd);
        printf("Type 'help' for available commands.\n");
    }

    return true;
}

int main(void) {
    TextEditor editor;
    DynamicArrayStack undoStack;
    DynamicArrayStack redoStack;

    initEditor(&editor);
    initStack(&undoStack, 16);
    initStack(&redoStack, 16);

    printMenu();

    char inputBuffer[2048];
    bool running = true;

    while (running) {
        printf("Enter command or menu number: ");
        fflush(stdout);
        if (!fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
            printf("\nGoodbye! Thank you for using Text Editor v1.0.\n");
            break;
        }
        running = handleCommand(&editor, &undoStack, &redoStack, inputBuffer);
    }

    destroyStack(&undoStack);
    destroyStack(&redoStack);
    cleanupEditor(&editor);

    return 0;
}
