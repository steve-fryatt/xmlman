/* Copyright 2018-2025, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file xmlman.h
 *
 * Shared Manual Data Structures
 *
 * To squash circular compilation errors, common shared type definitions
 * are held centrally in this file.
 */

#ifndef XMLMAN_H
#define XMLMAN_H

#include "manual_entity.h"
#include "filename.h"

/**
 * The possible mode types.
 */

enum modes_type {
	MODES_TYPE_NONE,
	MODES_TYPE_DEBUG,
	MODES_TYPE_TEXT,
	MODES_TYPE_STRONGHELP,
	MODES_TYPE_HTML,
};

/**
 * The possible node object types.
 */

enum manual_data_object_type {
	MANUAL_DATA_OBJECT_TYPE_MANUAL,
	MANUAL_DATA_OBJECT_TYPE_INDEX,
	MANUAL_DATA_OBJECT_TYPE_CHAPTER,
	MANUAL_DATA_OBJECT_TYPE_SECTION,

	MANUAL_DATA_OBJECT_TYPE_CREDIT,
	MANUAL_DATA_OBJECT_TYPE_DATE,
	MANUAL_DATA_OBJECT_TYPE_TITLE,
	MANUAL_DATA_OBJECT_TYPE_STRAPLINE,
	MANUAL_DATA_OBJECT_TYPE_SUMMARY,
	MANUAL_DATA_OBJECT_TYPE_VERSION,

	MANUAL_DATA_OBJECT_TYPE_CONTENTS,
	MANUAL_DATA_OBJECT_TYPE_DEFINITION_LIST,
	MANUAL_DATA_OBJECT_TYPE_ORDERED_LIST,
	MANUAL_DATA_OBJECT_TYPE_UNORDERED_LIST,
	MANUAL_DATA_OBJECT_TYPE_LIST_ITEM,
	MANUAL_DATA_OBJECT_TYPE_TABLE,
	MANUAL_DATA_OBJECT_TYPE_TABLE_ROW,
	MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN,
	MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_SET,
	MANUAL_DATA_OBJECT_TYPE_TABLE_COLUMN_DEFINITION,
	MANUAL_DATA_OBJECT_TYPE_CODE_BLOCK,
	MANUAL_DATA_OBJECT_TYPE_CALLOUT,
	MANUAL_DATA_OBJECT_TYPE_BLOCKQUOTE,
	MANUAL_DATA_OBJECT_TYPE_FOOTNOTE,
	MANUAL_DATA_OBJECT_TYPE_PARAGRAPH,

	MANUAL_DATA_OBJECT_TYPE_CITATION,
	MANUAL_DATA_OBJECT_TYPE_CODE,
	MANUAL_DATA_OBJECT_TYPE_COMMAND,
	MANUAL_DATA_OBJECT_TYPE_CONSTANT,
	MANUAL_DATA_OBJECT_TYPE_DEFINED_TEXT,
	MANUAL_DATA_OBJECT_TYPE_EVENT,
	MANUAL_DATA_OBJECT_TYPE_FILENAME,
	MANUAL_DATA_OBJECT_TYPE_FUNCTION,
	MANUAL_DATA_OBJECT_TYPE_ICON,
	MANUAL_DATA_OBJECT_TYPE_INTRO,
	MANUAL_DATA_OBJECT_TYPE_KEY,
	MANUAL_DATA_OBJECT_TYPE_KEYWORD,
	MANUAL_DATA_OBJECT_TYPE_LIGHT_EMPHASIS,
	MANUAL_DATA_OBJECT_TYPE_LINK,
	MANUAL_DATA_OBJECT_TYPE_MATHS,
	MANUAL_DATA_OBJECT_TYPE_MENU,
	MANUAL_DATA_OBJECT_TYPE_MESSAGE,
	MANUAL_DATA_OBJECT_TYPE_MOUSE,
	MANUAL_DATA_OBJECT_TYPE_NAME,
	MANUAL_DATA_OBJECT_TYPE_REFERENCE,
	MANUAL_DATA_OBJECT_TYPE_STRONG_EMPHASIS,
	MANUAL_DATA_OBJECT_TYPE_SWI,
	MANUAL_DATA_OBJECT_TYPE_TYPE,
	MANUAL_DATA_OBJECT_TYPE_USER_ENTRY,
	MANUAL_DATA_OBJECT_TYPE_VARIABLE,
	MANUAL_DATA_OBJECT_TYPE_WINDOW,

	MANUAL_DATA_OBJECT_TYPE_LINE_BREAK,
	MANUAL_DATA_OBJECT_TYPE_TEXT,
	MANUAL_DATA_OBJECT_TYPE_ENTITY,

	MANUAL_DATA_OBJECT_TYPE_RESOURCE_FILE,
	MANUAL_DATA_OBJECT_TYPE_RESOURCE_FOLDER,
	MANUAL_DATA_OBJECT_TYPE_RESOURCE_IMAGE,
	MANUAL_DATA_OBJECT_TYPE_RESOURCE_DOWNLOAD,

	MANUAL_DATA_OBJECT_TYPE_MULTI_LEVEL_ATTRIBUTE,
	MANUAL_DATA_OBJECT_TYPE_SINGLE_LEVEL_ATTRIBUTE,

	MANUAL_DATA_OBJECT_TYPE_NONE
};

/**
 * Flags relating to objects.
 */

enum manual_data_object_flags {
	MANUAL_DATA_OBJECT_FLAGS_NONE = 0,

	/* Flags relating to Links */

	MANUAL_DATA_OBJECT_FLAGS_LINK_EXTERNAL = 1,
	MANUAL_DATA_OBJECT_FLAGS_LINK_FLATTEN = 2,

	/* Flags relating to Callouts */

	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE = 0xf, // Type mask
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_ATTENTION = 0,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_CAUTION = 1,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_DANGER = 2,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_ERROR = 3,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_HINT = 4,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_IMPORTANT = 5,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_NOTE = 6,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_SEEALSO = 7,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_TIP = 8,
	MANUAL_DATA_OBJECT_FLAGS_CALLOUT_TYPE_WARNING = 9,

	/* Flags relating to Lists */

	MANUAL_DATA_OBJECT_FLAGS_LIST_COMPACT = 1
};

/**
 * Resource data for a data output mode.
 */

struct manual_data_mode {
	/**
	 * Relative filename for the section output.
	 */
	struct filename				*filename;

	/**
	 * Relative folder name for the section output.
	 */
	struct filename				*folder;

	/**
	 * Relative filename for a stylesheet.
	 */
	struct filename				*stylesheet;
};

/**
 * Resource data for a manual, chapter or section node.
 */

struct manual_data_resources {
	/**
	 * Resources for the text output mode.
	 */
	struct manual_data_mode			text;

	/**
	 * Resources for the StrongHelp output mode.
	 */
	struct manual_data_mode			strong;

	/**
	 * Resources for the HTML output mode.
	 */
	struct manual_data_mode			html;

	/**
	 * The relative path to the image source folder.
	 */
	struct filename				*images;

	/**
	 * The relative path to the download source folder.
	 */
	struct filename				*downloads;

	/**
	 * The object summary text, or NULL.
	 */
	struct manual_data			*summary;

	/**
	 * The object strapline text, or NULL.
	 */
	struct manual_data			*strapline;

	/**
	 * The object credit text, or NULL.
	 */
	struct manual_data			*credit;

	/**
	 * The object version text, or NULL.
	 */
	struct manual_data			*version;

	/**
	 * The object date text, or NULL.
	 */
	struct manual_data			*date;
};

/**
 * Data for a manual block chunk.
 */

struct manual_data_chunk {
	/**
	 * Flags relating to the object.
	 */
	enum manual_data_object_flags flags;

	union {
		/**
		 * Pointer to the target definition's name, or NULL if none has been set.
		 *
		 * Used by DEFINED_TEXT objects
		 */
		char				*name;

		/**
		 * Pointer to the target object's ID, or NULL if none has been set.
		 *
		 * Used by REFERENCE objects
		 */
		char				*id;

		/**
		 * Pointer to the target link text, or NULL if none has been set.
		 *
		 * Used by LINK objects.
		 */
		struct manual_data		*link;

		/**
		 * The width of an object.
		 *
		 * Used by TABLE_COLUMN_DEFINITION objects, where zero or less
		 * means "don't care".
		 */
		int width;
	};

	union {
		/**
		 * Pointer to the chunk text.
		 */
		char				*text;

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
	 * Pointer to the object's ID, or NULL if none has been set.
	 */

	char					*id;

	/**
	 * Has the chapter been processed, or is this just a placeholder?
	 */

	bool					processed;


	union {
		/**
		 * Pointer to the chapter source filename, or NULL if this
		 * is an inline or processed chapter.
		 */
		struct filename			*filename;

		/**
		 * Pointer to the chapter's resources, or NULL if none
		 * are present.
		 */
		struct manual_data_resources	*resources;

		/**
		 * Pointer to the table's column definitions, or NULL if
		 * none have been set.
		 */
		struct manual_data		*columns;
	};
};

/**
 * Top-Level data for a manual node.
 */

struct manual_data {
	/**
	 * The object's content type).
	 */

	enum manual_data_object_type		type;

	/**
	 * The index number of the node, or zero.
	 */

	int					index;

	/**
	 * Pointer to the object's title, or NULL if none has been set.
	 */

	struct manual_data			*title;

	/**
	 * Pointer to the object's first child, or NULL if none.
	 */

	struct manual_data			*first_child;

	/**
	 * Pointer to the object's parent, or NULL if none.
	 */

	struct manual_data			*parent;

	/**
	 * Pointer to the previous object, or NULL if none.
	 */

	struct manual_data			*previous;

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

#endif
