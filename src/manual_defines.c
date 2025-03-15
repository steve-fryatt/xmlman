/* Copyright 2025, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file manual_defines.c
 *
 * Manual Defines Indexing, implementation.
 *
 * Defines are constants that can be defined using the --define
 * parameter on the command line, then referenced in the manual.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xmlman.h"
#include "manual_data.h"
#include "manual_defines.h"
#include "msg.h"

/**
 * The size of the manual defines hash table.
 */

#define MANUAL_DEFINES_HASH_SIZE 100

/**
 * An entry in the define tag index.
 */

struct manual_defines_entry {
	/**
	 * The define's name.
	 */

	char				*name;

	/**
	 * The text associated with the define.
	 * This is currently just a UTF-8 string.
	 */

	char				*value;

	/**
	 * Pointer to the next define entry, or NULL.
	 */

	struct manual_defines_entry	*next;
};

/**
 * The define hash table.
 */

struct manual_defines_entry *manual_defines_table[MANUAL_DEFINES_HASH_SIZE];

/* Static Function Prototypes. */

static struct manual_defines_entry *manual_defines_find_name(char *name);
static int manual_defines_get_hash(char *name);

/**
 * Initialise the manual defines index.
 */

void manual_defines_initialise(void)
{
	int i;

	/* Zero the hash table entries. */

	for (i = 0; i < MANUAL_DEFINES_HASH_SIZE; i++)
		manual_defines_table[i] = NULL;
}

/**
 * Dump a manual defines index instance to the log.
 */

void manual_defines_dump(void)
{
	struct manual_defines_entry	*entry;
	int				i;

	msg_report(MSG_DEFINE_HASH_DUMP);

	for (i = 0; i < MANUAL_DEFINES_HASH_SIZE; i++) {
		entry = manual_defines_table[i];

		msg_report(MSG_DEFINE_HASH_LINE, i, entry);

		while (entry != NULL) {
			msg_report(MSG_DEFINE_HASH_ENTRY, entry->name);
			entry = entry->next;
		}
	}
}

/**
 * Add an entry to an index of defines.
 *
 * \param *entry	The entry to add to the index.
 * \return		True if successful; False on error.
 */

bool manual_defines_add_entry(char *entry)
{
	char				*name, *value;
	int				hash;
	struct manual_defines_entry	*new = NULL;

	if (entry == NULL)
		return false;

	printf("Adding define '%s'\n", entry);

	/* Copy the entry, and set the name pointer. */

	name = strdup(entry);
	if (name == NULL)
		return false;

	/* Scan through the string for an equals sign. If not found, or
	 * found at the start of the line, the content is invalid.
	 */

	for (value = name; *value != '=' && *value != '\0'; value++);

	if (*value == '\0' || value == name) {
		free(name);
		return false;
	}

	/* Split the string at the equals sign.*/

	*value++ = '\0';

	printf("Found name='%s', value='%s'\n", name, value);

	/* Check that the ID isn't in the table already. */

	if (manual_defines_find_name(name) != NULL) {
		msg_report(MSG_DEFINE_BAD_STORE, name);
		free(name);
		return false;
	}

	/* Create the new record and link it into the table. */

	new = malloc(sizeof(struct manual_defines_entry));
	if (new == NULL) {
		free(name);
		return false;
	}

	hash = manual_defines_get_hash(name);

	new->name = name;
	new->value = value;

	new->next = manual_defines_table[hash];
	manual_defines_table[hash] = new;

	return true;
}

/**
 * Given a name, return a pointer to the defined value that it refers to.
 *
 * \param *name		The name to look up.
 * \return		The define value, or NULL.
 */

char *manual_defines_find_value(char *name)
{
	struct manual_defines_entry *entry;

	if (name == NULL)
		return NULL;

	entry = manual_defines_find_name(name);
	if (entry == NULL) {
		msg_report(MSG_DEFINE_BAD_LOOKUP, name);
		return NULL;
	}

	return entry->value;
}

/**
 * Given a name, find a matching record in the index.
 *
 * \param *name		The name to search for.
 * \return		Pointer to the matching entry, or NULL.
 */

static struct manual_defines_entry *manual_defines_find_name(char *name)
{
	struct manual_defines_entry *entry = NULL;
	int hash;

	if (name == NULL)
		return NULL;

	hash = manual_defines_get_hash(name);
	entry = manual_defines_table[hash];

	while (entry != NULL) {
		if (entry->name != NULL && strcmp(name, entry->name) == 0)
			return entry;

		entry = entry->next;
	}

	return NULL;
}

/**
 * Calculate a hashing value for a define.
 *
 * \param *name		Pointer to the name to hash.
 * \return		The calculated hash value.
 */

static int manual_defines_get_hash(char *name)
{
	int hash = 0;

	if (name == NULL)
		return 0;

	while (*name != '\0')
		hash += *name++;

	return (hash % MANUAL_DEFINES_HASH_SIZE);
}

