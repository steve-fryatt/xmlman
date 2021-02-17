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
 * The maximum tag or entity name length. */

#define PARSE_XML_MAX_NAME_LEN 64

/**
 * The handle of the curren file, or NULL.
 */

static FILE *parse_xml_handle = NULL;

/**
 * Buffer to hold the name of the current tag or entity.
 */

static char parse_xml_object_name[PARSE_XML_MAX_NAME_LEN];

/* Static Function Prototypes. */

static bool parse_xml_read_tag(char c);
static bool parse_xml_read_entity(char c);

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
		parse_xml_read_entity(c);
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

static bool parse_xml_read_tag(char c)
{

}

static bool parse_xml_read_entity(char c)
{
	int len = 0;

	if (c != '&' || parse_xml_handle == NULL)
		return false;

	while (c != ';' && c != EOF) {
		c = fgetc(parse_xml_handle);

		if (c != ';' && c != EOF && len < PARSE_XML_MAX_NAME_LEN)
			parse_xml_object_name[len++] = c;
	}

	if (c != ';') {
		// Not Terminated.
		return false;
	}

	if (len >= PARSE_XML_MAX_NAME_LEN) {
		// Too Long!
		return false;
	}

	parse_xml_object_name[len] = '\0';

	printf("Found entity: %s\n", parse_xml_object_name);

	return true;
}
