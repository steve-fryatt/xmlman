/* Copyright 2014-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file msg.h
 *
 * Status Message Interface.
 */

#ifndef XMLMAN_MSG_H
#define XMLMAN_MSG_H

#include <stdbool.h>


/**
 * Error message codes.
 *
 * NB: The order of these values *must* match the order of the error message
 * definitions in msg_definitions[] in msgs.c.
 */

enum msg_type {
	MSG_UNKNOWN_ERROR = 0,
	MSG_PARSE_FAIL,
	MSG_FILE_MISSING,
	MSG_OPEN_FAIL,
	MSG_INVALID,
	MSG_XML_FAIL,
	MSG_UNEXPECTED_NODE,
	MSG_UNEXPECTED_CLOSE,
	MSG_UNEXPECTED_STACK,
	MSG_UNEXPECTED_XML,
	MSG_UNEXPECTED_BLOCK_ADD,
	MSG_UNEXPECTED_PUSH,
	MSG_MISSING_ATTRIBUTE,
	MSG_UNKNOWN_MODE,
	MSG_BAD_RESOURCE_BLOCK,
	MSG_UNEXPECTED_CONTENT,

	MSG_UNKNOWN_ELEMENT,
	MSG_DATA_MALLOC_FAIL,

	MSG_BAD_TYPE,
	MSG_STACK_FULL,
	MSG_STACK_ERROR,
	MSG_ENC_OUT_OF_SEQ,
	MSG_ENC_OUT_OF_RANGE,
	MSG_ENC_DUPLICATE,
	MSG_ENC_NO_MAP,
	MSG_ENC_BAD_UTF8,
	MSG_ENC_NO_OUTPUT,
	MSG_UNEXPECTED_BLOCK,
	MSG_UNEXPECTED_CHUNK,
	MSG_ENTITY_NO_MAP,
	MSG_TOO_DEEP,


	MSG_TEXT_LINE_MEM,
	MSG_TEXT_LINE_COL_MEM,
	MSG_TEST_LINE_NO_MEM,
	MSG_TEXT_LINE_BAD_REF,
	MSG_TEXT_LINE_BAD_COL_REF,
	MSG_TEXT_NO_LINE_END,

	MSG_STRONG_ROOT_FAIL,
	MSG_STRONG_COUNT_FAIL,
	MSG_STRONG_NEW_NODE_FAIL,
	MSG_STRONG_NO_FILE,
	MSG_STRONG_NAME_EXISTS,


	MSG_WRITE_NO_FILENAME,
	MSG_WRITE_OPEN_FAIL,
	MSG_WRITE_NO_FILE,
	MSG_WRITE_FAILED,
	MSG_UNKNOWN_MEM_ERROR,
	MSG_MAX_MESSAGES
};


/**
 * Initialise the message system.
 *
 * \param verbose	True to generate verbose output, otherwise false.
 */

void msg_initialise(bool verbose);


/**
 * Set the location for future messages, in the form of a file and line number
 * relating to the source files.
 *
 * \param line		The number of the current line.
 * \param *file		Pointer to the name of the current file.
 */

void msg_set_location(unsigned line, char *file);


/**
 * Generate a message to the user, based on a range of standard message tokens
 *
 * \param type		The message to be displayed.
 * \param ...		Additional printf parameters as required by the token.
 */

void msg_report(enum msg_type type, ...);


/**
 * Indicate whether an error has been reported at any point.
 *
 * \return		True if an error has been reported; else false.
 */

bool msg_errors(void);

#endif

