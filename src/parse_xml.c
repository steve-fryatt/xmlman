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

#include "manual_entity.h"
#include "msg.h"
#include "parse_element.h"
#include "parse_xml.h"

/**
 * The maximum tag or entity name length. */

#define PARSE_XML_MAX_NAME_LEN 64

/**
 * The handle of the curren file, or NULL.
 */

static FILE *parse_xml_handle = NULL;

/**
 * The current type of object.
 */

static enum parse_xml_result parse_xml_current_mode = PARSE_XML_RESULT_ERROR;

/**
 * Buffer to hold the name of the current tag or entity.
 */

static char parse_xml_object_name[PARSE_XML_MAX_NAME_LEN];

/* Static Function Prototypes. */

static void parse_xml_read_markup(char c);
static void parse_xml_read_comment(void);
static void parse_xml_read_element(char c);
static void parse_xml_read_entity(char c);
static bool parse_xml_match_ahead(const char *text);

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

	parse_xml_current_mode = PARSE_XML_RESULT_ERROR;

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


/**
 * Parse the next chunk from the current file.
 * 
 * \return		Details of the chunk.
 */

enum parse_xml_result parse_xml_read_next_chunk(void)
{
	int c;

	if (parse_xml_handle == NULL || feof(parse_xml_handle))
		return PARSE_XML_RESULT_ERROR;

	c = fgetc(parse_xml_handle);

	switch (c) {
	case EOF:
		parse_xml_current_mode = PARSE_XML_RESULT_EOF;
		break;

	case '<':
		parse_xml_read_markup(c);
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

		printf("\n");

		parse_xml_current_mode = PARSE_XML_RESULT_TEXT;
		break;
	}

	return parse_xml_current_mode;
}


/**
 * Read the details of the current element parsed from
 * the file.
 * 
 * \return		The element token, or PARSE_ELEMENT_NONE.
 */

enum parse_element_type parse_xml_get_element(void)
{
	if (parse_xml_current_mode != PARSE_XML_RESULT_TAG_START &&
			parse_xml_current_mode != PARSE_XML_RESULT_TAG_EMPTY &&
			parse_xml_current_mode != PARSE_XML_RESULT_TAG_END)
		return PARSE_ELEMENT_NONE;
	
	return parse_element_find_type(parse_xml_object_name);
}


/**
 * Read the details of the current entity parsed from
 * the file.
 * 
 * \return		The entity token, or MANUAL_ENTITY_NONE.
 */

enum manual_entity_type parse_xml_get_entity(void)
{
	if (parse_xml_current_mode != PARSE_XML_RESULT_TAG_ENTITY)
		return MANUAL_ENTITY_NONE;
	
	return manual_entity_find_type(parse_xml_object_name);
}


/**
 * Process a markup block from the file.
 * 
 * \param c		The first character of the markup sequence.
 */

static void parse_xml_read_markup(char c)
{
	/* Tags must start with a <; we shouldn't be here otherwise. */

	if (c != '<' || parse_xml_handle == NULL) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Check for special characters at the start of the tag. There's no
	 * need to test for EOF, as we'll fall through to the open tag handler
	 * later on.
	 */

	c = fgetc(parse_xml_handle);

	if (c == '!') {
		if (parse_xml_match_ahead("--")) {
			parse_xml_read_comment();
			return;
		} else {
			/* TODO CDATA, DOCTYPE, etc. */

			parse_xml_current_mode = PARSE_XML_RESULT_OTHER;
		}
	} else if (c == '?') {
		/* TODO Processing Instruction */
		parse_xml_current_mode = PARSE_XML_RESULT_OTHER;
	} else {
		parse_xml_read_element(c);
	}
}


/**
 * Process an element block from the file.
 * 
 * \param c		The first character of the tag sequence.
 */

static void parse_xml_read_element(char c)
{
	int len = 0;
	bool previous_was_slash = false;

	/* Assume an opening tag until we learn otherwise. */

	parse_xml_current_mode = PARSE_XML_RESULT_TAG_START;

	/* If the tag starts with a /, it's a closing tag. */

	if (c == '/') {
		parse_xml_current_mode = PARSE_XML_RESULT_TAG_END;
		c = fgetc(parse_xml_handle);
	}

	/* Copy the tag name until there's whitespace or a /. */

	do {
		if (c != EOF && c!= '>' && c != '/' && !isspace(c) && len < PARSE_XML_MAX_NAME_LEN)
			parse_xml_object_name[len++] = c;

		c = fgetc(parse_xml_handle);
	} while (c != EOF && c != '>' && c != '/' && !isspace(c));

	parse_xml_object_name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

	/* The tag wasn't terminated. */

	if (c == EOF) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_TAG, parse_xml_object_name);
		return;
	}

	/* The name is too long. */

	if (len >= PARSE_XML_MAX_NAME_LEN) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_TAG_TOO_LONG, parse_xml_object_name);
		return;
	}

	/* Terminate the valid name. */

	parse_xml_object_name[len] = '\0';

	previous_was_slash = (c == '/') ? true : false;

	/* TODO Run off the attributes! */

	while (c != EOF && c != '>') {
		c = fgetc(parse_xml_handle);
		if (c != '>')
			previous_was_slash = (c == '/') ? true : false;
	}

	/* The tag wasn't terminated. */

	if (c == EOF) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_TAG, parse_xml_object_name);
		return;
	}

	/* If the tag ended with a /, it was a self-closing tag. */

	if (previous_was_slash) {
		if (parse_xml_current_mode == PARSE_XML_RESULT_TAG_START) {
			parse_xml_current_mode = PARSE_XML_RESULT_TAG_EMPTY;
		} else {
			parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
			msg_report(MSG_PARSE_TAG_CLOSE_CONFLICT, parse_xml_object_name);
		}
	}
}


/**
 * Process a comment sequence from the file.
 */

static void parse_xml_read_comment(void)
{
	int c, dashes = 0;

	do {
		c = fgetc(parse_xml_handle);

		if (c == '-')
			dashes++;
		else if (dashes == 2 && c == '>')
			break;
		else
			dashes = 0;
	} while (c != EOF);

	if (c == EOF) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_COMMENT);
		return;
	}

	parse_xml_current_mode = PARSE_XML_RESULT_COMMENT;
}


/**
 * Process an entity from the file.
 * 
 * \param c		The first character of the entity sequence.
 */

static void parse_xml_read_entity(char c)
{
	int len = 0;

	/* Entities must start with &; we shouldn't be here otherwise! */

	if (c != '&' || parse_xml_handle == NULL) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		return;
	}

	/* Copy the entity name until it's terminated or there's whitespace. */

	while (c != EOF && c != ';' && !isspace(c)) {
		c = fgetc(parse_xml_handle);

		if (c != EOF && c != ';' && !isspace(c) && len < PARSE_XML_MAX_NAME_LEN)
			parse_xml_object_name[len++] = c;
	}

	parse_xml_object_name[PARSE_XML_MAX_NAME_LEN - 1] = '\0';

	/* If the character isn't ;, the entity wasn't terminated. */

	if (c != ';') {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_UNTERMINATED_ENTITY, parse_xml_object_name);
		return;
	}

	/* The name is too long. */

	if (len >= PARSE_XML_MAX_NAME_LEN) {
		parse_xml_current_mode = PARSE_XML_RESULT_ERROR;
		msg_report(MSG_PARSE_ENTITY_TOO_LONG, parse_xml_object_name);
		return;
	}

	/* Terminate the valid name. */

	parse_xml_object_name[len] = '\0';

	parse_xml_current_mode = PARSE_XML_RESULT_TAG_ENTITY;
}


/**
 * Test the next data in the file against a string. Leave the file
 * pointer after a match, or reset it if one isn't found.
 * 
 * \param *text		Pointer to the string to match.
 * \return		True if the string matches; else false.
 */

static bool parse_xml_match_ahead(const char *text)
{
	int c;
	long start;

	if (text == NULL || parse_xml_handle == NULL)
		return false;

	/* Remember where we started. */

	start = ftell(parse_xml_handle);

	/* Match through the required string. */

	while (*text != '\0') {
		c = fgetc(parse_xml_handle);

		if (c == EOF || c != *text)
			break;

		text++;
	}

	/* If there wasn't a match, reset the file pointer. */

	if (*text != '\0')
		fseek(parse_xml_handle, start, SEEK_SET);

	return (*text == '\0') ? true : false;
}

/* When parsing text (& attribute values), store start ptr and length. Then use
 * generic copy routine to copy into new malloc block for the client.
 * 
 * tag and entity names can go into a fixed buffer, as we know in advance how long
 * the longest is. Look up, and return enum.
 * 
 * Return open, close, complete tag, entity, text, whitespace, eof, error codes
 * 
 * Parsting tags, we know how many entities there may be, so pre-process into
 * a fixed, static array and allow each to be called up and flagged as used. The
 * client can then trigger an "unused" scan after it has processed what it needs.
 * 
 * Flattening Whitespace appears to be handed by the encoding module?
 * 
 * We can then do a parse using recursive functions, and lose the stack?
 */