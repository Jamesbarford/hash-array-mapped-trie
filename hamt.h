/* hamt -- A Hash Array Mapped Trie implementation.
 *
 * Version 1.0 May 2021
 *
 * Copyright (c) 2021, James Barford-Evans
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef HAMT_H
#define HAMT_H

struct hamt_t;

/* This is an example of a value type.  My idea is to create C macros
which take the name of a type, and a hash function for that type, and
then creates the required functions for creating a HAMT for that type.
Thus your keys and values can be of any type, as long as you can
produce a hash for the key type. Poor mans generics. */
union uvalue {
	char* string;
	unsigned char u8;
};
enum runtime_type {
	STRING,
	U8
};
typedef struct Value {
	enum runtime_type type;
	union uvalue actual_value;
} Value;

struct hamt_t *create_hamt();
struct hamt_t *hamt_set(struct hamt_t *hamt, Value *key, void *value);
struct hamt_t *hamt_remove(struct hamt_t *node, Value *key);
void *hamt_get(struct hamt_t *hamt, Value *key);
void print_hamt(struct hamt_t *hamt);
void visit_all(struct hamt_t *hamt, void (*visitor)(Value *key, void *value));

#endif
