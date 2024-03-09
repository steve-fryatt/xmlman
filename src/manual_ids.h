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

/**
 * \file manual_ids.h
 *
 * Manual ID Indexing Interface.
 */

#ifndef XMLMAN_MANUAL_IDS_H
#define XMLMAN_MANUAL_IDS_H

#include <stdbool.h>

#include "manual_data.h"

/**
 * An ID Index instance structure.
 */

struct manual_ids;

/**
 * Create a new manual IDs index instance.
 *
 * \return		Pointer to the new instance, or NULL on failure.
 */

struct manual_ids *manual_ids_create(void);

void manual_ids_dump(struct manual_ids *instance);

/**
 * Add a node to an index of IDs.
 *
 * \param *instance	The ID index instance to add the node to.
 * \param *node		The node to add to the index.
 * \return		True if successful; False on error.
 */

bool manual_ids_add_node(struct manual_ids *instance, struct manual_data *node);

#endif

