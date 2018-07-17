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
 * \file manual_data.c
 *
 * Manual Data Structures, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "manual_data.h"

/**
 * Create a new manual_data structure.
 *
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data *manual_data_create(void)
{
	struct manual_data		*manual;

	manual = malloc(sizeof(struct manual_data));
	if (manual == NULL)
		return NULL;

	manual->title = NULL;
	manual->first_chapter = NULL;

	return manual;
}

/**
 * Create a new manual_data_chapter structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data_chapter *manual_data_chapter_create(enum manual_data_object_type type)
{
	struct manual_data_chapter	*chapter;

	if (type != MANUAL_DATA_OBJECT_TYPE_INDEX && type != MANUAL_DATA_OBJECT_TYPE_CHAPTER)
		return NULL;

	chapter = malloc(sizeof(struct manual_data_chapter));
	if (chapter == NULL)
		return NULL;

	chapter->type = type;
	chapter->title = NULL;
	chapter->id = NULL;
	chapter->filename = NULL;
	chapter->processed = false;
	chapter->next_chapter = NULL;
	chapter->first_section = NULL;

	return chapter;
}

/**
 * Create a new manual_data_section structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data_section *manual_data_section_create(enum manual_data_object_type type)
{
	struct manual_data_section	*section;

	if (type != MANUAL_DATA_OBJECT_TYPE_SECTION)
		return NULL;

	section = malloc(sizeof(struct manual_data_section));
	if (section == NULL)
		return NULL;

	section->type = type;
	section->title = NULL;
	section->id = NULL;
	section->next_section = NULL;

	return section;
}

/**
 * Create a new manual_data_block structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data_block *manual_data_block_create(enum manual_data_object_type type)
{
	struct manual_data_block	*block;

	if (type != MANUAL_DATA_OBJECT_TYPE_TEXT)
		return NULL;

	block = malloc(sizeof(struct manual_data_block));
	if (block == NULL)
		return NULL;

	block->type = type;
	block->next_block = NULL;

	switch (type) {
	case MANUAL_DATA_OBJECT_TYPE_TEXT:
		block->text.first_chunk = NULL;
		break;
	}

	return block;
}

/**
 * Create a new manual_data_chunk structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data_chunk *manual_data_chunk_create(enum manual_data_object_type type)
{
	struct manual_data_chunk	*chunk;

	if (type != MANUAL_DATA_OBJECT_TYPE_TEXT && type != MANUAL_DATA_OBJECT_TYPE_ENTITY)
		return NULL;

	chunk = malloc(sizeof(struct manual_data_chunk));
	if (chunk == NULL)
		return NULL;

	chunk->type = type;
	chunk->next_chunk = NULL;

	switch (type) {
	case MANUAL_DATA_OBJECT_TYPE_TEXT:
		chunk->text = NULL;
		break;
	case MANUAL_DATA_OBJECT_TYPE_ENTITY:
		chunk->entity = MANUAL_ENTITY_NONE;
		break;
	}

	return chunk;
}

