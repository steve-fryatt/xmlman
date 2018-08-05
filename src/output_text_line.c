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
 * \file output_text_line.c
 *
 * Text Line Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_text_line.h"

#include "encoding.h"
#include "msg.h"

/**
 * A column within a text line instance.
 */

struct output_text_line_column {
	/**
	 * The parent output line.
	 */
	struct output_text_line		*parent;

	/**
	 * The left-hand starting position of the line, in characters.
	 */
	int				start;

	/**
	 * The width of the column, in characters.
	 */
	int				width;

	/**
	 * The column's text buffer.
	 */
	xmlChar				*text;

	/**
	 * The size of the column's text buffer.
	 */
	size_t				size;

	/**
	 * Pointer to the current position in the text during the
	 * write-out operation, or NULL on completion.
	 */
	xmlChar				*write_ptr;

	/**
	 * The current maximum written width, in charaters.
	 */
	int				written_width;

	/**
	 * Pointer to the next column structure, or NULL.
	 */
	struct output_text_line_column	*next;
};

/**
 * A text line output instance structure.
 */

struct output_text_line {
	struct output_text_line_column	*columns;

	int				position;
	bool				complete;
};


/**
 * The size of the character output buffer. This needs to hold a full
 * UTF8 character, and shouldn't require adjustment.
 */

#define OUTPUT_TEXT_LINE_CHAR_BUF_LEN 5

/**
 * The number of characters that we allocate at a time when adding text
 * to column buffers.
 */

#define OUTPUT_TEXT_LINE_COLUMN_BLOCK_SIZE 2048

/* Static Function Prototypes. */

static bool output_text_line_add_column_text(struct output_text_line_column *column, xmlChar *text);
static bool output_text_line_update_column_memory(struct output_text_line_column *column);
static struct output_text_line_column* output_text_line_find_column(struct output_text_line *line, int column);
static bool output_text_line_write_line(struct output_text_line *line, bool underline);
static bool output_text_line_write_column(struct output_text_line_column *column);
static bool output_text_line_write_column_underline(struct output_text_line_column *column);
static bool output_text_line_pad_to_column(struct output_text_line_column *column);
static bool output_text_line_write_char(struct output_text_line *line, int c);


/**
 * Create a new text line output instance.
 *
 * \return		Pointer to the new line block, or NULL on failure.
 */

struct output_text_line *output_text_line_create(void)
{
	struct output_text_line		*line = NULL;

	line = malloc(sizeof(struct output_text_line));
	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_MEM);
		return NULL;
	}

	line->columns = NULL;

	return line;
}

/**
 * Destroy a text line output instance.
 *
 * \param *line		The line output instance to destroy.
 */

void output_text_line_destroy(struct output_text_line *line)
{
	struct output_text_line_column	*column = NULL, *next = NULL;

	/* We can't destroy no line, but fail silently. */

	if (line == NULL)
		return;

	/* Free the column blocks. */

	column = line->columns;

	while (column != NULL) {
		next = column->next;

		if (column->text != NULL)
			free(column->text);

		free(column);

		column = next;
	}

	/* Free the line block. */

	free(line);
}

/**
 * Add a column to a text line output instance.
 *
 * \paran *line		The line output instance to add to.
 * \param margin	The margin before the column, in characters.
 * \param width		The width of the column, in characters.
 * \return		TRUE on success; FALSE on failure.
 */

bool output_text_line_add_column(struct output_text_line *line, int margin, int width)
{
	struct output_text_line_column	*column = NULL, *previous = NULL;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	column = malloc(sizeof(struct output_text_line_column));
	if (column == NULL) {
		return false;
		msg_report(MSG_TEXT_LINE_COL_MEM);
	}

	/* Find the previous column data. */

	previous = line->columns;

	while (previous != NULL && previous->next != NULL)
		previous = previous->next;

	/* Link the column to the end of the chain. */

	if (previous == NULL)
		line->columns = column;
	else
		previous->next = column;

	/* Initialise the column data. */

	if (previous == NULL)
		column->start = margin;
	else
		column->start = previous->start + previous->width + margin;

	column->parent = line;
	column->width = width;

	column->text = NULL;
	column->size = 0;
	column->write_ptr = NULL;
	column->written_width = 0;
	column->next = NULL;

	return output_text_line_update_column_memory(column);
}

/**
 * Reset a line instance ready for a new block to be built.
 *
 * \param *line		The current line instance.
 * \return		True on success; False on error.
 */

bool output_text_line_reset(struct output_text_line *line)
{
	struct output_text_line_column	*column = NULL;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	column = line->columns;

	while (column != NULL) {
		column->write_ptr = column->text;

		if (column->text != NULL && column->size > 0)
			column->text[0] = '\0';

		column->written_width = 0;

		column = column->next;
	}

	return true;
}

/**
 * Add text to a column, to be proessed when the line is complete.
 *
 * \param *line		The current line instance.
 * \param column	The index of the column to add to.
 * \param *text		Pointer to the text to be added.
 * \return		True on success; False on error.
 */

bool output_text_line_add_text(struct output_text_line *line, int column, xmlChar *text)
{
	struct output_text_line_column	*col = NULL;

	if (line == NULL) {
		return false;
		msg_report(MSG_TEXT_LINE_BAD_REF);
	}

	col = output_text_line_find_column(line, column);
	if (col == NULL)
		return false;

	return output_text_line_add_column_text(col, text);
}

/**
 * Add text to a column's text buffer, expanding the memory as required.
 *
 * \param *column	The column index to add the text to.
 * \param *text		Pointer to the text to add.
 * \return		True on success; False on error.
 */

static bool output_text_line_add_column_text(struct output_text_line_column *column, xmlChar *text)
{
	int	write_ptr;

	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	/* A NULL pointer in will always succeed. */

	if (text == NULL)
		return true;

	/* Find the end of the text currently in the buffer. The buffer
	 * should always be zero terminated, so if we get to the end
	 * without a zero, there's a problem that we can't fix.
	 */

	if (column->text == NULL) {
		msg_report(MSG_UNKNOWN_MEM_ERROR);
		return false;
	}

	write_ptr = 0;

	while ((column->text[write_ptr] != '\0') && (write_ptr < column->size))
		write_ptr++;

	if (column->text[write_ptr] != '\0') {
		msg_report(MSG_UNKNOWN_MEM_ERROR);
		return false;
	}

	/* The write_ptr is now pointing to the zero terminator, so
	 * there is space to write the first character, at least.
	 */

	while (*text != '\0') {
		column->text[write_ptr++] = *text++;

		/* If we're now out of memory, try to claim some more. */

		if ((write_ptr >= column->size) && !output_text_line_update_column_memory(column)) {
			column->text[write_ptr - 1] = '\0';
			msg_report(MSG_TEST_LINE_NO_MEM);
			return false;
		}
	}

	column->text[write_ptr] = '\0';

	return true;
}

/**
 * Update the text buffer memory for a column.
 *
 * \param *column	The column instance to update.
 * \return		True on success; False on error.
 */

static bool output_text_line_update_column_memory(struct output_text_line_column *column)
{
	xmlChar *new;

	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	if (column->text == NULL) {
		column->text = malloc(OUTPUT_TEXT_LINE_COLUMN_BLOCK_SIZE);
		column->size = 0;

		if (column->text == NULL)
			return false;

		column->size = OUTPUT_TEXT_LINE_COLUMN_BLOCK_SIZE;

		if (column->size > 0)
			column->text[0] = '\0';
	} else {
		new = realloc(column->text, column->size + OUTPUT_TEXT_LINE_COLUMN_BLOCK_SIZE);
		if (new == NULL)
			return false;

		column->size += OUTPUT_TEXT_LINE_COLUMN_BLOCK_SIZE;
	}

	return true;
}

/**
 * Find a column instance block based on the column index in a line.
 *
 * \param *line		The current line instance.
 * \param column	The required column index.
 * \return		Pointer to the column instance, or NULL.
 */

static struct output_text_line_column* output_text_line_find_column(struct output_text_line *line, int column)
{
	struct output_text_line_column	*col = NULL;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return NULL;
	}

	/* Don't try searching for negative indexes. */

	if (column < 0)
		return NULL;

	/* Scan for the column index. */

	col = line->columns;

	while (col != NULL && column-- > 0)
		col = col->next;

	return col;
}

/**
 * Write a block to the output.
 *
 * \param *line		The current line instance.
 * \param title		True to underline the text.
 * \return		True on success; False on error.
 */

bool output_text_line_write(struct output_text_line *line, bool title)
{
	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	do {
		line->complete = true;

		if (!output_text_line_write_line(line, false))
			return false;
	} while (line->complete == false);

	if (title == true) {
		if (!output_text_line_write_line(line, true))
			return false;
	}

	return true;
}

/**
 * Write one line from the current block to the output.
 *
 * \param *line		The current line instance.
 * \param underline	True to output an underline; False to output content.
 * \return		True on success; False on error.
 */

static bool output_text_line_write_line(struct output_text_line *line, bool underline)
{
	struct output_text_line_column	*column = NULL;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	line->position = 0;

	column = line->columns;

	while (column != NULL) {
		if (underline == true) {
			if (!output_text_line_write_column_underline(column))
				return false;
		} else {
			if (!output_text_line_write_column(column))
				return false;
		}

		column = column->next;
	}

	output_text_line_write_newline();

	return true;
}

/**
 * Write one column from a line to the output.
 *
 * \param *column	The current column instance.
 * \return		True on success; False on error.
 */

static bool output_text_line_write_column(struct output_text_line_column *column)
{
	int		width, breakpoint, c;
	xmlChar		*scan_ptr;
	bool		hyphenate = false, complete = false;

	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	if (column->write_ptr == NULL)
		return true;

	/* Find the next chunk of string to be written out. */

	width = 0;
	breakpoint = 0;

	scan_ptr = column->write_ptr;

	c = encoding_parse_utf8_string(&scan_ptr);

	while (c != '\0' && width <= column->width) {
		width++;

		/* If this is a possible breakpoint... */

		if (c == ' ' || c == '-') {
			/* If the first character of the column is a space, we skip it. */

			if (c == ' ' && width == 1) {
				width = 0;
				column->write_ptr = scan_ptr;
			}

			/* Remember the breakpoint. */

			breakpoint = width - 1;
		}

		c = encoding_parse_utf8_string(&scan_ptr);
	}

	/* If there's nothing to output, flag the column as complete and exit. */

	if (width == 0) {
		column->write_ptr = NULL;
		return true;
	}

	/* We've reached the end of the string. */

	if (c == '\0') {
		breakpoint = width;
		complete = true;
	}

	/* No breakpoint was found, and the line isn't done. Drop the
	 * last character to make space for a hyphen.
	 */

	if (breakpoint == 0) {
		breakpoint = width - 1;
		hyphenate = true;
	}

	/* Track the maximum line length seen. */

	if (breakpoint > column->written_width)
		column->written_width = (hyphenate) ? breakpoint + 1 : breakpoint;

	/* Write the line of text. */

	if (!output_text_line_pad_to_column(column))
		return false;

	do {
		c = (breakpoint-- > 0) ? encoding_parse_utf8_string(&(column->write_ptr)) : '\0';

		/* Change the special characters passed in by the formatter. */

		switch (c) {
		case ENCODING_UC_NBSP:
			c = ' ';
			break;
		case ENCODING_UC_NBHY:
			c = '-';
			break;
		}

		if (c != '\0' && !output_text_line_write_char(column->parent, c))
			return false;
	} while (c != '\0');

	/* If the line is to be hyphenated, write the hyphen. */

	if (hyphenate && !output_text_line_write_char(column->parent, '-'))
		return false;

	/* If complete, flag the column as done; else, flag the line as not done. */

	if (complete == true)
		column->write_ptr = NULL;
	else
		column->parent->complete = false;

	return true;
}

/**
 * Write an underline for one column from a line to the output.
 *
 * \param *column	The current column instance.
 * \return		True on success; False on error.
 */

static bool output_text_line_write_column_underline(struct output_text_line_column *column)
{
	int		i;

	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	/* If there's no content in the column, don't underline it. */

	if (column->written_width == 0)
		return true;

	/* Write the underline. */

	if (!output_text_line_pad_to_column(column))
		return false;

	for (i = 0; i < column->written_width; i++) {
		if (!output_text_line_write_char(column->parent, '-'))
			return false;
	}

	return true;
}

/**
 * Pad a line out to the start of the given column, using spaces.
 *
 * \param *column	The column instance to pad.
 * \return		True if successful; False on error.
 */

static bool output_text_line_pad_to_column(struct output_text_line_column *column)
{
	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	while (column->parent->position < column->start) {
		if (!output_text_line_write_char(column->parent, ' '))
			return false;
	}

	return true;
}

/**
 * Write a line ending sequence to the output.
 *
 * \return		True if successful; False on error.
 */

bool output_text_line_write_newline(void)
{
	const char *line_end = NULL;

	line_end = encoding_get_newline();
	if (line_end == NULL) {
		msg_report(MSG_TEXT_NO_LINE_END);
		return false;
	}

	if (fputs(line_end, stdout) == EOF) {
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

static bool output_text_line_write_char(struct output_text_line *line, int unicode)
{
	char	buffer[OUTPUT_TEXT_LINE_CHAR_BUF_LEN];

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return 0;
	}

	encoding_write_unicode_char(buffer, OUTPUT_TEXT_LINE_CHAR_BUF_LEN, unicode);

	if (fputs(buffer, stdout) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	line->position++;

	return true;
}

