#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "hamt.h"
#include "./testing/print_bits.h"

void test_case_1() {
	struct hamt_node_t *hamt = create_hamt();

	hamt = hamt_set(hamt, "hello", "world");
	hamt = hamt_set(hamt, "hey", "over there");
	hamt = hamt_set(hamt, "hey2", "over there again");
	char *value1 = (char *)hamt_get(hamt, "hello");
	char *value2 = (char *)hamt_get(hamt, "hey");
	char *value3 = (char *)hamt_get(hamt, "hey2");
	printf("value1: %s\n", value1);
	printf("value2: %s\n", value2);
	printf("value3: %s\n", value3);

	hamt = hamt_delete(hamt, "hey");
	printf("Should be null: %s\n", (char *)hamt_get(hamt, "hey"));

	hamt = hamt_set(hamt, "Aa", "collision 1");
	hamt = hamt_set(hamt, "BB", "collision 2");

	char *collision_1 = (char *)hamt_get(hamt, "Aa");
	char *collision_2 = (char *)hamt_get(hamt, "BB");
	printf("collision value1: %s\n", collision_1);
	printf("collision value2: %s\n", collision_2);
}

void insert_dictionary(struct hamt_node_t **hamt, char *dictionary) {
	char *ptr = dictionary;

	while (*dictionary != '\0') {
		if (*dictionary == '\n') {
			*dictionary = '\0';
			char *str = strdup(ptr);
			*hamt = hamt_set(*hamt, str, str);
			ptr = dictionary + 1;
			*dictionary = '\n';
		}
		dictionary++;
	}
}

void dictionary_check(struct hamt_node_t *hamt, char *dictionary) {
	char *ptr = dictionary;
	char *value;
	int missing_count = 0;
	int accounted_for = 0;

	while (*dictionary != '\0') {
		if (*dictionary == '\n') {
			*dictionary = '\0';

			value = (char *)hamt_get(hamt, ptr);
			if (value == NULL) {
				missing_count++;
			} else if (strcmp(value, ptr) == 0) {
				accounted_for++;
			}
			ptr = dictionary + 1;
		}
		dictionary++;
	}

	printf("Missing: %d\n", missing_count);
	printf("Present: %d\n", accounted_for);
}

void test_case_2(char *contents) {
	struct hamt_node_t *hamt = create_hamt();

	insert_dictionary(&hamt, contents);
	printf("finished insert\n");
	dictionary_check(hamt, contents);
}

int main(void) {
	int fd;
	struct stat sb;
	
	if ((fd = open("./testing/dictionary.txt", O_RDONLY)) == -1) {
		fprintf(stderr, "Failed to load file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((fstat(fd, &sb) == -1)) {
		fprintf(stderr, "Failed to fstat file: %s\n", strerror(errno));
		goto failed;
	}

	char *contents = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (contents == MAP_FAILED) {
		fprintf(stderr, "Failed to mmap file: %s\n", strerror(errno));
		goto failed;
	}

	test_case_1();
	test_case_2(contents);


	munmap(contents, sb.st_size);
	close(fd);

failed:
	(void)close(fd);
	exit(EXIT_FAILURE);
}
