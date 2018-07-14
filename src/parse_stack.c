/* Copyright 2018, Stephen Fryatt (info@stevefryatt.org.uk)
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
 * \file parse_stack.c
 *
 * XML Parser Stack, implementation.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "parse_stack.h"

/**
 * The maximum size of the parse stack. This must be enough to handle the
 * maximum valid nesting of XML tags, which is controlled by the DTD.
 */

#define PARSE_STACK_SIZE 20

/**
 * The number of entries on the stack.
 */

static int parse_stack_size = 0;

/**
 * The parse stack data.
 */

static struct parse_stack_entry parse_stack[PARSE_STACK_SIZE];

/**
 * Reset the parse stack.
 */

void parse_stack_reset(void)
{
	parse_stack_size = 0;
}


