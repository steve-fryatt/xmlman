/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
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

/* XmlMan
 *
 * Generate documentation from XML files.
 *
 * Syntax: XmlMan [<options>]
 *
 * Options -v  - Produce verbose output
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Local source headers. */

#include "xmlman.h"
#include "args.h"
#include "encoding.h"
#include "filename.h"
#include "manual.h"
#include "manual_defines.h"
#include "manual_ids.h"
#include "msg.h"
#include "output_debug.h"
#include "output_html.h"
#include "output_strong.h"
#include "output_text.h"
#include "parse.h"

/* OSLib source headers. */

#ifdef RISCOS
#include "oslib/osfile.h"
#endif

/* Static Function Prototypes. */

static bool xmlman_process_mode(char *file, struct manual *document, enum encoding_target encoding, enum encoding_line_end line_end,
		bool (*mode)(struct manual *, struct filename *, enum encoding_target, enum encoding_line_end));

/**
 * Main program entry point.
 *
 * \param argc			The number of command line arguments.
 * \param *argv[]		The array of command line arguments.
 * \return			The outcome of the execution.
 */

int main(int argc, char *argv[])
{
	bool			param_error = false;
	bool			output_help = false;
	bool			verbose_output = false;
	bool			debug_output = false;
	struct args_option	*options;
	struct args_data	*option_data;
	char			*input_file = NULL;
	char			*out_text = NULL, *out_html = NULL, *out_strong = NULL;
	struct manual		*document = NULL;
	enum encoding_target	output_encoding = ENCODING_TARGET_NONE;
	enum encoding_line_end	output_line_end = ENCODING_LINE_END_NONE;

	/* Initialise the database structures. */

	manual_defines_initialise();
	manual_ids_initialise();

	/* Decode the command line options. */

	options = args_process_line(argc, argv,
			"source/A,verbose/S,help/S,define/KM,encoding/K,lineend/K,debug/S,text/K,html/K,strong/K");
	if (options == NULL)
		param_error = true;

	while (options != NULL) {
		if (strcmp(options->name, "help") == 0) {
			if (options->data != NULL && options->data->value.boolean == true)
				output_help = true;
		} else if (strcmp(options->name, "define") == 0) {
			if (options->data != NULL) {
				option_data = options->data;

				while (option_data != NULL) {
					if (option_data->value.string != NULL)
						manual_defines_add_entry(option_data->value.string);
					else
						param_error = true;
					option_data = option_data->next;
				}
			}
		} else if (strcmp(options->name, "source") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL)
					input_file = options->data->value.string;
			} else {
				param_error = true;
			}
		} else if (strcmp(options->name, "encoding") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL) {
					output_encoding = encoding_find_target(options->data->value.string);
					if (output_encoding == ENCODING_TARGET_NONE)
						param_error = true;
				} else {
					param_error = true;
				}
			}
		} else if (strcmp(options->name, "lineend") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL) {
					output_line_end = encoding_find_line_end(options->data->value.string);
					if (output_line_end == ENCODING_LINE_END_NONE)
						param_error = true;
				} else {
					param_error = true;
				}
			}
		} else if (strcmp(options->name, "text") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL)
					out_text = options->data->value.string;
				else
					param_error = true;
			}
		} else if (strcmp(options->name, "html") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL)
					out_html = options->data->value.string;
				else
					param_error = true;
			}
		} else if (strcmp(options->name, "strong") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL)
					out_strong = options->data->value.string;
				else
					param_error = true;
			}
		} else if (strcmp(options->name, "debug") == 0) {
			if (options->data != NULL && options->data->value.boolean == true)
				debug_output = true;
		} else if (strcmp(options->name, "verbose") == 0) {
			if (options->data != NULL && options->data->value.boolean == true)
				verbose_output = true;
		}

		options = options->next;
	}

	/* Initialise the messaging system. */

	msg_initialise(verbose_output);

	/* Generate any necessary verbose or help output. If param_error is true,
	 * then we need to give some usage guidance and exit with an error.
	 */

	if (param_error || output_help || verbose_output) {
		printf("XMLMan %s - %s\n", BUILD_VERSION, BUILD_DATE);
		printf("Copyright Stephen Fryatt, %s\n", BUILD_DATE + 7);
	}

	if (param_error || output_help) {
		printf("\nXML Manual Creation -- Usage:\n");
		printf("xmlman <infile> [-text <outfile>] [-strong <outfile>] [-html <outfile>] [-debug <outfile>]\n");
		printf("       [-encoding <name>] [lineend <name>] [<options>]\n\n");

		printf(" -help                  Produce this help information.\n");
		printf(" -verbose               Generate verbose process information.\n");
		printf(" -encoding <name>       Override the output encoding.\n");
		printf(" -lineend <name>        Override the output line ending type.\n");
		printf(" -define <name>=<value> Define a constant for use in the output.\n");

		printf(" -text <outfile>        Generate text format output to <outfile>.\n");
		printf(" -html <outfile>        Generate HTML format output to <outfile>.\n");
		printf(" -strong <outfile>      Generate StrongHelp format output to <outfile>.\n");
		printf(" -debug                 Generate Debug format output to stdout.\n");

		return (output_help) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	/* Parse the source XML documents. */

	document = parse_document(input_file);
	if (document == NULL) {
		msg_report(MSG_PARSE_FAIL);
		return EXIT_FAILURE;
	}

	/* Generate the selected outputs. */

	if (!xmlman_process_mode((debug_output == true) ? "" : NULL, document, output_encoding, output_line_end, output_debug))
		return EXIT_FAILURE;

	if (!xmlman_process_mode(out_html, document, output_encoding, output_line_end, output_html))
		return EXIT_FAILURE;

	if (!xmlman_process_mode(out_strong, document, output_encoding, output_line_end, output_strong))
		return EXIT_FAILURE;

	if (!xmlman_process_mode(out_text, document, output_encoding, output_line_end, output_text))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * Run an output job for a given output mode.
 *
 * \param *file			The filename to output to, or NULL to skip.
 * \param *document		The document to be output.
 * \param encoding		The requested encoding for the output.
 * \param line_end		The requested line ending for the output.
 * \param *mode			The function to use to write the output.
 * \return			True if successful or skipped; False on
 *				failure or error.
 */

static bool xmlman_process_mode(char *file, struct manual *document, enum encoding_target encoding, enum encoding_line_end line_end,
		bool (*mode)(struct manual *, struct filename *, enum encoding_target, enum encoding_line_end))
{
	struct filename	*filename;
	bool		result;

	if (document == NULL || mode == NULL)
		return false;

	if (file == NULL)
		return true;

	filename = filename_make(file, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_LOCAL);
	if (filename == NULL) {
		msg_report(MSG_OUTPUT_FILENAME_NO_MEM);
		return false;
	}

	result = mode(document, filename, encoding, line_end);

	filename_destroy(filename);

	if (result == false)
		msg_report(MSG_OUTPUT_FILE_FAILED);

	return result;
}

