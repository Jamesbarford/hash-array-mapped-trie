#include <stdio.h>

void print_binary(unsigned long num) {
	for (unsigned long i = 0; i < 32; i++) {
		if ((num >> (unsigned long)(31ul - i)) & 1ul) {
			printf("1");
		} else {
			printf("0");
		}
	}

	printf("\n");
}

void print_hex(unsigned long num) {
	printf("0x%08lX\n", num);	
}
