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
 * \file encoding.c
 *
 * Text Encloding Support, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/xmlreader.h>

#include "encoding.h"



/**
 * Flatten down the white space in a text string, so that multiple spaces
 * and newlines become a single ASCII space. The supplied buffer is
 * assumed to be zero terminated, and its contents will be updated.
 *
 * \param *text			The text to be flattened.
 */

void encoding_flatten_whitespace(xmlChar *text)
{
	unsigned char	*head, *tail;
	bool		space = false, whitespace = false;

	head = (unsigned char*) text;
	tail = head;

	while (*head != '\0') {
		space = (*head == '\t' || *head == '\r' || *head == '\n' || *head == ' ');

		if (space && whitespace) {
			head++;
		} else if (space == true) {
			*tail++ = ' ';
			head++;
			whitespace = space;
		} else {
			*tail++ = *head++;
			whitespace = space;
		}
	}

	*tail = '\0';
}

