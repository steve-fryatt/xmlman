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

/**
 * An internal file node data block, used for tracking the contents of
 * the StrongHelp file as it is written.
 */

struct output_strong_file_object {
	/**
	 * File offset to the referenced object, in bytes, or zero.
	 */

	size_t					file_offset;

	/**
	 * Pointer to the object's filename, or NULL.
	 */

	char					*filename;

	/**
	 * The filetype of the object, or -1 for directories.
	 */

	int					type;

	/**
	 * The size of the referenced object, in bytes.
	 */

	size_t					size;

	/**
	 * If the node is a directory, pointer to the first node
	 * within it. NULL for files.
	 */

	struct output_strong_file_object	*contents;

	/**
	 * Pointer to the next node in the current directory, or NULL.
	 */

	struct output_strong_file_object	*next;
};

/**
 * A StrongHelp file root block.
 */

struct output_strong_file_root {
	int32_t		help;
	int32_t		size;
	int32_t		version;
	int32_t		free_offset;
};

/**
 * A StrongHelp directory entry block.
 */

struct output_strong_file_dir_entry {
	int32_t		object_offset;
	int32_t		load_address;
	int32_t		exec_address;
	int32_t		size;
	int32_t		flags;
	int32_t		reserved;
			/* The filename follows this block,
			 * zero terminated and padded
			 * to a word boundary.
			 */
};

/**
 * A StrongHelp data (file) block header.
 */

struct output_strong_file_dir_block {
	int32_t		dir;
	int32_t		size;
	int32_t		used;
};

/**
 * A StrongHelp directory block header.
 */

struct output_strong_file_data_block {
	int32_t		data;
	int32_t		size;
};

/**
 * A StrongHelp free block header.
 */

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

/**
 * Calculate the padding required to bring a file offset to a word boundary.
 */

#define OUTPUT_STRONG_FILE_PADDING(position) ((4 - ((position) % 4)) % 4)

/**
 * Convert a long file position into a RISC OS sized 32-bit word.
 */

#define OUTPUT_STRONG_FILE_TO_RISCOS(position) ((size_t) ((position) & 0xffffffff))

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

/**
 * The position of the root directory block.
 */

static size_t output_strong_file_root_dir_offset = 0;

/* Static Function Prototypes. */

static struct output_strong_file_object *output_strong_file_add_entry(struct output_strong_file_object *directory, char *filename, int type);
static struct output_strong_file_object *output_strong_file_link_object(struct output_strong_file_object *directory, char *filename, int type);
static struct output_strong_file_object *output_strong_file_create_object(char *filename, int type);
static bool output_strong_file_count_directory(struct output_strong_file_object *directory);
static bool output_strong_file_write_catalogue(struct output_strong_file_object *directory, size_t *offset, size_t *length);
static int output_strong_file_strcmp(char *s1, char *s2);
static bool output_strong_file_write_char(int unicode);
static bool output_strong_file_write_filename(char *filename);
static bool output_strong_file_pad(void);

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

	/* Create a root directory object. */

	output_strong_file_root = output_strong_file_create_object("$", OUTPUT_STRONG_FILE_TYPE_DIR);

	if (output_strong_file_root == NULL) {
		msg_report(MSG_STRONG_ROOT_FAIL);
		return false;
	}

	/* Open the file to disc. */

	output_strong_file_handle = fopen(filename, "w");

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_OPEN_FAIL, filename);
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

	output_strong_file_root_dir_offset = OUTPUT_STRONG_FILE_TO_RISCOS(ftell(output_strong_file_handle));

	dir.object_offset = 0;
	dir.load_address = 0xfffffd00;
	dir.exec_address = 0x00000000;
	dir.size = 0;
	dir.flags = 0x100;
	dir.reserved = 0;

	if (fwrite(&dir, sizeof(struct output_strong_file_dir_entry), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return output_strong_file_write_filename("$");
}

/**
 * Close the current StrongHelp output file.
 */

void output_strong_file_close(void)
{
	struct output_strong_file_dir_entry	dir;
	size_t					offset, length;

	if (output_strong_file_handle == NULL)
		return;

	/* Claculate the file's catalogue information, and then re-write
	 * the root entry with the correct size and offset.
	 */

	if (output_strong_file_count_directory(output_strong_file_root)) {
		if (output_strong_file_write_catalogue(output_strong_file_root, &offset, &length)) {

			/* Write the root directory entry. */

			dir.object_offset = offset;
			dir.load_address = 0xfffffd00;
			dir.exec_address = 0x00000000;
			dir.size = length - 8;
			dir.flags = 0x100;
			dir.reserved = 0;

			if (fseek(output_strong_file_handle, output_strong_file_root_dir_offset, SEEK_SET) == -1)
				msg_report(MSG_WRITE_FAILED);

			if (fwrite(&dir, sizeof(struct output_strong_file_dir_entry), 1, output_strong_file_handle) != 1)
				msg_report(MSG_WRITE_FAILED);
		}
	} else {
		msg_report(MSG_STRONG_COUNT_FAIL);
	}

	/* Close the file. */

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

	/* Link the new file into our internal node structure. */

	output_strong_file_current_block = output_strong_file_add_entry(output_strong_file_root, filename, type);
	if (output_strong_file_current_block == NULL) {
		msg_report(MSG_STRONG_NEW_NODE_FAIL);
		return false;
	}

	/* Record the new file's offset. */

	output_strong_file_current_block->file_offset = OUTPUT_STRONG_FILE_TO_RISCOS(ftell(output_strong_file_handle));

	/* Write a DIR$ header block, with a zero placeholder for size. */

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
	struct output_strong_file_data_block	data;
	size_t					position;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	if (output_strong_file_current_block == NULL) {
		msg_report(MSG_STRONG_NO_FILE);
		return false;
	}

	/* Find the position of the end of the file, and calculate its size. */

	position = OUTPUT_STRONG_FILE_TO_RISCOS(ftell(output_strong_file_handle));

	output_strong_file_current_block->size = position - output_strong_file_current_block->file_offset;

	/* Update the file's DIR$ header block with the correct size. */

	if (fseek(output_strong_file_handle, output_strong_file_current_block->file_offset, SEEK_SET) == -1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	data.data = 0x41544144;
	data.size = output_strong_file_current_block->size;

	if (fwrite(&data, sizeof(struct output_strong_file_data_block), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	/* Return the pointer to the end of the file. */

	if (fseek(output_strong_file_handle, position, SEEK_SET) == -1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	output_strong_file_current_block = NULL;

	/* Pad the file out to a multiple of four bytes. */

	return output_strong_file_pad();
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

	/* Extract and copy the next part of the filename. This is now in
	 * a newly claimed malloc block, so we can store a reference to it
	 * in any object node.
	 */

	for (p = filename; *p != '\0' && *p != '.'; p++)
		length++;

	name = malloc(length + 1);
	if (name == NULL)
		return NULL;

	for (p = name; *filename != '\0' && *filename != '.'; )
		*p++ = *filename++;

	*p = '\0';

	/* Process the object that we have found. */

	if (*filename == '.') {
		/* This is an intermediate directory, so create it. */

		directory = output_strong_file_link_object(directory, name, OUTPUT_STRONG_FILE_TYPE_DIR);

		/* Process the remaining filename based in the new directory. */

		filename++;
		return output_strong_file_add_entry(directory, filename, type);
	} else {
		/* This is a file, so link it in and return it. */

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

	/* Locate the object's position in the linked list. */

	current = directory->contents;

	while ((current != NULL) && ((compare = output_strong_file_strcmp(current->filename, filename)) < 0)) {
		previous = current;
		current = current->next;
	}

	if (compare == 0) {
		/* An existing object with the name has been found. */

		if (current->contents == NULL && type != OUTPUT_STRONG_FILE_TYPE_DIR) {
			/* Both entries are files, so there's a conflict that
			 * we can't resolve.
			 */

			msg_report(MSG_STRONG_NAME_EXISTS, filename, directory->filename);
			return NULL;
		} else if (current->contents == NULL) {
			/* There's already a file with the name that we want
			 * for the new directory, so move the existing file
			 * into the new directory and call it !Root.
			 */

			/* Create the new directory. */

			new = output_strong_file_create_object(filename, type);
			if (new == NULL)
				return NULL;

			/* Allocate a new name for the existing file. */

			new_name = malloc(strlen(root) + 1);
			if (new_name == NULL) {
				free(new);
				return NULL;
			}

			strcpy(new_name, root);

			free(current->filename);
			current->filename = new_name;

			/* Move the file and directory blocks around. */

			new->contents = current;
			new->next = current->next;
			previous->next = new;
			current->next = NULL;

			return new;
		} else {
			/* The existing entry is a directory, so we can simply
			 * use it again.
			 */

			return current;
		}
	} else {
		/* A new object is required. */

		new = output_strong_file_create_object(filename, type);
		if (new == NULL)
			return NULL;

		if (previous == NULL) {
			new->next = directory->contents;
			directory->contents = new;
		} else {
			new->next = previous->next;
			previous->next = new;
		}

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
 * Count the size of a directory node, and then recurse into all of
 * its subdirectories.
 *
 * \param *directory	Pointer to the directory to process.
 * \return		True on success; False on error.
 */

static bool output_strong_file_count_directory(struct output_strong_file_object *directory)
{
	struct output_strong_file_object	*node = NULL;
	size_t					bytes, node_size;

	if (directory == NULL)
		return false;

	/* Include the size of the DIR$ header. */

	bytes = sizeof(struct output_strong_file_dir_block);

	/* Count the size of the entries in the directory. */

	node = directory->contents;

	while (node != NULL) {
		node_size = sizeof(struct output_strong_file_dir_entry) + strlen(node->filename) + 1;
		node_size += OUTPUT_STRONG_FILE_PADDING(node_size);

		bytes += node_size;

		/* If this node is a directory, count it now. */

		if (node->contents != NULL && !output_strong_file_count_directory(node))
			return false;

		node = node->next;
	}

	/* Store the directory size. */

	directory->size = bytes;

	return true;
}

/**
 * Write the catalogue entries for a directory and any sub-directories
 * to the file.
 *
 * \param *directory	Pointer to the directory to write.
 * \param *offset	Pointer to a variable to return the file offset
 *			for the catalogue.
 * \param *length	Pointer to a variable to return the catalogue size.
 * \return		True if successful; False on error.
 */

static bool output_strong_file_write_catalogue(struct output_strong_file_object *directory, size_t *offset, size_t *length)
{
	struct output_strong_file_object	*node = NULL;
	struct output_strong_file_dir_block	dir;
	struct output_strong_file_dir_entry	entry;
	size_t					position;

	if (directory == NULL || offset == NULL || length == NULL)
		return false;

	/* Write out any subdirectories first, so that we know their details. */

	node = directory->contents;

	while (node != NULL) {
		if (node->contents != NULL) {
			if (!output_strong_file_write_catalogue(node, offset, length))
				return false;

			node->file_offset = *offset;
			node->size = *length;
		}

		node = node->next;
	}

	/* Write out this directory. */

	position = OUTPUT_STRONG_FILE_TO_RISCOS(ftell(output_strong_file_handle));

	/* Write the directory block header. */

	dir.dir = 0x24524944;
	dir.size = directory->size;
	dir.used = directory->size;

	if (fwrite(&dir, sizeof(struct output_strong_file_dir_block), 1, output_strong_file_handle) != 1) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	/* Write out the directory entries for the nodes. */

	node = directory->contents;

	while (node != NULL) {
		entry.object_offset = node->file_offset;
		entry.load_address = (node->type == OUTPUT_STRONG_FILE_TYPE_DIR) ? 0xfffffd00 : 0xfff00000 | (node->type << 8);
		entry.exec_address = 0x00000000;
		entry.size = (node->type == OUTPUT_STRONG_FILE_TYPE_DIR) ? node->size - 8 : node->size;
		entry.flags = (node->type == OUTPUT_STRONG_FILE_TYPE_DIR) ? 0x100 : 0x37;
		entry.reserved = 0;

		if (fwrite(&entry, sizeof(struct output_strong_file_dir_entry), 1, output_strong_file_handle) != 1) {
			msg_report(MSG_WRITE_FAILED);
			return false;
		}

		if (!output_strong_file_write_filename(node->filename))
			return false;

		node = node->next;
	}

	/* Pass this directory's details back to the parent. */

	*offset = position;
	*length = directory->size;

	return true;
}

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
		msg_report(MSG_STRONG_NO_FILE);
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
		msg_report(MSG_STRONG_NO_FILE);
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
		msg_report(MSG_STRONG_NO_FILE);
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
		msg_report(MSG_STRONG_NO_FILE);
		return false;
	}

	encoding_write_unicode_char(buffer, ENCODING_CHAR_BUF_LEN, unicode);

	if (fputs(buffer, output_strong_file_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return true;
}

/**
 * Write a filename for a catalogue entry, and pad it to a word boundary.
 *
 * \param *filename	Pointer to the filename to write.
 * \return		True on success; False on failure.
 */

static bool output_strong_file_write_filename(char *filename)
{
	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	if (fputs(filename, output_strong_file_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	if (fputc('\0', output_strong_file_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	return output_strong_file_pad();
}

/**
 * Pad the current output file to a word boundary.
 *
 * \return		True on success; False on failure.
 */

static bool output_strong_file_pad(void)
{
	size_t position, padding;

	if (output_strong_file_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	position = OUTPUT_STRONG_FILE_TO_RISCOS(ftell(output_strong_file_handle));
	padding = OUTPUT_STRONG_FILE_PADDING(position);

	for (; padding > 0; padding--) {
		if (fputc('\0', output_strong_file_handle) == EOF) {
			msg_report(MSG_WRITE_FAILED);
			return false;
		}
	}

	return true;
}

