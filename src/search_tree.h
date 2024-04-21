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
 * \file search_tree.h
 *
 * Search Tree Interface.
 */

#ifndef XMLMAN_SEARCH_TREE_H
#define XMLMAN_SEARCH_TREE_H

/**
 * A Search Tree instance.
 */

struct search_tree;

/**
 * Create a new search tree root node.
 *
 * \return		The new node, or NULL on failure.
 */

struct search_tree *search_tree_create(void);

/**
 * Add a record to a search tree.
 *
 * \param *node		The node to add the record beneath.
 * \param *key		The key text to use to look the node up.
 * \param *data		Client data to be returned if a match is found.
 * \return		True if successful; otherwise false.
 */

bool search_tree_add_entry(struct search_tree *instance, const char *key, void *data);

/**
 * Look up a key in a search tree, returning a pointer to the
 * associated client data if a match is found or NULL otherwise.
 *
 * \param *node		The node to search from.
 * \param *key		The key text to search from.
 * \return		The client data if a match is found, or
 *			NULL otherwise.
 */

void *search_tree_find_entry(struct search_tree *node, char *key);

#endif

