# Makefile

CC = gcc
CFLAGS = -Wall -I../include `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`
OBJDIR = ../obj
BINDIR = ../build
SRCDIR = ../src

OBJECTS = $(OBJDIR)/ssh_monitor.o
TARGET = $(BINDIR)/AccessSuiteMonitor

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
