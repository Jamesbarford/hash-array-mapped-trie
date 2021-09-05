/* hamt-poly -- A polymorphic Hash Array Mapped Trie implementation.

	 Version 1.1 Sept 2021
	 Original copyright notice below.
	 My updates are given under the same Copyright licence // Martin Josefsson

	 Copyright (c) 2021, James Barford-Evans
	 All rights reserved.

	 Redistribution and use in source and binary forms, with or without
	 modification, are permitted provided that the following conditions are met:

	 * Redistributions of source code must retain the above copyright notice,
	 this list of conditions and the following disclaimer.
	 * Redistributions in binary form must reproduce the above copyright
	 notice, this list of conditions and the following disclaimer in the
	 documentation and/or other materials provided with the distribution.

	 This software is provided by the copyright holders and contributors "as is"
	 and any express or implied warranties, including, but not limited to, the
	 implied warranties of merchantability and fitness for a particular purpose
	 are disclaimed. In no event shall the copyright owner or contributors be
	 liable for any direct, indirect, incidental, special, exemplary, or
	 consequential damages (including, but not limited to, procurement of
	 substitute goods or services; loss of use, data, or profits; or business
	 interruption) however caused and on any theory of liability, whether in
	 contract, strict liability, or tort (including negligence or otherwise)
	 arising in any way out of the use of this software, even if advised of the
	 possibility of such damage.
*/
#ifndef HAMT_H
#define HAMT_H

#include <stdbool.h>
#include <string.h>

struct hamt_t;

/* This is an example of a value type.  My idea is to create C macros
which take the name of a type, and a hash function for that type, and
then creates the required functions for creating a HAMT for that type.
Thus your keys and values can be of any type, as long as you can
produce a hash for the key type. Poor mans generics. */
enum NODE_TYPE {
	LEAF,
	BRANCH,
	COLLISON,
	ARRAY_NODE
};

#define BITS     5
#define SIZE     32
#define MASK     31

#define MIN_COLLISION_NODE_SIZE 8 // this is arbitrary, as a collision
																	// should only have two nodes
#define MAX_BRANCH_SIZE         16
#define MIN_ARRAY_NODE_SIZE     8

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

#define HAMT_DEFINE(name, hashof, equals)																\
	typedef struct name##_hamt_node_t {																		\
		enum NODE_TYPE type;																								\
		unsigned int hash;																									\
		/**																																	\
		 * This is only used by the collision node and array_node and is a	\
		 * count of the total number of children held in the node						\
		 */																																	\
		int bitmap;																													\
		name *key;																													\
		void *value;																												\
		struct name##_hamt_node_t **children;																\
	} name##_hamt_node_t;																									\
																																				\
	typedef struct name##_hamt_t {																				\
		name##_hamt_node_t *root;																						\
	} name##_hamt_t;																											\
																																				\
	/*======= hashing =========================*/													\
	/**																																		\
	 * From Ideal hash trees Phil Bagwell, page 3													\
	 * https://lampwww.epfl.ch/papers/idealhashtrees.pdf									\
	 * Count number of bits in a number																		\
	 */																																		\
	static const unsigned int name##_SK5 = 0x55555555;										\
	static const unsigned int name##_SK3 = 0x33333333;										\
	static const unsigned int name##_SKF0 = 0xF0F0F0F;										\
																																				\
	static inline int name##_popcount(unsigned int bits) {								\
		bits -= ((bits >> 1) & name##_SK5);																	\
		bits = (bits & name##_SK3)  + ((bits >> 2) & name##_SK3);						\
		bits = (bits & name##_SKF0) + ((bits >> 4) & name##_SKF0);					\
		bits += bits >> 8;																									\
		return (bits + (bits >> 16)) & 0x3F;																\
	}																																			\
																																				\
	static inline unsigned int name##_get_mask(unsigned int frag) {				\
		return 1 << frag;																										\
	}																																			\
																																				\
	/* take 5 bits of the hash */																					\
	static inline unsigned int name##_get_frag(unsigned int hash, int depth) { \
		return ((unsigned int)hash >> (BITS * depth)) & MASK;								\
	}																																			\
																																				\
	/**																																		\
	 * Get the position in the array where the child is located						\
	 *																																		\
	 */																																		\
	static unsigned int name##_get_position(unsigned int hash,						\
																					unsigned int frag) {					\
		return name##_popcount(hash & (name##_get_mask(frag) - 1));					\
	}																																			\
																																				\
	name##_hamt_t* name##_create_hamt() {																	\
		name##_hamt_t *hamt;																								\
																																				\
		if ((hamt =																													\
				 (name##_hamt_t *)malloc(sizeof(name##_hamt_t))) == NULL) {			\
			fprintf(stderr, "Failed to allocate memory for hamt\n");					\
		}																																		\
																																				\
		hamt->root = NULL;																									\
		return hamt;																												\
	}																																			\
	/* Insertion methods  */																							\
	typedef struct name##_insert_instruction_t {													\
		name##_hamt_node_t *node;																						\
		unsigned int hash;																									\
		name *key;																													\
		void *value;																												\
		int depth;																													\
	} name##_insert_instruction_t;																				\
																																				\
	static name##_hamt_node_t *name##_handle_collision_insert(name##_insert_instruction_t *ins); \
	static name##_hamt_node_t *name##_handle_branch_insert(name##_insert_instruction_t *ins); \
	static name##_hamt_node_t *name##_handle_leaf_insert(name##_insert_instruction_t *ins); \
	static name##_hamt_node_t *name##_handle_arraynode_insert(name##_insert_instruction_t *ins); \
																																				\
	/* Removal methods */																									\
	typedef struct name##_hamt_removal_t {																\
		name##_hamt_node_t *node;																						\
		unsigned int hash;																									\
		name *key;																													\
		int depth;																													\
	} name##_hamt_removal_t;																							\
																																				\
	static name##_hamt_node_t *name##_handle_collision_removal(name##_hamt_removal_t *rem); \
	static name##_hamt_node_t *name##_handle_branch_removal(name##_hamt_removal_t *rem); \
	static name##_hamt_node_t *name##_handle_leaf_removal(name##_hamt_removal_t *rem); \
	static name##_hamt_node_t *name##_handle_arraynode_removal(name##_hamt_removal_t *rem); \
																																				\
	/*======= node constructors =====================*/										\
	static name##_hamt_node_t *name##_create_node(int hash,								\
																								name *key,							\
																								void *value,						\
																								enum NODE_TYPE type,		\
																								name##_hamt_node_t **children, \
																								unsigned long bitmap) {	\
		name##_hamt_node_t *node;																						\
																																				\
		if ((node =																													\
				 (name##_hamt_node_t *)malloc(sizeof(name##_hamt_node_t))) == NULL) {	\
			fprintf(stderr, "failed to allocate memory for node\n");					\
			return NULL;																											\
		}																																		\
																																				\
		node->hash     = hash;																							\
		node->type     = type;																							\
		node->key      = key;																								\
		node->value    = value;																							\
		node->children = children;																					\
		node->bitmap   = bitmap;																						\
																																				\
		return node;																												\
	}																																			\
																																				\
	static name##_hamt_node_t *name##_create_leaf(unsigned int hash,			\
																								name *key,							\
																								void *value) {					\
		return name##_create_node(hash, key, value, LEAF, NULL, 0);					\
	}																																			\
																																				\
	static name##_hamt_node_t *name##_create_collision(unsigned int hash, \
																										 name##_hamt_node_t **children,	\
																										 int bitmap) {			\
		return name##_create_node(hash, NULL, NULL,													\
															COLLISON, children, bitmap);							\
	}																																			\
																																				\
	static name##_hamt_node_t *name##_create_branch(unsigned int hash,		\
																									name##_hamt_node_t **children) { \
			return name##_create_node(hash, NULL, NULL, BRANCH, children, 0);	\
																	 }																		\
																																				\
	/* again, bitmap is size  */																					\
	static name##_hamt_node_t *name##_create_arraynode(name##_hamt_node_t **children, \
																										 unsigned int bitmap) { \
		return name##_create_node(0,																				\
															NULL,																			\
															NULL,																			\
															ARRAY_NODE,																\
															children,																	\
															bitmap);																	\
	}																																			\
																																				\
	static bool name##_is_leaf(name##_hamt_node_t *node) {								\
		return node != NULL																									\
			&& (node->type == LEAF || node->type == COLLISON);								\
	}																																			\
																																				\
	/*======= Allocators ==============*/																	\
	/* Assign `n` number of children, at least `CAPACITY` in size */			\
	static name##_hamt_node_t **name##_alloc_children(int size) {					\
		name##_hamt_node_t **children;																			\
																																				\
		if ((children =																											\
				 (name##_hamt_node_t **)calloc(sizeof(name##_hamt_node_t *),		\
																			 size)) == NULL) {								\
			fprintf(stderr, "Failed to allocate memory for children");				\
			return NULL;																											\
		}																																		\
																																				\
		return children;																										\
	}																																			\
																																				\
	/*======= moving / inserting child nodes ==============*/							\
	/**																																		\
	 * Insert child at given position																			\
	 */																																		\
	static inline void name##_insert_child(name##_hamt_node_t *parent,		\
																				 name##_hamt_node_t *child,			\
																				 unsigned int position,					\
																				 unsigned int size) {						\
		name##_hamt_node_t *temp[sizeof(name##_hamt_node_t *) * size];			\
																																				\
		unsigned int i = 0, j = 0;																					\
																																				\
		while (j < position) {																							\
			temp[i++] = parent->children[j++];																\
		}																																		\
		temp[i++] = child;																									\
		while (j < size) {																									\
			temp[i++] = parent->children[j++];																\
		}																																		\
		memcpy(parent->children,																						\
					 temp,																												\
					 sizeof(name##_hamt_node_t *) * (size + 1));									\
	}																																			\
																																				\
	/**																																		\
	 * Remove child																												\
	 */																																		\
	static inline void name##_remove_child(name##_hamt_node_t *parent,		\
																				 unsigned int position,					\
																				 unsigned int size) {						\
		int arr_size = sizeof(name##_hamt_node_t *) * (size  - 1);					\
		name##_hamt_node_t **new_children = name##_alloc_children(arr_size); \
																																				\
		unsigned int i = 0, j = 0;																					\
																																				\
		while (j < position) {																							\
			new_children[i++] = parent->children[j++];												\
		}																																		\
		j++;																																\
		while (j < size) {																									\
			new_children[i++] = parent->children[j++];												\
		}																																		\
																																				\
		parent->children = new_children;																		\
	}																																			\
																																				\
	/**																																		\
	 * Replace child																											\
	 */																																		\
	static inline void name##_replace_child(name##_hamt_node_t *node,			\
																					name##_hamt_node_t *child,		\
																					unsigned int position) {			\
		node->children[position] = child;																		\
	}																																			\
																																				\
	/**																																		\
	 * Function is just to split out the other methods										\
	 * This is an atempt at polymorphism																	\
	 */																																		\
	static name##_hamt_node_t *name##_insert(name##_hamt_node_t *node,		\
																					 unsigned int hash,						\
																					 name *key,										\
																					 void *value,									\
																					 int depth) {									\
																																				\
		name##_insert_instruction_t ins = {																	\
			.node  = node,																										\
			.key   = key,																											\
			.hash  = hash,																										\
			.value = value,																										\
			.depth = depth																										\
		};																																	\
																																				\
		switch (node->type) {																								\
		case LEAF:       return name##_handle_leaf_insert(&ins);						\
		case BRANCH:     return name##_handle_branch_insert(&ins);					\
		case COLLISON:   return name##_handle_collision_insert(&ins);				\
		case ARRAY_NODE: return name##_handle_arraynode_insert(&ins);				\
		default:																														\
			return NULL;																											\
		}																																		\
	}																																			\
																																				\
	/**																																		\
	 * If the hashes clash create a new collision node										\
	 *																																		\
	 * If the partial hashes are the same recurse													\
	 *																																		\
	 * Otherwise create a new Branch with the new hash										\
	 */																																		\
	static inline name##_hamt_node_t *name##_merge_leaves(unsigned int depth,	\
																												unsigned int h1, \
																												name##_hamt_node_t *n1, \
																												unsigned int h2, \
																												name##_hamt_node_t *n2) { \
		name##_hamt_node_t **new_children = NULL;														\
																																				\
		if (h1 == h2) {																											\
			new_children = name##_alloc_children(MIN_COLLISION_NODE_SIZE);		\
			new_children[0] = n2;																							\
			new_children[1] = n1;																							\
			return name##_create_collision(h1, new_children, 2);							\
		}																																		\
																																				\
		unsigned int sub_h1 = name##_get_frag(h1, depth);										\
		unsigned int sub_h2 = name##_get_frag(h2, depth);										\
		unsigned int new_hash = name##_get_mask(sub_h1)	| name##_get_mask(sub_h2); \
		new_children = name##_alloc_children(MAX_BRANCH_SIZE);							\
																																				\
		if (sub_h1 == sub_h2) {																							\
			new_children[0] = name##_merge_leaves(depth + 1, h1, n1, h2, n2);	\
		} else if (sub_h1 < sub_h2) {																				\
			new_children[0] = n1;																							\
			new_children[1] = n2;																							\
		} else {																														\
			new_children[0] = n2;																							\
			new_children[1] = n1;																							\
		}																																		\
																																				\
		return name##_create_branch(new_hash, new_children);								\
	}																																			\
																																				\
	/**																																		\
	 * If what we are trying to insert matches key return a new LeafNode	\
	 *																																		\
	 * If we got here and there is no match we need to transform the node	\
	 * into a branch node using 'name##_merge_leaves'											\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_leaf_insert(name##_insert_instruction_t *ins) { \
		name##_hamt_node_t *new_child =																			\
			name##_create_leaf(ins->hash, ins->key, ins->value);							\
		if (equals(ins->node->key, ins->key)) {															\
			/* if (strcmp(ins->node->key, ins->key) == 0) { */								\
			return new_child;																									\
		}																																		\
																																				\
		return name##_merge_leaves(ins->depth,															\
															 ins->node->hash,													\
															 ins->node,																\
															 new_child->hash,													\
															 new_child);															\
	}																																			\
																																				\
	static inline name##_hamt_node_t																			\
	*name##_expand_branch_to_array_node(int idx,													\
																			name##_hamt_node_t *child,				\
																			unsigned int bitmap,							\
																			name##_hamt_node_t **children) {	\
		name##_hamt_node_t **new_children = name##_alloc_children(SIZE);		\
		unsigned int bit = bitmap;																					\
		unsigned int count = 0;																							\
																																				\
		for (unsigned int i = 0; bit; ++i) {																\
			if (bit & 1) {																										\
				new_children[i] = children[count++];														\
			}																																	\
			bit >>= 1U;																												\
		}																																		\
																																				\
		new_children[idx] = child;																					\
		return name##_create_arraynode(new_children, count+1);							\
	}																																			\
																																				\
	/**																																		\
	 * If there is no node at the given index insert the child and update	\
	 * the hash.																													\
	 *																																		\
	 * If there is no node at ^ ^^ ^^ but the size is bigger than the			\
	 * maximum capacity for a Branch, then expand into an ArrayNode				\
	 *																																		\
	 * If the child exists in the slot recurse into the tree.							\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_branch_insert(name##_insert_instruction_t *ins) { \
		unsigned int frag = name##_get_frag(ins->hash, ins->depth);					\
		unsigned int mask = name##_get_mask(frag);													\
		unsigned int pos = name##_get_position(ins->node->hash, frag);			\
		bool exists = ins->node->hash & mask;																\
																																				\
		if (!exists) {																											\
			unsigned int size = name##_popcount(ins->node->hash);							\
			name##_hamt_node_t *new_child = name##_create_leaf(ins->hash,			\
																												 ins->key,			\
																												 ins->value);		\
																																				\
			if (size >= MAX_BRANCH_SIZE) {																		\
				return name##_expand_branch_to_array_node(frag,									\
																									new_child,						\
																									ins->node->hash,			\
																									ins->node->children);	\
			} else {																													\
				name##_hamt_node_t *new_branch =																\
					name##_create_branch(ins->node->hash | mask,									\
															 ins->node->children);										\
				name##_insert_child(new_branch, new_child, pos, size);					\
																																				\
				return new_branch;																							\
			}																																	\
		} else {																														\
			name##_hamt_node_t *new_branch =																	\
				name##_create_branch(ins->node->hash, ins->node->children);			\
			name##_hamt_node_t *child = new_branch->children[pos];						\
																																				\
			/* go to next depth, inserting a branch as the child */						\
			name##_replace_child(new_branch,																	\
													 name##_insert(child,													\
																				 ins->hash,											\
																				 ins->key,											\
																				 ins->value,										\
																				 ins->depth + 1),								\
													pos);																					\
																																				\
			return new_branch;																								\
		}																																		\
	}																																			\
																																				\
	/**																																		\
	 * If the key string is the same as the one we are trying to					\
	 * name##_insert then replace the node. Otherwise											\
	 * name##_insert the node at the end of the collision node's					\
	 * children																														\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_collision_insert(name##_insert_instruction_t *ins) { \
		unsigned int len = ins->node->bitmap;																\
		name##_hamt_node_t *new_child = name##_create_leaf(ins->hash,				\
																											 ins->key,				\
																											 ins->value);			\
name##_hamt_node_t *collision_node =																		\
			name##_create_collision(ins->node->hash,													\
															ins->node->children,											\
															ins->node->bitmap);												\
																																				\
		if (ins->hash == ins->node->hash) {																	\
			for (int i = 0; i < collision_node->bitmap; ++i) {								\
				if (equals(ins->node->children[i]->key,													\
									 ins->key)) {																					\
					/* if (strcmp(ins->node->children[i]->key, ins->key) == 0) { */	\
					name##_replace_child(ins->node, new_child, i);								\
					return collision_node;																				\
				}																																\
			}																																	\
																																				\
			name##_insert_child(collision_node, new_child, len, len);					\
			collision_node->bitmap++;																					\
			return collision_node;																						\
		}																																		\
																																				\
		return name##_merge_leaves(ins->depth, ins->node->hash, ins->node,	\
															 new_child->hash, new_child);							\
	}																																			\
																																				\
	/**																																		\
	 * If there is a child in the place where we are trying to						\
	 * name##_insert, step into the tree.																	\
	 *																																		\
	 * Otherwise we can create a leaf and replace the empty slot. We			\
	 * allocate 'SIZE' worth of children and fill the blank spaces with		\
	 * null so it is easy to keep track off.															\
	 *																																		\
	 * Could switch out to a bitmap																				\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_arraynode_insert(name##_insert_instruction_t *ins) { \
		unsigned int frag = name##_get_frag(ins->hash, ins->depth);					\
		int size = ins->node->bitmap;																				\
																																				\
		name##_hamt_node_t *child = ins->node->children[frag];							\
		name##_hamt_node_t *new_child = NULL;																\
																																				\
		if (child) {																												\
			new_child = name##_insert(child,																	\
																ins->hash,															\
																ins->key,																\
																ins->value,															\
																ins->depth + 1);												\
		} else {																														\
			new_child = name##_create_leaf(ins->hash, ins->key, ins->value);	\
		}																																		\
																																				\
		name##_replace_child(ins->node, new_child, frag);										\
																																				\
		if (child == NULL && new_child != NULL) {														\
			return name##_create_arraynode(ins->node->children, size + 1);		\
		}																																		\
																																				\
		return name##_create_arraynode(ins->node->children, size);					\
	}																																			\
																																				\
	/**																																		\
	 * Return a new node																									\
	 */																																		\
	name##_hamt_t *name##_hamt_set(name##_hamt_t *hamt,										\
																 name *key,															\
																 void *value) {													\
		unsigned int hash = hashof(key);																		\
																																				\
		if (hamt->root != NULL) {																						\
			hamt->root = name##_insert(hamt->root, hash, key, value, 0);			\
		} else {																														\
			hamt->root = name##_create_leaf(hash, key, value);								\
		}																																		\
																																				\
		return hamt;																												\
	}																																			\
																																				\
	/**																																		\
	 * Wind down the tree to the leaf node using the hash.								\
	 */																																		\
	void *name##_hamt_get(name##_hamt_t *hamt, name *key) {								\
		unsigned int hash = hashof(key);																		\
		name##_hamt_node_t *node = hamt->root;															\
		int depth = 0;																											\
																																				\
		for (;;) {																													\
			if (node == NULL) {																								\
				return NULL;																										\
			}																																	\
			switch (node->type) {																							\
			case BRANCH: {																										\
				unsigned int frag = name##_get_frag(hash, depth);								\
				unsigned int mask = name##_get_mask(frag);											\
																																				\
				if (node->hash & mask) {																				\
					unsigned int idx = name##_get_position(node->hash, frag);			\
					node = node->children[idx];																		\
					depth++;																											\
					continue;																											\
				} else {																												\
					return NULL;																									\
				}																																\
			}																																	\
																																				\
			case COLLISON: {																									\
				int len = node->bitmap;																					\
				for (int i = 0; i < len; ++i) {																	\
					name##_hamt_node_t *child = node->children[i];								\
					if (child != NULL &&																					\
							equals(child->key,																				\
										 key)																								\
							/* strcmp(child->key, key) == 0 */)												\
						return child->value;																				\
				}																																\
				return NULL;																										\
			}																																	\
																																				\
			case LEAF: {																											\
				if (node != NULL &&																							\
						equals(node->key,																						\
									 key)																									\
						/* strcmp(node->key, key) == 0 */														\
						) {																													\
					return node->value;																						\
				}																																\
				return NULL;																										\
			}																																	\
																																				\
			case ARRAY_NODE: {																								\
				node = node->children[name##_get_frag(hash, depth)];						\
				if (node != NULL) {																							\
					depth++;																											\
					continue;																											\
				}																																\
				return NULL;																										\
			}																																	\
			}																																	\
		}																																		\
	}																																			\
																																				\
	/* Just to split out the functions, does nothing special */						\
	static name##_hamt_node_t *name##_remove_node(name##_hamt_removal_t *rem) { \
		if (rem->node == NULL) {																						\
			return NULL;																											\
		}																																		\
																																				\
		switch (rem->node->type) {																					\
		case LEAF:       return name##_handle_leaf_removal(rem);						\
		case BRANCH:     return name##_handle_branch_removal(rem);					\
		case COLLISON:   return name##_handle_collision_removal(rem);				\
		case ARRAY_NODE: return name##_handle_arraynode_removal(rem);				\
																																				\
		default:																														\
			/* NOT REACHED  */																								\
			return rem->node;																									\
		}																																		\
	}																																			\
																																				\
	/**																																		\
	 * Removing a child from the CollisionNode or collapsing if there is	\
	 * only one child left.																								\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_collision_removal(name##_hamt_removal_t *rem) { \
		if (rem->node->hash == rem->hash) {																	\
			for (int i = 0; i < rem->node->bitmap; ++i) {											\
				name##_hamt_node_t *child = rem->node->children[i];							\
																																				\
				if (equals(child->key,																					\
									 rem->key)) {																					\
					/* if (strcmp(child->key, rem->key) == 0) { */								\
					name##_remove_child(rem->node, i, rem->node->bitmap);					\
					/* could free rem->node here */																\
					if ((rem->node->bitmap - 1) > 1) {														\
						return name##_create_collision(rem->node->hash,							\
																					 rem->node->children,					\
																					 rem->node->bitmap - 1);			\
					}																															\
					/* Collapse collision node */																	\
					return rem->node->children[0];																\
				}																																\
			}																																	\
		}																																		\
																																				\
		return rem->node;																										\
	}																																			\
																																				\
	/**																																		\
	 * Removing an element from a branch node. Either traversing down			\
	 * the tree, collapsing the node, removing a child or a noop.					\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_branch_removal(name##_hamt_removal_t *rem) {	\
		unsigned int frag = name##_get_frag(rem->hash, rem->depth);					\
		unsigned int mask = name##_get_mask(frag);													\
																																				\
		name##_hamt_node_t *branch_node = rem->node;												\
		bool exists = branch_node->hash & mask;															\
																																				\
		if (!exists) {																											\
			return branch_node;																								\
		}																																		\
																																				\
		unsigned int pos = name##_get_position(branch_node->hash, frag);		\
		int size = name##_popcount(branch_node->hash);											\
		name##_hamt_node_t *child = branch_node->children[pos];							\
		rem->node = child;																									\
		rem->depth++;																												\
																																				\
		name##_hamt_node_t *new_child = name##_remove_node(rem);						\
																																				\
		if (child == new_child) {																						\
			return branch_node;																								\
		}																																		\
																																				\
		if (new_child == NULL) {																						\
			unsigned int new_hash = branch_node->hash & ~mask;								\
			if (!new_hash) {																									\
				return NULL;																										\
			}																																	\
																																				\
			/* Collapse the node */																						\
			if (size == 2 &&																									\
					name##_is_leaf(branch_node->children[pos ^ 1])) {							\
				return branch_node->children[pos ^ 1];													\
			}																																	\
																																				\
			name##_remove_child(branch_node, pos, size);											\
			return name##_create_branch(new_hash, branch_node->children);			\
		}																																		\
																																				\
		if (size == 1 && name##_is_leaf(new_child)) {												\
			return new_child;																									\
		}																																		\
																																				\
		name##_replace_child(branch_node, new_child, pos);									\
		return name##_create_branch(branch_node->hash,											\
																branch_node->children);									\
	}																																			\
																																				\
	/**																																		\
	 * Remove the node, as a modification if you passed through a free		\
	 * function from the top, you could then free your object here.				\
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_leaf_removal(name##_hamt_removal_t *rem) { \
		if (equals(rem->node->key,																					\
							 rem->key)) {																							\
			/* if (strcmp(rem->node->key, rem->key) == 0) { */								\
			/* could free rem->node here */																		\
			return NULL;																											\
		}																																		\
																																				\
		return rem->node;																										\
	}																																			\
																																				\
	/**																																		\
	 * Transform ArrayNode into a BranchNode. Setting each bit in the hash for \
	 * where a child is not NULL.																					\
	 *																																		\
	 * We alloc MIN_ARRAY_NODE_SIZE as inorder to have got here the lower bound	\
	 * limit for the ArrayNode must have been met.												\
	 */																																		\
	static inline name##_hamt_node_t *name##_compress_array_to_branch(unsigned int idx,	\
																																		name##_hamt_node_t **children) { \
																																				\
		name##_hamt_node_t **new_children =																	\
			name##_alloc_children(MAX_BRANCH_SIZE);														\
		name##_hamt_node_t *child = NULL;																		\
		int j = 0;																													\
		unsigned int hash = 0;																							\
																																				\
		for (unsigned int i = 0; i < SIZE; ++i) {														\
			if (i != idx) {																										\
				child = children[i];																						\
				if (child != NULL) {																						\
					new_children[j++] = child;																		\
					hash |= 1 << i;																								\
				}																																\
			}																																	\
		}																																		\
																																				\
		return name##_create_branch(hash, new_children);											\
	}																																			\
																																				\
	/**																																		\
	 * Returns a new array node with the child with key `rem->key` removed \
	 * from the children																									\
	 *																																		\
	 * Or if the total number of children is less than `MIN_ARRAY_NODE_SIZE` \
	 * will compress the node to a branch node and create the branch node hash \
	 */																																		\
	static inline name##_hamt_node_t *name##_handle_arraynode_removal(name##_hamt_removal_t *rem) { \
		unsigned int idx = name##_get_frag(rem->hash, rem->depth);					\
																																				\
		/* The node we are looking at */																		\
		name##_hamt_node_t *array_node = rem->node;													\
		/* This is nasty as the bitmap is used for different things.				\
			 Here it is just a counter with the number of elements in the			\
			 array `children` */																							\
																																				\
		int size = array_node->bitmap;																			\
																																				\
		name##_hamt_node_t *child = array_node->children[idx];								\
		name##_hamt_node_t *new_child = NULL;																\
																																				\
		if (child != NULL) {																								\
			rem->node = child;																								\
			rem->depth++;																											\
			/* go 'in' to the structure */																		\
			new_child = name##_remove_node(rem);																\
		} else {																														\
			new_child = NULL;																									\
		}																																		\
																																				\
		if (child == new_child) {																						\
			return array_node;																								\
		}																																		\
																																				\
		if (child != NULL && new_child == NULL) {														\
			if ((size - 1) <= MIN_ARRAY_NODE_SIZE) {													\
				return name##_compress_array_to_branch(idx, array_node->children);	\
			}																																	\
			name##_replace_child(array_node, NULL, idx);												\
			return name##_create_arraynode(array_node->children, array_node->bitmap - 1); \
		}																																		\
																																				\
		name##_replace_child(array_node, new_child, idx);										\
		return name##_create_arraynode(array_node->children,								\
																	 array_node->bitmap);									\
	}																																			\
																																				\
	/** 																																	\
	 * Remove a node from the tree, the delete happens on the leaf or			\
	 * collision node layer.																							\
	 *																																		\
	 * I've been testing this rather horribly with a counter to ensure		\
	 * the 466550 from the test dictionary actually get removed.					\
	 */																																		\
	name##_hamt_t *name##_hamt_remove(name##_hamt_t *hamt, name *key) {		\
		unsigned int hash = hashof(key);																		\
		name##_hamt_removal_t rem;																					\
		rem.hash = hash;																										\
		rem.depth = 0;																											\
		rem.key = key;																											\
		rem.node = hamt->root;																							\
																																				\
		if (hamt->root != NULL) {																						\
			hamt->root = name##_remove_node(&rem);														\
		}																																		\
																																				\
		return hamt;																												\
	}
/* \ */
/* /\*=========== Printing / visiting functions ====== *\/									\ */
/* static void name##_visit_all_nodes(name##_hamt_node_t *hamt,					\ */
/* 																	 void(*visitor)(name *key, void *value)) { \ */
/* 	if (hamt) {																													\ */
/* 		switch (hamt->type) {																							\ */
/* 		case ARRAY_NODE:																									\ */
	/* 		case BRANCH: {																										\ */
	/* 			int len = name##_popcount(hamt->bitmap);																\ */
	/* 			for (int i = 0; i < len; ++i) {																	\ */
	/* 				name##_hamt_node_t *child = hamt->children[i];								\ */
	/* 				name##_visit_all_nodes(child, visitor);												\ */
	/* 			}																																\ */
	/* 			return;																													\ */
	/* 		}																																	\ */
	/* 		case COLLISON: {																									\ */
	/* 			int len = name##_popcount(hamt->bitmap);																\ */
	/* 			for (int i = 0; i < len; ++i) {																	\ */
	/* 				name##_hamt_node_t *child = hamt->children[i];								\ */
	/* 				name##_visit_all_nodes(child, visitor);												\ */
	/* 			}																																\ */
	/* 			return;																													\ */
	/* 		}																																	\ */
	/* 		case LEAF: {																											\ */
	/* 			visitor(hamt->key, hamt->value);																\ */
	/* 		}																																	\ */
	/* 		}																																	\ */
	/* 	}																																		\ */
	/* }																																			\ */
	/* 																																			\ */
	/* void name##_visit_all(name##_hamt_t *hamt,														\ */
	/* 											void (*visitor)(name *, void *)) {							\ */
	/* 	name##_visit_all_nodes(hamt->root, visitor);												\ */
	/* }																																			\ */
	/* 																																			\ */
	/* static void Value_print_node(Value *key, void *value) {								\ */
	/* 	(void)value;																												\ */
	/* 	printf("key: %s\n", string_of_value(key));													\ */
	/* }																																			\ */
	/* 																																			\ */
	/* void Value_print_hamt(Value_hamt_t *hamt) {														\ */
	/* 	Value_visit_all(hamt, Value_print_node);														\ */
	/* } */

/* typedef struct Coolstr { */
/* 	char str[20]; */
/* } Coolstr; */

/* bool coolstr_equals(Coolstr* s0, Coolstr* s1) { */
/* 	return strcmp(s0->str, */
/* 								s1->str) == 0; */
/* } */
/* unsigned int get_hash_of_coolstr(Coolstr* s) {return get_hash(s->str);} */

/* HAMT_DEFINE(Coolstr, get_hash_of_coolstr, coolstr_equals) */

#endif
