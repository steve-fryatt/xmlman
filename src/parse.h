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
 * \file parse.h
 *
 * XML Parser Interface.
 */

#ifndef XMLMAN_PARSE_H
#define XMLMAN_PARSE_H

#include <stdbool.h>

enum parse_object_type {
	PARSE_OBJECT_TYPE_NONE,
	PARSE_OBJECT_TYPE_MANUAL,
	PARSE_OBJECT_TYPE_INDEX,
	PARSE_OBJECT_TYPE_CHAPTER,
	PARSE_OBJECT_TYPE_SECTION
};

struct parse_chapter {


};

struct parse_manual {
	/**
	 * Pointer to the manual title.
	 */

	char		*title;
};

/**
 * Parse an XML file and its descendents.
 *
 * \param *filename	The name of the file to parse.
 * \return
 */

struct parse_manual *parse_file(char *filename);

#endif

