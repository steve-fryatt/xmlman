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
 * \file parse.c
 *
 * XML Parser, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>

#include "parse_element.h"
#include "parse_stack.h"

/* Static Function Prototypes. */

static void parse_process_top_level_node(xmlTextReaderPtr reader);
static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);


/**
 * Parse an XML file and its descendents.
 *
 * \param *filename	The name of the file to parse.
 */

void parse_file(char *filename)
{
	xmlTextReaderPtr	reader;
	int			ret;

	reader = xmlReaderForFile(filename, NULL, XML_PARSE_DTDATTR | XML_PARSE_DTDVALID);

	if (reader == NULL) {
		fprintf(stderr, "Unable to open %s\n", filename);
		return;
	}

	xmlTextReaderSetErrorHandler(reader, parse_error_handler, NULL);

	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		parse_process_top_level_node(reader);
		ret = xmlTextReaderRead(reader);
	}

	if (xmlTextReaderIsValid(reader) != 1)
		fprintf(stderr, "Document %s does not validate\n", filename);

	xmlFreeTextReader(reader);

	if (ret != 0)
		fprintf(stderr, "%s : failed to parse\n", filename);
}




static void parse_process_top_level_node(xmlTextReaderPtr reader)
{
	xmlReaderTypes		type;
	enum parse_element_type	element;

	type = xmlTextReaderNodeType(reader);
	printf("Type: %d\n", type);
	if (type != XML_READER_TYPE_ELEMENT)
		return;

	element = parse_find_element_type(reader);
	switch (element) {
	case PARSE_ELEMENT_MANUAL:
		printf("Found manual\n");
		break;
	}

/*
	const		xmlChar *name, *value;

	name = xmlTextReaderConstName(reader);
	if (name == NULL)
		name = BAD_CAST "--";


	value = xmlTextReaderConstValue(reader);


	printf("%d %d %s %d %d", 
		xmlTextReaderDepth(reader),
		type,
		name,
		xmlTextReaderIsEmptyElement(reader),
		xmlTextReaderHasValue(reader));

	if (value == NULL) {
		printf("\n");
	} else {
		if (xmlStrlen(value) > 40)
			printf(" %.40s...\n", value);
		else
			printf(" %s\n", value);
	}
*/
}




static void parse_error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator)
{
	int	line;

	line = xmlTextReaderLocatorLineNumber(locator);

	printf("Error at line %d: %s", line, msg);
}

