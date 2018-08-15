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
#include <stdint.h>

#include <libxml/xmlstring.h>

#include "output_strong_file.h"

#include "encoding.h"
#include "msg.h"


struct output_strong_file_object {
	int					file_offset;

	char					*filename;

	int32_t					load_address;

	int32_t					exec_address;

	int					size;

	int32_t					flags;

	struct output_strong_file_object	*contents;

	struct output_strong_file_object	*next;
};


struct output_strong_file_root {
	int32_t		help;
	int32_t		size;
	int32_t		version;
	int32_t		free_offset;
};

struct output_strong_file_dir_entry {
	int32_t		object_offset;
	int32_t		load_address;
	int32_t		exec_address;
	int32_t		size;
	int32_t		flags;
	int32_t		reserved;
	union {
		int32_t	name_space;
		char	name[4];
	};
};

struct output_strong_file_dir_block {
	int32_t		dir;
	int32_t		size;
	int32_t		used;
};

struct output_strong_file_data_block {
	int32_t		data;
	int32_t		size;
};

struct output_strong_file_free_block {
	int32_t		free;
	int32_t		free_size;
	int32_t		next_offset;
}; 

/* Global Variables. */

/**
 * The output file handle.
 */
 
static FILE *output_strong_file_handle = NULL;

static struct output_strong_file_object *output_strong_file_blocks = NULL;

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
	struct output_strong_file_root		root;
	struct output_strong_file_dir_entry	dir;

	output_strong_file_handle = fopen(filename, "w");

	if (output_strong_file_handle == NULL) {
		fprintf(stderr, "Failed to open StrongHelp file for output.\n");
		return false;
	}

	root.help = 0x504c4548;
	root.size = 44;
	root.version = 290;
	root.free_offset = -1;

	if (fwrite(&root, sizeof(struct output_strong_file_root), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	printf("Root dir offset: %d\n", ftell(output_strong_file_handle));

	dir.object_offset = 0;
	dir.load_address = 0xfffffd00;
	dir.exec_address = 0x00000000;
	dir.size = 0;
	dir.flags = 0x100;
	dir.reserved = 0;
	dir.name_space = 0;

	if (fwrite(&dir, sizeof(struct output_strong_file_dir_entry), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
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
 * Open a file within the current StrongHelp file, ready for writing.
 *
 * \param *filename	The internal filename, in Filecore format.
 * \param type		The RISC OS numeric filetype.
 * \return		True on success; False on failure.
 */

bool output_strong_file_sub_open(char *filename, int type)
{
	struct output_strong_file_data_block data;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	data.data = 0x41544144;
	data.size = 0;

	if (fwrite(&data, sizeof(struct output_strong_file_data_block), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return true;
}

/**
 * Close the current file within the current StrongHelp output file.
 *
 * \return		True on success; False on failure.
 */

bool output_strong_file_sub_close(void)
{
	long position, padding;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	position = ftell(output_strong_file_handle);
	padding = position % 4;

	printf("Padding with %d bytes\n", padding);

	for (; padding > 0; padding--) {
		if (fputc('\0', output_strong_file_handle) == EOF) {
			msg_report(MSG_WRITE_FAILED);
			return false;
		}
	}

	return true;
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

