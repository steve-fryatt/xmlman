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


	MANUAL_DATA_OBJECT_TYPE_TEXT,
	MANUAL_DATA_OBJECT_TYPE_ENTITY,
};


/**
 * Data for a manual block chunk.
 */

struct manual_data_chunk {
	/**
	 * The chunk's content type.
	 */

	enum manual_data_object_type	type;

	/**
	 * The chunk's contents.
	 */

	union {
		xmlChar			*text;		/**< Pointer to the chunk text.	*/
		enum manual_entity_type	entity;		/**< The chunk entity type.	*/
	};

	/**
	 * Pointer to the next chunk in the data block.
	 */

	struct manual_data_chunk	*next_chunk;
};

/**
 * Data for a text-type manual block.
 */

struct manual_data_block_text {
	struct manual_data_chunk	*first_chunk;
};

/**
 * Data for a manual block.
 */

struct manual_data_block {
	/**
	 * The object's type (Section or otherwise).
	 */

	enum manual_data_object_type	type;

	/**
	 * The object's contents.
	 */

	union {
		struct manual_data_block_text	text;
	};

	/**
	 * Pointer to the next block.
	 */

	struct manual_data_block	*next_block;
};

/**
 * Data for a manual page section.
 */

struct manual_data_section {
	/**
	 * The object's type (Section or otherwise).
	 */

	enum manual_data_object_type	type;

	/**
	 * Pointer to the section's ID, or NULL if one has not been set.
	 */

	xmlChar				*id;

	/**
	 * Pointer to the next section of the chapter.
	 */

	struct manual_data_section	*next_section;

	/**
	 * Pointer to the section title.
	 */

	xmlChar				*title;
};

/**
 * Data for a manual chapter or index page.
 */

struct manual_data_chapter {
	/**
	 * The object's type (Index or Chapter).
	 */

	enum manual_data_object_type	type;

	/**
	 * Has the chapter been processed, or is this just a placeholder?
	 */

	bool				processed;

	/**
	 * Pointer to the chapter filename, or NULL if this is an inline
	 * chapter.
	 */

	xmlChar				*filename;

	/**
	 * Pointer to the chapter's ID, or NULL if one has not been set.
	 */

	xmlChar				*id;

	/**
	 * Pointer to the next chapter of the manual.
	 */

	struct manual_data_chapter	*next_chapter;

	/**
	 * Pointer to the chapter title.
	 */

	xmlChar				*title;

	/**
	 * Pointer to the first section of the chapter.
	 */

	struct manual_data_section	*first_section;
};

/**
 * Top-Level data for a manual.
 */

struct manual_data {
	/**
	 * Pointer to the manual title.
	 */

	xmlChar				*title;

	/**
	 * Pointer to the first chapter of the manual.
	 */

	struct manual_data_chapter	*first_chapter;
};

/**
 * Create a new manual_data structure.
 *
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data *manual_data_create(void);

/**
 * Create a new manual_data_chapter structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data_chapter *manual_data_chapter_create(enum manual_data_object_type type);

/**
 * Create a new manual_data_section structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data_section *manual_data_section_create(enum manual_data_object_type type);

#endif

