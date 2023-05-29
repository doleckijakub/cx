default: cx

cx: cx.c
	gcc -o cx cx.c -Wall -Wextra -Werror -pedantic -ggdb