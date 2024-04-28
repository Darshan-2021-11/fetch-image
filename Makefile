FLAGS =-Wall -Wextra -Wshadow -Wpedantic
# -Werror
LIBS =-lcurl

fetch: main.c
	gcc -o $@ $< $(FLAGS) $(LIBS)
