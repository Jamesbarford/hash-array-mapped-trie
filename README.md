# `hamt-poly` - polymorphic hash array mapped trie

`hamt-poly` implements polymorphic and immutable Hash Array Mapped Tries in C.
The use case it was built for was to handle storing api routes as the key with a function pointer/struct as the value, but it is extremely versatile.

`hamt-poly` is a fork of [`hamt`](https://github.com/Jamesbarford/hash-array-mapped-trie).

Most code is moved into a preprocessor macro, in order to achieve polymorphism, i.e. to be able to have keys of arbitrary types, rather than a hard-coded type such as `char*`. Thank you James for publishing your code, it was a real delight to find!

## Testing

Tests are defined in `hamt-testing.c`. To build and run the tests, run:

```sh
$ mkdir build
$ make
$ ./hamt-testing.out
```

## Usage

### Initialisation
In this key-value data store, keys can be of arbitrary types. Thus we must also provide a way to create a hash of the key type (the H in HAMT), as well as how to check if two keys are equal. From `hamt-testing.c` comes this example:

```c
/* For the next test we create a new key data structure.  This will
	 demonstrate that we can use multiple different types of HAMTs */
typedef struct My_KeyType {
	char str[20];
} My_KeyType;
/* It must be able to check equality, for example when inserting with
	 a hash collission */
bool My_KeyType_equals(My_KeyType* s0, My_KeyType* s1) {
	return strcmp(s0->str,
								s1->str) == 0;
}
unsigned int get_hash_of_My_KeyType(My_KeyType* s) {return get_hash(s->str);}
/* This will actually define a bunch of function prefixed with My_KeyType */
HAMT_DEFINE(My_KeyType, get_hash_of_My_KeyType, My_KeyType_equals)
```

### Insert, Retrieve, and Remove


Any arbitrary data `void *` can be stored against a `My_KeyType *` key.
```c
#include "hamt.h"

int main(void) {
	My_KeyType* keyptr = malloc(sizeof(My_KeyType));
	strcpy(keyptr->str, "the key");

	My_KeyType_hamt_t* hamt = My_KeyType_create_hamt();
	/* Insert */
	hamt = My_KeyType_hamt_set(hamt, keyptr, "polymorphic...");
	/* Retrieve */
	char* value = (char*)My_KeyType_hamt_get(hamt, keyptr);
	printf("My_KeyType value = %s\n", value);

	/* Remove */
	hamt = My_KeyType_hamt_remove(hamt, keyptr);
	value = My_KeyType_hamt_get(hamt, keyptr);
	assert(value == NULL);
}
```
