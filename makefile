CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -Werror
LFLAGS = 

INCLUDES = 
LIBS = 

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
EXE = compiler

all: $(EXE)
	@echo Compiler has been compiled! Executable is named compiler.

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(EXE) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
