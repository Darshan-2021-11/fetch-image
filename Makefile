FLAGS =-Wall -Wextra -Wshadow -Wpedantic -I./curl-8.7.1_7-win64-mingw/include
# -Werror
LIBS =-L./curl-8.7.1_7-win64-mingw/lib -lcurl

fetch: main.c
	gcc -o $@ $< $(FLAGS) $(LIBS)
