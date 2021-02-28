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

#ifdef LINUX
#include <errno.h>
#include <sys/stat.h>
#endif

/* Local source headers. */

#include "filename.h"

#include "msg.h"

/**
 * A filename node, containing a root, directory or leafname.
 */

struct filename_node {
	/**
	 * The filename component.
	 */
	char			*name;

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

static size_t filename_get_storage_size(struct filename *name);
static bool filename_copy_to_buffer(struct filename *name, char *buffer, size_t length, enum filename_platform platform, int levels);
static int filename_count_nodes(struct filename *name);
static struct filename_node *filename_create_node(struct filename_node *node, size_t length);
static char filename_get_separator(enum filename_platform platform);
static char *filename_get_parent_name(enum filename_platform platform);


/**
 * Convert a textual filename into a filename instance.
 *
 * \param *name			Pointer to the filename to convert, or NULL for an empty name.
 * \param type			The type of filename.
 * \param platform		The platform for which the supplied name
 *				is formatted.
 * \return			The new filename instance, or NULL.
 */

struct filename *filename_make(char *name, enum filename_type type, enum filename_platform platform)
{
	struct filename		*root = NULL;
	struct filename_node	*current_node = NULL, *previous_node = NULL;
	char			*part = NULL;
	int			position = 0, length = 0;
	char			separator;

	/* Claim memory for the root of the name. */

	root = malloc(sizeof(struct filename));
	if (root == NULL)
		return NULL;

	root->type = type;
	root->name = NULL;

	if (name == NULL)
		return root;

	/* Break the name down into chunks. */

	separator = filename_get_separator(platform);

	while (name[position] != '\0') {
		/* Find the next part of the name. */

		length = 0;

		while (name[position + length] != '\0' && name[position + length] != separator)
			length++;

		/* Allocate memory for the part name. */

		current_node = filename_create_node(NULL, length);
		if (current_node == NULL) {
			filename_destroy(root);
			return NULL;
		}

		/* Copy the part name into the new structure. */

		part = current_node->name;

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
 * Open a file using a filename instance.
 *
 * \param *name			The instance to open.
 * \param *mode			The required read/write mode.
 * \return			The resulting file handle, or NULL.
 */

FILE *filename_fopen(struct filename *name, const char *mode)
{
	char	*filename = NULL;
	FILE	*handle = NULL;

	filename = filename_convert(name, FILENAME_PLATFORM_LOCAL, 0);
	if (filename == NULL) {
		msg_report(MSG_WRITE_NO_FILENAME);
		return NULL;
	}

	handle = fopen(filename, mode);

	if (handle == NULL)
		msg_report(MSG_WRITE_OPEN_FAIL, filename);
	else
		msg_report(MSG_WRITE_OPENED_FILE, filename);

	free(filename);

	return handle;
}

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

bool filename_mkdir(struct filename *name, bool intermediate)
{
	size_t		length = 0;
	char		*filename = NULL;
	int		levels, nodes = 0;

	nodes = filename_count_nodes(name);
	if (nodes == 0)
		return false;

	length = filename_get_storage_size(name);
	if (length == 0)
		return false;

	filename = malloc(length);
	if (filename == NULL)
		return false;

	for (levels = (intermediate) ? 1 : nodes; levels <= nodes; levels++) {
		if (!filename_copy_to_buffer(name, filename, length, FILENAME_PLATFORM_LOCAL, levels)) {
			free(filename);
			return false;
		}

#ifdef LINUX
		if ((mkdir(filename, 0755) != 0) && (errno != EEXIST)) {
			msg_report(MSG_WRITE_CDIR_FAIL, filename);
			free(filename);
			return false;
		}
#endif
#ifdef RISCOS
		free(filename);

		return false;
#endif
	}

	free(filename);

	return true;
}

/**
 * Dump the contents of a filename instance for debug purposes.
 *
 * \param *name			The name instance to dump.
 * \param *label		A label to apply, or NULL for none.
 */

void filename_dump(struct filename *name, char *label)
{
	struct filename_node *node;

	printf(">=======================\n");

	if (label != NULL)
		printf("%s\n------------------------\n", label);

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
 * Duplicate a filename, optionally removing one or more leaves to remove
 * the leaf filename or move up to a parent directory.
 *
 * \param *name			The name to be duplicated.
 * \param up			The number of levels to move up, or zero for a
 *				straight duplication.
 * \return			A new filename instance, or NULL on error.
 */

struct filename *filename_up(struct filename *name, int up)
{
	struct filename		*new_name = NULL;
	int			levels = 0;

	if (name == NULL)
		return NULL;

	/* Count the number of levels in the filename. */

	levels = filename_count_nodes(name);

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

	if (levels > 0 && !filename_append(new_name, name, levels)) {
		filename_destroy(new_name);
		return NULL;
	}

	return new_name;
}


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

bool filename_prepend(struct filename *name, struct filename *add, int levels)
{
	struct filename_node	*original = NULL, *node = NULL, *new_node = NULL, *previous_node = NULL;
	char			*new_part = NULL;
	int			count = 0;

	if (name == NULL || add == NULL)
		return false;

	/* Remember the start of the first name. */

	original = name->name;

	/* Copy the required number of nodes on to the end of the name. */

	node = add->name;

	while (node != NULL && ((levels == 0) || (count++ < levels))) {
		new_node = filename_create_node(node, 0);
		if (new_node == NULL)
			return false;

		/* Link the new node in to the instance. */

		if (previous_node == NULL)
			name->name = new_node;
		else
			previous_node->next = new_node;

		previous_node = new_node;

		node = node->next;
	}

	new_node->next = original;

	return true;
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

bool filename_append(struct filename *name, struct filename *add, int levels)
{
	struct filename_node	*node = NULL, *new_node = NULL, *previous_node = NULL;
	char			*new_part = NULL;
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
		new_node = filename_create_node(node, 0);
		if (new_node == NULL)
			return false;

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
 * Join two filenames together, returing the result as a new filename.
 * This is effectively an alternative to filename_append(), where the full
 * names are required and the result is in a new instance.
 * 
 * If *second is NULL, *first is simply duplicated and returned.
 *
 * \param *first		The first filename instance to be included.
 * \param *second		The second filename instance to be included, or NULL.
 * \return			The new filename instance, or NULL on failure.
 */

struct filename *filename_join(struct filename *first, struct filename *second)
{
	struct filename *name;

	/* Duplicate the first filename. */

	name = filename_up(first, 0);
	if (name == NULL)
		return NULL;

	/* If there's no second filename, just return the duplicate. */

	if (second == NULL)
		return name;

	/* Append the second filename. */

	if (!filename_append(name, second, 0)) {
		filename_destroy(name);
		return NULL;
	}

	return name;
}


/**
 * Create a new filename as the relative path between two other names.
 *
 * \param *from			The name to be the origin of the new name.
 * \param *to			The name to be the destination of the new name.
 * \return			The new filename instance, or NULL on failure.
 */

struct filename *filename_get_relative(struct filename *from, struct filename *to)
{
	struct filename *filename;
	struct filename_node *node1 = NULL, *node2 = NULL, *tail = NULL, *node = NULL;
	char *parent = NULL;
	size_t parent_length;

	if (from == NULL || to == NULL)
		return NULL;

	if (from->type != FILENAME_TYPE_LEAF || to->type != FILENAME_TYPE_LEAF)
		return NULL;

	/* Identify the parent directory symbol. */

	parent = filename_get_parent_name(FILENAME_PLATFORM_NONE);
	if (parent == NULL)
		return NULL;

	parent_length = strlen(parent);

	/* Create the new name. */

	filename = filename_make(NULL, FILENAME_TYPE_LEAF, FILENAME_PLATFORM_NONE);
	if (filename == NULL)
		return NULL;

	/* Find the common parts of the two names. */

	node1 = from->name;
	node2 = to->name;

	while (node1 != NULL && node2 != NULL && strcmp(node1->name, node2->name) == 0) {
		printf("Match %s and %s\n", node1->name, node2->name);
		node1 = node1->next;
		node2 = node2->next;
	}

	/* Drop one more from the first name, to allow for the leafname
	 * that we don't need to step back up over.
	 */

	if (node1 != NULL)
		node1 = node1->next;

	/* Track up the tree from the first name. */

	while (node1 != NULL) {
		node = filename_create_node(NULL, parent_length);
		if (node == NULL) {
			filename_destroy(filename);
			return NULL;
		}

		strncpy(node->name, parent, parent_length + 1);

		if (tail == NULL)
			filename->name = node;
		else
			tail->next = node;

		tail = node;
		node1 = node1->next;
	}

	/* Track back down to the second name. */

	while (node2 != NULL) {
		node = filename_create_node(node2, 0);
		if (node == NULL) {
			filename_destroy(filename);
			return NULL;
		}

		if (tail == NULL)
			filename->name = node;
		else
			tail->next = node;

		tail = node;
		node2 = node2->next;
	}

	return filename;
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
 * \param levels		The number of levels to copy, or zero for all.
 * \return			Pointer to the filename, or NULL on failure.
 */

char *filename_convert(struct filename *name, enum filename_platform platform, int levels)
{
	size_t			length = 0;
	char			*filename = NULL;

	if (name == NULL)
		return NULL;

	if (levels <= 0)
		levels = filename_count_nodes(name);

	/* Calculate and allocate the amount of memory required. */

	length = filename_get_storage_size(name);
	if (length == 0)
		return NULL;

	filename = malloc(length);
	if (filename == NULL)
		return NULL;

	/* Copy the filename. */

	if (!filename_copy_to_buffer(name, filename, length, platform, levels)) {
		free(filename);
		return NULL;
	}

	return filename;
}

/**
 * Return the size of buffer required to hold a representation of a filename.
 *
 * \param *name			The filename instance to be counted.
 * \return			The number of bytes, or zero on failure.
 */

static size_t filename_get_storage_size(struct filename *name)
{
	size_t			length = 0;
	char			*c = NULL;
	struct filename_node	*node = NULL;

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

	return length;
}

/**
 * Copy a filename into a supplied buffer in the given form.
 *
 * \param *name			The filename instance to be copied.
 * \param *buffer		Pointer to the buffer to take the filename.
 * \param length		The length of the supplied buffer, in bytes.
 * \param platform		The required target platform.
 * \param levels		The number of levels to copy, or zero for all.
 * \return			True if successful; false on error.
 */

static bool filename_copy_to_buffer(struct filename *name, char *buffer, size_t length, enum filename_platform platform, int levels)
{
	struct filename_node	*node = NULL;
	int			ptr = 0;
	char			*c = NULL; 
	char			separator;

	if (buffer == NULL || length == 0)
		return false;

	node = name->name;
	separator = filename_get_separator(platform);

	/* Copy the name into the buffer, byte by byte. */

	while ((ptr < length) && (node != NULL) && (levels-- > 0)) {
		c = node->name;

		/* Copy the character bytes in the name. In StrongHelp mode,
		 * folders which start '[...]' are omitted from link names.
		 */

		if (platform != FILENAME_PLATFORM_STRONGHELP || *c != '[') {
			while ((ptr < length) && (*c != '\0'))
				buffer[ptr++] = *c++;
		}

		/* Copy a separator after each intermediate node. In StrongHelp
		 * mode, separators are ignored when assembling link names.
		 */

		if ((ptr < length) && (node->next != NULL) && (levels > 0) && platform != FILENAME_PLATFORM_STRONGHELP)
			buffer[ptr++] = separator;

		node = node->next;
	}

	/* If the buffer overran, terminate it and exit. */

	if (ptr >= length) {
		buffer[0] = '\0';
		return false;
	}

	buffer[ptr] = '\0';

	return true;
}

/**
 * Count the number of nodes in a filename.
 *
 * \param *name			The filename instance to be counted.
 * \return			The number of nodes, or zero on failure.
 */

static int filename_count_nodes(struct filename *name)
{
	struct filename_node	*node = NULL;
	int			levels = 0;

	node = name->name;

	while (node != NULL) {
		levels++;
		node = node->next;
	}

	return levels;
}


/**
 * Duplicate a node, or create a new one with a given name
 * buffer length allocated. Either node is not NULL and length
 * is zero, or node is NULL and length is not zero.
 * 
 * \param *node		Pointer to the node to duplicate, or NULL.
 * \param length	Length of the filename buffer to allocate, or 0.
 * \return		Pointer to the duplicate node, or NULL.
 */

static struct filename_node *filename_create_node(struct filename_node *node, size_t length)
{
	struct filename_node *new_node = NULL;
	char *new_part = NULL;

	if (node == NULL && length == 0)
		return NULL;

	/* Allocate the text buffer. */

	if (node != NULL && length == 0) {
		if (node->name != NULL)
			new_part = strdup(node->name);
	} else if (node == NULL && length > 0) {
		new_part = malloc(length + 1);
		new_part[0] = '\0';
	}

	new_node = malloc(sizeof(struct filename_node));

	/* Tidy up the loose nodes in the event of a memory error. */

	if (new_node == NULL || new_part == NULL) {
		if (new_node != NULL)
			free(new_node);

		if (new_part != NULL)
			free(new_part);

		return NULL;
	}

	new_node->name = new_part;
	new_node->next = NULL;

	return new_node;
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
	case FILENAME_PLATFORM_NONE:
	case FILENAME_PLATFORM_LINUX:
		return '/';
	case FILENAME_PLATFORM_RISCOS:
		return '.';
	case FILENAME_PLATFORM_STRONGHELP:
	default:
		return '\0';
	};
}


/**
 * Return the parent directory name for a given platform.
 *
 * \param platform		The target platform.
 * \return			The parent directort name, or NULL.
 */

static char *filename_get_parent_name(enum filename_platform platform)
{
	switch (platform) {
	case FILENAME_PLATFORM_LOCAL:
#ifdef LINUX
		return "..";
#endif
#ifdef RISCOS
		return "^";
#endif
	case FILENAME_PLATFORM_NONE:
	case FILENAME_PLATFORM_LINUX:
		return "..";
	case FILENAME_PLATFORM_RISCOS:
		return "^";
	case FILENAME_PLATFORM_STRONGHELP:
	default:
		return "";
	};
}
