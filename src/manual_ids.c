/* Copyright 2018-2024, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file manual_ids.c
 *
 * Manual ID Indexing, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "manual_data.h"
#include "manual_ids.h"
#include "msg.h"

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

/**
 * The default instance.
 */

static struct manual_ids *manual_ids_default_instance = NULL;

/* Static Function Prototypes. */

static struct manual_ids_entry *manual_ids_find_id(char *id);
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

	/* Set the default instance. */

	if (manual_ids_default_instance == NULL)
		manual_ids_default_instance = new;

	return new;
}

/**
 * Dump a manual IDs index instance to the log.
 */

void manual_ids_dump(void)
{
	struct manual_ids_entry	*entry;
	int			i;

	msg_report(MSG_ID_HASH_DUMP, manual_ids_default_instance);

	if (manual_ids_default_instance == NULL)
		return;

	for (i = 0; i < MANUAL_IDS_HASH_SIZE; i++) {
		entry = manual_ids_default_instance->table[i];

		msg_report(MSG_ID_HASH_LINE, i, entry);

		while (entry != NULL) {
			msg_report(MSG_ID_HASH_ENTRY, entry->id);
			entry = entry->next;
		}
	}
}

/**
 * Add a node to an index of IDs.
 *
 * \param *node		The node to add to the index.
 * \return		True if successful; False on error.
 */

bool manual_ids_add_node(struct manual_data *node)
{
	struct manual_ids_entry	*new;
	int			hash;

	if (manual_ids_default_instance == NULL || node == NULL || node->chapter.id == NULL)
		return false;

	/* Only non-chunk types can have IDs. */

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_CHAPTER:
	case MANUAL_DATA_OBJECT_TYPE_INDEX:
	case MANUAL_DATA_OBJECT_TYPE_SECTION:
		break;
	default:
		msg_report(MSG_ID_BAD_TARGET, manual_data_find_object_name(node->type));
		return false;
	}

	/* Check that the ID isn't in the table already. */

	if (manual_ids_find_id(node->chapter.id) != NULL) {
		msg_report(MSG_ID_BAD_STORE, node->chapter.id);
		return false;
	}

	/* Create the new record and link it into the table. */

	new = malloc(sizeof(struct manual_ids_entry));
	if (new == NULL)
		return false;

	hash = manual_ids_get_hash(node->chapter.id);

	new->id = node->chapter.id;
	new->node = node;

	new->next = manual_ids_default_instance->table[hash];
	manual_ids_default_instance->table[hash] = new;

	return true;
}

/**
 * Given a reference node, find the node that it refers to.
 *
 * \param *node		The reference node to start from.
 * \return		The target node, or NULL.
 */

struct manual_data *manual_ids_find_node(struct manual_data *node)
{
	struct manual_data *target = NULL;
	struct manual_ids_entry *entry;

	if (node == NULL || node->chunk.id == NULL)
		return NULL;

	/* Only chunk types can have IDs. */

	switch (node->type) {
	case MANUAL_DATA_OBJECT_TYPE_REFERENCE:
		break;
	default:
		msg_report(MSG_ID_BAD_REFERENCE, manual_data_find_object_name(node->type));
		return false;
	}

	entry = manual_ids_find_id(node->chunk.id);
	if (entry == NULL) {
		msg_report(MSG_ID_BAD_LOOKUP, node->chunk.id);
		return NULL;
	}

	return entry->node;
}

/**
 * Given and ID, find a matching record in the index.
 *
 * \param *id		The ID to search for.
 * \return		Pointer to the matching entry, or NULL.
 */

static struct manual_ids_entry *manual_ids_find_id(char *id)
{
	struct manual_ids_entry *entry = NULL;
	int hash;

	if (manual_ids_default_instance == NULL || id == NULL)
		return NULL;

	hash = manual_ids_get_hash(id);
	entry = manual_ids_default_instance->table[hash];

	while (entry != NULL) {
		if (entry->id != NULL && strcmp(id, entry->id) == 0)
			return entry;

		entry = entry->next;
	}

	return NULL;
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

