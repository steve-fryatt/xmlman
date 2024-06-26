/* Copyright 2018-2024, Stephen Fryatt (info@stevefryatt.org.uk)
 *
 * This file is part of XmlMan:
 *
 *   http://www.stevefryatt.org.uk/risc-os
 *
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
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

#include "output_text_line.h"

#include "encoding.h"
#include "filename.h"
#include "msg.h"

/**
 * A column within a text line instance.
 */

struct output_text_line_column {
	/**
	 * The parent output line.
	 */
	struct output_text_line			*parent;

	/**
	 * Configuration flags for the column.
	 */
	enum output_text_line_column_flags	flags;

	/**
	 * The requested left margin for the column, in characters.
	 */
	int					requested_margin;

	/**
	 * The requested width of the column, in characters.
	 */
	int					requested_width;

	/**
	 * The left-hand starting position of the line, in characters.
	 */
	int					start;

	/**
	 * The width of the column, in characters.
	 */
	int					width;

	/**
	 * The hanging indent to apply to subsequent lines.
	 */
	int					hanging_indent;

	/**
	 * The column's text buffer.
	 */
	char					*text;

	/**
	 * The size of the column's text buffer.
	 */
	size_t					size;

	/**
	 * Pointer to the current position in the text during the
	 * write-out operation, or NULL on completion.
	 */
	char					*write_ptr;

	/**
	 * The current maximum written width, in charaters.
	 */
	int					written_width;

	/**
	 * The number of spare lines at the foot of a formatted column.
	 */
	int					blank_rows;

	/**
	 * Pointer to the next column structure, or NULL.
	 */
	struct output_text_line_column		*next;
};

/**
 * A text line output instance structure.
 */

struct output_text_line {
	/**
	 * The number of columns in the current page.
	 */
	int				page_width;

	/**
	 * The left margin of the line, from which columns will be calculated.
	 */
	int				left_margin;

	/**
	 * Has the line been prepared with output_text_line_write() but not
	 * yet written to the file?
	 */
	bool				has_content;

	/**
	 * Has the line been added to with output_text_line_add_text() but
	 * not yet written to the file?
	 */

	bool				is_prepared;

	/**
	 * The linked list of columns in the line.
	 */
	struct output_text_line_column	*columns;

	/**
	 * The right-most column to have been written in the line.
	*/
	int				position;

	/**
	 * Is the current line completely written to the output?
	 */
	bool				complete;

	/**
	 * Pointer to the next line in the stack.
	 */
	struct output_text_line		*next;
};

/**
 * The number of characters that we allocate at a time when adding text
 * to column buffers.
 */

#define OUTPUT_TEXT_LINE_COLUMN_BLOCK_SIZE 2048

/**
 * The minimum column width in which hypjenation will occur.
 */

#define OUTPUT_TEXT_LINE_HYPHENATION_LIMIT 3

/* Global Variables. */

/**
 * The output file handle.
 */

static FILE *output_text_line_handle = NULL;

/**
 * The stack of output lines.
 */

static struct output_text_line *output_text_line_stack = NULL;

/**
 * The width of the output page.
 */

static int output_text_line_page_width = 0;

/* Static Function Prototypes. */

static struct output_text_line *output_text_line_create(int page_width, int left_margin);
static void output_text_line_destroy(struct output_text_line *line);
static bool output_text_line_set_column_widths(struct output_text_line *line);
static bool output_text_line_add_column_text(struct output_text_line_column *column, char *text);
static bool output_text_line_update_column_memory(struct output_text_line_column *column);
static bool output_text_line_set_column_hanging_indent(struct output_text_line_column *column, int spaces);
static struct output_text_line_column* output_text_line_find_column(struct output_text_line *line, int column);
static bool output_text_line_size_columns(struct output_text_line *line);
static bool output_text_line_write_line(struct output_text_line *line, bool underline);
static bool output_text_line_write_column(struct output_text_line_column *column, bool trial);
static bool output_text_line_write_column_underline(struct output_text_line_column *column);
static bool output_text_line_pad_to_column(struct output_text_line_column *column);
static bool output_text_line_pad_to_position(struct output_text_line *line, int position);
static bool output_text_line_write_char(struct output_text_line *line, int c);


/**
 * Open a file to write the text output to.
 *
 * \param *filename	Pointer to the name of the file to write.
 * \param page_width	The page width, in characters.
 * \return		True on success; False on failure.
 */

bool output_text_line_open(struct filename *filename, int page_width)
{
	output_text_line_handle = filename_fopen(filename, "w");
	output_text_line_page_width = page_width;

	if (output_text_line_handle == NULL)
		return false;

	output_text_line_stack = output_text_line_create(page_width, 0);

	return true;
}

/**
 * Close the current text output file.
 */

void output_text_line_close(void)
{
	/* Close the output file. */

	if (output_text_line_handle != NULL) {
		fclose(output_text_line_handle);
		output_text_line_handle = NULL;
	}

	/* Clear the line stack. */

	while (output_text_line_stack != NULL)
		output_text_line_pop();
}

/**
 * Push a new output line on to the stack, insetting it the given number
 * of character positions relative to the start of the file line.
 * 
 * \param inset		The number of character positions to inset the line.
 * \return		TRUE if successful; else FALSE.
 */

bool output_text_line_push_absolute(int inset)
{
	int left_margin = 0;
	struct output_text_line *line = NULL;

	line = output_text_line_create(output_text_line_page_width, inset);
	if (line == NULL)
		return false;

	if (output_text_line_stack != NULL)
		left_margin = output_text_line_stack->left_margin;

	if (inset < left_margin)
		msg_report(MSG_TEXT_LINE_HANGING, inset, left_margin);

	line->next = output_text_line_stack;
	output_text_line_stack = line;

	return true;
}

/**
 * Push a new output line on to the stack, insetting it the given number
 * of character positions from the left margin of the line below it on
 * the stack.
 * 
 * \param left		The number of character positions to inset the line
 *			from the parent on the left.
 * \param right		The number of character positions to inset the line
 *			from the parent on the right.
 * \return		TRUE if successful; else FALSE.
 */

bool output_text_line_push(int left, int right)
{
	int left_margin = 0, right_margin = output_text_line_page_width;
	struct output_text_line *line = NULL;

	if (output_text_line_stack != NULL)
		left_margin = output_text_line_stack->left_margin;

	line = output_text_line_create(right_margin - right, left_margin + left);
	if (line == NULL)
		return false;

	line->next = output_text_line_stack;
	output_text_line_stack = line;

	return true;
}

/**
 * Push a new output line on to the stack, insetting it the given number
 * of character positions from the left margin of a specified column on
 * the line below it on the stack.
 *
 * \param column	The column to align the margin with.
 * \param left		The number of character positions to inset the line
 *			from the parent column on the left.
 * \param right		The number of character positions to inset the line
 *			from the parent on the right.
 * \return		TRUE if successful; else FALSE.
 */

bool output_text_line_push_to_column(int column, int left, int right)
{
	int left_margin = 0, right_margin = output_text_line_page_width;
	struct output_text_line *line = NULL;
	struct output_text_line_column	*col = NULL;

	if (output_text_line_stack != NULL) {
		col = output_text_line_find_column(output_text_line_stack, column);
		if (col == NULL) {
			msg_report(MSG_TEXT_LINE_BAD_COL_REF);
			return false;
		}

		left_margin = col->start;
	}

	line = output_text_line_create(right_margin - right, left_margin + left);
	if (line == NULL)
		return false;

	line->next = output_text_line_stack;
	output_text_line_stack = line;

	return true;
}

/**
 * Pop a line from the top of the line stack and dispose of it.
 *
 * \return		TRUE if successful; else FALSE.
 */

bool output_text_line_pop(void)
{
	struct output_text_line *line;

	if (output_text_line_stack == NULL)
		return false;

	line = output_text_line_stack;
	output_text_line_stack = line->next;

	output_text_line_destroy(line);

	return true;
}

/**
 * Create a new text line output instance.
 *
 * \param page_width	The page width, in characters.
 * \param left_margin	The line left margin, in characters.
 * \return		Pointer to the new line block, or NULL on failure.
 */

static struct output_text_line *output_text_line_create(int page_width, int left_margin)
{
	struct output_text_line *line = NULL;

	line = malloc(sizeof(struct output_text_line));
	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_MEM);
		return NULL;
	}

	line->columns = NULL;
	line->page_width = page_width;
	line->left_margin = left_margin;
	line->next = NULL;
	line->is_prepared = false;
	line->has_content = false;

	return line;
}

/**
 * Destroy a text line output instance.
 *
 * \param *line		The line output instance to destroy.
 */

static void output_text_line_destroy(struct output_text_line *line)
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
 * Test the line at the top of the stack to see if it has been prepaed.
 *
 * \return		TRUE if the line is prepared; else false.
 */

bool output_text_line_is_prepared(void)
{
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	return line->is_prepared;
}

/**
 * Test the line at the top of the stack to see if it has content.
 *
 * \return		TRUE if the line has content; else false.
 */

bool output_text_line_has_content(void)
{
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	return line->is_prepared && line->has_content;
}

/**
 * Add a column to the text line at the top of the output stack.
 *
 * \param margin	The margin before the column, in characters.
 * \param width		The width of the column, in characters.
 * \return		TRUE on success; FALSE on failure.
 */

bool output_text_line_add_column(int margin, int width)
{
	struct output_text_line_column	*column = NULL, *previous = NULL;
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	column = malloc(sizeof(struct output_text_line_column));
	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_COL_MEM);
		return false;
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

	column->requested_margin = margin;
	column->requested_width = width;
	column->parent = line;
	column->flags = OUTPUT_TEXT_LINE_COLUMN_FLAGS_NONE;
	column->width = 0;
	column->text = NULL;
	column->size = 0;
	column->write_ptr = NULL;
	column->written_width = 0;
	column->hanging_indent = 0;
	column->blank_rows = 0;
	column->next = NULL;

	return output_text_line_update_column_memory(column);
}

/**
 * Set the flags for a column in the line at the top of the stack.
 *
 * \param column	The index of the column to update.
 * \param flags		The new column flags.
 * \return		True on success; False on error.
 */

bool output_text_line_set_column_flags(int column, enum output_text_line_column_flags flags)
{
	struct output_text_line_column	*col = NULL;
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	col = output_text_line_find_column(line, column);
	if (col == NULL)
		return false;

	col->flags = flags;

	return true;
}

/**
 * Reset the line at the top of the stack, ready for a new block to be built.
 *
 * \return		True on success; False on error.
 */

bool output_text_line_reset(void)
{
	struct output_text_line_column *column = NULL;
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	/* Initialise the write pointer, written width count, row counts
	 * and hanging indents.
	 */

	column = line->columns;

	while (column != NULL) {
		column->write_ptr = column->text;

		if (column->text != NULL && column->size > 0)
			column->text[0] = '\0';

		column->written_width = 0;
		column->blank_rows = 0;
		column->hanging_indent = 0;

		column = column->next;
	}

	/* Calculate the column widths. */

	if (!output_text_line_set_column_widths(line))
		return false;

	/* Mark the line as prepared. */

	line->is_prepared = true;

	return true;
}

/**
 * Set the display width of a column to the length of the text that
 * is in its output buffer.
 *
 * \param column	The index of the column to be updated.
 * \return		True if successful; else False.
 */

bool output_text_line_set_column_width(int column)
{
	struct output_text_line_column *col = NULL;
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	col = output_text_line_find_column(line, column);
	if (col == NULL)
		return false;
		
	col->requested_width = encoding_get_utf8_string_length(col->text);

	/* Recalculate the column widths. */

	if (!output_text_line_set_column_widths(line))
		return false;

	return true;
}

/**
 * Set the widths of the columns in a line.
 *
 * \param *line		The line instance to work on.
 * \return		True if successful; else False.
 */

static bool output_text_line_set_column_widths(struct output_text_line *line)
{
	struct output_text_line_column *column = NULL, *previous = NULL;
	int used_width = 0, free_width = 0, auto_width = 0, auto_columns = 0;
	bool success = true;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	/* Count up the known column widths. */

	used_width = line->left_margin;
	column = line->columns;

	while (column != NULL) {
		used_width += column->requested_margin;
		if (column->requested_width >= 0)
			used_width += column->requested_width;
		else
			auto_columns++;

		column = column->next;
	}

	free_width = line->page_width - used_width;
	auto_width = (auto_columns == 0) ? free_width : free_width / auto_columns;

	/* Assign the positions and widths. */

	column = line->columns;

	while (column != NULL) {
		/* Set the start position. */

		if (previous == NULL)
			column->start = line->left_margin + column->requested_margin;
		else
			column->start = previous->start + previous->width + column->requested_margin;

		/* Set the width. */

		if (column->requested_width >= 0) {
			column->width = column->requested_width;
		} else {
			column->width = (auto_columns > 1) ? auto_width : free_width;
			free_width -= auto_width;
		}

		if (column->start + column->width > line->page_width) {
			msg_report(MSG_TEXT_LINE_TOO_WIDE);
			return false;
		}

		if (column->hanging_indent > column->width) {
			column->hanging_indent = 0;
			msg_report(MSG_TEXT_LINE_HANGING_TOO_LATE);
			success = false;
		}

		previous = column;
		column = column->next;
	}

	return success;
}

/**
 * Add text to a column at the top of the stack, to be proessed when
 * the line is complete.
 *
 * \param column	The index of the column to add to.
 * \param *text		Pointer to the text to be added.
 * \return		True on success; False on error.
 */

bool output_text_line_add_text(int column, char *text)
{
	struct output_text_line_column	*col = NULL;
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	if (line->is_prepared == false) {
		msg_report(MSG_TEXT_LINE_UNPREPARED);
		return false;
	}

	col = output_text_line_find_column(line, column);
	if (col == NULL)
		return false;

	line->has_content = true;

	return output_text_line_add_column_text(col, text);
}

/**
 * Add text to a column's text buffer, expanding the memory as required.
 *
 * \param *column	The column index to add the text to.
 * \param *text		Pointer to the text to add.
 * \return		True on success; False on error.
 */

static bool output_text_line_add_column_text(struct output_text_line_column *column, char *text)
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
			msg_report(MSG_TEXT_LINE_NO_MEM);
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
	char *new;

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
 * Set a hanging indent in a column, based on the current text
 * width. This will be ignored if it falls outside of the width
 * of the column.
 *
 * The algorithm will either count spaces and stop after including
 * the last one, or include the whole text.
 *
 * \param column	The index of the column to update.
 * \param spaces	The number of spaces to include, or zero
 *			for all of the text.
 * \return		True on success; False on error.
 */

bool output_text_line_set_hanging_indent(int column, int spaces)
{
	struct output_text_line_column	*col = NULL;
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	if (line->is_prepared == false) {
		msg_report(MSG_TEXT_LINE_UNPREPARED);
		return false;
	}

	col = output_text_line_find_column(line, column);
	if (col == NULL)
		return false;

	line->has_content = true;

	return output_text_line_set_column_hanging_indent(col, spaces);
}

/**
 * Set a hanging indent in a column, based on the current text
 * width. This will be ignored if it falls outside of the width
 * of the column.
 *
 * \param column	The column instance to be updated.
 * \return		True on success; False on error.
 */

static bool output_text_line_set_column_hanging_indent(struct output_text_line_column *column, int spaces)
{
	int c, width;
	char *text;

	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	if (column->flags & OUTPUT_TEXT_LINE_COLUMN_FLAGS_RIGHT || column->flags & OUTPUT_TEXT_LINE_COLUMN_FLAGS_RIGHT) {
		msg_report(MSG_TEXT_LINE_HANGING_LEFT);
		return false;
	}

	/* Calculate the width. Either do it the hard way if we're counting
	 * spaces, or just pass it to the encoding module to sort out.
	 */

	if (spaces > 0) {
		text = column->text;
		width = 0;

		do {
			c = encoding_parse_utf8_string(&text);
			if (c == ' ' && spaces > 0)
				spaces--;

			if (c != '\0')
				width++;
		} while (c != '\0' && spaces > 0);
	} else {
		width = encoding_get_utf8_string_length(column->text);
	}

	if (width >= column->width) {
		msg_report(MSG_TEXT_LINE_HANGING_TOO_LATE);
		return false;
	}

	column->hanging_indent = width;

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
 * Write the line at the top of the stack to the output.
 *
 * \param underline	True to underline the text.
 * \param align_bottom	True to align the text to the bottom
 *			of the columns in the row.
 * \return		True on success; False on error.
 */

bool output_text_line_write(bool underline, bool align_bottom)
{
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	if (align_bottom && !output_text_line_size_columns(line))
		return false;

	/* Write the line content to the file. */

	do {
		line->complete = true;

		if (!output_text_line_write_line(line, false))
			return false;
	} while (line->complete == false);

	/* Perform any underlining that's required. */

	if (underline == true) {
		if (!output_text_line_write_line(line, true))
			return false;
	}

	/* Mark the line as processed. */

	line->has_content = false;
	line->is_prepared = false;

	return true;
}

/**
 * Calculate the size in rows of the text when formatted into
 * each of the columns, and update blank_rows to be the number
 * of spare rows in each column.
 *
 * \param *line		The current line instance.
 */

static bool output_text_line_size_columns(struct output_text_line *line)
{
	struct output_text_line_column *column = NULL;
	int max_count = 0;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	/* Count the lines taken by the text in the column when formatted. */

	column = line->columns;

	while (column != NULL) 
	{
		column->blank_rows = 0;

		while (column->write_ptr != NULL) {
			if (!output_text_line_write_column(column, true))
				return false;

			column->blank_rows++;
		}

		column->write_ptr = column->text;

		if (column->blank_rows > max_count)
			max_count = column->blank_rows;

		column = column->next;
	}

	/* Calculate the blank rows in each column relative to the longest one. */

	column = line->columns;

	while (column != NULL) 
	{
		column->blank_rows = max_count - column->blank_rows;

		column = column->next;
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
	struct output_text_line_column *column = NULL;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	line->position = 0;

	column = line->columns;

	while (column != NULL) {
		if (column->blank_rows > 0) {
			column->blank_rows--;
		} else {
			if (underline == true) {
				if (!output_text_line_write_column_underline(column))
					return false;
			} else {
				if (!output_text_line_write_column(column, false))
					return false;
			}
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
 * \param trial		Is this a trial run?
 * \return		True on success; False on error.
 */

static bool output_text_line_write_column(struct output_text_line_column *column, bool trial)
{
	int	available_width, written_width, breakpoint, c;
	char	*scan_ptr;
	bool	hyphenate = false, skip = false, complete = false, preformat = false;

	if (column == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_COL_REF);
		return false;
	}

	/* The line is already fully written. */

	if (column->write_ptr == NULL)
		return true;

	/* If there's no width, complete the line and exit. */

	available_width = (column->write_ptr == column->text) ?
			column->width : column->width - column->hanging_indent;

	if (available_width <= 0) {
		column->write_ptr = NULL;
		return true;
	}

	/* Find the next chunk of string to be written out. */

	if (column->flags & OUTPUT_TEXT_LINE_COLUMN_FLAGS_PREFORMAT)
		preformat = true;

	written_width = 0;
	breakpoint = 0;

	scan_ptr = column->write_ptr;

	c = encoding_parse_utf8_string(&scan_ptr);

	while (c != '\0' && c != '\n' && written_width < available_width) {
		written_width++;

		/* If this is a possible breakpoint... */

		if (c == ' ' || c == '-') {
			

			if (preformat == false && c == ' ' && written_width == 1) {
				/* If the first character of the column is a space, we skip it. */
				written_width = 0;
				column->write_ptr = scan_ptr;
			} else {
				/* Otherwise, we remember the breakpoint. */
				breakpoint = (c == '-') ? written_width : written_width - 1;
			}
		}

		c = encoding_parse_utf8_string(&scan_ptr);
	}

	/* If there's nothing to output, flag the column as complete and exit. */

	if (c != '\n' && written_width == 0) {
		column->write_ptr = NULL;
		return true;
	}

	/* We've reached the end of the string. */

	if (c == '\0') {
		breakpoint = written_width;
		complete = true;
	}

	/* This is a forced line termination. Update the breakpoint and step
	 * past the terminator.
	 */

	if (c == '\n') {
		breakpoint = written_width;
		skip = true;
	}

	/* No breakpoint was found, and the line isn't done. Drop the
	 * last character to make space for a hyphen.
	 */

	else if (breakpoint == 0) {
		if (c == '-' || available_width < OUTPUT_TEXT_LINE_HYPHENATION_LIMIT) {
			/* If the next character is a hyphen, use it at the start
			 * of the following line. Or, if there's no space to hyphenate,
			 * just write the text as it is. Otherwise...
			 */
			breakpoint = written_width;
		} else if (available_width > 1) {
			/* ...back off one and hyphenate. */
			breakpoint = written_width - 1;
			hyphenate = true;
		}
	}

	/* Track the maximum line length seen. */

	if (breakpoint > column->written_width)
		column->written_width = (hyphenate) ? breakpoint + 1 : breakpoint;

	/* Write the line of text. */

	if (trial == false) {
		if (!output_text_line_pad_to_column(column))
			return false;

		if (column->flags & OUTPUT_TEXT_LINE_COLUMN_FLAGS_RIGHT) {
			if (!output_text_line_pad_to_position(column->parent, column->start + (column->width - breakpoint)))
				return false;
		} else if (column->flags & OUTPUT_TEXT_LINE_COLUMN_FLAGS_CENTRE) {
			if (!output_text_line_pad_to_position(column->parent, column->start + (column->width - breakpoint) / 2))
				return false;
		} else {
			if (!output_text_line_pad_to_position(column->parent, column->start + (column->width - available_width)))
				return false;
		}

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
	} else {
		while (breakpoint-- > 0)
			encoding_parse_utf8_string(&(column->write_ptr));
	}

	/* Skip past any terminating newline. */

	if (skip)
		encoding_parse_utf8_string(&(column->write_ptr));

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
	int i;

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

	return output_text_line_pad_to_position(column->parent, column->start);
}

/**
 * Pad a line out to a given position, using spaces.
 *
 * \param *line		The line instance to pad.
 * \return		True if successful; False on error.
 */

static bool output_text_line_pad_to_position(struct output_text_line *line, int position)
{
	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	while (line->position < position) {
		if (!output_text_line_write_char(line, ' '))
			return false;
	}

	return true;
}

/**
 * Write a ruleoff to the output, from the current line's left margin to
 * the extent of the page width.
 *
 * \param unicode	The unicode character to use for the ruleoff.
 * \return		True if successful; False on error.
 */

bool output_text_line_write_ruleoff(int unicode)
{
	int position = 0;
	char buffer[ENCODING_CHAR_BUF_LEN];
	struct output_text_line *line = output_text_line_stack;

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	encoding_write_unicode_char(buffer, ENCODING_CHAR_BUF_LEN, ' ');

	while (position < line->left_margin) {
		if (fputs(buffer, output_text_line_handle) == EOF) {
			msg_report(MSG_WRITE_FAILED);
			return false;
		}

		position++;
	}

	encoding_write_unicode_char(buffer, ENCODING_CHAR_BUF_LEN, unicode);

	while (position < line->page_width) {
		if (fputs(buffer, output_text_line_handle) == EOF) {
			msg_report(MSG_WRITE_FAILED);
			return false;
		}

		position++;
	}

	return output_text_line_write_newline();
}

/**
 * Write a line ending sequence to the output.
 *
 * \return		True if successful; False on error.
 */

bool output_text_line_write_newline(void)
{
	const char *line_end = NULL;

	if (output_text_line_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	line_end = encoding_get_newline();
	if (line_end == NULL) {
		msg_report(MSG_TEXT_NO_LINE_END);
		return false;
	}

	if (fputs(line_end, output_text_line_handle) == EOF) {
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
	char buffer[ENCODING_CHAR_BUF_LEN];

	if (line == NULL) {
		msg_report(MSG_TEXT_LINE_BAD_REF);
		return false;
	}

	if (output_text_line_handle == NULL) {
		msg_report(MSG_WRITE_NO_FILE);
		return false;
	}

	encoding_write_unicode_char(buffer, ENCODING_CHAR_BUF_LEN, unicode);

	if (fputs(buffer, output_text_line_handle) == EOF) {
		msg_report(MSG_WRITE_FAILED);
		return false;
	}

	line->position++;

	return true;
}

