/* Copyright 2021, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file modes.h
 *
 * Mode Lookup Interface.
 */

#ifndef XMLMAN_MODES_H
#define XMLMAN_MODES_H

/**
 * The maximum length of a mode name.
 */

#define MODES_MAX_NAME_LEN 64

/**
 * The possible mode types.
 */

enum modes_type {
	MODES_TYPE_NONE,
	MODES_TYPE_DEBUG,
	MODES_TYPE_TEXT,
	MODES_TYPE_STRONGHELP,
	MODES_TYPE_HTML,
};

/**
 * Given a mode name, find the mode type.
 *
 * \param *name		The name to look up.
 * \return		The mode type, or MODES_TYPE_NONE on failure.
 */

enum modes_type modes_find_type(char *name);

#endif

