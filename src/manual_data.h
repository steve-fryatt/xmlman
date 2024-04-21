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
 * \file manual_data.h
 *
 * Manual Data Structures Interface.
 */

#ifndef XMLMAN_MANUAL_DATA_H
#define XMLMAN_MANUAL_DATA_H

#include "xmlman.h"
#include "manual_entity.h"
#include "modes.h"
#include "filename.h"

/**
 * Create a new manual_data structure.
 *
 * \param type		The type of object to create.
 * \return		Pointer to the new structure, or NULL on failure.
 */

struct manual_data *manual_data_create(enum manual_data_object_type type);

/**
 * Return a pointer to an object's resources structure, if one would be
 * valid, creating it first if required.
 *
 * \param *object	Pointer to the object of interest.
 * \return		Pointer to the resources block, or NULL.
 */

struct manual_data_resources *manual_data_get_resources(struct manual_data *object);

/**
 * Given an object type, return the textual object type name.
 *
 * \param type		The object type to look up.
 * \return		Pointer to the object's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_data_find_object_name(enum manual_data_object_type type);

/**
 * Given a node and a current nesting level for the parent node,
 * determine the nesting level if the node is descended into.
 *
 * \param *node		The node of interest.
 * \param current_level	The nesting level of the parent node.
 * \return		The new nesting level.
 */

int manual_data_get_nesting_level(struct manual_data *node, int current_level);

/**
 * Given a node, return a pointer to its display number in string format,
 * or NULL if no number is defined.
 *
 * This is the full number, including the numbers of any parent sections.
 *
 * \param *node		The node to return a number for.
 * \param include_name	Should we prefix the number with the object name?
 * \return		Pointer to the display number, or NULL.
 */

char *manual_data_get_node_number(struct manual_data *node, bool include_name);

#include "modes.h"

/**
 * Search a node and its children for any filename data associated with
 * a given manual type.
 *
 * \param *node		The node to search down from.
 * \param type		The target output type to search for.
 * \return		True if filename data was found; otherwise false.
 */

bool manual_data_find_filename_data(struct manual_data *node, enum modes_type type);

/**
 * Return a filename for a node, given a default  root filename and the
 * target output type.
 *
 * \param *node		The node to return a filename for.
 * \param *root		A default root filename.
 * \param type		The target output type.
 * \return		Pointer to a filename, or NULL on failure.
 */

struct filename *manual_data_get_node_filename(struct manual_data *node, struct filename *root, enum modes_type type);

/**
 * Given a node, return a pointer to the first parent node which contains
 * a stylesheet filename for the chosen output type.
 *
 * \param *node		The node to return a filename for.
 * \param type		The target output type.
 * \return		Pointer to a node, or NULL on failure.
 */

struct manual_data *manual_data_get_node_stylesheet(struct manual_data *node, enum modes_type type);

/**
 * Test a pair of nodes, to determine whether or not they are
 * in the same file for a given output mode.
 *
 * \param *node1	The first node to be compared.
 * \param *node2	The second node to be compared.
 * \param type		The target output type.
 * \return		True if the nodes are both in the same file;
 *			otherwise false.
 */

bool manual_data_nodes_share_file(struct manual_data *node1, struct manual_data *node2, enum modes_type type);

/**
 * Given a callout node, return an appropriate callout name in
 * the form of a set of node chunks.
 * 
 * \param *callout	Pointer to the callout node.
 * \return		Pointer to the callout name.
 */

struct manual_data *manual_data_get_callout_name(struct manual_data* callout);

#endif
