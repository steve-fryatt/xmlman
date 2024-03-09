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
 * \file parse_link.h
 *
 * XML Parser Document Linking Interface.
 */

#ifndef XMLMAN_PARSE_LINK_H
#define XMLMAN_PARSE_LINK_H

#include <stdbool.h>

#include "manual_data.h"

/**
 * Link a node and its children, connecting the previous and parent node
 * references.
 *
 * \param *root		The root node to link from.
 * \return		An ID Index instance, or NULL on failure..
 */

struct manual_ids *parse_link(struct manual_data *root);

#endif

