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
 * \file parse_element.h
 *
 * XML Parser Element Decoding Interface.
 */

#ifndef XMLMAN_PARSE_ELEMENT_H
#define XMLMAN_PARSE_ELEMENT_H

/**
 * A list of element types known to the parser.
 *
 * The order of the types is alphabetical, with NONE at the
 * end as an end-stop. It *must* correspond to the order of the
 * tags in parse_element.c.
 */

enum parse_element_type {
	PARSE_ELEMENT_BR,
	PARSE_ELEMENT_CALLOUT,
	PARSE_ELEMENT_CHAPTER,
	PARSE_ELEMENT_CHAPTERLIST,
	PARSE_ELEMENT_CITE,
	PARSE_ELEMENT_CODE,
	PARSE_ELEMENT_COL,
	PARSE_ELEMENT_COLDEF,
	PARSE_ELEMENT_COLUMNS,
	PARSE_ELEMENT_COMMAND,
	PARSE_ELEMENT_CONST,
	PARSE_ELEMENT_CREDIT,
	PARSE_ELEMENT_DATE,
	PARSE_ELEMENT_DEFINE,
	PARSE_ELEMENT_DOWNLOADS,
	PARSE_ELEMENT_EM,
	PARSE_ELEMENT_ENTRY,
	PARSE_ELEMENT_EVENT,
	PARSE_ELEMENT_FILE,
	PARSE_ELEMENT_FILENAME,
	PARSE_ELEMENT_FOOTNOTE,
	PARSE_ELEMENT_FOLDER,
	PARSE_ELEMENT_FUNCTION,
	PARSE_ELEMENT_ICON,
	PARSE_ELEMENT_IMAGES,
	PARSE_ELEMENT_INTRO,
	PARSE_ELEMENT_INDEX,
	PARSE_ELEMENT_KEY,
	PARSE_ELEMENT_KEYWORD,
	PARSE_ELEMENT_LI,
	PARSE_ELEMENT_LINK,
	PARSE_ELEMENT_MANUAL,
	PARSE_ELEMENT_MATHS,
	PARSE_ELEMENT_MENU,
	PARSE_ELEMENT_MESSAGE,
	PARSE_ELEMENT_MODE,
	PARSE_ELEMENT_MOUSE,
	PARSE_ELEMENT_NAME,
	PARSE_ELEMENT_OL,
	PARSE_ELEMENT_PARAGRAPH,
	PARSE_ELEMENT_REF,
	PARSE_ELEMENT_RESOURCES,
	PARSE_ELEMENT_ROW,
	PARSE_ELEMENT_SECTION,
	PARSE_ELEMENT_STRAPLINE,
	PARSE_ELEMENT_STRONG,
	PARSE_ELEMENT_STYLESHEET,
	PARSE_ELEMENT_SUMMARY,
	PARSE_ELEMENT_SWI,
	PARSE_ELEMENT_TABLE,
	PARSE_ELEMENT_TITLE,
	PARSE_ELEMENT_TYPE,
	PARSE_ELEMENT_UL,
	PARSE_ELEMENT_VARIABLE,
	PARSE_ELEMENT_VERSION,
	PARSE_ELEMENT_WINDOW,
	PARSE_ELEMENT_NONE
};

/**
 * Given a node containing an element, return the element type.
 *
 * \param *name		Pointer to the name of the element to decode.
 * \return		The element type, or PARSE_ELEMENT_NONE if unknown.
 */

enum parse_element_type parse_element_find_type(char *name);

/**
 * Given an element type, return the textual node tag.
 *
 * \param type		The node type to look up.
 * \return		Pointer to the node's textual name, or to "" if
 *			the type was not recognised.
 */

const char *parse_element_find_tag(enum parse_element_type type);

#endif

