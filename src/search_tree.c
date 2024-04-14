/* Copyright 2024, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/risc-os
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 *
 * You may obtain a copy of the Licence at:
 *
 *   http://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 */

/**
 * \file search_tree.c
 *
 * Search Tree, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

/* Local source headers. */

#include "search_tree.h"

#include "msg.h"

/**
 * The number of bins in the bin table.
 */

#define SEARCH_TREE_BIN_COUNT 27

/* Static function prototypes. */

static struct search_tree *search_tree_create_new_node(char c);
static bool search_tree_add_next_node(struct search_tree *node, char *key_tail);
static struct search_tree *search_tree_find_next_node(struct search_tree *node, char *key_tail);
static int search_tree_find_bin(char c);

/**
 * A tree search node.
 */

struct search_tree {
	/**
	 * The character to which this node relates.
	 */
	char	c;

	/**
	 * The bins containing child nodes in the tree.
	 */
	struct search_tree *bins[SEARCH_TREE_BIN_COUNT];

	/**
	 * Pointer to a next sibling node, if present.
	 */
	struct search_tree *next;

	/**
	 * Client data for the node.
	 */
	void *data;
};

/**
 * Create a new search tree root node.
 *
 * \return		The new node, or NULL on failure.
 */

struct search_tree *search_tree_create(void)
{
	return search_tree_create_new_node('\0');
}

/**
 * Add a record to a search tree.
 *
 * \param *node		The node to add the record beneath.
 * \param *key		The key text to use to look the node up.
 * \param *data		Client data to be returned if a match is found.
 * \return		True if successful; otherwise false.
 */

bool search_tree_add_entry(struct search_tree *node, char *key, void *data)
{
	struct search_tree *next = NULL;
	int bin;

	if (node == NULL || key == NULL || data == NULL)
		return false;

	/* This is the end of the key, so store the data and return. */

	if (*key == '\0') {
		/* If there's data, this must be a duplicate. */

		if (node->data != NULL) {
			msg_report(MSG_TREE_DUPLICATE);
			return false;
		}

		node->data = data;

		return true;
	}

	/* Look for an existing node. */

	next = search_tree_find_next_node(node, key);

	if (next == NULL) {
		bin = search_tree_find_bin(*key);
		if (bin == -1)
			return false;

		next = search_tree_create_new_node(*key);
		if (next == NULL)
			return false;

		next->next = node->bins[bin];
		node->bins[bin] = next;
	}

	return search_tree_add_entry(next, key + 1, data);
}

/**
 * Look up a key in a search tree, returning a pointer to the
 * associated client data if a match is found or NULL otherwise.
 *
 * \param *node		The node to search from.
 * \param *key		The key text to search from.
 * \return		The client data if a match is found, or
 *			NULL otherwise.
 */

void *search_tree_find_entry(struct search_tree *node, char *key)
{
	struct search_tree *next = NULL;
	int bin;

	if (node == NULL || key == NULL)
		return false;

	/* This is the end of the key, so store the data and return. */

	if (*key == '\0')
		return node->data;

	/* Look for an existing node. */

	next = search_tree_find_next_node(node, key);
	if (next == NULL)
		return NULL;

	return search_tree_find_entry(next, key + 1);
}

/**
 * Search for a key in a search tree node, using the first character
 * of the supplied bey tail and returning the appropriate child node
 * if a match is found or NULL otherwise.
 *
 * \param *node		The tree node to search in.
 * \param *key_tail	The tail of the key to be matched.
 * \return		Pointer to the child node, or NULL.
*/

static struct search_tree *search_tree_find_next_node(struct search_tree *node, char *key_tail)
{
	struct search_tree *next;
	int bin;

	if (node == NULL || key_tail == NULL)
		return NULL;

	/* If the key has ended, this is the node. */

	if (*key_tail == '\0')
		return NULL;

	bin = search_tree_find_bin(*key_tail);
	if (bin == -1)
		return NULL;

	next = node->bins[bin];

	while (next != NULL) {
		if (next->c == *key_tail)
			return next;

		next = next->next;
	}

	return NULL;
}

/**
 * Create a new search tree node. This will be unlinked, but all
 * of its data entries will be initialised.
 *
 * \param c		The character to which the node will relate.
 * \return		Pointer to the new node, or NULL on failure.
 */

static struct search_tree *search_tree_create_new_node(char c)
{
	struct search_tree *new;
	int i;

	new = malloc(sizeof(struct search_tree));
	if (new == NULL) {
		msg_report(MSG_TREE_MALLOC_FAIL);
		return NULL;
	}

	new->c = c;

	for (i = 0; i < SEARCH_TREE_BIN_COUNT; i++)
		new->bins[i] = NULL;

	new->next = NULL;
	new->data = NULL;

	return new;
}

/**
 * Given a character, return a bin number to search in.
 *
 * \param c		The character to look up.
 * \return		The associated bin number.
 */

static int search_tree_find_bin(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 1 - 'A';
	else if (c >= 'a' && c <= 'z')
		return c + 1 - 'a';
	else if (c == '\0')
		return -1;
	else
		return 0;
}
