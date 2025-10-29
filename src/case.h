/* Copyright 2025, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file case.h
 *
 * Unicode Case Conversion Support Interface.
 */

#ifndef XMLMAN_CASE_H
#define XMLMAN_CASE_H

#include <stdbool.h>



/**
 * Convert a unicode character to upper case.
 *
 * \param codepoint	The character to be converted.
 * \return		The upper case variant of the character.
 */

int case_convert_to_upper_case(int codepoint);


/**
 * Convert a unicode character to lower case.
 *
 * \param codepoint	The character to be converted.
 * \return		The uppercase variant of the character.
 */

int case_convert_to_lower_case(int codepoint);


/**
 * Convert a unicode character to title case.
 *
 * \param codepoint	The character to be converted.
 * \return		The title case variant of the character.
 */

int case_convert_to_title_case(int codepoint);

#endif