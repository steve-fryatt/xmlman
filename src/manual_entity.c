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
 * \file manual_entity.c
 *
 * XML Manual Entity Decoding, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "manual_entity.h"

#include "msg.h"
#include "search_tree.h"

/**
 * An entity definition structure.
 */

struct manual_entity_definition {
	enum manual_entity_type	type;		/**< The type of entity.			*/
	const char		*name;		/**< The name of the entity	.		*/
	int			unicode;	/**< The unicode code point for the entity.	*/
};

/**
 * The lookup tree instance for the entities.
 */

static struct search_tree *manual_entity_search_tree = NULL;

/**
 * The number of entries in the entity list.
 */

static int manual_entities_max_entries = -1;

/**
 * The list of known element definitions.
 *
 * The order of this table is by ascending unicode point, with non-unicode
 * entities at the start using -1 and NONe at the end as an end stop. The
 * order of the entity texts are not important.
 * 
 * It *must* correspond to the order that the enum values are
 * defined in manual_entity.h, so that the array indices match the
 * values of their enum entries.
 */

static struct manual_entity_definition manual_entity_names[] = {
	{ MANUAL_ENTITY_SMILEYFACE,		"smileyface",		-1},
	{ MANUAL_ENTITY_SADFACE,		"sadface",		-1},
	{ MANUAL_ENTITY_MSEP,			"msep",			-1},

	{ MANUAL_ENTITY_QUOT,			"quot",			34 },
	{ MANUAL_ENTITY_AMP,			"amp",			38 },
	{ MANUAL_ENTITY_APOS,			"apos",			39 },
	{ MANUAL_ENTITY_LT,			"lt",			60 },
	{ MANUAL_ENTITY_GT,			"gt",			62 },

	{ MANUAL_ENTITY_NBSP,			"nbsp",			160 },
	{ MANUAL_ENTITY_IEXCL,			"iexcl",		161 },
	{ MANUAL_ENTITY_CENT,			"cent",			162 },
	{ MANUAL_ENTITY_POUND,			"pound",		163 },
	{ MANUAL_ENTITY_CURREN,			"curren",		164 },
	{ MANUAL_ENTITY_YEN,			"yen",			165 },
	{ MANUAL_ENTITY_BRVBAR,			"brvbar",		166 },
	{ MANUAL_ENTITY_SECT,			"sect",			167 },
	{ MANUAL_ENTITY_UML,			"uml",			168 },
	{ MANUAL_ENTITY_COPY,			"copy",			169 },

	{ MANUAL_ENTITY_PLUSMN,			"plusmn",		177 },
	{ MANUAL_ENTITY_TIMES,			"times",		215 },

	{ MANUAL_ENTITY_NDASH,			"ndash",		8211 },
	{ MANUAL_ENTITY_MDASH,			"mdash",		8212 },

	{ MANUAL_ENTITY_LSQUO,			"lsquo",		8216 },
	{ MANUAL_ENTITY_RSQUO,			"rsquo",		8217 },
	{ MANUAL_ENTITY_LDQUO,			"ldquo",		8220 },
	{ MANUAL_ENTITY_RDQUO,			"rdquo",		8221 },

	{ MANUAL_ENTITY_MINUS,			"minus",		8722 },

	{ MANUAL_ENTITY_LE,			"le",			8804 },
	{ MANUAL_ENTITY_GE,			"ge",			8805 },
	{ MANUAL_ENTITY_LEQQ,			"leqq",			8806 },
	{ MANUAL_ENTITY_GEQQ,			"geqq",			8807 },

	{ MANUAL_ENTITY_NONE,			"",			0x7fffffff }
};

/* Static function prototypes. */

static bool manual_entity_initialise_lists(void);

/**
 * Given a node containing an entity, return the entity type.
 *
 * \param *name		Pointer to the textual entity name.
 * \return		The entity type, or MANUAL_ENTITY_NONE if unknown.
 */

enum manual_entity_type manual_entity_find_type(char *name)
{
	struct manual_entity_definition *entity;

	if (name == NULL)
		return MANUAL_ENTITY_NONE;

	/* If the search tree hasn't been initialised, do it now.*/

	if (manual_entity_search_tree == NULL && !manual_entity_initialise_lists())
		return MANUAL_ENTITY_NONE;

	/* Find the entity definition. */

	entity = search_tree_find_entry(manual_entity_search_tree, name);
	if (entity == NULL) {
		msg_report(MSG_UNKNOWN_ENTITY, name);
		return MANUAL_ENTITY_NONE;
	}

	return entity->type;
}

/**
 * Given an entity type, return the textual entity name.
 *
 * \param type		The entity type to look up.
 * \return		Pointer to the entity's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_entity_find_name(enum manual_entity_type type)
{
	if (manual_entities_max_entries <= 0 && !manual_entity_initialise_lists())
		return "*error*";

	if (type < 0 || type >= manual_entities_max_entries)
		return "*none*";

	/* Look up the tag name.
	 *
	 * WARNING: This relies on the array indices matching the values of
	 * the entity enum entries.
	 */

	return manual_entity_names[type].name;
}


/**
 * Initialise the parse tree and entity list.
 * 
 * \return		True if successful; False on failure.
 */

static bool manual_entity_initialise_lists(void)
{
	int i, current_code = -1;

	if (manual_entity_search_tree != NULL || manual_entities_max_entries > 0)
		return false;

	/* Create a new search tree. */

	manual_entity_search_tree = search_tree_create();
	if (manual_entity_search_tree == NULL)
		return false;

	/* Add the element tags to the search tree. */

	for (i = 0; manual_entity_names[i].type != MANUAL_ENTITY_NONE; i++) {
		if (!search_tree_add_entry(manual_entity_search_tree, manual_entity_names[i].name, &(manual_entity_names[i])))
			return false;

		/* Check that the values are in the correct index slots, to allow for
		 * quick lookups.
		 */

		if (manual_entity_names[i].type != i || manual_entity_names[i].unicode < current_code) {
			msg_report(MSG_ENTITY_OUT_OF_SEQ);
			return false;
		}

		current_code = manual_entity_names[i].unicode;
	}

	manual_entities_max_entries = i;

	return true;
}
