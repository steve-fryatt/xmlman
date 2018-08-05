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

#include "output_text.h"

#include "encoding.h"

struct output_text_line_column {
	int				start;
	int				width;

	xmlChar				*text;
	bool				complete;

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
 * Create a new text line output instance.
 *
 * \return		Pointer to the new line block, or NULL on failure.
 */

struct output_text_line *output_text_line_create(void)
{
	struct output_text_line		*line = NULL;

	line = malloc(sizeof(struct output_text_line));
	if (line == NULL)
		return NULL;

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

	if (line == NULL)
		return;

	/* Free the column blocks. */

	column = line->columns;

	while (column != NULL) {
		next = column->next;
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

	if (line == NULL)
		return false;

	column = malloc(sizeof(struct output_text_line_column));
	if (column == NULL)
		return false;

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

	column->width = width;

	return true;
}


static xmlChar *Line = "The Quick Brown Fox Jumped Over The Lazy Dog. Just £10. The Quick Brown Fox Jumped Over The Lazy Dog.";


bool output_text_line_reset(struct output_text_line *line)
{
	struct output_text_line_column	*column = NULL;

	if (line == NULL)
		return false;

	column = line->columns;

	while (column != NULL) {
		column->text = Line;
		column->complete = false;

		column = column->next;
	}

	return true;
}


static bool output_text_line_write_line(struct output_text_line *line);
static bool output_text_line_write_column(struct output_text_line *line, struct output_text_line_column *column);
static bool output_text_line_write_char(struct output_text_line *line, int c);


bool output_text_line_write(struct output_text_line *line)
{
	if (line == NULL)
		return false;

	do {
		line->complete = true;

		if (!output_text_line_write_line(line))
			return false;
	} while (line->complete == false);

	return true;
}


static bool output_text_line_write_line(struct output_text_line *line)
{
	struct output_text_line_column	*column = NULL;

	if (line == NULL)
		return false;

	line->position = 0;

	column = line->columns;

	while (column != NULL) {
		if (!output_text_line_write_column(line, column))
			return false;
	
		column = column->next;
	}

	output_text_line_write_char(line, '\n');

	return true;
}

static bool output_text_line_write_column(struct output_text_line *line, struct output_text_line_column *column)
{
	int		width, breakpoint, c;
	xmlChar		*text;
	bool		hyphenate = false, complete = false;

	if (line == NULL || column == NULL)
		return false;

	if (column->text == NULL)
		return true;

	/* Find the next chunk of string to be written out. */

	width = 0;
	breakpoint = 0;

	text = column->text;

	c = encoding_parse_utf8_string(&text);

	while (c != '\0' && width <= column->width) {
		width++;

		/* If this is a possible breakpoint... */

		if (c == ' ' || c == '-') {
			/* If the first character of the column is a space, we skip it. */

			if (c == ' ' && width == 1) {
				width = 0;
				column->text = text;
			}

			/* Remember the breakpoint. */

			breakpoint = width;
		}

		c = encoding_parse_utf8_string(&text);
	}

	/* If there's nothing to output, flag the column as complete and exit. */

	if (width == 0) {
		column->text = NULL;
		return true;
	}

	/* We've reached the end of the string. */

	if (c == 0) {
		breakpoint = width;
		complete = true;
	}

	if (breakpoint == 0) {
		breakpoint = width - 1;
		hyphenate = true;
	}

	/* Pad out to the start of the column. */

	while (line->position < column->start)
		output_text_line_write_char(line, '~');

	/* Write the line of text. */

	do {
		c = (--breakpoint > 0) ? encoding_parse_utf8_string(&(column->text)) : '\0';

		if (c != '\0')
			output_text_line_write_char(line, c);
	} while (c != '\0');

	if (hyphenate)
		output_text_line_write_char(line, '-');

	if (complete == true)
		column->text = NULL;
	else
		line->complete = false;

	return true;
}



static bool output_text_line_write_char(struct output_text_line *line, int c)
{
	int	i;
	char	buffer[5];

	encoding_write_unicode_char(buffer, 5, c);

	if (fputs(buffer, stdout) == EOF)
		return false;

	line->position++;

	return true;
}

