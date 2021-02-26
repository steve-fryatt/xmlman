/* Copyright 2018-2021, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file manual_queue.h
 *
 * Manual Output File Queueing Interface.
 */

#ifndef XMLMAN_MANUAL_QUEUE_H
#define XMLMAN_MANUAL_QUEUE_H

#include <stdbool.h>

#include "manual_queue.h"

/**
 * Initialise the queue.
 */

void manual_queue_initialise(void);

/**
 * Add a node to the queue for later processing.
 * 
 * \param *node		Pointer to the node to be added.
 * \return		True if successful; otherwise false.
 */

bool manual_queue_add_node(struct manual_data *node);

/**
 * Remove the next node to be processed from the queue.
 *
 * \return		Pointer to the next node, or NULL if done.
 */

struct manual_data *manual_queue_remove_node(void);

#endif

