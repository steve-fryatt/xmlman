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
 * \file output_text.c
 *
 * Text Output Engine, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

#include "output_text.h"

#include "encoding.h"
#include "manual_data.h"
#include "output_text_line.h"


/**
 * Output a manual in text form.
 *
 * \param *manual	The manual to be output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_text(struct manual_data *manual)
{
	struct output_text_line	*line;

	encoding_select_table(ENCODING_TARGET_ACORN_LATIN1);

	line = output_text_line_create();

	output_text_line_add_column(line, 3, 5);
	output_text_line_add_column(line, 3, 65);

	output_text_line_reset(line);

	output_text_line_write(line);

	output_text_line_destroy(line);

	return true;
}

