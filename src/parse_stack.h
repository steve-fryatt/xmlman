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

#include "manual_data.h"
#include "parse_element.h"

/**
 * The types of object which can be held on the stack.
 */

enum parse_stack_content {
	PARSE_STACK_CONTENT_NONE,		/**< No, or undefined, content.		*/
	PARSE_STACK_CONTENT_MANUAL,		/**< A top-level manual structure.	*/
	PARSE_STACK_CONTENT_CHAPTER,		/**< A chapter or index structure.	*/
	PARSE_STACK_CONTENT_SECTION,		/**< A section structure.		*/
	PARSE_STACK_CONTENT_BLOCK		/**< An object block structure.		*/
};

/**
 * An entry in the parse stack.
 */

struct parse_stack_entry {
	/**
	 * The content of the level.
	 */

	enum parse_stack_content		content;

	/**
	 * The element expected to close the level.
	 */

	enum parse_element_type			closing_element;

	/**
	 * The parent object.
	 */

	struct manual_data			*object;

	/**
	 * The current child object.
	 */

	struct manual_data			*current_child;
};

/**
 * Offset to the top of the stack from the top.
 */

#define PARSE_STACK_TOP ((int) 0)

/**
 * Offset to the parent item to the top of the stack.
 */

#define PARSE_STACK_PARENT ((int) 1)

/**
 * Reset the parse stack.
 */

void parse_stack_reset(void);

/**
 * Push an entry on to the parse stack.
 *
 * \param content		The content type of the new entry.
 * \param closing_element	The type of element which will close the
 *				entry.
 * \return			Pointer to the new entry, or NULL.
 */

struct parse_stack_entry *parse_stack_push(enum parse_stack_content content, enum parse_element_type closing_element);

/**
 * Push an entry from the parse stack.
 *
 * \return			Pointer to the popped entry, or NULL.
 */

struct parse_stack_entry *parse_stack_pop(void);

/**
 * Peek the entry relative to the top of the parse stack.
 *
 * \param offset		The number of entries to offset from
 *				the top of the stack (0 for top).
 * \return			Pointer to the peeked entry, or NULL.
 */

struct parse_stack_entry *parse_stack_peek(int offset);

/**
 * Peek the closest entry to the top of the parse stack with the
 * given content type.
 *
 * \param content		The required content type.
 * \return			Pointer to the peeked entry, or NULL.
 */

struct parse_stack_entry *parse_stack_peek_content(enum parse_stack_content content);

#endif

