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
 * \file encoding.h
 *
 * Text Encloding Support Interface.
 */

#ifndef XMLMAN_ENCODING_H
#define XMLMAN_ENCODING_H

#include <libxml/xmlreader.h>

/**
 * The size of a character output buffer. This needs to hold a full
 * UTF8 character, and shouldn't require adjustment.
 */

#define ENCODING_CHAR_BUF_LEN 5

/* Useful Unicode Characters */

/**
 * Unicode Non-Breaking Space
 */
#define ENCODING_UC_NBSP (0xa0)

/**
 * UTF8 Non-Breaking Space Sequence
 */
#define ENCODING_UTF8_NBSP "\xc2\xa0"

/**
 * Unicode Non-Breaking Hyphen
 */
#define ENCODING_UC_NBHY (0x2011)

/**
 * UTF8 Non-Breaking Space Sequence
 */
#define ENCODING_UTF8_NBHY "\xe2\x80\x91"

#define ENCODING_UTF8_LSQUO "\xe2\x80\x98"
#define ENCODING_UTF8_RSQUO "\xe2\x80\x99"
#define ENCODING_UTF8_LDQUO "\xe2\x80\x9c"
#define ENCODING_UTF8_RDQUO "\xe2\x80\x9d"
#define ENCODING_UTF8_NDASH "\xe2\x80\x93"
#define ENCODING_UTF8_MDASH "\xe2\x80\x94"
#define ENCODING_UTF8_MINUS "\xe2\x88\x92"
#define ENCODING_UTF8_TIMES "\xc3\x97"

/**
 * A list of possible target encodings.
 *
 * The order of this list must match the encoding_list table in encoding.c
 */

enum encoding_target {
	ENCODING_TARGET_UTF8,		/**< UTF8 encoding.				*/
	ENCODING_TARGET_ACORN_LATIN1,	/**< RISC OS Latin 1 encoding.			*/
	ENCODING_TARGET_ACORN_LATIN2,	/**< RISC OS Latin 2 encoding.			*/
	ENCODING_TARGET_MAX,		/**< The maximum nubmber of encodings.		*/
	ENCODING_TARGET_NONE		/**< No encoding.				*/
};

/**
 * A list of possible line endings.
 *
 * The order of this list must match the encoding_line_end_list table in encoding.c
 */

enum encoding_line_end {
	ENCODING_LINE_END_CR,		/**< A single Carriage Return.			*/
	ENCODING_LINE_END_LF,		/**< A single Line Feed (RISC OS or Unix).	*/
	ENCODING_LINE_END_CRLF,		/**< Carriage Return then Line Feed (DOS).	*/
	ENCODING_LINE_END_LFCR,		/**< Line Feed then Carriage Return.		*/
	ENCODING_LINE_END_MAX,		/**< The maximum nubmber of line endings.	*/
	ENCODING_LINE_END_NONE		/**< No line ending.				*/
};

/**
 * Find an encoding type based on a textual name.
 *
 * \param *name			The encoding name to match.
 * \return			The encoding type, or
 *				ENCODING_TARGET_NONE.
 */

enum encoding_target encoding_find_target(char *name);
/**
 * Select an encoding table.
 *
 * \param target		The encoding target to use.
 */

bool encoding_select_table(enum encoding_target target);

/**
 * Find a line ending type based on a textual name.
 *
 * \param *name			The line ending name to match.
 * \return			The line ending type, or
 *				ENCODING_LINE_END_NONE.
 */

enum encoding_line_end encoding_find_line_end(char *name);

/**
 * Select a type of line ending.
 *
 * \param type		The line ending type to use.
 */

bool encoding_select_line_end(enum encoding_line_end type);

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
 * Return a pointer to the currently selected line end sequence.
 *
 * \return			A pointer to the sequence, or NULL.
 */

const char *encoding_get_newline(void);

/**
 * Flatten down the white space in a text string, so that multiple spaces
 * and newlines become a single ASCII space. The supplied buffer is
 * assumed to be zero terminated, and its contents will be updated.
 *
 * \param *text			The text to be flattened.
 */

void encoding_flatten_whitespace(xmlChar *text);

#endif

