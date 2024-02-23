#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t i32;

u32 run_length(char* code, u32 code_size, u32 *ptr, char consuming) {
	u32 i = 0;
	while (*ptr < code_size) {
		if (!strchr("+-><.,[]", code[*ptr]) != !(code[(*ptr)] == consuming)) break;
		if (code[*ptr] == consuming) i++;
		(*ptr)++;
	} (*ptr)--;

	return i;
}

int main(int argc, char *argv[]) {
	if (argc < 2) return 1;

	u8 *arena = malloc(0x4000);
	u32 arena_ptr = 0;

	char *code = (char *)(arena + arena_ptr);
	u32 code_size = 1;
	code[0] = 0;
	{
		i32 in = open(argv[1], O_RDONLY);
		i32 n = 0;
		while ( (n = read(in, code + code_size, 256), n > 0) ) {
			code_size += n;
		}

		arena_ptr += code_size;
		close(in);
	}

	puts("format ELF64 executable 3");
	puts("entry main");

	puts("segment readable writable");
	puts("	tape db 256 dup 0");

	puts("segment readable executable");

	puts("	exit:");
 	puts("		mov rdi, 0"); // return code
	puts(" 		mov rax, 60"); // write syscall
	puts(" 		syscall");
	puts(" 		ret");

	puts("	write:");
	puts(" 		mov rax, 1"); // write syscall
 	puts("		mov rdi, 1"); // stdout filedes
	puts(" 		syscall");
	puts(" 		ret");

	puts("	read:");
 	puts("		mov rdi, 0"); // stdin filedes
	puts(" 		mov rax, 0"); // read syscall
	puts(" 		syscall");
	puts(" 		ret");

	puts("	main:");
 	puts("		mov rdx, 1"); // 1 bytes
 	puts("		lea rsi, [tape]");
	putchar('\n');

	{
		u32 n = 0;
		u8 *stack = (u8 *)(arena + arena_ptr);
		u32 stack_ptr = 0;
		u32 loop = 0;

		char cur;
		while (n < code_size) {
			cur = code[n];

			switch (cur) {
				case '+': {
					u32 i = run_length(code, code_size, &n, '+');

					printf("\t\tadd byte [rsi], %u ; +\n", i);
					break;
				}

				case '-': {
					u32 i = run_length(code, code_size, &n, '-');

					printf("\t\tsub byte [rsi], %u ; -\n", i);
					break;
				}

				case '>': {
					u32 i = run_length(code, code_size, &n, '>');

					printf("\t\tadd rsi, %u ; >\n", i);
					break;
				}

				case '<': {
					u32 i = run_length(code, code_size, &n, '<');

					printf("\t\tsub rsi, %u ; <\n", i);
					break;
				}

				case '.': {
					puts("\t\tcall write ; .");
					break;
				}

				case ',': {
					puts("\t\tcall read ; ,");
					break;
				}

				case '[': {
					putchar('\n');
					puts("\t\tcmp byte [rsi], 0");
					printf("\t\tjz LP_END%u\n", loop);
					printf("\t\tLP_START%u: ; [\n", loop);
					putchar('\n');

					stack[stack_ptr++] = loop++;

					break;
				}

				case ']': {
					putchar('\n');
					puts("\t\tcmp byte [rsi], 0");

					u32 val = stack[--stack_ptr];

					printf("\t\tjnz LP_START%u\n", val);
					printf("\t\tLP_END%u: ; ]\n", val);
					putchar('\n');

					break;
				}

				default: {
					break;
				}

			} // switch

			n++;
		} // while

		putchar('\n');
		puts("		call exit");

	}

	free(arena);
	return 0;
}
