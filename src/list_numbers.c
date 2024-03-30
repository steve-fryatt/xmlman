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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"
#include "msg.h"

/**
 * The size of a list number buffer, which should be sufficient
 * to hold all of the possible values expressed in UTF-8.
 */

#define LIST_NUMBERS_BUFFER_LEN 20

/**
 * The maximum list entry number that we support.
 *
 * This is mainly dictated by the ability of Roman Numerals to work
 * up to only 3999 without any extra tricks being employed.
 * For convenience, all styles are limited to the same range.
 */

#define LIST_NUMBERS_MAX_VALUE 3999

/**
 * The types of list.
 */

enum list_numbers_type {
	LIST_NUMBERS_TYPE_UNORDERED,
	LIST_NUMBERS_TYPE_NUMERIC,
	LIST_NUMBERS_TYPE_LOWER,
	LIST_NUMBERS_TYPE_UPPER,
	LIST_NUMBERS_TYPE_ROMAN_LOWER,
	LIST_NUMBERS_TYPE_ROMAN_UPPER
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

/* Static Global Variables. */

/**
 * The list lengths at which numeric numbers get one character longer.
 */
static int list_numbers_numeric_length_points[] = { 1, 10, 100, 1000, -1 };

/**
 * The list lengths at which alphabetic numbers get one character longer.
 */
static int list_numbers_alphabetic_length_points[] = { 1, 27, 703, -1 };

/**
 * The list lengths at which Roman numbers get one character longer.
 */
static int list_numbers_roman_length_points[] = { 1, 2, 3, 8, 18, 28, 38, 88, 188, 288, 388, 888, 1888, 2888, 3888, -1 };

/**
 * The break points for calculating Roman numerals.
 */
static int list_numbers_roman_break_points[] = { 1, 4, 5, 9, 10, 40, 50, 90, 100, 400, 500, 900, 1000 };

/**
 * The upper case Roman numeral components.
 */
static char *list_numbers_roman_upper_symbols[] = { "I", "IV", "V", "IX", "X", "XL", "L", "XC", "C", "CD", "D", "CM", "M" };

/**
 * The lower case Roman numeral components.
 */
static char *list_numbers_roman_lower_symbols[] = { "i", "iv", "v", "ix", "x", "xl", "l", "xc", "c", "cd", "d", "cm", "m" };


/* Static Function Prototypes. */

static char *list_numbers_build_numeric(struct list_numbers *instance);
static char *list_numbers_build_alphabetic(struct list_numbers *instance, bool upper_case);
static char *list_numbers_build_roman(struct list_numbers *instance, bool upper_case);

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
 * Create a new ordered list instance at a specific level.
 * 
 * \param length	The length of the list, in terms of the number of
 *			entries.
 * \param level		The level of the list, for pointer selection.
 * \return		The new list instance, or NULL on failure.
 */

struct list_numbers *list_numbers_create_ordered(int length, int level)
{
	struct list_numbers *new = NULL;
	enum list_numbers_type bullets[] = {
			LIST_NUMBERS_TYPE_NUMERIC,
			LIST_NUMBERS_TYPE_LOWER,
			LIST_NUMBERS_TYPE_ROMAN_LOWER,
			LIST_NUMBERS_TYPE_UPPER,
			LIST_NUMBERS_TYPE_ROMAN_UPPER
	};
	int bullet_count = 5;
	int i = 0, *break_points;

	/* Check the proposed length of the list. */

	if (length > LIST_NUMBERS_MAX_VALUE) {
		msg_report(MSG_LIST_TOO_LONG);
		return NULL;
	}

	/* Allocate and initialise the instance block. */

	new = malloc(sizeof(struct list_numbers));
	if (new == NULL)
		return NULL;

	new->current_value = 0;

	/* Initialise the buffer. */

	new->buffer[0] = '\0';

	/* Find the length of the longest entry.
	 *
	 * The break points are the list lengths (number of items that they
	 * contain) at which the length increases by 1 from a base of zero.
	 * So at the first value, the length is 1 character, at the second
	 * it steps to 2, and so on.
	 */

	new->type = bullets[level % bullet_count];

	switch (new->type) {
	case LIST_NUMBERS_TYPE_NUMERIC:
		break_points = list_numbers_numeric_length_points;
		break;
	case LIST_NUMBERS_TYPE_LOWER:
	case LIST_NUMBERS_TYPE_UPPER:
		break_points = list_numbers_alphabetic_length_points;
		break;
	case LIST_NUMBERS_TYPE_ROMAN_LOWER:
	case LIST_NUMBERS_TYPE_ROMAN_UPPER:
		break_points = list_numbers_roman_length_points;
		break;
	default:
		free(new);
		return NULL;
	}

	while (length >= break_points[i] && break_points[i] >= 0)
		i++;

	new->max_length = i + 1; /* Add one extra for a . terminator. */

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

	/* Get the next number in the sequence. */

	if (++instance->current_value > LIST_NUMBERS_MAX_VALUE) {
		msg_report(MSG_LIST_TOO_LONG);
		return instance->buffer;
	}

	/* Build and return the next number. */

	switch (instance->type) {
	case LIST_NUMBERS_TYPE_NUMERIC:
		return list_numbers_build_numeric(instance);

	case LIST_NUMBERS_TYPE_LOWER:
	case LIST_NUMBERS_TYPE_UPPER:
		return list_numbers_build_alphabetic(instance, (instance->type == LIST_NUMBERS_TYPE_UPPER) ? true : false);

	case LIST_NUMBERS_TYPE_ROMAN_LOWER:
	case LIST_NUMBERS_TYPE_ROMAN_UPPER:
		return list_numbers_build_roman(instance, (instance->type == LIST_NUMBERS_TYPE_ROMAN_UPPER) ? true : false);

	case LIST_NUMBERS_TYPE_UNORDERED:
		break;

	default:
		instance->buffer[0] = '\0';
		break;
	}

	return instance->buffer;
}

/**
 * Build a numeric list number, and return a pointer to it.
 *
 * \param *instance	Pointer to the instance to use.
 * \return		Pointer to the numeric number.
 */

static char *list_numbers_build_numeric(struct list_numbers *instance)
{
	snprintf(instance->buffer, LIST_NUMBERS_BUFFER_LEN, "%d.", instance->current_value);
	instance->buffer[LIST_NUMBERS_BUFFER_LEN - 1] = '\0';

	return instance->buffer;
}

/**
 * Build a alphabetic list number, and return a pointer to it.
 *
 * \param *instance	Pointer to the instance to use.
 * \param upper_case	True if the number should be in upper case;
 *			else false for lower case.
 * \return		Pointer to the alphabetic number.
 */

static char *list_numbers_build_alphabetic(struct list_numbers *instance, bool upper_case)
{
	int i = LIST_NUMBERS_BUFFER_LEN, value;
	char base;

	value = instance->current_value;
	base = (upper_case == true) ? 'A' : 'a';

	instance->buffer[--i] = '\0';
	instance->buffer[--i] = '.';

	if (i <= 0 || value <= 0)
		return instance->buffer;

	do {
		value--;
		instance->buffer[--i] = base + (value % 26);
		value = value / 26;
	} while (value > 0 && i > 0);

	return instance->buffer + i;
}

/**
 * Build a Roman list number, and return a pointer to it.
 *
 * \param *instance	Pointer to the instance to use.
 * \param upper_case	True if the number should be in upper case;
 *			else false for lower case.
 * \return		Pointer to the Roman number.
 */

static char *list_numbers_build_roman(struct list_numbers *instance, bool upper_case)
{
	int i = 12, value, div;
	char **symbols;

	value = instance->current_value;
	symbols = (upper_case == true) ? list_numbers_roman_upper_symbols : list_numbers_roman_lower_symbols;

	instance->buffer[0] = '\0';

	while (value > 0 && i >= 0) {
		div = value / list_numbers_roman_break_points[i];
		value %= list_numbers_roman_break_points[i];

		// "-4" to leave space in buffer for trailing . and \0 terminator after adding 2 chars.

		while (div-- > 0 && strlen(instance->buffer) < (LIST_NUMBERS_BUFFER_LEN - 4))
			strncat(instance->buffer, symbols[i], 2);

		i--;
	}
	
	strncat(instance->buffer, ".", 1);

	instance->buffer[LIST_NUMBERS_BUFFER_LEN - 1] = '\0';

	return instance->buffer;
}