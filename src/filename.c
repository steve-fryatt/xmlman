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
 * \file filename.c
 *
 * Filename Manipulation, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlstring.h>

/* Local source headers. */

#include "filename.h"

/**
 * A filename node, containing a root, directory or leafname.
 */

struct filename_node {
	/**
	 * The filename component.
	 */
	xmlChar			*name;

	/**
	 * Pointer to the next node in the name, or NULL.
	 */
	struct filename_node	*next;
};

/**
 * A filename instance.
 */

struct filename {
	/**
	 * The type of filename stored.
	 */
	enum filename_type	type;

	/**
	 * Pointer to the component parts of the name.
	 */
	struct filename_node	*name;
};


/* Static Function Prototypes */

static char filename_get_separator(enum filename_platform platform);

/**
 * Convert a textual filename into a filename instance.
 *
 * \param *name			Pointer to the filename to convert.
 * \param type			The type of filename.
 * \param platform		The platform for which the supplied name
 *				is formatted.
 * \return			The new filename instance, or NULL.
 */

struct filename *filename_make(xmlChar *name, enum filename_type type, enum filename_platform platform)
{
	struct filename		*root = NULL;
	struct filename_node	*current_node = NULL, *previous_node = NULL;
	xmlChar			*part = NULL;
	int			position = 0, length = 0;
	char			separator;

	if (name == NULL)
		return NULL;

	/* Claim memory for the root of the name. */

	root = malloc(sizeof(struct filename));
	if (root == NULL)
		return NULL;

	root->type = type;
	root->name = NULL;

	/* Break the name down into chunks. */

	separator = filename_get_separator(platform);

	while (name[position] != '\0') {
		/* Find the next part of the name. */

		length = 0;

		while (name[position + length] != '\0' && name[position + length] != separator)
			length++;

		/* Allocate memory for the part name. */

		current_node = malloc(sizeof(struct filename_node));
		part = malloc(length + 1);

		if (current_node == NULL || part == NULL) {
			if (current_node != NULL)
				free(current_node);

			if (part != NULL)
				free(part);

			filename_destroy(root);
			return NULL;
		}

		current_node->name = part;
		current_node->next = NULL;

		/* Copy the part name into the new structure. */

		while (name[position] != '\0' && name[position] != separator)
			*part++ = name[position++];

		*part = '\0';

		/* Step past any directory separator. */

		if (name[position] != '\0')
			position++;

		/* Link the new node in to the instance. */

		if (previous_node == NULL)
			root->name = current_node;
		else
			previous_node->next = current_node;

		previous_node = current_node;
	}

	return root;
}

/**
 * Destroy a filename instance and free its memory.
 *
 * \param *name			The name instance to destroy.
 */

void filename_destroy(struct filename *name)
{
	struct filename_node *current_node = NULL, *next_node = NULL;

	if (name == NULL)
		return;

	/* Free the nodes in the name. */

	current_node = name->name;

	while (current_node != NULL) {
		next_node = current_node->next;

		if (current_node->name != NULL)
			free(current_node->name);

		free(current_node);

		current_node = next_node;
	}

	/* Free the instance data itself. */

	free(name);
}

/**
 * Dump the contents of a filename instance for debug purposes.
 *
 * \param *name			The name instance to dump.
 */

void filename_dump(struct filename *name)
{
	struct filename_node *node;

	printf(">=======================\n");

	if (name == NULL) {
		printf("No name!\n");
		return;
	}

	node = name->name;

	while (node != NULL) {
		if (node->name != NULL)
			printf("Node: '%s'\n", node->name);
		else
			printf("Empty node!\n");

		node = node->next;
	}

	printf("<-----------------------\n");
}

/**
 * Duplicate a filename, optionally removing one or more leaves to remove the
 * leave filename or move up to a parent directory.
 *
 * \param *name			The name to be duplicated.
 * \param up			The number of levels to move up, or zero for a
 *				straight duplication.
 * \return			A new filename instance, or NULL on error.
 */

struct filename *filename_up(struct filename *name, int up)
{
	struct filename		*new_name = NULL;
	struct filename_node	*node = NULL;
	int			levels = 0;

	if (name == NULL)
		return NULL;

	/* Count the number of levels in the filename. */

	node = name->name;

	while (node != NULL) {
		levels++;
		node = node->next;
	}

	/* Adjust the levels if a parent directory is required. */

	if (up < 0)
		up = 0;

	levels -= up;
	if (levels < 0)
		levels = 0;

	/* Claim memory for the new filename root. */

	new_name = malloc(sizeof(struct filename));
	if (new_name == NULL)
		return NULL;

	new_name->type = name->type;
	new_name->name = NULL;

	/* Copy across the required number of name nodes. */

	if (levels > 0 && !filename_add(new_name, name, levels)) {
		filename_destroy(new_name);
		return NULL;
	}

	return new_name;
}

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

bool filename_add(struct filename *name, struct filename *add, int levels)
{
	struct filename_node	*node = NULL, *new_node = NULL, *previous_node = NULL;
	xmlChar			*new_part = NULL;
	int			count = 0;

	if (name == NULL || add == NULL)
		return false;

	/* Find the end of the first name. */

	node = name->name;

	while (node != NULL) {
		previous_node = node;
		node = node->next;
	}

	/* Copy the required number of nodes on to the end of the name. */

	node = add->name;

	while (node != NULL && ((levels == 0) || (count++ < levels))) {
		new_node = malloc(sizeof(struct filename_node));
		new_part = xmlStrdup(node->name);

		/* Tidy up the loose nodes in the event of a memory error. */

		if (new_node == NULL || new_part == NULL) {
			if (new_node != NULL)
				free(new_node);

			if (new_part != NULL)
				free(new_part);

			return false;
		}

		new_node->name = new_part;
		new_node->next = NULL;

		/* Link the new node in to the instance. */

		if (previous_node == NULL)
			name->name = new_node;
		else
			previous_node->next = new_node;

		previous_node = new_node;

		node = node->next;
	}

	return true;
}

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
 * \return			Pointer to the filename, or NULL on failure.
 */

char *filename_convert(struct filename *name, enum filename_platform platform)
{
	size_t			length = 0;
	xmlChar			*c = NULL, *d = NULL, *filename = NULL;
	char			separator;
	struct filename_node	*node = NULL;

	if (name == NULL)
		return NULL;

	/* Calculate the amount of memory required. */

	node = name->name;

	while (node != NULL) {
		c = node->name;

		/* Count the character bytes in the name. */

		while (*c++ != '\0')
			length++;

		/* Add one for the separator or terminator. */

		length++;

		node = node->next;
	}

	filename = malloc(length);
	if (filename == NULL)
		return NULL;

	/* Copy the filename. */

	node = name->name;
	d = filename;
	separator = filename_get_separator(platform);

	while (node != NULL) {
		c = node->name;

		/* Copy the character bytes in the name. */

		while (*c != '\0')
			*d++ = *c++;

		/* Add one for the separator or terminator. */

		if (node->next != NULL)
			*d++ = separator;

		node = node->next;
	}

	*d = '\0';

	return (char *) filename;
}

/**
 * Return the filename separator for a given platform.
 *
 * \param platform		The target platform.
 * \return			The filename separator, or '\0' on
 *				failure.
 */

static char filename_get_separator(enum filename_platform platform)
{
	switch (platform) {
	case FILENAME_PLATFORM_LOCAL:
#ifdef LINUX
		return '/';
#endif
#ifdef RISCOS
		return '.';
#endif
	case FILENAME_PLATFORM_LINUX:
		return '/';
	case FILENAME_PLATFORM_RISCOS:
		return '.';
	default:
		return '\0';
	};
}

