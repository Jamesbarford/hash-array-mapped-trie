#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#include "hamt.h"

/* I want to add the functionality of having HAMT keys of any value,
	 not only char* */
Value* mkkey_string(char* cool_string) {
  Value* v;
  v = malloc(sizeof(Value));
	v->type = STRING;
	v->actual_value.string = cool_string;
  return v;
}
Value* mkkey_u8(int cool_int) {
   Value* v;
   v = malloc(sizeof(Value));
	 v->type = U8;
	 v->actual_value.u8 = cool_int;
   return v;
}
void* put_value_heap(Value v) {
  void* ptr = malloc(sizeof(Value));
  memcpy(ptr, (void*)&v, sizeof(Value));
  return ptr;
}
void value_test() {
  struct Value_hamt_t* hamt = Value_create_hamt();
  Value value;
  value.actual_value.u8 = 13;
  value.actual_value.string = "HEllo";

	hamt = Value_hamt_set(hamt,
									mkkey_u8(1337),
									put_value_heap(value));

  Value* gotten_value = Value_hamt_get(hamt, mkkey_u8(1337));
  printf("%s\n\n", gotten_value->actual_value.string);
}

void martins_test() {
  struct Value_hamt_t* hamt1 = Value_create_hamt();
  struct Value_hamt_t* hamt2 = Value_create_hamt();
  hamt1 = Value_hamt_set(hamt1,
									 mkkey_string("hello"),
									 "world");
  char *value11 = (char *)Value_hamt_get(hamt1,
																	 mkkey_string("hello"));
  printf("value11: %s\n", value11);
	assert(strcmp(value11, "world") == 0);
	char *value12 = (char *)Value_hamt_get(hamt1,
																	 mkkey_string("good night"));

	printf("value12: %s\n", value12);
	assert(value12 == NULL);
	hamt2 = Value_hamt_set(hamt1,
									 mkkey_string("good night"),
									 "friend");
	char *value21 = (char *)Value_hamt_get(hamt2,
																	 mkkey_string("hello"));
	printf("value21: %s\n", value21);
	assert(strcmp(value21, "world") == 0);
	char *value22 =  (char *)Value_hamt_get(hamt2,
																		mkkey_string("good night"));
	printf("value22: %s\n", value22);
	assert(strcmp(value22, "friend") == 0);

	printf("value11: %s,\n value12: %s,\n value21: %s,\n value 22: %s\n",
				 value11,
				 value12,
				 value21,
				 value22);
}

void martins_test_int() {
	struct Value_hamt_t* hamt1 = Value_create_hamt();
  struct Value_hamt_t* hamt2 = Value_create_hamt();
  hamt1 = Value_hamt_set(hamt1,
									 mkkey_u8(1337),
									 "3l337");
  char *value11 = (char *)Value_hamt_get(hamt1,
																	 mkkey_u8(1337));
	printf("value11: %s\n", value11);
	assert(strcmp(value11, "3l337") == 0);
  char *value12 = (char *)Value_hamt_get(hamt2,
																	 mkkey_u8(6969));
	printf("value12: %s\n", value12);
	assert(value12 == NULL);
  hamt2 = Value_hamt_set(hamt1,
									 mkkey_u8(6969),
									 "buddy");
  char *value21 = (char *)Value_hamt_get(hamt2,
																	 mkkey_u8(1337));
	printf("value21: %s\n", value21);
	assert(strcmp(value21, "buddy") == 0);
  char *value22 = (char *)Value_hamt_get(Value_hamt_set(hamt2,
																						mkkey_string("wooo dude"),
																						"let's mix it up"),
																	 mkkey_string("wooo dude"));
	printf("value22: %s\n", value22);
	assert(strcmp(value22, "let's mix it up") == 0);
}

void test_case_1() {
	struct Value_hamt_t *hamt = Value_create_hamt();

  hamt = Value_hamt_set(hamt, mkkey_string("hello"), "world");
  hamt = Value_hamt_set(hamt, mkkey_string("hey"), "over there");
  hamt = Value_hamt_set(hamt, mkkey_string("hey2"), "over there again");
  char *value1 = (char *)Value_hamt_get(hamt, mkkey_string("hello"));
  char *value2 = (char *)Value_hamt_get(hamt, mkkey_string("hey"));
  char *value3 = (char *)Value_hamt_get(hamt, mkkey_string("hey2"));
  printf("value1: %s\n", value1);
  printf("value2: %s\n", value2);
  printf("value3: %s\n", value3);

  hamt = Value_hamt_set(hamt, mkkey_string("Aa"), "collision 1");
  hamt = Value_hamt_set(hamt, mkkey_string("BB"), "collision 2");

  char *collision_1 = (char *)Value_hamt_get(hamt, mkkey_string("Aa"));
  char *collision_2 = (char *)Value_hamt_get(hamt, mkkey_string("BB"));
  printf("collision value1: %s\n", collision_1);
  printf("collision value2: %s\n", collision_2);
}

void insert_dictionary(struct Value_hamt_t **hamt, char *dictionary) {
	char *ptr = dictionary;

	while (*dictionary != '\0') {
		if (*dictionary == '\n') {
			*dictionary = '\0';
			char *str = strdup(ptr);
			*hamt = Value_hamt_set(*hamt, mkkey_string(str), str);
			ptr = dictionary + 1;
			*dictionary = '\n';
		}
		dictionary++;
	}
}

void dictionary_check(struct Value_hamt_t *hamt, char *dictionary) {
	char *ptr = dictionary;
	char *value;
	int missing_count = 0;
	int accounted_for = 0;

	printf("Checking HAMT entries..\n");
	while (*dictionary != '\0') {
		if (*dictionary == '\n') {
			*dictionary = '\0';

			value = (char *)Value_hamt_get(hamt, mkkey_string(ptr));
			if (value == NULL) {
				missing_count++;
			} else if(strcmp(value, ptr) != 0) {
				printf("Mismatch\n");
			} else {
				accounted_for++;
			}
			ptr = dictionary + 1;
		}
		dictionary++;
	}

	printf("Missing: %d\n", missing_count);
	printf("Present: %d\n", accounted_for);
}

void remove_all(struct Value_hamt_t *hamt, char *dictionary) {
	char *ptr = dictionary;
	int missing_count = 0;
	int removal_count = 0;

	while (*dictionary != '\0') {
		if (*dictionary == '\n') {
			*dictionary = '\0';

			hamt = Value_hamt_remove(hamt, mkkey_string(ptr));
			if (hamt == NULL) {
				missing_count++;
				goto failed;
			} else {
				removal_count++;
			}
			ptr = dictionary + 1;
		}
		dictionary++;
	}

	printf("Missing: %d\n", missing_count);
	printf("Removed: %d\n", removal_count);
	return;
failed:
	printf("Failed Missing: %d\n", missing_count);
	printf("Failed Removed: %d\n", removal_count);
}


void test_case_2(char *contents) {
	struct Value_hamt_t *hamt = Value_create_hamt();

	insert_dictionary(&hamt, strdup(contents));
	dictionary_check(hamt, strdup(contents));
	printf("finished insert\n");
	remove_all(hamt, strdup(contents));
	printf("Finished removing\n");
	dictionary_check(hamt, strdup(contents));
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

	value_test();
	martins_test();
	martins_test_int();

	test_case_1();
	test_case_2(contents);


	munmap(contents, sb.st_size);
	close(fd);
	exit(0);

failed:
	(void)close(fd);
	exit(EXIT_FAILURE);
}
