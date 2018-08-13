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
 * \file output_text.h
 *
 * Text Output Engine Interface.
 */

#ifndef XMLMAN_OUTPUT_TEXT_H
#define XMLMAN_OUTPUT_TEXT_H

#include <stdbool.h>
#include "filename.h"
#include "manual.h"

/**
 * Output a manual in text form.
 *
 * \param *document	The manual to be output.
 * \param *filename	The filename to use to write to.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_text(struct manual *document, struct filename *filename);

#endif

