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
 * \file output_strong.h
 *
 * StrongHelp Output Engine Interface.
 */

#ifndef XMLMAN_OUTPUT_STRONG_H
#define XMLMAN_OUTPUT_STRONG_H

#include <stdbool.h>
#include "manual_data.h"

/**
 * Output a manual in StrongHelp form.
 *
 * \param *document	The manual to be output.
 * \return		TRUE if successful, otherwise FALSE.
 */

bool output_strong(struct manual *document);

#endif

