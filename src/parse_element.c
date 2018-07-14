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
 * \file parse_element.c
 *
 * XML Parser Element Decoding, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/xmlreader.h>

#include "parse_element.h"

/**
 * An element definition structure.
 */

struct parse_element_definition {
	enum parse_element_type	type;		/**< The type of element.		*/
	const xmlChar		*tag;		/**< The tag name for the element.	*/
};

/**
 * The list of known element definitions.
 */

static struct parse_element_definition parse_element_tags[] = {
	{PARSE_ELEMENT_CHAPTER,		(const xmlChar *) "chapter"},
	{PARSE_ELEMENT_INDEX,		(const xmlChar *) "index"},
	{PARSE_ELEMENT_MANUAL,		(const xmlChar *) "manual"},
	{PARSE_ELEMENT_NONE,		(const xmlChar *) ""}
};


/**
 * Given a node containing an element, return the element type.
 *
 * \param reader	The reader to take the node from.
 * \return		The element type, or PARSE_ELEMENT_NONE if unknown.
 */

enum parse_element_type parse_find_element_type(xmlTextReaderPtr reader)
{
	const xmlChar	*name;
	int		i;

	name = xmlTextReaderConstName(reader);
	if (name == NULL)
		return PARSE_ELEMENT_NONE;

	for (i = 0; parse_element_tags[i].type != PARSE_ELEMENT_NONE && strcmp((const char *) parse_element_tags[i].tag, (const char *) name) != 0; i++);

	return parse_element_tags[i].type;
}

