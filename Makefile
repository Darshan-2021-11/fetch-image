FLAGS =-Wall -Werror -Wextra -Wshadow -Wpedantic -I./include/
LIBS =-L./lib/ -lcurl

fetchImage: main.c
	CC -o $@ $< $(FLAGS) $(LIBS)
