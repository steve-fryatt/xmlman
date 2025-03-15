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
 * \file manual_defines.h
 *
 * Manual Define Indexing Interface.
 *
 * Defines are constants that can be defined using the --define
 * parameter on the command line, then referenced in the manual.
 */

#ifndef XMLMAN_MANUAL_DEFINES_H
#define XMLMAN_MANUAL_DEFINES_H

#include <stdbool.h>

#include "xmlman.h"
#include "manual_data.h"

/**
 * Initialise the manual defines index.
 */

void manual_defines_initialise(void);

/**
 * Dump a manual defines index instance to the log.
 */

void manual_defines_dump();

/**
 * Add an entry to an index of defines.
 *
 * \param *entry	The entry to add to the index.
 * \return		True if successful; False on error.
 */

 bool manual_defines_add_entry(char *entry);

/**
 * Given a name, return a pointer to the defined value that it refers to.
 *
 * \param *name		The name to look up.
 * \return		The define value, or NULL.
 */

 char *manual_defines_find_value(char *name);

#endif

