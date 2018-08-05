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
 * \file manual_entity.h
 *
 * Text Encloding Support Interface.
 */

#ifndef XMLMAN_ENCODING_H
#define XMLMAN_ENCODING_H

#include <libxml/xmlreader.h>

/**
 * A list of possible target encodings.
 *
 * The order of this list must match the encoding_list table in encoding.c
 */

enum encoding_target {
	ENCODING_TARGET_UTF8,		/**< UTF8 encoding.			*/
	ENCODING_TARGET_ACORN_LATIN1,	/**< RISC OS Latin 1 encoding.		*/
	ENCODING_TARGET_ACORN_LATIN2,	/**< RISC OS Latin 2 encoding.		*/
	ENCODING_TARGET_MAX		/**< The maximum nubmber of encodings.	*/
};

/**
 * Select an encoding table.
 *
 * \param target		The encoding target to use.
 */

bool encoding_select_table(enum encoding_target target);

/**
 * Parse a UTF8 string, returning the individual characters in the current
 * target encoding. The supplied string pointer is updated on return, to
 * point to the next character to be processed (but stops on the zero
 * terminator).
 *
 * \param *text			Pointer to the the UTF8 string to parse.
 * \return			The next character in the text.
 */

int encoding_parse_utf8_string(xmlChar **text);

/**
 * Write a unicode character to a buffer in the current encoding.
 *
 * \param *buffer		Pointer to the buffer to write to.
 * \param length		The length of the supplied buffer.
 * \param unicode		The unicode character to write.
 * \return			The number of bytes written to the buffer.
 */

int encoding_write_unicode_char(char *buffer, size_t length, int unicode);

/**
 * Flatten down the white space in a text string, so that multiple spaces
 * and newlines become a single ASCII space. The supplied buffer is
 * assumed to be zero terminated, and its contents will be updated.
 *
 * \param *text			The text to be flattened.
 */

void encoding_flatten_whitespace(xmlChar *text);

#endif

