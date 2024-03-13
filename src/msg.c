/* Copyright 2014-2024, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file msg.c
 *
 * Status Message, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Local source headers. */

#include "msg.h"

/**
 * The maximum length allowed for a message location text.
 */

#define MSG_MAX_LOCATION_TEXT 256

/**
 * The maximum length allowed for a complete message.
 */ 

#define MSG_MAX_MESSAGE 256

/**
 * Message level definitions.
 */

enum msg_level {
	MSG_VERBOSE,			/**< A Verbose informational message.				*/
	MSG_INFO,			/**< An informational message.					*/
	MSG_WARNING,			/**< A warning message.						*/
	MSG_ERROR			/**< An error message (sets the 'error reported' flag).		*/
};

/**
 * Message definitions.
 */

struct msg_data {
	enum msg_level	level;		/**< The message level.						*/
	char		*text;		/**< Pointer to the message text.				*/
	bool		show_location;	/**< TRUE to indicate file and line number, where known.	*/
};

/**
 * Message text colours.
 */

#ifdef LINUX
#define MSG_TEXT_ERROR "\033[1;31m"
#define MSG_TEXT_WARN "\033[1;33m"
#define MSG_TEXT_INFO "\033[32m"
#define MSG_TEXT_VERBOSE "\033[30m"
#define MSG_TEXT_RESET "\033[0m"
#else
#define MSG_TEXT_ERROR ""
#define MSG_TEXT_WARN ""
#define MSG_TEXT_INFO ""
#define MSG_TEXT_VERBOSE ""
#define MSG_TEXT_RESET ""
#endif

/**
 * Error message definitions.
 *
 * NB: The order of these messages *must* match the order of the corresponding
 * entries in enum msg_type in msgs.h
 */

static struct msg_data msg_messages[] = {
	{MSG_ERROR,	"Unknown error",						true},

	{MSG_ERROR,	"Unterminated entity &%s.",					true},
	{MSG_ERROR,	"Entity name &%s... too long.",					true},
	{MSG_ERROR,	"Unterminated tag <%s.",					true},
	{MSG_ERROR,	"Tag name <%s... too long.",					true},
	{MSG_ERROR,	"<%s> tag is opening and closing in one.",			true},
	{MSG_ERROR,	"Found %c instead of closing > in <%s... tag.",			true},
	{MSG_ERROR,	"Attribute name %s... too long.",				true},
	{MSG_ERROR,	"Unterminated attribute value for %s.",				true},
	{MSG_ERROR,	"Too many attributes.",						true},
	{MSG_ERROR,	"Unterminated comment.",					true},

	{MSG_VERBOSE,	"Push %s Object (%s).",						false},
	{MSG_VERBOSE,	"Pop %s Object (%s).",						false},

	{MSG_VERBOSE,	"Parser Set Error!",						false},
	{MSG_VERBOSE,	"Parsed Text.",							false},
	{MSG_VERBOSE,	"Parsed Whitespace",						false},
	{MSG_VERBOSE,	"Parsed Opening Tag: <%s>.",					false},
	{MSG_VERBOSE,	"Parsed Self-Closing Tag: <%s />.",				false},
	{MSG_VERBOSE,	"Parsed Closing Tag: </%s>.",					false},
	{MSG_VERBOSE,	"Parsed Comment.",						false},
	{MSG_VERBOSE,	"Parsed Entity: &%s;.",						false},

	{MSG_VERBOSE,	"Dumping index table 0x%x",					false},
	{MSG_VERBOSE,	"Hash entry %d starting at 0x%x",				false},
	{MSG_VERBOSE,	"- Entry for '%s'",						false},
	{MSG_ERROR,	"Failed to store duplicate ID '%s'",				false},
	{MSG_ERROR,	"Failed to find ID '%s'",					false},
	{MSG_ERROR,	"Attempt to store id reference from invalid object type %s",	false},
	{MSG_ERROR,	"Attempt to store id target for invalid object type %s",	false},

	{MSG_INFO,	"Parsing source file '%s'...",					false},
	{MSG_ERROR,	"Failed to parse manual",					false},
	{MSG_ERROR,	"Missing source file",						false},
	{MSG_ERROR,	"Failed to open source document '%s'",				false},
	{MSG_ERROR,	"Source document '%s' did not parse with XML Reader",		false},
	{MSG_WARNING,	"Unexpected '<%s>' element found in %s node",			false},
	{MSG_WARNING,	"Unexpected '<%s>' closing element in block",			false},
	{MSG_ERROR,	"Unexpected stack entry type found",				false},
	{MSG_WARNING,	"Unexpected XML result %s found in %s",				true},
	{MSG_WARNING,	"Attempt to add unexpected block of type %s",			false},
	{MSG_ERROR,	"Attempt to push incorrect %s block on to stack (expected %s)",	false},
	{MSG_ERROR,	"Missing '%s' attribute",					false},
	{MSG_ERROR,	"Duplicate <%s> tag in <%s>",					true},
	{MSG_ERROR,	"Unknown '%s' mode",						false},
	{MSG_ERROR,	"Attempt to read resources from unexpected block",		false},
	{MSG_ERROR,	"Resource fields can only contain non-entity characters",	false},

	{MSG_ERROR,	"Attempt to add content into unexpected stack location %d",	false},

	{MSG_ERROR,	"Unknown element '<%s>'",					true},
	{MSG_ERROR,	"Failed to allocate new manual data node",			false},

	{MSG_INFO,	"Writing %s output...",						false},
	{MSG_ERROR,	"Unexpected object type found",					false},
	{MSG_ERROR,	"Stack full",							false},
	{MSG_ERROR,	"Stack error in node",						false},
	{MSG_ERROR,	"Encoding %d is out of sequence at line %d of table",		false},
	{MSG_ERROR,	"Encoding %d has target out of range at line %d of table",	false},
	{MSG_ERROR,	"Encoding %d has duplicate target %d at line %d of table",	false},
	{MSG_INFO,	"Character %d (0x%x) is not mapped to UTF8",			false},
	{MSG_WARNING,	"Unexpected UTF8 sequence",					false},
	{MSG_WARNING,	"Character %d, (0x%x) is not mapped into selected encoding",	false},
	{MSG_ERROR,	"Content block not of expected type (expected %s, found %s)",	false},
	{MSG_ERROR,	"Content chunk not of expected type (found %s in %s)",		false},
	{MSG_WARNING,	"Entity '&%s;' is not mapped in the selected target output",	false},
	{MSG_ERROR,	"Sections are nested too deep, at %d levels",			false},


	{MSG_ERROR,	"Out of memory creating text output line",			false},
	{MSG_ERROR,	"Out of memory creating text output column",			false},
	{MSG_ERROR,	"Out of memory allocating text storage",			false},
	{MSG_ERROR,	"Missing line instance reference",				false},
	{MSG_ERROR,	"Missing column instance reference",				false},
	{MSG_ERROR,	"No line ending selected",					false},

	{MSG_INFO,	"Opening image file '%s' for output", 				false},
	{MSG_ERROR,	"Failed to convert to Filecore name",				false},
	{MSG_ERROR,	"Failed to create StrongHelp root structure",			false},
	{MSG_ERROR,	"Failed to calculate StrongHelp directory sizes",		false},
	{MSG_ERROR,	"Failed to create new object node block",			false},
	{MSG_ERROR,	"No active StrongHelp file block",				false},
	{MSG_ERROR,	"A '%s' object already exists in the '%s' directory",		false},


	{MSG_INFO,	"Opened file '%s' for output",					false},
	{MSG_ERROR,	"No filename supplied",						false},
	{MSG_ERROR,	"Failed to open file '%s'",					false},
	{MSG_ERROR,	"Failed to create folder '%s'",					false},
	{MSG_ERROR,	"No file open for output",					false},
	{MSG_ERROR,	"Failure to write to output file",				false},
	{MSG_ERROR,	"Unknown memory error",						false}
};

/**
 * The current error location message.
 */

static char	msg_location[MSG_MAX_LOCATION_TEXT];

/**
 * The current error line.
 */

static unsigned	msg_line;

/**
 * Set to true if an error is reported.
 */

static bool	msg_error_reported = false;

/**
 * Set to true if verbose output is required; otherwise false.
 */

static bool	msg_verbose_output = false;

/**
 * Initialise the message system.
 *
 * \param verbose	True to generate verbose output, otherwise false.
 */

void msg_initialise(bool verbose)
{
	*msg_location = '\0';
	msg_line = 0;
	msg_error_reported = false;
	msg_verbose_output = verbose;
}

/**
 * Set the location for future messages, in the form of a file and line number
 * relating to the source files.
 *
 * \param *file		Pointer to the name of the current file.
 */

void msg_set_location(char *file)
{
	strncpy(msg_location, (file != NULL) ? file : "", MSG_MAX_LOCATION_TEXT);
	msg_location[MSG_MAX_LOCATION_TEXT - 1] = '\0';
}

/**
 * Set the location for future messages, in the form of a line number
 * relating to the source files.
 *
 * \param line		The number of the current line.
 * \param *file		Pointer to the name of the current file.
 */

void msg_set_line(unsigned line)
{
	msg_line = line;
}

/**
 * Generate a message to the user, based on a range of standard message tokens
 *
 * \param type		The message to be displayed.
 * \param ...		Additional printf parameters as required by the token.
 */

void msg_report(enum msg_type type, ...)
{
	char		message[MSG_MAX_MESSAGE], *level, *start;
	va_list		ap;

	/* Check that the message code is valid. */

	if (type < 0 || type >= MSG_MAX_MESSAGES)
		return;

	/* Discard verbose messages unless we're in verbose mode. */

	if (msg_messages[type].level == MSG_VERBOSE && !msg_verbose_output)
		return;

	/* Build the message. */

	va_start(ap, type);
	vsnprintf(message, MSG_MAX_MESSAGE, msg_messages[type].text, ap);
	va_end(ap);

	message[MSG_MAX_MESSAGE - 1] = '\0';

	/* Select the message prefix. */

	switch (msg_messages[type].level) {
	case MSG_VERBOSE:
		level = "Verbose";
		start = MSG_TEXT_VERBOSE;
		break;
	case MSG_INFO:
		level = "Info";
		start = MSG_TEXT_INFO;
		break;
	case MSG_WARNING:
		level = "Warning";
		start = MSG_TEXT_WARN;
		break;
	case MSG_ERROR:
		level = "Error";
		start = MSG_TEXT_ERROR;
		msg_error_reported = true;
		break;
	default:
		level = "Message";
		start = MSG_TEXT_VERBOSE;
		break;
	}

	/* Output the message to screen. */

	if (msg_messages[type].show_location)
		fprintf(stderr, "%s%s: %s at line %u of '%s'%s\n", start, level, message, msg_line, msg_location, MSG_TEXT_RESET);
	else
		fprintf(stderr, "%s%s: %s%s\n", start, level, message, MSG_TEXT_RESET);
}

/**
 * Indicate whether an error has been reported at any point.
 *
 * \return		True if an error has been reported; else false.
 */

bool msg_errors(void)
{
	return msg_error_reported;
}

