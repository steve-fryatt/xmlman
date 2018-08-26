/* Copyright 2014-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file filename.h
 *
 * Filename Manipulation Interface.
 */

#ifndef XMLMAN_FILENAME_H
#define XMLMAN_FILENAME_H

#include <stdbool.h>

/**
 * A filename instance.
 */

struct filename;

/**
 * The type of filename being handled.
 */

enum filename_type {
	/**
	 * A root filename, starting from the root of a filesystem.
	 */
	FILENAME_TYPE_ROOT,

	/**
	 * A relative directory or group of directories.
	 */
	FILENAME_TYPE_DIRECTORY,

	/**
	 * A leaf filename, or partial path ending in a file.
	 */
	FILENAME_TYPE_LEAF
};

/**
 * The target platform for a filename.
 */

enum filename_platform {
	/**
	 * A filename for the target platform on which XmlMan is running.
	 */
	FILENAME_PLATFORM_LOCAL,

	/**
	 * A linux filename.
	 */
	FILENAME_PLATFORM_LINUX,

	/**
	 * A RISC OS filename.
	 */
	FILENAME_PLATFORM_RISCOS
};

/**
 * Convert a textual filename into a filename instance.
 *
 * \param *name			Pointer to the filename to convert.
 * \param type			The type of filename.
 * \param platform		The platform for which the supplied name
 *				is formatted.
 * \return			The new filename instance, or NULL.
 */

struct filename *filename_make(xmlChar *name, enum filename_type type, enum filename_platform platform);

/**
 * Destroy a filename instance and free its memory.
 *
 * \param *name			The name instance to destroy.
 */

void filename_destroy(struct filename *name);

/**
 * Open a file using a filename instance.
 *
 * \param *filename		The instance to open.
 * \param *mode			The required read/write mode.
 * \return			The resulting file handle, or NULL.
 */

FILE *filename_fopen(struct filename *filename, const char *mode);

/**
 * Dump the contents of a filename instance for debug purposes.
 *
 * \param *name			The name instance to dump.
 */

void filename_dump(struct filename *name);

/**
 * Duplicate a filename, optionally removing one or more leaves to remove the
 * leave filename or move up to a parent directory.
 *
 * \param *name			The name to be duplicated.
 * \param up			The number of levels to move up, or zero for a
 *				straight duplication.
 * \return			A new filename instance, or NULL on error.
 */

struct filename *filename_up(struct filename *name, int up);

/**
 * Add two filenames together. The nodes in the second name are duplicated and
 * added to the end of the first, so the second name can be deleted afterwards
 * if required. In the event of a failure, the number of nodes in the first
 * name is undefined.
 *
 * \param *name			Pointer to the first name, to which the nodes of
 *				the second will be appended.
 * \param *add			Pointer to the name whose nodes will be appended
 *				to the first.
 * \param levels		The number of levels to copy, or zero for all.
 * \return			True if successful; False on failure.
 */

bool filename_add(struct filename *name, struct filename *add, int levels);

/**
 * Convert a filename instance into a string suitable for a given target
 * platform. Conversion between platforms of root filenames is unlikely
 * to have the intended results.
 * 
 * The name is returned as a malloc() block which the client must free()
 * when no longer required.
 *
 * \param *name			The filename instance to be converted.
 * \param platform		The required target platform.
 * \param levels		The number of levels to copy, or zero for all.
 * \return			Pointer to the filename, or NULL on failure.
 */

char *filename_convert(struct filename *name, enum filename_platform platform, int levels);

#endif

