/* Copyright 2014-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * Error message definitions.
 *
 * NB: The order of these messages *must* match the order of the corresponding
 * entries in enum msg_type in msgs.h
 */

static struct msg_data msg_messages[] = {
	{MSG_ERROR,	"Unknown error",						true},
	{MSG_ERROR,	"Failed to parse manual",					false},
	{MSG_ERROR,	"Missing source file",						false},
	{MSG_ERROR,	"Failed to open source document '%s'",				false},
	{MSG_ERROR,	"Source document '%s' does not validate",			false},
	{MSG_ERROR,	"Source document '%s' did not parse with XML Reader",		false},
	{MSG_WARNING,	"Unexpected '<%s>' element found in %s node",			false},
	{MSG_WARNING,	"Unexpected '<%s>' closing element in block",			false},
	{MSG_ERROR,	"Unexpected stack entry type found",				false},
	{MSG_WARNING,	"Unexpected XML object type %d found in %s",			false},
	{MSG_WARNING,	"Attempt to add unexpected block of type %s",			false},
	{MSG_ERROR,	"Attempt to push incorrect %s block on to stack (expected %s)",	false},
	{MSG_ERROR,	"Missing '%s' attribute",					false},
	{MSG_ERROR,	"Unknown '%s' mode",						false},
	{MSG_ERROR,	"Attempt to read resources from unexpected block",		false},

	{MSG_ERROR,	"Attempt to add content into unexpected stack location %d",	false},

	{MSG_ERROR,	"Unknown element '<%s>'",					false},
	{MSG_ERROR,	"Failed to allocate new manual data node",			false},

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
	{MSG_ERROR,	"Content chunk not of expected type (found %s)",		false},
	{MSG_WARNING,	"Entity %s is not mapped in the selected target output",	false},
	{MSG_ERROR,	"Sections are nested too deep, at %d levels",			false},


	{MSG_ERROR,	"Out of memory creating text output line",			false},
	{MSG_ERROR,	"Out of memory creating text output column",			false},
	{MSG_ERROR,	"Out of memory allocating text storage",			false},
	{MSG_ERROR,	"Missing line instance reference",				false},
	{MSG_ERROR,	"Missing column instance reference",				false},
	{MSG_ERROR,	"No line ending selected",					false},


	{MSG_ERROR,	"Failed to create StrongHelp root structure",			false},
	{MSG_ERROR,	"Failed to calculate StrongHelp directory sizes",		false},
	{MSG_ERROR,	"Failed to create new object node block",			false},
	{MSG_ERROR,	"No active StrongHelp file block",				false},
	{MSG_ERROR,	"A '%s' object already exists in the '%s' directory",		false},


	{MSG_ERROR,	"No filename supplied",						false},
	{MSG_ERROR,	"Failed to open file '%s'",					false},
	{MSG_ERROR,	"No file open for output",					false},
	{MSG_ERROR,	"Failure to write to output file",				false},
	{MSG_ERROR,	"Unknown memory error",						false}
};

/**
 * The current error location message.
 */

static char	msg_location[MSG_MAX_LOCATION_TEXT];

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
	msg_error_reported = false;
	msg_verbose_output = verbose;
}

/**
 * Set the location for future messages, in the form of a file and line number
 * relating to the source files.
 *
 * \param line		The number of the current line.
 * \param *file		Pointer to the name of the current file.
 */

void msg_set_location(unsigned line, char *file)
{
	snprintf(msg_location, MSG_MAX_LOCATION_TEXT, "at line %u of '%s'", line, (file != NULL) ? file : "");
	msg_location[MSG_MAX_LOCATION_TEXT - 1] = '\0';
}

/**
 * Generate a message to the user, based on a range of standard message tokens
 *
 * \param type		The message to be displayed.
 * \param ...		Additional printf parameters as required by the token.
 */

void msg_report(enum msg_type type, ...)
{
	char		message[MSG_MAX_MESSAGE], *level;
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
	case MSG_INFO:
	case MSG_VERBOSE:
		level = "Info";
		break;
	case MSG_WARNING:
		level = "Warning";
		break;
	case MSG_ERROR:
		level = "Error";
		msg_error_reported = true;
		break;
	default:
		level = "Message:";
		break;
	}

	/* Output the message to screen. */

	if (msg_messages[type].show_location)
		fprintf(stderr, "%s: %s %s\n", level, message, msg_location);
	else
		fprintf(stderr, "%s: %s\n", level, message);
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

