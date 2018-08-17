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

	int					type;

	int					size;

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

/* Static Constants. */

/**
 * A file type indicating a directory in our internal data structures.
 */
#define OUTPUT_STRONG_FILE_TYPE_DIR ((int) -1)

/* Global Variables. */

/**
 * The output file handle.
 */
 
static FILE *output_strong_file_handle = NULL;

/**
 * The current output file block descriptor.
 */

static struct output_strong_file_object *output_strong_file_current_block = NULL;

/**
 * The root file block descriptor.
 */

static struct output_strong_file_object *output_strong_file_root = NULL;

/* Static Function Prototypes. */

static bool output_strong_file_write_char(int unicode);

static struct output_strong_file_object *output_strong_file_add_entry(struct output_strong_file_object *directory, char *filename, int type);
static struct output_strong_file_object *output_strong_file_link_object(struct output_strong_file_object *directory, char *filename, int type);
static struct output_strong_file_object *output_strong_file_create_object(char *filename, int type);
static int output_strong_file_strcmp(char *s1, char *s2);

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
	int					offset;

	/* Create a root directory object. */

	output_strong_file_root = output_strong_file_create_object("$", OUTPUT_STRONG_FILE_TYPE_DIR);

	if (output_strong_file_root == NULL) {
		fprintf(stderr, "Failed to create file directory structure.\n");
		return false;
	}

	/* Open the file to disc. */

	output_strong_file_handle = fopen(filename, "w");

	if (output_strong_file_handle == NULL) {
		fprintf(stderr, "Failed to open StrongHelp file for output.\n");
		return false;
	}

	/* Write the file header block. */

	root.help = 0x504c4548;
	root.size = 44;
	root.version = 290;
	root.free_offset = -1;

	if (fwrite(&root, sizeof(struct output_strong_file_root), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	/* Write the root directory entry. */

	offset = ftell(output_strong_file_handle);

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
	struct output_strong_file_data_block	data;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	output_strong_file_current_block = output_strong_file_add_entry(output_strong_file_root, filename, type);
	if (output_strong_file_current_block == NULL) {
		fprintf(stderr, "Failed to create new file block.\n");
		return false;
	}

	output_strong_file_current_block->file_offset = ftell(output_strong_file_handle);

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

	if (output_strong_file_current_block == NULL) {
		fprintf(stderr, "No file block.\n");
		return false;
	}

	/* Find the position of the end of the file, and calculate its size. */

	position = ftell(output_strong_file_handle);

	output_strong_file_current_block->size = position - output_strong_file_current_block->file_offset;

	output_strong_file_current_block = NULL;

	/* Pad the file out to a multiple of four bytes. */

	padding = (4 - (position % 4)) % 4;

	printf("Position %d, padding with %d bytes\n", position, padding);

	for (; padding > 0; padding--) {
		if (fputc('\0', output_strong_file_handle) == EOF) {
			msg_report(MSG_WRITE_FAILED);
			return false;
		}
	}

	return true;
}

/**
 * Add a new file object into the internal tree, creating the necessary
 * directories on the way.
 *
 * \param *directory	Pointer to the directory node to hold the object.
 * \param *filename	Pointer to the required filename, in Filecore format.
 * \parm type		Pointer to the required file type.
 * \return		Pointer to the new object, or NULL on failure.
 */

static struct output_strong_file_object *output_strong_file_add_entry(struct output_strong_file_object *directory, char *filename, int type)
{
	int	length = 0;
	char	*p, *name;

	if (filename == NULL)
		return NULL;

	printf("Input name '%s'\n", filename);

	/* Extract and copy the next part of the filename. */

	for (p = filename; *p != '\0' && *p != '.'; p++)
		length++;

	name = malloc(length + 1);
	if (name == NULL)
		return NULL;

	for (p = name; *filename != '\0' && *filename != '.'; )
		*p++ = *filename++;

	*p = '\0';

	printf("Found name '%s' to process\n", name);

	if (*filename == '.') {
		/* This is an intermediate directory. */

		directory = output_strong_file_link_object(directory, name, OUTPUT_STRONG_FILE_TYPE_DIR);
		filename++;
		return output_strong_file_add_entry(directory, filename, type);
	} else {
		/* This is the root file. */

		return output_strong_file_link_object(directory, name, type);
	}

	return NULL;
}

/**
 * Link an object with the given filename and type into the specified
 * directory node. In the case of directory objects, an existing directory
 * can be returned if a suitable one exists. If a directory is created
 * on top of an existing file, that file is moved to become !Root within
 * the new directory.
 *
 * \param *directory	Pointer to the directory to link in to.
 * \param *filename	Pointer to the required filename.
 * \param type		The required file type.
 * \return		Pointer to the linked object, or NULL on failure.
 */

static struct output_strong_file_object *output_strong_file_link_object(struct output_strong_file_object *directory, char *filename, int type)
{
	struct output_strong_file_object	*current = NULL, *previous = NULL, *new = NULL;
	int					compare;
	char					*new_name = NULL, *root = "!Root";

	if (directory == NULL || filename == NULL)
		return NULL;

	current = directory->contents;

	printf("Starting to scan for '%s' in directory '%s'\n", filename, directory->filename);

	while ((current != NULL) && ((compare = output_strong_file_strcmp(current->filename, filename)) < 0)) {
		printf("Comparing '%s' to '%s'\n", current->filename, filename);
	
		previous = current;
		current = current->next;
	}

	if (compare == 0) {
		printf("Existing object found.\n");

		if (current->contents == NULL && type != OUTPUT_STRONG_FILE_TYPE_DIR) {
			/* Both entries are files, so it can't work. */
			fprintf(stderr, "This object already exists.\n");
			return NULL;
		} else if (current->contents == NULL) {
			/* There's already a file with the name that we want for the
			 * new directory, so move the existing file into the
			 * new directory and call it !Root.
			 */

			printf("Moving File to File.!Root\n");

			new = output_strong_file_create_object(filename, type);
			if (new == NULL)
				return NULL;

			new_name = malloc(strlen(root) + 1);
			if (new_name == NULL) {
				free(new);
				return NULL;
			}
			strcpy(new_name, root);

			free(current->filename);
			current->filename = new_name;

			new->contents = current;
			new->next = current->next;
			previous->next = new;
			current->next = NULL;

			return new;
		}

		return current;
	} else {
		new = output_strong_file_create_object(filename, type);
		if (new == NULL)
			return NULL;

		if (previous == NULL) {
			printf("Link to head.\n");
			directory->contents = new;
		} else {
			printf("Link in line.\n");
			new->next = previous->next;
			previous->next = new;
		}

		printf("Created and linked new object.\n");

		return new;
	}

	return NULL;
}

/**
 * Create a new file entry in the internal data structure.
 *
 * \param *filename	Pointer to the required filename.
 * \param type		Pointer to the required filetype.
 * \return		Pointer to the new object, or NULL on failure.
 */

static struct output_strong_file_object *output_strong_file_create_object(char *filename, int type)
{
	struct output_strong_file_object *new = NULL;

	new = malloc(sizeof(struct output_strong_file_object));
	if (new == NULL)
		return NULL;

	new->file_offset = 0;
	new->filename = filename;
	new->type = type;
	new->size = 0;
	new->contents = NULL;
	new->next = NULL;

	return new;
};

/**
 * Compare two filenames in a Filecore compatible way.
 *
 * \param *s1		Pointer to the first string to compare.
 * \param *s2		Pointer to the second string to compare.
 * \return		Integer describing the result of the comparison.
 */

static int output_strong_file_strcmp(char *s1, char *s2)
{
	while (*s1 != '\0' && *s2 != '\0' && (toupper(*s1) - toupper(*s2)) == 0) {
		s1++;
		s2++;
	}

	return (toupper(*s1) - toupper(*s2));
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

	if (output_strong_file_current_block == NULL) {
		fprintf(stderr, "No file block.\n");
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

	if (output_strong_file_current_block == NULL) {
		fprintf(stderr, "No file block.\n");
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

	if (output_strong_file_current_block == NULL) {
		fprintf(stderr, "No file block.\n");
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

	if (output_strong_file_current_block == NULL) {
		fprintf(stderr, "No file block.\n");
		return false;
	}

	encoding_write_unicode_char(buffer, ENCODING_CHAR_BUF_LEN, unicode);

	if (fputs(buffer, output_strong_file_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return true;
}

