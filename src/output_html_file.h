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
 * \file output_html_file.h
 *
 * HTML File Output Engine Interface.
 */

#ifndef XMLMAN_OUTPUT_HTML_FILE_H
#define XMLMAN_OUTPUT_HTML_FILE_H

#include <stdbool.h>

#include "filename.h"

/**
 * Open a file to write the HTML output to.
 *
 * \param *filename	Pointer to the name of the file to write.
 * \return		True on success; False on failure.
 */

bool output_html_file_open(struct filename *filename);

/**
 * Close the current HTML output file.
 */

void output_html_file_close(void);


/**
 * Write a UTF8 string to the current HTML output file, in the currently
 * selected encoding.
 *
 * \param *text		Pointer to the text to be written.
 * \return		True if successful; False on error.
 */

bool output_html_file_write_text(char *text);

/**
 * Write an ASCII string to the output.
 *
 * \param *text		Pointer to the text to be written.
 * \param ...		Additional parameters for formatting.
 * \return		True if successful; False on error.
 */

bool output_html_file_write_plain(char *text, ...);

/**
 * Write a line ending sequence to the output.
 *
 * \return		True if successful; False on error.
 */

bool output_html_file_write_newline(void);

#endif

