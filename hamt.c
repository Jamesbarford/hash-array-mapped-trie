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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "hamt.h"

#define BITS     5
#define SIZE     32
#define MASK     31

#define MIN_COLLISION_NODE_SIZE 8 // this is arbitrary, as a collision should only have
																	// two nodes
#define MAX_BRANCH_SIZE         16
#define MIN_ARRAY_NODE_SIZE     8

enum NODE_TYPE {
	LEAF,
	BRANCH,
	COLLISON,
	ARRAY_NODE
};

typedef struct hamt_node_t {
	enum NODE_TYPE type;
	unsigned int hash;
	/**
	 * This is only used by the collision node and array_node and is a count of
	 * the total number of children held in the node
	 */
	int bitmap;
	char *key;
	void *value;
	struct hamt_node_t **children;
} hamt_node_t;

typedef struct hamt_t {
	hamt_node_t *root;
} hamt_t;

// Insertion methods
typedef struct insert_instruction_t {
	hamt_node_t *node;
	unsigned int hash;
	char *key;
	void *value;
	int depth;
} insert_instruction_t;

static hamt_node_t *handle_collision_insert(insert_instruction_t *ins);
static hamt_node_t *handle_branch_insert(insert_instruction_t *ins);
static hamt_node_t *handle_leaf_insert(insert_instruction_t *ins);
static hamt_node_t *handle_arraynode_insert(insert_instruction_t *ins);

// Removal methods
typedef struct hamt_removal_t {
	hamt_node_t *node;
	unsigned int hash;
	char *key;
	int depth;
} hamt_removal_t;

static hamt_node_t *handle_collision_removal(hamt_removal_t *rem);
static hamt_node_t *handle_branch_removal(hamt_removal_t *rem);
static hamt_node_t *handle_leaf_removal(hamt_removal_t *rem);
static hamt_node_t *handle_arraynode_removal(hamt_removal_t *rem);

/*======= node constructors =====================*/
static hamt_node_t *create_node(int hash, char *key, void *value,
		enum NODE_TYPE type, hamt_node_t **children, unsigned long bitmap) {
	hamt_node_t *node;

	if ((node = (hamt_node_t *)malloc(sizeof(hamt_node_t))) == NULL) {
		fprintf(stderr, "failed to allocate memory for node\n");
		return NULL;
	}

	node->hash     = hash;
	node->type     = type;
	node->key      = key;
	node->value    = value;
	node->children = children;
	node->bitmap   = bitmap;

	return node;
}

hamt_t *create_hamt() {
	hamt_t *hamt;

	if ((hamt = (hamt_t *)malloc(sizeof(hamt_t))) == NULL) {
		fprintf(stderr, "Failed to allocate memory for hamt\n");
	}

	hamt->root = NULL;
	return hamt;
}

static hamt_node_t *create_leaf(unsigned int hash, char *key, void *value) {
	return create_node(hash, key, value, LEAF, NULL, 0);
}

static hamt_node_t *create_collision(unsigned int hash, hamt_node_t **children,
		int bitmap) {
	return create_node(hash, NULL, NULL, COLLISON, children, bitmap);
}

static hamt_node_t *create_branch(unsigned int hash, hamt_node_t **children) {
	return create_node(hash, NULL, NULL, BRANCH, children, 0);
}

/* again, bitmap is size  */
static hamt_node_t *create_arraynode(hamt_node_t **children, unsigned int bitmap) {
	return create_node(0, NULL, NULL, ARRAY_NODE, children, bitmap);
}

static bool is_leaf(hamt_node_t *node) {
	return node != NULL && (node->type == LEAF || node->type == COLLISON);
}

/*======= hashing =========================*/
/**
 * From Ideal hash trees Phil Bagley, page 3
 * https://lampwww.epfl.ch/papers/idealhashtrees.pdf
 * Count number of bits in a number
 */
static const unsigned int SK5 = 0x55555555;
static const unsigned int SK3 = 0x33333333;
static const unsigned int SKF0 = 0xF0F0F0F;

static inline int popcount(unsigned int bits) {
	bits -= ((bits >> 1) & SK5);
	bits = (bits & SK3)  + ((bits >> 2) & SK3);
	bits = (bits & SKF0) + ((bits >> 4) & SKF0);
	bits += bits >> 8;
	return (bits + (bits >> 16)) & 0x3F;
}

/**
 * convert a string to a 32bit unsigned integer
 */
static inline unsigned int get_hash(char *str) {
	unsigned int hash = 0;	
	char *ptr = str;

	while (*ptr != '\0') {
		hash = ((hash << BITS) - hash) + *(ptr++);
	}

	return hash;
}

static inline unsigned int get_mask(unsigned int frag) {
	return 1 << frag;
}

/* take 5 bits of the hash */
static inline unsigned int get_frag(unsigned int hash, int depth) {
	return ((unsigned int)hash >> (BITS * depth)) & MASK;
}

/**
 * Get the position in the array where the child is located
 *
 */
static unsigned int get_position(unsigned int hash, unsigned int frag) {
	return popcount(hash & (get_mask(frag) - 1));
}

/*======= Allocators ==============*/
/* Assign `n` number of children, at least `CAPACITY` in size */
static hamt_node_t **alloc_children(int size) {
	hamt_node_t **children;

	if ((children = (hamt_node_t **)calloc(sizeof(hamt_node_t *), size)) == NULL) {
		fprintf(stderr, "Failed to allocate memory for children");
		return NULL;
	}

	return children;
}	

/*======= moving / inserting child nodes ==============*/
/**
 * Insert child at given position
 */
static inline void insert_child(hamt_node_t *parent, hamt_node_t *child,
		unsigned int position, unsigned int size) {

	hamt_node_t *temp[sizeof(hamt_node_t *) * size];

	unsigned int i = 0, j = 0;

	while (j < position) {
		temp[i++] = parent->children[j++];
	}
	temp[i++] = child;
	while (j < size) {
		temp[i++] = parent->children[j++];
	}
	memcpy(parent->children, temp, sizeof(hamt_node_t *) * (size + 1));
}

/**
 * Remove child
 */
static inline void remove_child(hamt_node_t *parent, unsigned int position,
		unsigned int size) {
	int arr_size = sizeof(hamt_node_t *) * (size  - 1);
	hamt_node_t **new_children = alloc_children(arr_size);

	unsigned int i = 0, j = 0;

	while (j < position) {
		new_children[i++] = parent->children[j++];
	}
	j++;
	while (j < size) {
		new_children[i++] = parent->children[j++];
	}

	parent->children = new_children;
}

/**
 * Replace child
 */
static inline void replace_child(hamt_node_t *node, hamt_node_t *child,
		unsigned int position) {
	node->children[position] = child;
}

/**
 * Function is just to split out the other methods
 * This is an atempt at polymorphism
 */
static hamt_node_t *insert(hamt_node_t *node, unsigned int hash, char *key,
		void *value, int depth) {
	
	insert_instruction_t ins = {
		.node  = node,
		.key   = key,
		.hash  = hash,
		.value = value,
		.depth = depth
	};

	switch (node->type) {
		case LEAF:       return handle_leaf_insert(&ins);
		case BRANCH:     return handle_branch_insert(&ins);
		case COLLISON:   return handle_collision_insert(&ins);
		case ARRAY_NODE: return handle_arraynode_insert(&ins); 
		default:
			return NULL;
	}
}

/**
 * If the hashes clash create a new collision node
 *
 * If the partial hashes are the same recurse
 *
 * Otherwise create a new Branch with the new hash
 */
static inline hamt_node_t *merge_leaves(unsigned int depth, unsigned int h1,
		hamt_node_t *n1, unsigned int h2, hamt_node_t *n2) {
	hamt_node_t **new_children = NULL;

	if (h1 == h2) {
		new_children = alloc_children(MIN_COLLISION_NODE_SIZE);
		new_children[0] = n2;
		new_children[1] = n1;
		return create_collision(h1, new_children, 2);
	}

	unsigned int sub_h1 = get_frag(h1, depth);
	unsigned int sub_h2 = get_frag(h2, depth);
	unsigned int new_hash = get_mask(sub_h1) | get_mask(sub_h2);
	new_children = alloc_children(MAX_BRANCH_SIZE);

	if (sub_h1 == sub_h2) {
		new_children[0] = merge_leaves(depth + 1, h1, n1, h2, n2);
	} else if (sub_h1 < sub_h2) {
		new_children[0] = n1;
		new_children[1] = n2;
	} else {
		new_children[0] = n2;
		new_children[1] = n1;
	}

	return create_branch(new_hash, new_children);
}

/**
 * If what we are trying to insert matches key return a new LeafNode
 * 
 * If we got here and there is no match we need to transform the node
 * into a branch node using 'merge_leaves'
 */
static inline hamt_node_t *handle_leaf_insert(insert_instruction_t *ins) {
	hamt_node_t *new_child = create_leaf(ins->hash, ins->key, ins->value);
	if (strcmp(ins->node->key, ins->key) == 0) {
		return new_child;
	}

	return merge_leaves(ins->depth, ins->node->hash, ins->node, new_child->hash,
			new_child);
}

static inline hamt_node_t *expand_branch_to_array_node(int idx, hamt_node_t *child,
		unsigned int bitmap, hamt_node_t **children) {

	hamt_node_t **new_children = alloc_children(SIZE);
	unsigned int bit = bitmap;
	unsigned int count = 0;

	for (unsigned int i = 0; bit; ++i) {
		if (bit & 1) {
			new_children[i] = children[count++];
		}
		bit >>= 1UL;
	}

	new_children[idx] = child;
	return create_arraynode(new_children, count+1);
}

/**
 * If there is no node at the given index insert the child and update the hash.
 * 
 * If there is no node at ^   ^^     ^^   but the size is bigger than the
 * maximum capacity for a Branch, then expand into an ArrayNode
 * 
 * If the child exists in the slot recurse into the tree.
 */
static inline hamt_node_t *handle_branch_insert(insert_instruction_t *ins) {
	unsigned int frag = get_frag(ins->hash, ins->depth);
	unsigned int mask = get_mask(frag);
	unsigned int pos = get_position(ins->node->hash, frag);
	bool exists = ins->node->hash & mask;

	if (!exists) {
		unsigned int size = popcount(ins->node->hash);
		hamt_node_t *new_child = create_leaf(ins->hash, ins->key, ins->value);
		
		if (size >= MAX_BRANCH_SIZE) {
			return expand_branch_to_array_node(frag, new_child, ins->node->hash,
					ins->node->children);
		} else {
			hamt_node_t *new_branch = create_branch(ins->node->hash | mask,
					ins->node->children);
			insert_child(new_branch, new_child, pos, size);

			return new_branch;
		}
	} else {
		hamt_node_t *new_branch = create_branch(ins->node->hash, ins->node->children);
		hamt_node_t *child = new_branch->children[pos];

		// go to next depth, inserting a branch as the child
		replace_child(new_branch, insert(child, ins->hash, ins->key, ins->value,
					ins->depth + 1), pos);

		return new_branch;
	}
}

/**
 * If the key string is the same  as the one we are trying to insert then
 * replace the node.
 *
 * Otherwise insert the node at the end of the collision node's children
 */
static inline hamt_node_t *handle_collision_insert(insert_instruction_t *ins) {
	unsigned int len = ins->node->bitmap;	
	hamt_node_t *new_child = create_leaf(ins->hash, ins->key, ins->value);
	hamt_node_t *collision_node = create_collision(ins->node->hash,
			ins->node->children, ins->node->bitmap);

	if (ins->hash == ins->node->hash) {
		for (int i = 0; i < collision_node->bitmap; ++i) {	
			if (strcmp(ins->node->children[i]->key, ins->key) == 0) {
				replace_child(ins->node, new_child, i);
				return collision_node;
			}
		}

		insert_child(collision_node, new_child, len, len);
		collision_node->bitmap++;
		return collision_node;
	}

	return merge_leaves(ins->depth, ins->node->hash, ins->node,
			new_child->hash, new_child);
}

/**
 * If there is a child in the place where we are trying to insert, step
 * into the tree.
 *
 * Otherwise we can create a leaf and replace the empty slot. We allocate
 * 'SIZE' worth of children and fill the blank spaces with null so it is
 * easy to keep track off.
 *
 * Could switch out to a bitmap
 */
static inline hamt_node_t *handle_arraynode_insert(insert_instruction_t *ins) {
	unsigned int frag = get_frag(ins->hash, ins->depth);
	int size = ins->node->bitmap;

	hamt_node_t *child = ins->node->children[frag];
	hamt_node_t *new_child = NULL;

	if (child) {
		new_child = insert(child, ins->hash, ins->key, ins->value, ins->depth + 1);
	} else {
		new_child = create_leaf(ins->hash, ins->key, ins->value);
	}

	replace_child(ins->node, new_child, frag);

	if (child == NULL && new_child != NULL) {
		return create_arraynode(ins->node->children, size + 1);
	}

	return create_arraynode(ins->node->children, size);
}

/**
 * Return a new node 
 */
hamt_t *hamt_set(hamt_t *hamt, char *key, void *value) {
	unsigned int hash = get_hash(key);	

	if (hamt->root != NULL) {
		hamt->root = insert(hamt->root, hash, key, value, 0);
	} else {
		hamt->root = create_leaf(hash, key, value);
	}

	return hamt;
}

/**
 * Wind down the tree to the leaf node using the hash.
 */
void *hamt_get(hamt_t *hamt, char *key) {
	unsigned int hash = get_hash(key);
	hamt_node_t *node = hamt->root;
	int depth = 0;

	for (;;) {
		if (node == NULL) {
			return NULL;
		}
		switch (node->type) {
			case BRANCH: {
				unsigned int frag = get_frag(hash, depth);
				unsigned int mask = get_mask(frag);

				if (node->hash & mask) {
					unsigned int idx = get_position(node->hash, frag);
					node = node->children[idx];
					depth++;
					continue;
				} else {
					return NULL;
				}
			}

			case COLLISON: {
				int len = node->bitmap;
				for (int i = 0; i < len; ++i) {
					hamt_node_t *child = node->children[i];
					if (child != NULL && strcmp(child->key, key) == 0)
						return child->value;
				}	
				return NULL;
			}
	
			case LEAF: {
				if (node != NULL && strcmp(node->key, key) == 0) {
					return node->value;
				}
				return NULL;
			}

			case ARRAY_NODE: {
				node = node->children[get_frag(hash, depth)];
				if (node != NULL) {
					depth++;
					continue;
				}
				return NULL;
			}
		}	
	}
}

// Just to split out the functions, does nothing special
static hamt_node_t *remove_node(hamt_removal_t *rem) {
	if (rem->node == NULL) {
		return NULL;
	}

	switch (rem->node->type) {
		case LEAF:       return handle_leaf_removal(rem);
		case BRANCH:     return handle_branch_removal(rem);
		case COLLISON:   return handle_collision_removal(rem);
		case ARRAY_NODE: return handle_arraynode_removal(rem);

		default:
			/* NOT REACHED  */
			return rem->node;
	}
}

/**
 * Removing a child from the CollisionNode or collapsing if there is
 * only one child left.
 */
static inline hamt_node_t *handle_collision_removal(hamt_removal_t *rem) {
	if (rem->node->hash == rem->hash) {
		for (int i = 0; i < rem->node->bitmap; ++i) {
			hamt_node_t *child = rem->node->children[i];

			if (strcmp(child->key, rem->key) == 0) {
				remove_child(rem->node, i, rem->node->bitmap);
				// could free rem->node here
				if ((rem->node->bitmap - 1) > 1) {
					return create_collision(rem->node->hash, rem->node->children,
							rem->node->bitmap - 1);
				}
				// Collapse collision node
				return rem->node->children[0];
			}
		}
	}

	return rem->node;
}

/**
 * Removing an element from a branch node. Either traversing down the tree,
 * collapsing the node, removing a child or a noop.
 */
static inline hamt_node_t *handle_branch_removal(hamt_removal_t *rem) {
	unsigned int frag = get_frag(rem->hash, rem->depth);
	unsigned int mask = get_mask(frag);

	hamt_node_t *branch_node = rem->node;
	bool exists = branch_node->hash & mask;

	if (!exists) {
		return branch_node;
	}

	unsigned int pos = get_position(branch_node->hash, frag);
	int size = popcount(branch_node->hash);
	hamt_node_t *child = branch_node->children[pos];
	rem->node = child;
	rem->depth++;

	hamt_node_t *new_child = remove_node(rem);

	if (child == new_child) {
		return branch_node;
	}

	if (new_child == NULL) {
		unsigned int new_hash = branch_node->hash & ~mask;
		if (!new_hash) {
			return NULL;
		}

		// Collapse the node
		if (size == 2 && is_leaf(branch_node->children[pos ^ 1])) {
			return branch_node->children[pos ^ 1];
		}

		remove_child(branch_node, pos, size);
		return create_branch(new_hash, branch_node->children);
	}

	if (size == 1 && is_leaf(new_child)) {
		return new_child;
	}

	replace_child(branch_node, new_child, pos);
	return create_branch(branch_node->hash, branch_node->children);
}


/**
 * Remove the node, as a modification if you passed through a free function
 * from the top, you could then free your object here.
 */
static inline hamt_node_t *handle_leaf_removal(hamt_removal_t *rem) {
	if (strcmp(rem->node->key, rem->key) == 0) {
		// could free rem->node here
		return NULL;
	}

	return rem->node;
}

/**
 * Transform ArrayNode into a BranchNode. Setting each bit in the hash for
 * where a child is not NULL.
 *
 * We alloc MIN_ARRAY_NODE_SIZE as inorder to have got here the lower bound
 * limit for the ArrayNode must have been met.
 */
static inline hamt_node_t *compress_array_to_branch(unsigned int idx,
		hamt_node_t **children) {

	hamt_node_t **new_children = alloc_children(MAX_BRANCH_SIZE);
	hamt_node_t *child = NULL;
	int j = 0;
	unsigned int hash = 0;

	for (unsigned int i = 0; i < SIZE; ++i) {
		if (i != idx) {
			child = children[i];
			if (child != NULL) {
				new_children[j++] = child;
				hash |= 1 << i;
			}
		}
	}

	return create_branch(hash, new_children);
}

/**
 * Returns a new array node with the child with key `rem->key` removed
 * from the children
 *
 * Or if the total number of children is less than `MIN_ARRAY_NODE_SIZE`
 * will compress the node to a branch node and create the branch node hash
 */
static inline hamt_node_t *handle_arraynode_removal(hamt_removal_t *rem) {
	unsigned int idx = get_frag(rem->hash, rem->depth);

	// the node we are looking at
	hamt_node_t *array_node = rem->node;
	// this is nasty as the bitmap is used for different things
	// here it is just a counter with the number of elements in the array
	// `children`
	int size = array_node->bitmap;

	hamt_node_t *child = array_node->children[idx];
	hamt_node_t *new_child = NULL;

	if (child != NULL) {
		rem->node = child;
		rem->depth++;
		// go 'in' to the structure
		new_child = remove_node(rem);
	} else {
		new_child = NULL;
	}

	if (child == new_child) {
		return array_node;
	}

	if (child != NULL && new_child == NULL) {
		if ((size - 1) <= MIN_ARRAY_NODE_SIZE) {
			return compress_array_to_branch(idx, array_node->children);
		}
		replace_child(array_node, NULL, idx);
		return create_arraynode(array_node->children, array_node->bitmap - 1);
	}

	replace_child(array_node, new_child, idx);
	return create_arraynode(array_node->children, array_node->bitmap);
}

/**
 * Remove a node from the tree, the delete happens on the leaf or collision
 * node layer.
 *
 * I've been testing this rather horribly with a counter to ensure the 466550
 * from the test dictionary actually get removed.
 */
hamt_t *hamt_remove(hamt_t *hamt, char *key) {
	unsigned int hash = get_hash(key);
	hamt_removal_t rem;
	rem.hash = hash;
	rem.depth = 0;
	rem.key = key;
	rem.node = hamt->root;

	if (hamt->root != NULL) {
		hamt->root = remove_node(&rem);
	}

	return hamt;
}


/*=========== Printing / visiting functions ====== */
static void visit_all_nodes(hamt_node_t *hamt, void(*visitor)(char *key, void *value)) {
	if (hamt) {
		switch (hamt->type) {
			case ARRAY_NODE:
			case BRANCH: {
				int len = popcount(hamt->bitmap);
				for (int i = 0; i < len; ++i) {
					hamt_node_t *child = hamt->children[i];
					visit_all_nodes(child, visitor);
				}
				return;
			}
			case COLLISON: {
				int len = popcount(hamt->bitmap);
				for (int i = 0; i < len; ++i) {
					hamt_node_t *child = hamt->children[i];
					visit_all_nodes(child, visitor);
				}
				return;
			}
			case LEAF: {
				visitor(hamt->key, hamt->value);
			}
		}
	}
}

void visit_all(hamt_t *hamt, void (*visitor)(char *, void *)) {
	visit_all_nodes(hamt->root, visitor);
}

static void print_node(char *key, void *value) {
	(void)value;
	printf("key: %s\n", key);
}

void print_hamt(hamt_t *hamt) {
	visit_all(hamt, print_node);
}
