CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
TARGET = text_editor
SRCS = text_editor.c
HEADERS = text_editor.h text_ops.h file_ops.h curser_navigations.h cursor_nav.h display_search.h redo_undo.h

all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

debug: $(SRCS) $(HEADERS)
	$(CC) -std=c99 -Wall -Wextra -g -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: all debug clean
