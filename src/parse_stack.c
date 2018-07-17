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
 * \file parse_stack.c
 *
 * XML Parser Stack, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "parse_stack.h"

/**
 * The maximum size of the parse stack. This must be enough to handle the
 * maximum valid nesting of XML tags, which is controlled by the DTD.
 */

#define PARSE_STACK_SIZE 20

/**
 * The number of entries on the stack.
 */

static int parse_stack_size = 0;

/**
 * The parse stack data.
 */

static struct parse_stack_entry parse_stack[PARSE_STACK_SIZE];


/**
 * Reset the parse stack.
 */

void parse_stack_reset(void)
{
	parse_stack_size = 0;

	parse_stack_push(PARSE_STACK_CONTENT_NONE, PARSE_ELEMENT_NONE);
}

/**
 * Push an entry on to the parse stack.
 *
 * \param content		The content type of the new entry.
 * \param closing_element	The type of element which will close the
 *				entry.
 * \return			Pointer to the new entry, or NULL.
 */

struct parse_stack_entry *parse_stack_push(enum parse_stack_content content, enum parse_element_type closing_element)
{
	if (parse_stack_size >= PARSE_STACK_SIZE) {
		printf("Stack full!\n");
		return NULL;
	}

	parse_stack[parse_stack_size].content = content;
	parse_stack[parse_stack_size].closing_element = closing_element;

	switch (content) {
	case PARSE_STACK_CONTENT_NONE:
		break;
	case PARSE_STACK_CONTENT_MANUAL:
		parse_stack[parse_stack_size].data.manual.manual = NULL;
		parse_stack[parse_stack_size].data.manual.current_chapter = NULL;
		break;
	case PARSE_STACK_CONTENT_CHAPTER:
		parse_stack[parse_stack_size].data.chapter.chapter = NULL;
		parse_stack[parse_stack_size].data.chapter.current_section = NULL;
		break;
	case PARSE_STACK_CONTENT_SECTION:
		break;
	case PARSE_STACK_CONTENT_TITLE:
		break;
	}

	parse_stack_size++;

	return parse_stack + (parse_stack_size - 1);
}

/**
 * Push an entry from the parse stack.
 *
 * \return			Pointer to the popped entry, or NULL.
 */

struct parse_stack_entry *parse_stack_pop(void)
{
	if (parse_stack_size == 0)
		return NULL;

	parse_stack_size--;

	return parse_stack + parse_stack_size;
}

/**
 * Peek the entry relative to the top of the parse stack.
 *
 * \param offset		The number of entries to offset from
 *				the top of the stack (0 for top).
 * \return			Pointer to the peeked entry, or NULL.
 */

struct parse_stack_entry *parse_stack_peek(int offset)
{
	int	stack_pos = parse_stack_size - (offset + 1);

	if (stack_pos < 0)
		return NULL;

	return parse_stack + stack_pos;
}

/**
 * Peek the closest entry to the top of the parse stack with the
 * given content type.
 *
 * \param content		The required content type.
 * \return			Pointer to the peeked entry, or NULL.
 */

struct parse_stack_entry *parse_stack_peek_content(enum parse_stack_content content)
{
	int	stack_pos = parse_stack_size - 1;

	while (stack_pos >= 0 && parse_stack[stack_pos].content != content)
		stack_pos--;

	if (stack_pos < 0)
		return NULL;

	return parse_stack + stack_pos;
}

