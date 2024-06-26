/* Copyright 2014-2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file filename.h
 *
 * Filename Manipulation Interface.
 */

#ifndef XMLMAN_FILENAME_H
#define XMLMAN_FILENAME_H

#include <stdbool.h>
#include <stdio.h>

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
	 * The platform isn't applicable.
	 */
	FILENAME_PLATFORM_NONE,

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
	FILENAME_PLATFORM_RISCOS,

	/**
	 * A filename suitable for StrongHelp links.
	 */
	FILENAME_PLATFORM_STRONGHELP
};

/**
 * The filetypes which can be set on a file object.
 */

enum filename_filetype {
	FILENAME_FILETYPE_NONE,
	FILENAME_FILETYPE_TEXT,
	FILENAME_FILETYPE_HTML,
	FILENAME_FILETYPE_STRONGHELP
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

struct filename *filename_make(char *name, enum filename_type type, enum filename_platform platform);

/**
 * Destroy a filename instance and free its memory.
 *
 * \param *name			The name instance to destroy.
 */

void filename_destroy(struct filename *name);

/**
 * Open a file using a filename instance.
 *
 * \param *name			The instance to open.
 * \param *mode			The required read/write mode.
 * \return			The resulting file handle, or NULL.
 */

FILE *filename_fopen(struct filename *name, const char *mode);

/**
 * Create a directory, and optionally any intermediate directories which are
 * required. If the intermediate directories are not created, the call will fail
 * if they do not exist.
 *
 * \param *name			A filename instance referring to the directory
 *				to be created.
 * \param intermediate		True to create intermediate directories;
 *				otherwise False.
 * \return			True if successful; False on failure.
 */

bool filename_mkdir(struct filename *name, bool intermediate);

/**
 * Set the RISC OS filetype of a file
 *
 * \param *name 		A filename instance referring to the file
 *				which is to have its type set.
 * \param type			The required filetype.
 * \return			True if the type is set; else False.
 */

bool filename_set_type(struct filename *name, enum filename_filetype type);

/**
 * Dump the contents of a filename instance for debug purposes.
 *
 * \param *name			The name instance to dump.
 * \param *label		A label to apply, or NULL for none.
 */

void filename_dump(struct filename *name, char *label);

/**
 * Duplicate a filename, optionally removing one or more leaves to remove
 * the leaf filename or move up to a parent directory.
 *
 * \param *name			The name to be duplicated.
 * \param up			The number of levels to move up, or zero for a
 *				straight duplication.
 * \return			A new filename instance, or NULL on error.
 */

struct filename *filename_up(struct filename *name, int up);

/**
 * Add two filenames together. The nodes in the second name are duplicated and
 * added to the start of the first, so the second name can be deleted afterwards
 * if required. In the event of a failure, the number of nodes in the first
 * name is undefined.
 *
 * \param *name			Pointer to the first name, to which the nodes of
 *				the second will be prepended.
 * \param *add			Pointer to the name whose nodes will be prepended
 *				to the first.
 * \param levels		The number of levels to copy, or zero for all.
 * \return			True if successful; False on failure.
 */

bool filename_prepend(struct filename *name, struct filename *add, int levels);

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

bool filename_append(struct filename *name, struct filename *add, int levels);

/**
 * Join two filenames together, returing the result as a new filename.
 * This is effectively an alternative to filename_append(), where the full
 * names are required and the result is in a new instance.
 *
 * \param *first		The first filename instance to be included.
 * \param *second		The second filename instance to be included.
 * \return			The new filename instance, or NULL on failure.
 */

struct filename *filename_join(struct filename *first, struct filename *second);

/**
 * Create a new filename as the relative path between two other names.
 *
 * \param *from			The name to be the origin of the new name.
 * \param *to			The name to be the destination of the new name.
 * \return			The new filename instance, or NULL on failure.
 */

struct filename *filename_get_relative(struct filename *from, struct filename *to);

/**
 * Test a filename to see if it is empty.
 * 
 * \param *name			The filename instance to be tested.
 * \return			True if the name is empty; otherwise false.
 */

bool filename_is_empty(struct filename *name);

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

