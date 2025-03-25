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
 * \file parse_element.c
 *
 * XML Parser Element Decoding, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "parse_element.h"

#include "msg.h"
#include "search_tree.h"

/**
 * An element definition structure.
 */

struct parse_element_definition {
	enum parse_element_type	type;		/**< The type of element.		*/
	const char		*tag;		/**< The tag name for the element.	*/
};

/**
 * The lookup tree instance for the elements.
 */

static struct search_tree *parse_element_search_tree = NULL;

/**
 * The number of entries in the element list.
 */

static int parse_element_max_entries = -1;

/**
 * The list of known element definitions.
 * 
 * The order of the tags is alphabetical, with NONE at the end as an
 * end stop. It *must* correspond to the order that the enum values are
 * defined in parse_element.h, so that the array indices match the
 * values of their enum entries.
 */

static struct parse_element_definition parse_element_tags[] = {
	{PARSE_ELEMENT_BLOCKQUOTE,	"blockquote"},
	{PARSE_ELEMENT_BR,		"br"},
	{PARSE_ELEMENT_CALLOUT,		"callout"},
	{PARSE_ELEMENT_CHAPTER,		"chapter"},
	{PARSE_ELEMENT_CHAPTERLIST,	"chapterlist"},
	{PARSE_ELEMENT_CITE,		"cite"},
	{PARSE_ELEMENT_CODE,		"code"},
	{PARSE_ELEMENT_COL,		"col"},
	{PARSE_ELEMENT_COLDEF,		"coldef"},
	{PARSE_ELEMENT_COLUMNS,		"columns"},
	{PARSE_ELEMENT_COMMAND,		"command"},
	{PARSE_ELEMENT_CONST,		"const"},
	{PARSE_ELEMENT_CREDIT,		"credit"},
	{PARSE_ELEMENT_DATE,		"date"},
	{PARSE_ELEMENT_DEFINE,		"define"},
	{PARSE_ELEMENT_DL,		"dl"},
	{PARSE_ELEMENT_DOWNLOADS,	"downloads"},
	{PARSE_ELEMENT_EM,		"em"},
	{PARSE_ELEMENT_ENTRY,		"entry"},
	{PARSE_ELEMENT_EVENT,		"event"},
	{PARSE_ELEMENT_FILE,		"file"},
	{PARSE_ELEMENT_FILENAME,	"filename"},
	{PARSE_ELEMENT_FOOTNOTE,	"footnote"},
	{PARSE_ELEMENT_FOLDER,		"folder"},
	{PARSE_ELEMENT_FUNCTION,	"function"},
	{PARSE_ELEMENT_ICON,		"icon"},
	{PARSE_ELEMENT_IMAGES,		"images"},
	{PARSE_ELEMENT_INTRO,		"intro"},
	{PARSE_ELEMENT_INDEX,		"index"},
	{PARSE_ELEMENT_KEY,		"key"},
	{PARSE_ELEMENT_KEYWORD,		"keyword"},
	{PARSE_ELEMENT_LI,		"li"},
	{PARSE_ELEMENT_LINK,		"link"},
	{PARSE_ELEMENT_MANUAL,		"manual"},
	{PARSE_ELEMENT_MATHS,		"maths"},
	{PARSE_ELEMENT_MENU,		"menu"},
	{PARSE_ELEMENT_MESSAGE,		"message"},
	{PARSE_ELEMENT_MODE,		"mode"},
	{PARSE_ELEMENT_MOUSE,		"mouse"},
	{PARSE_ELEMENT_NAME,		"name"},
	{PARSE_ELEMENT_OL,		"ol"},
	{PARSE_ELEMENT_PARAGRAPH,	"p"},
	{PARSE_ELEMENT_REF,		"ref"},
	{PARSE_ELEMENT_RESOURCES,	"resources"},
	{PARSE_ELEMENT_ROW,		"row"},
	{PARSE_ELEMENT_SECTION,		"section"},
	{PARSE_ELEMENT_STRAPLINE,	"strapline"},
	{PARSE_ELEMENT_STRONG,		"strong"},
	{PARSE_ELEMENT_STYLESHEET,	"stylesheet"},
	{PARSE_ELEMENT_SUMMARY,		"summary"},
	{PARSE_ELEMENT_SWI,		"swi"},
	{PARSE_ELEMENT_TABLE,		"table"},
	{PARSE_ELEMENT_TITLE,		"title"},
	{PARSE_ELEMENT_TYPE,		"type"},
	{PARSE_ELEMENT_UL,		"ul"},
	{PARSE_ELEMENT_VARIABLE,	"variable"},
	{PARSE_ELEMENT_VERSION,		"version"},
	{PARSE_ELEMENT_WINDOW,		"window"},
	{PARSE_ELEMENT_NONE,		"*none*"}
};

/* Static function prototypes. */

static bool parse_element_initialise_lists(void);

/**
 * Given a node containing an element, return the element type.
 *
 * \param *name		Pointer to the name of the element to decode.
 * \return		The element type, or PARSE_ELEMENT_NONE if unknown.
 */

enum parse_element_type parse_element_find_type(char *name)
{
	struct parse_element_definition *element;

	if (name == NULL)
		return PARSE_ELEMENT_NONE;

	/* If the search tree hasn't been initialised, do it now.*/

	if (parse_element_search_tree == NULL && !parse_element_initialise_lists())
		return PARSE_ELEMENT_NONE;

	/* Find the element definition. */

	element = search_tree_find_entry(parse_element_search_tree, name);
	if (element == NULL) {
		msg_report(MSG_UNKNOWN_ELEMENT, name);
		return PARSE_ELEMENT_NONE;
	}

	return element->type;
}

/**
 * Given an element type, return the textual node tag.
 *
 * \param type		The node type to look up.
 * \return		Pointer to the node's textual name, or to "" if
 *			the type was not recognised.
 */

const char *parse_element_find_tag(enum parse_element_type type)
{
	if (parse_element_max_entries <= 0 && !parse_element_initialise_lists())
		return "*error*";

	if (type < 0 || type >= parse_element_max_entries)
		return "*none*";

	/* Look up the tag name.
	 *
	 * WARNING: This relies on the array indices matching the values of
	 * the element enum entries.
	 */

	return parse_element_tags[type].tag;
}

/**
 * Initialise the parse tree and element list.
 * 
 * \return		True if successful; False on failure.
 */

static bool parse_element_initialise_lists(void)
{
	int i;

	if (parse_element_search_tree != NULL || parse_element_max_entries > 0)
		return false;

	/* Create a new search tree. */

	parse_element_search_tree = search_tree_create();
	if (parse_element_search_tree == NULL)
		return false;

	/* Add the element tags to the search tree. */

	for (i = 0; parse_element_tags[i].type != PARSE_ELEMENT_NONE; i++) {
		if (!search_tree_add_entry(parse_element_search_tree, parse_element_tags[i].tag, &(parse_element_tags[i])))
			return false;

		/* Check that the values are in the correct index slots, to allow for
		 * quick lookups.
		 */

		if (parse_element_tags[i].type != i) {
			msg_report(MSG_ELEMENT_OUT_OF_SEQ);
			return false;
		}
	}

	parse_element_max_entries = i;

	return true;
}
