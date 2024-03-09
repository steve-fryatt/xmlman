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
 * \file parse_xml.h
 *
 * XML Chunk Parser Interface.
 */

#ifndef XMLMAN_PARSE_XML_H
#define XMLMAN_PARSE_XML_H

#include <stdbool.h>
#include "manual_entity.h"

/**
 * The range of possible results from calling the chunk parser.
 */

enum parse_xml_result {
	PARSE_XML_RESULT_START,		/**< The parser hasn't started yet.		*/
	PARSE_XML_RESULT_ERROR,		/**< An error occurred.				*/
	PARSE_XML_RESULT_EOF,		/**< The end of the file has been reached.	*/
	PARSE_XML_RESULT_TAG_START,	/**< An opening tag.				*/
	PARSE_XML_RESULT_TAG_END,	/**< A closing tag.				*/
	PARSE_XML_RESULT_TAG_EMPTY,	/**< An empty element tag.			*/
	PARSE_XML_RESULT_TAG_ENTITY,	/**< A character element.			*/
	PARSE_XML_RESULT_TEXT,		/**< A block of text.				*/
	PARSE_XML_RESULT_WHITESPACE,	/**< A block of white space.			*/
	PARSE_XML_RESULT_COMMENT,	/**< A comment.					*/
	PARSE_XML_RESULT_OTHER		/**< Another element, which wasn't recognised.	*/
};

/**
 * Structure holding an instance of the parser.
 */

struct parse_xml_block;

/**
 * Open a new file in the XML parser.
 *
 * \param *filename	The name of the file to open.
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct parse_xml_block *parse_xml_open_file(char *filename);

/**
 * Close a file in the XML parser.
 *
 * \param *instance	Pointer to the instance to be closed.
 * \return		True if successful; else False.
 */

void parse_xml_close_file(struct parse_xml_block *instance);

/**
 * Set the parser state to error.
 *
 * \param *instance	Pointer to the instance to update.
 * \return		An error state.
 */

enum parse_xml_result parse_xml_set_error(struct parse_xml_block *instance);

/**
 * Parse the next chunk from the specified file.
 * 
 * \param *instance	Pointer to the instance to be read.
 * \return		Details of the chunk.
 */

enum parse_xml_result parse_xml_read_next_chunk(struct parse_xml_block *instance);

/**
 * Return a copy of the current text block parsed from
 * the file.
 * 
 * \param *instance		Pointer to the instance to be used.
 * \return			Pointer to a copy of the block, or NULL.
 */

char *parse_xml_get_text(struct parse_xml_block *instance);

/**
 * Read the details of the current element parsed from
 * the file.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \return		The element token, or PARSE_ELEMENT_NONE.
 */

enum parse_element_type parse_xml_get_element(struct parse_xml_block *instance);

/**
 * Copy the current text block parsed from the file into a buffer
 * 
 * \param *instance		Pointer to the instance to be used.
 * \param *buffer		Pointer to a buffer to hold the value.
 * \param length		The size of the supplied buffer.
 * \return			The number of bytes copued into the buffer.
 */

size_t parse_xml_copy_text(struct parse_xml_block *instance, char *buffer, size_t length);

/**
 * Find a parser for a named attribute.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \return		A parser for the attribute value, or NULL.
 */

struct parse_xml_block *parse_xml_get_attribute_parser(struct parse_xml_block *instance, const char *name);

/**
 * Return a copy of the text from an attribute, without considering
 * the validity of any of the characters within.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \return		Pointer to a copy of the text, or NULL.
 */

char *parse_xml_get_attribute_text(struct parse_xml_block *instance, const char *name);

/**
 * Copy the text from an attribute into a buffer, without
 * considering the validity of any characters within.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \param *name		The name of the attribute to be matched.
 * \param *buffer	Pointer to a buffer to hold the value.
 * \param length	The size of the supplied buffer.
 * \return		The number of bytes copied into the buffer.
 */

size_t parse_xml_copy_attribute_text(struct parse_xml_block *instance, const char *name, char *buffer, size_t length);

/**
 * Read the details of the current entity parsed from
 * the file.
 * 
 * \param *instance	Pointer to the instance to be used.
 * \return		The entity token, or MANUAL_ENTITY_NONE.
 */

enum manual_entity_type parse_xml_get_entity(struct parse_xml_block *instance);

/**
 * Given an XML result code, return a human-readbale name.
 * 
 * \param result	The result code to look up.
 * \return		Pointer to a name for the result.
 */

char *parse_xml_get_result_name(enum parse_xml_result result);

#endif
