CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -pedantic -Iinclude
LDFLAGS = -lm
TARGET  = sim
SRCDIR  = src
OBJDIR  = build

SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/memory.c \
       $(SRCDIR)/loader.c \
       $(SRCDIR)/queues.c \
       $(SRCDIR)/process.c \
       $(SRCDIR)/scheduler.c \
       $(SRCDIR)/manager.c \
       $(SRCDIR)/report.c

OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

test: $(TARGET)
	@echo "=== Teste FCFS basico ==="
	./$(TARGET) --scheduler fcfs --quantum 4 \
	            --plan data/plan.txt \
	            --control data/control.txt
	@echo ""
	@echo "=== Teste Priority ==="
	./$(TARGET) --scheduler priority --quantum 3 \
	            --plan data/plan.txt \
	            --control data/control.txt
