ysh: ysh.c ysh_utils.h ysh_utils.c utils.h utils.c
	gcc -g -lc ysh_utils.c ysh.c utils.c -o ysh
