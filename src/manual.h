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
 * \file manual.h
 *
 * Manual Data Structures Interface.
 */

#ifndef XMLMAN_MANUAL_H
#define XMLMAN_MANUAL_H

#include <libxml/xmlstring.h>

#include "manual_data.h"
#include "manual_ids.h"

/**
 * A top-level manual structure.
 */

struct manual {
	/**
	 * Poiinter to the first node in the manual.
	 */

	struct manual_data	*manual;

	/**
	 * The ID Index instance for the manual.
	 */

	struct manual_ids	*id_index;
};

/**
 * Create a new manual structure.
 *
 * \param *node		Pointer to the top-level node for the structure.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual *manual_create(struct manual_data *node);

#endif

