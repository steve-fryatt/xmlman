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

/* Tokenize
 *
 * Generate tokenized BBC BASIC files from ASCII text.
 *
 * Syntax: Tokenize [<options>]
 *
 * Options -v  - Produce verbose output
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Local source headers. */

#include "args.h"
#include "parse.h"

/* OSLib source headers. */

#ifdef RISCOS
#include "oslib/osfile.h"
#endif

int main(int argc, char *argv[])
{
	bool			param_error = false;
	bool			output_help = false;
	bool			verbose_output = false;
	struct args_option	*options;
	char			*input_file = NULL;

	/* Decode the command line options. */

	options = args_process_line(argc, argv,
			"source/A,verbose/S,help/S");
	if (options == NULL)
		param_error = true;

	while (options != NULL) {
		if (strcmp(options->name, "help") == 0) {
			if (options->data != NULL && options->data->value.boolean == true)
				output_help = true;
		} else if (strcmp(options->name, "source") == 0) {
			if (options->data != NULL) {
				if (options->data->value.string != NULL)
					input_file = options->data->value.string;
			} else {
				param_error = true;
			}
		} else if (strcmp(options->name, "verbose") == 0) {
			if (options->data != NULL && options->data->value.boolean == true)
				verbose_output = true;
		}

		options = options->next;
	}

	/* Generate any necessary verbose or help output. If param_error is true,
	 * then we need to give some usage guidance and exit with an error.
	 */

	if (param_error || output_help || verbose_output) {
		printf("XMLMan %s - %s\n", BUILD_VERSION, BUILD_DATE);
		printf("Copyright Stephen Fryatt, %s\n", BUILD_DATE + 7);
	}

	if (param_error || output_help) {
		printf("XML Manual Creation -- Usage:\n");
		printf("xmlman <infile> [-text <outfile>] [-strong <outfile>] [-html <outfile] [<options>]\n\n");

		printf(" -help                  Produce this help information.\n");
		printf(" -verbose               Generate verbose process information.\n");

		return (output_help) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	/* Run the tokenisation. */

	parse_file(input_file);

	return EXIT_SUCCESS;
}

