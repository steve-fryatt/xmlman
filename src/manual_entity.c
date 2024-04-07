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
 * \file manual_entity.c
 *
 * XML Manual Entity Decoding, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "manual_entity.h"

/**
 * An entity definition structure.
 */

struct manual_entity_definition {
	enum manual_entity_type	type;		/**< The type of entity.		*/
	const char		*name;		/**< The name of the entity	.	*/
};

/**
 * The list of known element definitions.
 */

static struct manual_entity_definition manual_entity_names[] = {
	{MANUAL_ENTITY_AMP,		"amp"},
	{MANUAL_ENTITY_QUOT,		"quot"},
	{MANUAL_ENTITY_APOS,		"apos"},
	{MANUAL_ENTITY_LT,		"lt"},
	{MANUAL_ENTITY_GT,		"gt"},
	{MANUAL_ENTITY_LE,		"le"},
	{MANUAL_ENTITY_GE,		"ge"},
	{MANUAL_ENTITY_NBSP,		"nbsp"},
	{MANUAL_ENTITY_LSQUO,		"lsquo"},
	{MANUAL_ENTITY_LDQUO,		"ldquo"},
	{MANUAL_ENTITY_RSQUO,		"rsquo"},
	{MANUAL_ENTITY_RDQUO,		"rdquo"},
	{MANUAL_ENTITY_NDASH,		"ndash"},
	{MANUAL_ENTITY_MDASH,		"mdash"},
	{MANUAL_ENTITY_MINUS,		"minus"},
	{MANUAL_ENTITY_TIMES,		"times"},
	{MANUAL_ENTITY_PLUSMN,		"plusmn"},
	{MANUAL_ENTITY_COPY,		"copy"},
	{MANUAL_ENTITY_MSEP,		"msep"},
	{MANUAL_ENTITY_SMILE,		"smile"},
	{MANUAL_ENTITY_SAD,		"sad"},
	{MANUAL_ENTITY_NONE,		""}
};


/**
 * Given a node containing an entity, return the entity type.
 *
 * \param *name		Pointer to the textual entity name.
 * \return		The entity type, or MANUAL_ENTITY_NONE if unknown.
 */

enum manual_entity_type manual_entity_find_type(char *name)
{
	int i;

	if (name == NULL)
		return MANUAL_ENTITY_NONE;

	for (i = 0; manual_entity_names[i].type != MANUAL_ENTITY_NONE && strcmp(manual_entity_names[i].name, name) != 0; i++);

	return manual_entity_names[i].type;
}

/**
 * Given an entity type, return the textual entity name.
 *
 * \param type		The entity type to look up.
 * \return		Pointer to the entity's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_entity_find_name(enum manual_entity_type type)
{
	int i;

	for (i = 0; manual_entity_names[i].type != MANUAL_ENTITY_NONE && manual_entity_names[i].type != type; i++);

	return manual_entity_names[i].name;
}

