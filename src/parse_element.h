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
 * \file parse_element.h
 *
 * XML Parser Element Decoding Interface.
 */

#ifndef XMLMAN_PARSE_ELEMENT_H
#define XMLMAN_PARSE_ELEMENT_H

#include <libxml/xmlreader.h>

/**
 * A list of element types known to the parser.
 */

enum parse_element_type {
	PARSE_ELEMENT_NONE,
	PARSE_ELEMENT_MANUAL,
	PARSE_ELEMENT_INDEX,
	PARSE_ELEMENT_CHAPTER,
	PARSE_ELEMENT_TITLE
};

/**
 * Given a node containing an element, return the element type.
 *
 * \param reader	The reader to take the node from.
 * \return		The element type, or PARSE_ELEMENT_NONE if unknown.
 */

enum parse_element_type parse_find_element_type(xmlTextReaderPtr reader);

#endif

