/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file parse_link.c
 *
 * XML Parser Document Linking, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "parse_link.h"

#include "manual_data.h"
#include "manual_ids.h"
#include "msg.h"

/* Static Function Prototypes. */

static void parse_link_node(struct manual_data *node, struct manual_data *parent, struct manual_ids *id_index);

/**
 * Link a node and its children, connecting the previous and parent node
 * references.
 *
 * \param *root		The root node to link from.
 * \return		An ID Index instance, or NULL on failure..
 */

struct manual_ids *parse_link(struct manual_data *root)
{
	struct manual_ids *id_index;

	id_index = manual_ids_create();
	if (id_index == NULL)
		return NULL;

	parse_link_node(root, NULL, id_index);

	return id_index;
}

/**
 * Recursively link a node with its siblings and its children.
 *
 * \param *node		Pointer to the node to link.
 * \param *parent	Pointer to the node's parent.
 * \param *id_index	ID Index instance to store node IDs in.
 */

static void parse_link_node(struct manual_data *node, struct manual_data *parent, struct manual_ids *id_index)
{
	struct manual_data	*previous = NULL;
	int			index = 1;

	while (node != NULL) {
		node->previous = previous;
		node->parent = parent;

		/* Index the node ID. */

		if (node->id != NULL)
			manual_ids_add_node(id_index, node);

		/* Number the node, if appropriate. */

		switch (node->type) {
		case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
		case MANUAL_DATA_OBJECT_TYPE_SECTION:
			if (node->title != NULL)
				node->index = index++;
			break;
		/* Images, code blocks and so on might need to rely on
		 * global variables. Zero these at chapter level, and then
		 * let them count up as the tree beneath is processed.
		 */
		default:
			break;
		}

		/* Process any child nodes. */

		if (node->first_child != NULL)
			parse_link_node(node->first_child, node, id_index);

		/* Move on to the next sibling. */

		previous = node;
		node = node->next;
	}
}

