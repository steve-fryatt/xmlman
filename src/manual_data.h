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
 * \file manual_data.h
 *
 * Manual Data Structures Interface.
 */

#ifndef XMLMAN_MANUAL_DATA_H
#define XMLMAN_MANUAL_DATA_H

#include <libxml/xmlstring.h>

#include "manual_entity.h"

enum manual_data_object_type {
	MANUAL_DATA_OBJECT_TYPE_NONE,
	MANUAL_DATA_OBJECT_TYPE_MANUAL,
	MANUAL_DATA_OBJECT_TYPE_INDEX,
	MANUAL_DATA_OBJECT_TYPE_CHAPTER,
	MANUAL_DATA_OBJECT_TYPE_SECTION,
	MANUAL_DATA_OBJECT_TYPE_TITLE,
	MANUAL_DATA_OBJECT_TYPE_PARAGRAPH,
	MANUAL_DATA_OBJECT_TYPE_CITATION,
	MANUAL_DATA_OBJECT_TYPE_CODE,
	MANUAL_DATA_OBJECT_TYPE_USER_ENTRY,
	MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS,
	MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS,
	MANUAL_DATA_OBJECT_TYPE_FILENAME,
	MANUAL_DATA_OBJECT_TYPE_ICON,
	MANUAL_DATA_OBJECT_TYPE_KEY,
	MANUAL_DATA_OBJECT_TYPE_MOUSE,
	MANUAL_DATA_OBJECT_TYPE_WINDOW,


	MANUAL_DATA_OBJECT_TYPE_TEXT,
	MANUAL_DATA_OBJECT_TYPE_ENTITY,
};


/**
 * Data for a manual block chunk.
 */

struct manual_data_chunk {
	union {
	/**
	 * Pointer to the chunk text.
	 */

		xmlChar				*text;

	/**
	 * The chunk entity type.
	 */

		enum manual_entity_type		entity;
	};
};

/**
 * Data for a manual chapter or index page.
 */

struct manual_data_chapter {
	/**
	 * Has the chapter been processed, or is this just a placeholder?
	 */

	bool					processed;

	/**
	 * Pointer to the chapter filename, or NULL if this is an inline
	 * chapter.
	 */

	xmlChar					*filename;
};

/**
 * Top-Level data for a manual.
 */

struct manual_data {
	/**
	 * The object's content type).
	 */

	enum manual_data_object_type		type;

	/**
	 * Pointer to the object's ID, or NULL if none has been set.
	 */

	xmlChar					*id;

	/**
	 * Pointer to the object's title, or NULL if none has been set.
	 */

	struct manual_data			*title;

	/**
	 * Pointer to the object's first child, or NULL if none.
	 */

	struct manual_data			*first_child;

	/**
	 * Pointer to the next object, or NULL if none.
	 */

	struct manual_data			*next;

	/**
	 * Additional data for specific object types.
	 */

	union {
		struct manual_data_chapter	chapter;
		struct manual_data_chunk	chunk;
	};

};

/**
 * Create a new manual_data structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data *manual_data_create(enum manual_data_object_type type);

/**
 * Given an object type, return the textual object type name.
 *
 * \param type		The object type to look up.
 * \return		Pointer to the object's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_data_find_object_name(enum manual_data_object_type type);

#endif

