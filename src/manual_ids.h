/* Copyright 2018-2024, Stephen Fryatt (info@stevefryatt.org.uk)
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

#include "xmlman.h"
#include "manual_data.h"

/**
 * Initialise the manual IDs index.
 */

void manual_ids_initialise(void);

/**
 * Dump a manual IDs index instance to the log.
 */

void manual_ids_dump();

/**
 * Add a node to an index of IDs.
 *
 * \param *node		The node to add to the index.
 * \return		True if successful; False on error.
 */

bool manual_ids_add_node(struct manual_data *node);

/**
 * Given a reference node, find the node that it refers to.
 *
 * \param *node		The reference node to start from.
 * \return		The target node, or NULL.
 */

struct manual_data *manual_ids_find_node(struct manual_data *node);

#endif

