/* Copyright 2018-2021, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file manual_queue.c
 *
 * Manual Output File Queueing, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "manual_data.h"
#include "manual_ids.h"

/**
 * An entry in the node queue.
 */

struct manual_queue_entry {

	/**
	 * The node in the queue.
	 */

	struct manual_data		*node;

	/**
	 * Pointer to the next queue entry, or NULL.
	 */

	struct manual_queue_entry	*next;
};

/**
 * Pointer to the queue structure.
 */

static struct manual_queue_entry *manual_queue_root = NULL;

/**
 * Pointer to the first free queue entry.
 */

static struct manual_queue_entry *manual_queue_head = NULL;

/**
 * Pointer to the next queue entry to be read back.
 */

static struct manual_queue_entry *manual_queue_tail = NULL;


/**
 * Initialise the queue.
 */

void manual_queue_initialise(void)
{
	struct manual_queue_entry *entry;

	manual_queue_head = manual_queue_root;
	manual_queue_tail = NULL;

	/* Clear out the node details. */

	entry = manual_queue_root;

	while (entry != NULL) {
		entry->node = NULL;
		entry = entry->next;
	}
}

/**
 * Add a node to the queue for later processing.
 * 
 * \param *node		Pointer to the node to be added.
 * \return		True if successful; otherwise false.
 */

bool manual_queue_add_node(struct manual_data *node)
{
	struct manual_queue_entry *entry = NULL;

	/* Make sure that there's an entry to use. */

	if (manual_queue_head == NULL || manual_queue_head->next == NULL) {
		entry = malloc(sizeof(struct manual_queue_entry));
		if (entry == NULL)
			return false;

		entry->node = NULL;
		entry->next = NULL;

		if (manual_queue_head != NULL) {
			manual_queue_head->next = entry;
			manual_queue_head = entry;
		} else {
			manual_queue_root = entry;
			manual_queue_head = entry;
		}
	} else {
		manual_queue_head = manual_queue_head->next;
		entry = manual_queue_head;
	}

	entry->node = node;

	if (manual_queue_tail == NULL)
		manual_queue_tail = entry;

	return true;
}

/**
 * Remove the next node to be processed from the queue.
 *
 * \return		Pointer to the next node, or NULL if done.
 */

struct manual_data *manual_queue_remove_node(void)
{
	struct manual_data *node = NULL;

	if (manual_queue_tail == NULL)
		return NULL;

	node = manual_queue_tail->node;
	manual_queue_tail = manual_queue_tail->next;

	return node;
}
