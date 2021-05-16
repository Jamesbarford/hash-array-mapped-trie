# `hamt` - hash array mapped trie

`hamt` is a simple implementation of a Hash Array Mapped Trie. The use case it was built for was to handle storing api routes as the key with a function pointer/struct as the value. 

Some examples have been provided in `main.c`, along with a dictionary of `466550` words. To build them:

```sh
$ mkdir build
$ make
$ ./hamt-testing.out
```

## Usage

Currently only get and set are implemented. Any arbitrary data `void *` can be stored against a `char *` key.
```c
#include "hamt.h"

int main(void) {
  struct hamt_node_t *hamt = create_hamt();

  hamt = hamt_set(hamt, "hello", "world");
  hamt = hamt_set(hamt, "hey", "hows it going");
  hamt = hamt_set(hamt, "hey2", "wow so doge!");
}
```

To get a value, pass the `hamt` and key to `hamt_get`. The return value is either `void *` or `NULL`. Cast the type to whatever the data type it is you gave inserted. Follow from aboves example we cast the result to `char *` and then print to stdout.

```c
#include "hamt.h"

int main(void) {
  struct hamt_node_t *hamt = create_hamt();

  // Assuming the hamt has been made as above ^
  char *found = (char *)hamt_get(hamt, "Hello");
  if (found != NULL) {
    printf("%s\n", found);
  }
}
```
