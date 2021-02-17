/* Copyright 2018-2021, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file parse_xml.c
 *
 * XML Chunk Parser, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include "parse_xml.h"

/**
 * The handle of the curren file, or NULL.
 */

static FILE *parse_xml_handle = NULL;

/**
 * Open a new file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		True if successful; else False.
 */

bool parse_xml_open_file(char *filename)
{
	if (parse_xml_handle != NULL) {
		msg_report(MSG_PARSE_IN_PROGRESS);
		return false;
	}

	parse_xml_handle = fopen(filename, "rb");
	if (parse_xml_handle == NULL)
		return false;

	return true;
}


/**
 * Close a file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		True if successful; else False.
 */

void parse_xml_close_file(void)
{
	if (parse_xml_handle != NULL)
		fclose(parse_xml_handle);

	parse_xml_handle = NULL;
}

bool parse_xml_read_next_chunk(void)
{
	int c;

	if (parse_xml_handle == NULL || feof(parse_xml_handle))
		return false;

	c = fgetc(parse_xml_handle);

	switch (c) {
	case EOF:
		return false;

	case '<':
		printf("Tag: %c", c);
		do {
			c = fgetc(parse_xml_handle);

			if (c != EOF)
				printf("%c", c);
		} while (c != '>' && c != EOF);
		break;

	case '&':
		printf("Entity: %c", c);
		do {
			c = fgetc(parse_xml_handle);

			if (c != EOF)
				printf("%c", c);
		} while (c != ';' && c != EOF);
		break;

	default:
		printf("Text: ");
		do {
			printf("%c", c);

			c = fgetc(parse_xml_handle);
		} while (c != '<' && c != '&' && c != EOF);

		if (c != EOF)
			fseek(parse_xml_handle, -1, SEEK_CUR);
		break;
	}

	printf("\n");

	return true;
}