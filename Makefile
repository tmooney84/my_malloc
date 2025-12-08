# Compiler
CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -g -D_POSIX_C_SOURCE=200809L

# Target executable name
TARGET  := my_malloc

# Source and object files
SRC     := my_malloc.c bitwise_helpers.c  
OBJ     := $(SRC:.c=.o)

# Headers
HEADERS := arena.h

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o

fclean: clean
	rm -f $(TARGET)
	rm -rf tests/actual tests/expected runner backup.txt

re: fclean all