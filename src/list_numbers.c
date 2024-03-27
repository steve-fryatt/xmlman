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
 * \file list_numbers.c
 *
 * List Numbering, implementation.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"

/**
 * The size of a list number buffer, which should be sufficient
 * to hold all of the possible values expressed in UTF-8.
 */

#define LIST_NUMBERS_BUFFER_LEN 20

/**
 * The types of list.
 */

enum list_numbers_type {
	LIST_NUMBERS_TYPE_UNORDERED
};

/**
 * A list instance implementation.
 */

struct list_numbers {
	/**
	 * The type of list.
	 */
	enum list_numbers_type	type;

	/**
	 * The maximum length of a number or bullet in the instance,
	 * in visible characters (not UTF-8 bytes).
	 */
	int		max_length;

	/**
	 * The most recent value to have been written to the list.
	 */
	int		current_value;

	/**
	 * Buffer to hold the next number or bullet to be output.
	 * 
	 * The text is held in UTF-8 format.
	 */
	char		buffer[LIST_NUMBERS_BUFFER_LEN];
};

/**
 * Create a new unordered list instance at a specific level.
 *
 * The list of bullet texts is an array of pointers to UTF-8 strings, terminated
 * by a NULL entry. A symbol will be chosed from this in sequental order,
 * wrapping around when the list is completed.
 * 
 * \param *bullets[]	Pointer to an array of bullet texts, terminated by
 *			a NULL pointer.
 * \param level		The level of the list, for pointer selection.
 * \return		The new list instance, or NULL on failure.
 */

struct list_numbers *list_numbers_create_unordered(char *bullets[], int level)
{
	struct list_numbers *new = NULL;
	int bullet_count = 0;

	/* Identify the bullet that we want to use. */

	if (bullets == NULL)
		return NULL;

	while (bullets[bullet_count] != NULL) {
		bullet_count++;
	}

	if (bullet_count == 0)
		return NULL;

	/* Allocate and initialise the instance block. */

	new = malloc(sizeof(struct list_numbers));
	if (new == NULL)
		return NULL;

	new->type = LIST_NUMBERS_TYPE_UNORDERED;
	new->current_value = 0;

	/* Copy the selected bullet into the buffer, and find its length. */

	strncpy(new->buffer, bullets[level % bullet_count], LIST_NUMBERS_BUFFER_LEN);
	new->buffer[LIST_NUMBERS_BUFFER_LEN - 1] = '\0';

	new->max_length = encoding_get_utf8_string_length(new->buffer);

	return new;
}

/**
 * Destroy a list numbers instance.
 * 
 * \param *instance	The instance to be destroyed.
 */

void list_numbers_destroy(struct list_numbers *instance)
{
	if (instance == NULL)
		return;

	free(instance);
}

/**
 * Find the maximum length (in visible characters) of the numbers or
 * bullets used in a list.
 *
 * \param *instance	The instance to be queried.
 * \return		The length required for the entries.
 */

int list_numbers_get_max_length(struct list_numbers *instance)
{
	if (instance == NULL)
		return 0;

	return instance->max_length;
}

/**
 * Return a pointer to a buffer containing the next entry in a
 * list of numbers or bullets. The text is in UTF-8 format.
 *
 * \param *instance	The instance to be queried.
 * \return		A pointer to the text buffer, or NULL.
 */

char *list_numbers_get_next_entry(struct list_numbers *instance)
{
	if (instance == NULL)
		return NULL;

	switch (instance->type) {
	case LIST_NUMBERS_TYPE_UNORDERED:
		break;
	}

	return instance->buffer;
}

