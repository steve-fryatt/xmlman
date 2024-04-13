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
 * \file output_text_line.h
 *
 * Text Line Output Engine Interface.
 */

#ifndef XMLMAN_OUTPUT_TEXT_LINE_H
#define XMLMAN_OUTPUT_TEXT_LINE_H

#include <stdbool.h>
#include "filename.h"

/**
 * Set the column width to the available space.
 */

#define OUTPUT_TEXT_LINE_FULL_WIDTH (-1)

/**
 * Flags relating to output line columns.
 */

enum output_text_line_column_flags {
	OUTPUT_TEXT_LINE_COLUMN_FLAGS_NONE = 0,
	OUTPUT_TEXT_LINE_COLUMN_FLAGS_CENTRE = 1,
	OUTPUT_TEXT_LINE_COLUMN_FLAGS_RIGHT = 2,
	OUTPUT_TEXT_LINE_COLUMN_FLAGS_PREFORMAT = 4
};

/**
 * Open a file to write the text output to.
 *
 * \param *filename	Pointer to the name of the file to write.
 * \param page_width	The page width, in characters.
 * \return		True on success; False on failure.
 */

bool output_text_line_open(struct filename *filename, int page_width);

/**
 * Close the current text output file.
 */

void output_text_line_close(void);

/**
 * Push a new output line on to the stack, insetting it the given number
 * of character positions relative to the start of the file line.
 * 
 * \param inset		The number of character positions to inset the line.
 * \return		TRUE if successful; else FALSE.
 */

bool output_text_line_push_absolute(int inset);

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

bool output_text_line_push(int left, int right);

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

bool output_text_line_push_to_column(int column, int left, int right);

/**
 * Pop a line from the top of the line stack and dispose of it.
 *
 * \return		TRUE if successful; else FALSE.
 */

bool output_text_line_pop(void);

/**
 * Test the line at the top of the stack to see if it has been prepaed.
 *
 * \return		TRUE if the line is prepared; else false.
 */

bool output_text_line_is_prepared(void);

/**
 * Test the line at the top of the stack to see if it has content.
 *
 * \return		TRUE if the line has content; else false.
 */

bool output_text_line_has_content(void);

/**
 * Add a column to the text line at the top of the output stack.
 *
 * \param margin	The margin before the column, in characters.
 * \param width		The width of the column, in characters.
 * \return		TRUE on success; FALSE on failure.
 */

bool output_text_line_add_column(int margin, int width);

/**
 * Set the flags for a column in the line at the top of the stack.
 *
 * \param column	The index of the column to update.
 * \param flags		The new column flags.
 * \return		True on success; False on error.
 */

bool output_text_line_set_column_flags(int column, enum output_text_line_column_flags flags);

/**
 * Reset the line at the top of the stack, ready for a new block to be built.
 *
 * \return		True on success; False on error.
 */

bool output_text_line_reset(void);

/**
 * Set the display width of a column to the length of the text that
 * is in its output buffer.
 *
 * \param column	The index of the column to be updated.
 * \return		True if successful; else False.
 */

bool output_text_line_set_column_width(int column);

/**
 * Add text to a column at the top of the stack, to be proessed when
 * the line is complete.
 *
 * \param column	The index of the column to add to.
 * \param *text		Pointer to the text to be added.
 * \return		True on success; False on error.
 */

bool output_text_line_add_text(int column, char *text);

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

bool output_text_line_set_hanging_indent(int column, int spaces);

/**
 * Write the line at the top of the stack to the output.
 *
 * \param underline	True to underline the text.
 * \param align_bottom	True to align the text to the bottom
 *			of the columns in the row.
 * \return		True on success; False on error.
 */

bool output_text_line_write(bool underline, bool align_bottom);

/**
 * Write a ruleoff to the output, from the current line's left margin to
 * the extent of the page width.
 *
 * \param unicode	The unicode character to use for the ruleoff.
 * \return		True if successful; False on error.
 */

bool output_text_line_write_ruleoff(int unicode);

/**
 * Write a line ending sequence to the output.
 *
 * \return		True if successful; False on error.
 */

bool output_text_line_write_newline(void);

#endif

