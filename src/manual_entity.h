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
 * \file manual_entity.h
 *
 * XML Manual Entity Decoding Interface.
 */

#ifndef XMLMAN_MANUAL_ENTITY_H
#define XMLMAN_MANUAL_ENTITY_H

/**
 * A list of entities known to the parser.
 */

enum manual_entity_type {
	MANUAL_ENTITY_NONE,
	MANUAL_ENTITY_AMP,
	MANUAL_ENTITY_QUOT,
	MANUAL_ENTITY_APOS,
	MANUAL_ENTITY_LT,
	MANUAL_ENTITY_GT,
	MANUAL_ENTITY_LE,
	MANUAL_ENTITY_GE,
	MANUAL_ENTITY_NBSP,
	MANUAL_ENTITY_LSQUO,
	MANUAL_ENTITY_LDQUO,
	MANUAL_ENTITY_RSQUO,
	MANUAL_ENTITY_RDQUO,
	MANUAL_ENTITY_NDASH,
	MANUAL_ENTITY_MDASH,
	MANUAL_ENTITY_MINUS,
	MANUAL_ENTITY_TIMES,
	MANUAL_ENTITY_MSEP,
	MANUAL_ENTITY_SMILE,
	MANUAL_ENTITY_SAD
};

/**
 * Given a node containing an entity, return the entity type.
 *
 * \param *name		Pointer to the textual entity name.
 * \return		The entity type, or MANUAL_ENTITY_NONE if unknown.
 */

enum manual_entity_type manual_entity_find_type(char *name);

/**
 * Given an entity type, return the textual entity name.
 *
 * \param type		The entity type to look up.
 * \return		Pointer to the entity's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_entity_find_name(enum manual_entity_type type);

#endif

