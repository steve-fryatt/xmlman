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

/**
 * The range of possible results from calling the chunk parser.
 */

enum parse_xml_result {
	PARSE_XML_RESULT_ERROR,
	PARSE_XML_RESULT_EOF,
	PARSE_XML_RESULT_TAG_OPEN,
	PARSE_XML_RESULT_TAG_SELF,
	PARSE_XML_RESULT_TAG_CLOSE,
	PARSE_XML_RESULT_TAG_ENTITY,
	PARSE_XML_RESULT_TEXT,
	PARSE_XML_RESULT_COMMENT,
	PARSE_XML_RESULT_OTHER
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

enum parse_xml_result parse_xml_read_next_chunk(void);

#endif

