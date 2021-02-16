/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/software/
 *
 * Licensed under the EUPL, Version 1.1 only (the "Licence");
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
 * \file manual_ids.c
 *
 * Manual ID Indexing, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "manual_data.h"
#include "manual_ids.h"

/**
 * The size of the manual IDs hash table.
 */

#define MANUAL_IDS_HASH_SIZE 40

/**
 * An entry in the ID tag index.
 */

struct manual_ids_entry {
	/**
	 * The ID tag for the node. This will just be set to point to the
	 * data held by the node itself.
	 */

	char			*id;

	/**
	 * The node relating to the tag.
	 */

	struct manual_data	*node;

	/**
	 * Pointer to the next ID entry, or NULL.
	 */

	struct manual_ids_entry	*next;
};

/**
 * An ID Index instance structure.
 */

struct manual_ids {
	/**
	 * The hash table.
	 */

	struct manual_ids_entry	*table[MANUAL_IDS_HASH_SIZE];
};

/* Static Function Prototypes. */

static int manual_ids_get_hash(char *id);

/**
 * Create a new manual IDs index instance.
 *
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct manual_ids *manual_ids_create(void)
{
	struct manual_ids	*new = NULL;
	int			i;

	/* Allocate memory for the structure. */

	new = malloc(sizeof(struct manual_ids));
	if (new == NULL)
		return NULL;

	/* Zero the hash table entries. */

	for (i = 0; i < MANUAL_IDS_HASH_SIZE; i++)
		new->table[i] = NULL;

	return new;
}

void manual_ids_dump(struct manual_ids *instance)
{
	struct manual_ids_entry	*entry;
	int			i;

	printf("Dumping index table 0x%x\n", instance);

	if (instance == NULL)
		return;

	for (i = 0; i < MANUAL_IDS_HASH_SIZE; i++) {
		entry = instance->table[i];

		printf("Hash entry %d = 0x%x\n", i, entry);

		while (entry != NULL) {
			printf("Entry for '%s'\n", entry->id);
			entry = entry->next;
		}
	}
}

/**
 * Add a node to an index of IDs.
 *
 * \param *instance	The ID index instance to add the node to.
 * \param *node		The node to add to the index.
 * \return		True if successful; False on error.
 */

bool manual_ids_add_node(struct manual_ids *instance, struct manual_data *node)
{
	struct manual_ids_entry	*new;
	int			hash;

	if (instance == NULL)
		return false;

	if (node == NULL || node->id == NULL)
		return false;

	new = malloc(sizeof(struct manual_ids_entry));
	if (new == NULL)
		return false;

	hash = manual_ids_get_hash(node->id);

	new->id = node->id;
	new->node = node;

	new->next = instance->table[hash];
	instance->table[hash] = new;

	return true;
}

/**
 * Calculate a hashing value for an ID.
 *
 * \param *id		Pointer to the ID to hash.
 * \return		The calculated hash value.
 */

static int manual_ids_get_hash(char *id)
{
	int hash = 0;

	if (id == NULL)
		return 0;

	while (*id != '\0')
		hash += *id++;

	return (hash % MANUAL_IDS_HASH_SIZE);
}

