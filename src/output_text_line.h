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
 * \file output_text_line.h
 *
 * Text Line Output Engine Interface.
 */

#ifndef XMLMAN_OUTPUT_TEXT_LINE_H
#define XMLMAN_OUTPUT_TEXT_LINE_H

#include <stdbool.h>

#include <libxml/xmlstring.h>

struct output_text_line;

/**
 * Create a new text line output instance.
 *
 * \return		Pointer to the new line block, or NULL on failure.
 */

struct output_text_line *output_text_line_create(void);

/**
 * Destroy a text line output instance.
 *
 * \param *line		The line output instance to destroy.
 */

void output_text_line_destroy(struct output_text_line *line);

/**
 * Add a column to a text line output instance.
 *
 * \paran *line		The line output instance to add to.
 * \param margin	The margin before the column, in characters.
 * \param width		The width of the column, in characters.
 * \return		TRUE on success; FALSE on failure.
 */

bool output_text_line_add_column(struct output_text_line *line, int margin, int width);

bool output_text_line_reset(struct output_text_line *line);

bool output_text_line_write(struct output_text_line *line);

#endif
