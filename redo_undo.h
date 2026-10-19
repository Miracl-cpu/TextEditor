#ifndef REDO_UNDO_H
#define REDO_UNDO_H

#include "text_editor.h"

// Action structure for storing undo/redo operation metadata
typedef struct {
    char *operation;  // Operation type ("insert", "delete", "newline", "del_line")
    char *text;       // Text involved in the operation
    int cursorLine;   // Line number where operation occurred
    int cursorPos;    // Column position where operation occurred
    int endLine;      // Line number after operation (for multi-line/cursor tracking)
    int endPos;       // Column position after operation
} Action;

// Dynamic Array Stack for Undo/Redo history
typedef struct {
    Action *actions;  // Dynamic array of actions
    int size;         // Current size of the stack
    int capacity;     // Current capacity of the stack
} DynamicArrayStack;

// Initialize a dynamic array stack
static inline void initStack(DynamicArrayStack *stack, int initialCapacity) {
    if (initialCapacity <= 0) initialCapacity = 10;
    stack->actions = (Action *)malloc(sizeof(Action) * initialCapacity);
    if (!stack->actions) {
        perror("Failed to allocate stack memory");
        exit(EXIT_FAILURE);
    }
    stack->size = 0;
    stack->capacity = initialCapacity;
}

// Check if stack is empty
static inline bool isStackEmpty(const DynamicArrayStack *stack) {
    return stack->size == 0;
}

// Free internals of a single Action
static inline void freeAction(Action *action) {
    if (action->operation) free(action->operation);
    if (action->text) free(action->text);
    action->operation = NULL;
    action->text = NULL;
}

// Clear all elements in a stack without freeing the stack array itself
static inline void clearStack(DynamicArrayStack *stack) {
    for (int i = 0; i < stack->size; i++) {
        freeAction(&stack->actions[i]);
    }
    stack->size = 0;
}

// Destroy stack and free its dynamic array
static inline void destroyStack(DynamicArrayStack *stack) {
    clearStack(stack);
    free(stack->actions);
    stack->actions = NULL;
    stack->capacity = 0;
}

// Push a new action onto the stack with automatic resizing
static inline void pushAction(DynamicArrayStack *stack, const char *operation, const char *text,
                              int cursorLine, int cursorPos, int endLine, int endPos) {
    if (stack->size >= stack->capacity) {
        stack->capacity *= 2;
        Action *newActions = (Action *)realloc(stack->actions, sizeof(Action) * stack->capacity);
        if (!newActions) {
            perror("Failed to resize stack");
            exit(EXIT_FAILURE);
        }
        stack->actions = newActions;
    }
    Action *act = &stack->actions[stack->size++];
    act->operation = safe_strdup(operation);
    act->text = safe_strdup(text ? text : "");
    act->cursorLine = cursorLine;
    act->cursorPos = cursorPos;
    act->endLine = endLine;
    act->endPos = endPos;
}

// Pop the top action from the stack (caller must free operation and text of returned Action)
static inline Action popAction(DynamicArrayStack *stack) {
    Action empty = {NULL, NULL, 0, 0, 0, 0};
    if (isStackEmpty(stack)) return empty;
    return stack->actions[--stack->size];
}

#endif // REDO_UNDO_H
