# GCC 4.9+
CC = gcc
CFLAGS += -Wall -Wextra -Wformat -Werror=implicit-function-declaration -pedantic
INCLUDE += -Iinclude
SRCDIR = src
OBJDIR = obj
SRC = $(wildcard $(SRCDIR)/*.c)
PLSH_OBJ = $(OBJDIR)/plsh.o $(OBJDIR)/errors.o $(OBJDIR)/context.o

.PHONY: all clean

all: plsh

plsh: $(PLSH_OBJ)
	$(CC) -o $@ $(PLSH_OBJ)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJDIR)/*
