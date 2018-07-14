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
 * \file parse_stack.h
 *
 * XML Parser Stack Interface.
 */

#ifndef XMLMAN_PARSE_STACK_H
#define XMLMAN_PARSE_STACK_H

#include <stdbool.h>

#include "parse_element.h"

/**
 * The types of object which can be held on the stack.
 */

enum parse_stack_content {
	PARSE_STACK_CONTENT_NONE,		/**< No, or undefined, content.		*/
	PARSE_STACK_CONTENT_MANUAL,		/**< A top-level manual structure.	*/
	PARSE_STACK_CONTENT_CHAPTER		/**< A chapter structure.		*/
};

/**
 * An entry in the parse stack.
 */

struct parse_stack_entry {
	/**
	 * The content of the level.
	 */

	enum parse_stack_content	content;

	/**
	 * The element expected to close the level.
	 */

	enum parse_element_type		closing_element;
};

/**
 * Reset the parse stack.
 */

void parse_stack_reset(void);

#endif

