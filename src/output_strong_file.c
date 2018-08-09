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
 * \file output_strong_file.c
 *
 * StrongHelp File Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_strong_file.h"

#include "encoding.h"
#include "msg.h"

struct output_strong_file {
	struct manual_data	*object;

	char			*filename;

	// Filename??
	
	
};

/* Global Variables. */

/**
 * The output file handle.
 */
static FILE *output_strong_file_handle = NULL;

/* Static Function Prototypes. */

static bool output_strong_file_write_char(int unicode);

/**
 * Open a file to write the StrongHelp output to.
 *
 * \param *filename	Pointer to the name of the file to write.
 * \return		True on success; False on failure.
 */

bool output_strong_file_open(char *filename)
{
	output_strong_file_handle = fopen(filename, "w");

	if (output_strong_file_handle == NULL) {
		fprintf(stderr, "Failed to open StrongHelp file for output.\n");
		return false;
	}

	return true;
}

/**
 * Close the current StrongHelp output file.
 */

void output_strong_file_close(void)
{
	if (output_strong_file_handle == NULL)
		return;

	fclose(output_strong_file_handle);
	output_strong_file_handle = NULL;
}

/**
 * Write a UTF8 string to the current StrongHelp output file, in the
 * currently selected encoding.
 *
 * \param *text		Pointer to the text to be written.
 * \return		True if successful; False on error.
 */

bool output_strong_file_write_text(xmlChar *text)
{
	int c;

	if (text == NULL)
		return true;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	do {
		c = encoding_parse_utf8_string(&text);

		if (c != '\0' && !output_strong_file_write_char(c))
			return false;
	} while (c != '\0');

	return true;
}

/**
 * Write an ASCII string to the output.
 *
 * \param *text		Pointer to the text to be written.
 * \param ...		Additional parameters for formatting.
 * \return		True if successful; False on error.
 */

bool output_strong_file_write_plain(char *text, ...)
{
	va_list ap;

	if (text == NULL)
		return true;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	va_start(ap, text);
  
	if (vfprintf(output_strong_file_handle, text, ap) < 0) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return true;
}

/**
 * Write a line ending sequence to the output.
 *
 * \return		True if successful; False on error.
 */

bool output_strong_file_write_newline(void)
{
	const char *line_end = NULL;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	line_end = encoding_get_newline();
	if (line_end == NULL) {
		msg_report(MSG_TEXT_NO_LINE_END);
		return false;
	}

	if (fputs(line_end, output_strong_file_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return true;
}

/**
 * Write a single unicode character to the output in the currently
 * selected encoding.
 *
 * \param *line		The line instance to work with.
 * \param unicode	The unicode character to be written.
 * \return		True if successful; False on error.
 */

static bool output_strong_file_write_char(int unicode)
{
	char	buffer[ENCODING_CHAR_BUF_LEN];

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	encoding_write_unicode_char(buffer, ENCODING_CHAR_BUF_LEN, unicode);

	if (fputs(buffer, output_strong_file_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return true;
}

