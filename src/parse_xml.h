/* Copyright 2018-2021, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file parse_xml.h
 *
 * XML Chunk Parser Interface.
 */

#ifndef XMLMAN_PARSE_XML_H
#define XMLMAN_PARSE_XML_H

#include <stdbool.h>
#include "manual_entity.h"

/**
 * The range of possible results from calling the chunk parser.
 */

enum parse_xml_result {
	PARSE_XML_RESULT_ERROR,		/**< An error occurred.				*/
	PARSE_XML_RESULT_EOF,		/**< The end of the file has been reached.	*/
	PARSE_XML_RESULT_TAG_START,	/**< An opening tag.				*/
	PARSE_XML_RESULT_TAG_END,	/**< A closing tag.				*/
	PARSE_XML_RESULT_TAG_EMPTY,	/**< An empty element tag.			*/
	PARSE_XML_RESULT_TAG_ENTITY,	/**< A character element.			*/
	PARSE_XML_RESULT_TEXT,		/**< A block of text.				*/
	PARSE_XML_RESULT_WHITESPACE,	/**< A block of white space.			*/
	PARSE_XML_RESULT_COMMENT,	/**< A comment.					*/
	PARSE_XML_RESULT_OTHER		/**< Another element, which wasn't recognised.	*/
};

/**
 * Open a new file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		True if successful; else False.
 */

bool parse_xml_open_file(char *filename);

/**
 * Close a file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		True if successful; else False.
 */

void parse_xml_close_file(void);

/**
 * Parse the next chunk from the current file.
 * 
 * \return		Details of the chunk.
 */

enum parse_xml_result parse_xml_read_next_chunk(void);

/**
 * Read the details of the current element parsed from
 * the file.
 * 
 * \return		The element token, or PARSE_ELEMENT_NONE.
 */

enum parse_element_type parse_xml_get_element(void);

enum manual_entity_type parse_xml_get_entity(void);

#endif
