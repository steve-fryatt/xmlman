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
 * \file output_strong_file.h
 *
 * StrongHelp File Output Engine Interface.
 */

#ifndef XMLMAN_OUTPUT_STRONG_FILE_H
#define XMLMAN_OUTPUT_STRONG_FILE_H

#include <stdbool.h>

#include <libxml/xmlstring.h>

/**
 * Open a file to write the StrongHelp output to.
 *
 * \param *filename	Pointer to the name of the file to write.
 * \return		True on success; False on failure.
 */

bool output_strong_file_open(char *filename);

/**
 * Close the current StrongHelp output file.
 */

void output_strong_file_close(void);

/**
 * Open a file within the current StrongHelp file, ready for writing.
 *
 * \param *filename	The internal filename, in Filecore format.
 * \param type		The RISC OS numeric filetype.
 * \return		True on success; False on failure.
 */

bool output_strong_file_sub_open(char *filename, int type);

/**
 * Close the current file within the current StrongHelp output file.
 *
 * \return		True on success; False on failure.
 */

bool output_strong_file_sub_close(void);

/**
 * Write a UTF8 string to the current StrongHelp output file, in the
 * currently selected encoding.
 *
 * \param *text		Pointer to the text to be written.
 * \return		True if successful; False on error.
 */

bool output_strong_file_write_text(xmlChar *text);

/**
 * Write an ASCII string to the output.
 *
 * \param *text		Pointer to the text to be written.
 * \param ...		Additional parameters for formatting.
 * \return		True if successful; False on error.
 */

bool output_strong_file_write_plain(char *text, ...);

/**
 * Write a line ending sequence to the output.
 *
 * \return		True if successful; False on error.
 */

bool output_strong_file_write_newline(void);

#endif
