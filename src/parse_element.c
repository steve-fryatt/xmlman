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

/**
 * An element definition structure.
 */

struct parse_element_definition {
	enum parse_element_type	type;		/**< The type of element.		*/
	const char		*tag;		/**< The tag name for the element.	*/
};

/**
 * The list of known element definitions.
 */

static struct parse_element_definition parse_element_tags[] = {
	{PARSE_ELEMENT_CITE,		"cite"},
	{PARSE_ELEMENT_CODE,		"code"},
	{PARSE_ELEMENT_EMPHASIS,	"em"},
	{PARSE_ELEMENT_ENTRY,		"entry"},
	{PARSE_ELEMENT_FILE,		"file"},
	{PARSE_ELEMENT_ICON,		"icon"},
	{PARSE_ELEMENT_KEY,		"key"},
	{PARSE_ELEMENT_LI,		"li"},
	{PARSE_ELEMENT_LINK,		"link"},
	{PARSE_ELEMENT_MOUSE,		"mouse"},
	{PARSE_ELEMENT_STRONG,		"strong"},
	{PARSE_ELEMENT_VARIABLE,	"variable"},
	{PARSE_ELEMENT_WINDOW,		"window"},
	{PARSE_ELEMENT_REF,		"ref"},
	{PARSE_ELEMENT_COL,		"col"},
	{PARSE_ELEMENT_ROW,		"row"},
	{PARSE_ELEMENT_PARAGRAPH,	"p"},
	{PARSE_ELEMENT_OL,		"ol"},
	{PARSE_ELEMENT_UL,		"ul"},
	{PARSE_ELEMENT_BR,		"br"},
	{PARSE_ELEMENT_COLDEF,		"coldef"},
	{PARSE_ELEMENT_COLUMNS,		"columns"},
	{PARSE_ELEMENT_TABLE,		"table"},
	{PARSE_ELEMENT_FOOTNOTE,	"footnote"},
	{PARSE_ELEMENT_SECTION,		"section"},
	{PARSE_ELEMENT_TITLE,		"title"},
	{PARSE_ELEMENT_RESOURCES,	"resources"},
	{PARSE_ELEMENT_MODE,		"mode"},
	{PARSE_ELEMENT_FILENAME,	"filename"},
	{PARSE_ELEMENT_FOLDER,		"folder"},
	{PARSE_ELEMENT_STYLESHEET,	"stylesheet"},
	{PARSE_ELEMENT_IMAGES,		"images"},
	{PARSE_ELEMENT_DOWNLOADS,	"downloads"},
	{PARSE_ELEMENT_CHAPTERLIST,	"chapterlist"},
	{PARSE_ELEMENT_SUMMARY,		"summary"},
	{PARSE_ELEMENT_CHAPTER,		"chapter"},
	{PARSE_ELEMENT_INDEX,		"index"},
	{PARSE_ELEMENT_MANUAL,		"manual"},
	{PARSE_ELEMENT_NONE,		"*none*"}
};


/**
 * Given a node containing an element, return the element type.
 *
 * \param *name		Pointer to the name of the element to decode.
 * \return		The element type, or PARSE_ELEMENT_NONE if unknown.
 */

enum parse_element_type parse_element_find_type(char *name)
{
	int		i;

	if (name == NULL)
		return PARSE_ELEMENT_NONE;

	for (i = 0; parse_element_tags[i].type != PARSE_ELEMENT_NONE && strcmp(parse_element_tags[i].tag, name) != 0; i++);

	if (parse_element_tags[i].type == PARSE_ELEMENT_NONE)
		msg_report(MSG_UNKNOWN_ELEMENT, name);

	return parse_element_tags[i].type;
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
	int i;

	for (i = 0; parse_element_tags[i].type != PARSE_ELEMENT_NONE && parse_element_tags[i].type != type; i++);

	return parse_element_tags[i].tag;
}

