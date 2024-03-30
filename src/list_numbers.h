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
 * \file list_numbers.h
 *
 * List Numbering Interface.
 */

#ifndef XMLMAN_LIST_NUMBERS_H
#define XMLMAN_LIST_NUMBERS_H

#include <stdbool.h>

/**
 * A list numbers instance.
 */

struct list_numbers;

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

struct list_numbers *list_numbers_create_unordered(char *bullets[], int level);

/**
 * Create a new ordered list instance at a specific level.
 * 
 * \param length	The length of the list, in terms of the number of
 *			entries.
 * \param level		The level of the list, for pointer selection.
 * \return		The new list instance, or NULL on failure.
 */

struct list_numbers *list_numbers_create_ordered(int length, int level);

/**
 * Destroy a list numbers instance.
 * 
 * \param *instance	The instance to be destroyed.
 */

void list_numbers_destroy(struct list_numbers *instance);

/**
 * Find the maximum length (in visible characters) of the numbers or
 * bullets used in a list.
 *
 * \param *instance	The instance to be queried.
 * \return		The length required for the entries.
 */

int list_numbers_get_max_length(struct list_numbers *instance);

/**
 * Return a pointer to a buffer containing the next entry in a
 * list of numbers or bullets. The text is in UTF-8 format.
 *
 * \param *instance	The instance to be queried.
 * \return		A pointer to the text buffer, or NULL.
 */

char *list_numbers_get_next_entry(struct list_numbers *instance);

#endif
