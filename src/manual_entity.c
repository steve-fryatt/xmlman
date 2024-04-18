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

#include "msg.h"
#include "search_tree.h"

/**
 * An entity definition structure.
 */

struct manual_entity_definition {
	enum manual_entity_type	type;		/**< The type of entity.			*/
	const char		*name;		/**< The name of the entity	.		*/
	int			unicode;	/**< The unicode code point for the entity.	*/
};

/**
 * The lookup tree instance for the entities.
 */

static struct search_tree *manual_entity_search_tree = NULL;

/**
 * The number of entries in the entity list.
 */

static int manual_entities_max_entries = -1;

/**
 * The list of known entity definitions.
 *
 * The order of this table is by ascending unicode point, with non-unicode
 * entities at the start using -1 and NONE at the end as an end stop. The
 * order of the entity texts are not important.
 * 
 * It *must* correspond to the order that the enum values are
 * defined in manual_entity.h, so that the array indices match the
 * values of their enum entries.
 */

static struct manual_entity_definition manual_entity_names[] = {
	/* Non-Standard Special Cases */

	{ MANUAL_ENTITY_SMILEYFACE,		"smileyface",		MANUAL_ENTITY_NO_CODEPOINT },
	{ MANUAL_ENTITY_SADFACE,		"sadface",		MANUAL_ENTITY_NO_CODEPOINT },
	{ MANUAL_ENTITY_MSEP,			"msep",			MANUAL_ENTITY_NO_CODEPOINT },

	/* Basic Latin */

	{ MANUAL_ENTITY_QUOT,			"quot",			34 },
	{ MANUAL_ENTITY_AMP,			"amp",			38 },
	{ MANUAL_ENTITY_APOS,			"apos",			39 },
	{ MANUAL_ENTITY_LT,			"lt",			60 },
	{ MANUAL_ENTITY_GT,			"gt",			62 },

	/* Latin-1 Supplement */

	{ MANUAL_ENTITY_NBSP,			"nbsp",			160 },
	{ MANUAL_ENTITY_IEXCL,			"iexcl",		161 },
	{ MANUAL_ENTITY_CENT,			"cent",			162 },
	{ MANUAL_ENTITY_POUND,			"pound",		163 },
	{ MANUAL_ENTITY_CURREN,			"curren",		164 },
	{ MANUAL_ENTITY_YEN,			"yen",			165 },
	{ MANUAL_ENTITY_BRVBAR,			"brvbar",		166 },
	{ MANUAL_ENTITY_SECT,			"sect",			167 },
	{ MANUAL_ENTITY_UML,			"uml",			168 },
	{ MANUAL_ENTITY_COPY,			"copy",			169 },
	{ MANUAL_ENTITY_ORDF,			"ordf",			170 },
	{ MANUAL_ENTITY_LAQUO,			"laquo",		171 },
	{ MANUAL_ENTITY_NOT,			"not",			172 },
	{ MANUAL_ENTITY_SHY,			"shy",			173 },
	{ MANUAL_ENTITY_REG,			"reg",			174 },
	{ MANUAL_ENTITY_MACR,			"macr",			175 },
	{ MANUAL_ENTITY_DEG,			"deg",			176 },
	{ MANUAL_ENTITY_PLUSMN,			"plusmn",		177 },
	{ MANUAL_ENTITY_SUP2,			"sup2",			178 },
	{ MANUAL_ENTITY_SUP3,			"sup3",			179 },
	{ MANUAL_ENTITY_ACUTE,			"acute",		180 },
	{ MANUAL_ENTITY_MICRO,			"micro",		181 },
	{ MANUAL_ENTITY_PARA,			"para",			182 },
	{ MANUAL_ENTITY_MIDDOT,			"middot",		183 },
	{ MANUAL_ENTITY_CEDIL,			"cedil",		184 },
	{ MANUAL_ENTITY_SUP1,			"sup1",			185 },
	{ MANUAL_ENTITY_ORDM,			"ordm",			186 },
	{ MANUAL_ENTITY_RAQUO,			"raquo",		187 },
	{ MANUAL_ENTITY_FRAC14,			"frac14",		188 },
	{ MANUAL_ENTITY_FRAC12,			"frac12",		189 },
	{ MANUAL_ENTITY_FRAC34,			"frac34",		190 },
	{ MANUAL_ENTITY_IQUEST,			"iquest",		191 },
	{ MANUAL_ENTITY_AGRAVE_U,		"Agrave",		192 },
	{ MANUAL_ENTITY_AACUTE_U,		"Aacute",		193 },
	{ MANUAL_ENTITY_ACIRC_U,		"Acirc",		194 },
	{ MANUAL_ENTITY_ATILDE_U,		"Atilde",		195 },
	{ MANUAL_ENTITY_AUML_U,			"Auml",			196 },
	{ MANUAL_ENTITY_ARING_U,		"Aring",		197 },
	{ MANUAL_ENTITY_AELIG_U,		"AElig",		198 },
	{ MANUAL_ENTITY_CCEDIL_U,		"Ccedil",		199 },
	{ MANUAL_ENTITY_EGRAVE_U,		"Egrave",		200 },
	{ MANUAL_ENTITY_EACUTE_U,		"Eacute",		201 },
	{ MANUAL_ENTITY_ECIRC_U,		"Ecirc",		202 },
	{ MANUAL_ENTITY_EUML_U,			"Euml",			203 },
	{ MANUAL_ENTITY_IGRAVE_U,		"Igrave",		204 },
	{ MANUAL_ENTITY_IACUTE_U,		"Iacute",		205 },
	{ MANUAL_ENTITY_ICIRC_U,		"Icirc",		206 },
	{ MANUAL_ENTITY_IUML_U,			"Iuml",			207 },
	{ MANUAL_ENTITY_ETH_U,			"ETH",			208 },
	{ MANUAL_ENTITY_NTILDE_U,		"Ntilde",		209 },
	{ MANUAL_ENTITY_OGRAVE_U,		"Ograve",		210 },
	{ MANUAL_ENTITY_OACUTE_U,		"Oacute",		211 },
	{ MANUAL_ENTITY_OCIRC_U,		"Ocirc",		212 },
	{ MANUAL_ENTITY_OTILDE_U,		"Otilde",		213 },
	{ MANUAL_ENTITY_OUML_U,			"Ouml",			214 },
	{ MANUAL_ENTITY_TIMES,			"times",		215 },
	{ MANUAL_ENTITY_OSLASH_U,		"Oslash",		216 },
	{ MANUAL_ENTITY_UGRAVE_U,		"Ugrave",		217 },
	{ MANUAL_ENTITY_UACUTE_U,		"Uacute",		218 },
	{ MANUAL_ENTITY_UCIRC_U,		"Ucirc",		219 },
	{ MANUAL_ENTITY_UUML_U,			"Uuml",			220 },
	{ MANUAL_ENTITY_YACUTE_U,		"Yacute",		221 },
	{ MANUAL_ENTITY_THORN_U,		"THORN",		222 },
	{ MANUAL_ENTITY_SZLIG_L,		"szlig",		223 },
	{ MANUAL_ENTITY_AGRAVE_L,		"agrave",		224 },
	{ MANUAL_ENTITY_AACUTE_L,		"aacute",		225 },
	{ MANUAL_ENTITY_ACIRC_L,		"acirc",		226 },
	{ MANUAL_ENTITY_ATILDE_L,		"atilde",		227 },
	{ MANUAL_ENTITY_AUML_L,			"auml",			228 },
	{ MANUAL_ENTITY_ARING_L,		"aring",		229 },
	{ MANUAL_ENTITY_AELIG_L,		"aelig",		230 },
	{ MANUAL_ENTITY_CCEDIL_L,		"ccedil",		231 },
	{ MANUAL_ENTITY_EGRAVE_L,		"egrave",		232 },
	{ MANUAL_ENTITY_EACUTE_L,		"eacute",		233 },
	{ MANUAL_ENTITY_ECIRC_L,		"ecirc",		234 },
	{ MANUAL_ENTITY_EUML_L,			"euml",			235 },
	{ MANUAL_ENTITY_IGRAVE_L,		"igrave",		236 },
	{ MANUAL_ENTITY_IACUTE_L,		"iacute",		237 },
	{ MANUAL_ENTITY_ICIRC_L,		"icirc",		238 },
	{ MANUAL_ENTITY_IUML_L,			"iuml",			239 },
	{ MANUAL_ENTITY_ETH_L,			"eth",			240 },
	{ MANUAL_ENTITY_NTILDE_L,		"ntilde",		241 },
	{ MANUAL_ENTITY_OGRAVE_L,		"ograve",		242 },
	{ MANUAL_ENTITY_OACUTE_L,		"oacute",		243 },
	{ MANUAL_ENTITY_OCIRC_L,		"ocirc",		244 },
	{ MANUAL_ENTITY_OTILDE_L,		"otilde",		245 },
	{ MANUAL_ENTITY_OUML_L,			"ouml",			246 },
	{ MANUAL_ENTITY_DIV,			"div",			247 },
	{ MANUAL_ENTITY_OSLASH_L,		"oslash",		248 },
	{ MANUAL_ENTITY_UGRAVE_L,		"ugrave",		249 },
	{ MANUAL_ENTITY_UACUTE_L,		"uacute",		250 },
	{ MANUAL_ENTITY_UCIRC_L,		"ucirc",		251 },
	{ MANUAL_ENTITY_UUML_L,			"uuml",			252 },
	{ MANUAL_ENTITY_YACUTE_L,		"yacute",		253 },
	{ MANUAL_ENTITY_THORN_L,		"thorn",		254 },
	{ MANUAL_ENTITY_YUML_L,			"yuml",			255 },

	/* Latin Extended A */

	{ MANUAL_ENTITY_AMACR_U,		"Amacr",		256 },
	{ MANUAL_ENTITY_AMACR_L,		"amacr",		257 },
	{ MANUAL_ENTITY_ABREVE_U,		"Abreve",		258 },
	{ MANUAL_ENTITY_ABREVE_L,		"abreve",		259 },
	{ MANUAL_ENTITY_AOGON_U,		"Aogon",		260 },
	{ MANUAL_ENTITY_AOGON_L,		"aogon",		261 },
	{ MANUAL_ENTITY_CACUTE_U,		"Cacute",		262 },
	{ MANUAL_ENTITY_CACUTE_L,		"cacute",		263 },
	{ MANUAL_ENTITY_CCIRC_U,		"Ccirc",		264 },
	{ MANUAL_ENTITY_CCIRC_L,		"ccirc",		265 },
	{ MANUAL_ENTITY_CDOT_U,			"Cdot",			266 },
	{ MANUAL_ENTITY_CDOT_L,			"cdot",			267 },
	{ MANUAL_ENTITY_CCARON_U,		"Ccaron",		268 },
	{ MANUAL_ENTITY_CCARON_L,		"ccaron",		269 },
	{ MANUAL_ENTITY_DCARON_U,		"Dcaron",		270 },
	{ MANUAL_ENTITY_DCARON_L,		"dcaron",		271 },
	{ MANUAL_ENTITY_DSTROK_U,		"Dstrok",		272 },
	{ MANUAL_ENTITY_DSTROK_L,		"dstrok",		273 },
	{ MANUAL_ENTITY_EMACR_U,		"Emacr",		274 },
	{ MANUAL_ENTITY_EMACR_L,		"emacr",		275 },
	{ MANUAL_ENTITY_EDOT_U,			"Edot",			278 },
	{ MANUAL_ENTITY_EDOT_L,			"edot",			279 },
	{ MANUAL_ENTITY_EOGON_U,		"Eogon",		280 },
	{ MANUAL_ENTITY_EOGON_L,		"eogon",		281 },
	{ MANUAL_ENTITY_ECARON_U,		"Ecaron",		282 },
	{ MANUAL_ENTITY_ECARON_L,		"ecaron",		283 },
	{ MANUAL_ENTITY_GCIRC_U,		"Gcirc",		284 },
	{ MANUAL_ENTITY_GCIRC_L,		"gcirc",		285 },
	{ MANUAL_ENTITY_GBREVE_U,		"Gbreve",		286 },
	{ MANUAL_ENTITY_GBREVE_L,		"gbreve",		287 },
	{ MANUAL_ENTITY_GDOT_U,			"Gdot",			288 },
	{ MANUAL_ENTITY_GDOT_L,			"gdot",			289 },
	{ MANUAL_ENTITY_GCEDIL,			"Gcedil",		290 },
	{ MANUAL_ENTITY_HCIRC_U,		"Hcirc",		292 },
	{ MANUAL_ENTITY_HCIRC_L,		"hcirc",		293 },
	{ MANUAL_ENTITY_HSTROK_U,		"Hstrok",		294 },
	{ MANUAL_ENTITY_HSTROK_L,		"hstrok",		295 },
	{ MANUAL_ENTITY_ITILDE_U,		"Itilde",		296 },
	{ MANUAL_ENTITY_ITILDE_L,		"itilde",		297 },
	{ MANUAL_ENTITY_IMACR_U,		"Imacr",		298 },
	{ MANUAL_ENTITY_IMACR_L,		"imacr",		299 },
	{ MANUAL_ENTITY_IOGON_U,		"Iogon",		302 },
	{ MANUAL_ENTITY_IOGON_L,		"iogon",		303 },
	{ MANUAL_ENTITY_IDOT_U,			"Idot",			304 },
	{ MANUAL_ENTITY_INODOT_L,		"inodot",		305 },
	{ MANUAL_ENTITY_IJLIG_U,		"IJlig",		306 },
	{ MANUAL_ENTITY_IJLIG_L,		"ijlig",		307 },
	{ MANUAL_ENTITY_JCIRC_U,		"Jcirc",		308 },
	{ MANUAL_ENTITY_JCIRC_L,		"jcirc",		309 },
	{ MANUAL_ENTITY_KCEDIL_U,		"Kcedil",		310 },
	{ MANUAL_ENTITY_KCEDIL_L,		"kcedil",		311 },
	{ MANUAL_ENTITY_KGREEN_L,		"kgreen",		312 },
	{ MANUAL_ENTITY_LACUTE_U,		"Lacute",		313 },
	{ MANUAL_ENTITY_LACUTE_L,		"lacute",		314 },
	{ MANUAL_ENTITY_LCEDIL_U,		"Lcedil",		315 },
	{ MANUAL_ENTITY_LCEDIL_L,		"lcedil",		316 },
	{ MANUAL_ENTITY_LCARON_U,		"Lcaron",		317 },
	{ MANUAL_ENTITY_LCARON_L,		"lcaron",		318 },
	{ MANUAL_ENTITY_LMIDOT_U,		"Lmidot",		319 },
	{ MANUAL_ENTITY_LMIDOT_L,		"lmidot",		320 },
	{ MANUAL_ENTITY_LSTROK_U,		"Lstrok",		321 },
	{ MANUAL_ENTITY_LSTROK_L,		"lstrok",		322 },
	{ MANUAL_ENTITY_NACUTE_U,		"Nacute",		323 },
	{ MANUAL_ENTITY_NACUTE_L,		"nacute",		324 },
	{ MANUAL_ENTITY_NCEDIL_U,		"Ncedil",		325 },
	{ MANUAL_ENTITY_NCEDIL_L,		"ncedil",		326 },
	{ MANUAL_ENTITY_NCARON_U,		"Ncaron",		327 },
	{ MANUAL_ENTITY_NCARON_L,		"ncaron",		328 },
	{ MANUAL_ENTITY_NAPOS_L,		"napos",		329 },
	{ MANUAL_ENTITY_ENG_U,			"ENG",			330 },
	{ MANUAL_ENTITY_ENG_L,			"eng",			331 },
	{ MANUAL_ENTITY_OMACR_U,		"Omacr",		332 },
	{ MANUAL_ENTITY_OMACR_L,		"omacr",		333 },
	{ MANUAL_ENTITY_ODBLAC_U,		"Odblac",		336 },
	{ MANUAL_ENTITY_ODBLAC_L,		"odblac",		337 },
	{ MANUAL_ENTITY_OELIG_U,		"OElig",		338 },
	{ MANUAL_ENTITY_OELIG_L,		"oelig",		339 },
	{ MANUAL_ENTITY_RACUTE_U,		"Racute",		340 },
	{ MANUAL_ENTITY_RACUTE_L,		"racute",		341 },
	{ MANUAL_ENTITY_RCEDIL_U,		"Rcedil",		342 },
	{ MANUAL_ENTITY_RCEDIL_L,		"rcedil",		343 },
	{ MANUAL_ENTITY_RCARON_U,		"Rcaron",		344 },
	{ MANUAL_ENTITY_RCARON_L,		"rcaron",		345 },
	{ MANUAL_ENTITY_SACUTE_U,		"Sacute",		346 },
	{ MANUAL_ENTITY_SACUTE_L,		"sacute",		347 },
	{ MANUAL_ENTITY_SCIRC_U,		"Scirc",		348 },
	{ MANUAL_ENTITY_SCIRC_L,		"scirc",		349 },
	{ MANUAL_ENTITY_SCEDIL_U,		"Scedil",		350 },
	{ MANUAL_ENTITY_SCEDIL_L,		"scedil",		351 },
	{ MANUAL_ENTITY_SCARON_U,		"Scaron",		352 },
	{ MANUAL_ENTITY_SCARON_L,		"scaron",		353 },
	{ MANUAL_ENTITY_TCEDIL_U,		"Tcedil",		354 },
	{ MANUAL_ENTITY_TCEDIL_L,		"tcedil",		355 },
	{ MANUAL_ENTITY_TCARON_U,		"Tcaron",		356 },
	{ MANUAL_ENTITY_TCARON_L,		"tcaron",		357 },
	{ MANUAL_ENTITY_TSTROK_U,		"Tstrok",		358 },
	{ MANUAL_ENTITY_TSTROK_L,		"tstrok",		359 },
	{ MANUAL_ENTITY_UTILDE_U,		"Utilde",		360 },
	{ MANUAL_ENTITY_UTILDE_L,		"utilde",		361 },
	{ MANUAL_ENTITY_UMACR_U,		"Umacr",		362 },
	{ MANUAL_ENTITY_UMACR_L,		"umacr",		363 },
	{ MANUAL_ENTITY_UBREVE_U,		"Ubreve",		364 },
	{ MANUAL_ENTITY_UBREVE_L,		"ubreve",		365 },
	{ MANUAL_ENTITY_URING_U,		"Uring",		366 },
	{ MANUAL_ENTITY_URING_L,		"uring",		367 },
	{ MANUAL_ENTITY_UDBLAC_U,		"Udblac",		368 },
	{ MANUAL_ENTITY_UDBLAC_L,		"udblac",		369 },
	{ MANUAL_ENTITY_UOGON_U,		"Uogon",		370 },
	{ MANUAL_ENTITY_UOGON_L,		"uogon",		371 },
	{ MANUAL_ENTITY_WCIRC_U,		"Wcirc",		372 },
	{ MANUAL_ENTITY_WCIRC_L,		"wcirc",		373 },
	{ MANUAL_ENTITY_YCIRC_U,		"Ycirc",		374 },
	{ MANUAL_ENTITY_YCIRC_L,		"ycirc",		375 },
	{ MANUAL_ENTITY_YUML_U,			"Yuml",			376 },
	{ MANUAL_ENTITY_ZACUTE_U,		"Zacute",		377 },
	{ MANUAL_ENTITY_ZACUTE_L,		"zacute",		378 },
	{ MANUAL_ENTITY_ZDOT_U,			"Zdot",			379 },
	{ MANUAL_ENTITY_ZDOT_L,			"zdot",			380 },
	{ MANUAL_ENTITY_ZCARON_U,		"Zcaron",		381 },
	{ MANUAL_ENTITY_ZCARON_L,		"zcaron",		382 },

	/* General Punctuation */

	{ MANUAL_ENTITY_ENSP,			"ensp",			8194 },
	{ MANUAL_ENTITY_EMSP,			"emsp",			8195 },
	{ MANUAL_ENTITY_EMSP13,			"emsp13",		8196 },
	{ MANUAL_ENTITY_EMSP14,			"emsp14",		8197 },
	{ MANUAL_ENTITY_NUMSP,			"numsp",		8199 },
	{ MANUAL_ENTITY_PUNCSP,			"puncsp",		8200 },
	{ MANUAL_ENTITY_THINSP,			"thinsp",		8201 },
	{ MANUAL_ENTITY_HAIRSP,			"hairsp",		8202 },
	{ MANUAL_ENTITY_ZEROWIDTHSPACE,		"ZeroWidthSpace",	8203 },
	{ MANUAL_ENTITY_ZWNJ,			"zwnj",			8204 },
	{ MANUAL_ENTITY_ZWJ,			"zwj",			8205 },
	{ MANUAL_ENTITY_LRM,			"lrm",			8206 },
	{ MANUAL_ENTITY_RLM,			"rlm",			8207 },
	{ MANUAL_ENTITY_DASH,			"dash",			8208 },
	{ MANUAL_ENTITY_NDASH,			"ndash",		8211 },
	{ MANUAL_ENTITY_MDASH,			"mdash",		8212 },
	{ MANUAL_ENTITY_HORBAR,			"horbar",		8213 },
	{ MANUAL_ENTITY_VERBAR,			"Verbar",		8214 },
	{ MANUAL_ENTITY_LSQUO,			"lsquo",		8216 },
	{ MANUAL_ENTITY_RSQUO,			"rsquo",		8217 },
	{ MANUAL_ENTITY_LDQUO,			"ldquo",		8220 },
	{ MANUAL_ENTITY_RDQUO,			"rdquo",		8221 },

	/* Mathematical Operators */

	{ MANUAL_ENTITY_FORALL,			"forall",		8704 },
	{ MANUAL_ENTITY_COMP,			"comp",			8705 },
	{ MANUAL_ENTITY_PART,			"part",			8706 },
	{ MANUAL_ENTITY_EXIST,			"exist",		8707 },
	{ MANUAL_ENTITY_NEXIST,			"nexist",		8708 },
	{ MANUAL_ENTITY_EMPTY,			"empty",		8709 },
	{ MANUAL_ENTITY_NABLA,			"nabla",		8711 },
	{ MANUAL_ENTITY_IN,			"in",			8712 },
	{ MANUAL_ENTITY_NOTIN,			"notin",		8713 },
	{ MANUAL_ENTITY_NI,			"ni",			8715 },
	{ MANUAL_ENTITY_NOTNI,			"notni",		8716 },
	{ MANUAL_ENTITY_PROD,			"prod",			8719 },
	{ MANUAL_ENTITY_COPROD,			"coprod",		8720 },
	{ MANUAL_ENTITY_SUM,			"sum",			8721 },
	{ MANUAL_ENTITY_MINUS,			"minus",		8722 },
	{ MANUAL_ENTITY_MNPLUS,			"mnplus",		8723 },

	{ MANUAL_ENTITY_LE,			"le",			8804 },
	{ MANUAL_ENTITY_GE,			"ge",			8805 },
	{ MANUAL_ENTITY_LEQQ,			"leqq",			8806 },
	{ MANUAL_ENTITY_GEQQ,			"geqq",			8807 },

	/* End of List */

	{ MANUAL_ENTITY_NONE,			"",			0x7fffffff }
};

/* Static function prototypes. */

static bool manual_entity_initialise_lists(void);

/**
 * Given a node containing an entity, return the entity type.
 *
 * \param *name		Pointer to the textual entity name.
 * \return		The entity type, or MANUAL_ENTITY_NONE if unknown.
 */

enum manual_entity_type manual_entity_find_type(char *name)
{
	struct manual_entity_definition *entity;

	if (name == NULL)
		return MANUAL_ENTITY_NONE;

	/* If the search tree hasn't been initialised, do it now.*/

	if (manual_entity_search_tree == NULL && !manual_entity_initialise_lists())
		return MANUAL_ENTITY_NONE;

	/* Find the entity definition. */

	entity = search_tree_find_entry(manual_entity_search_tree, name);
	if (entity == NULL) {
		msg_report(MSG_UNKNOWN_ENTITY, name);
		return MANUAL_ENTITY_NONE;
	}

	return entity->type;
}

/**
 * Given an entity type, return the textual entity name.
 *
 * \param type		The entity type to look up.
 * \return		Pointer to the entity's textual name, or to NULL if
 *			the type was not recognised.
 */

const char *manual_entity_find_name(enum manual_entity_type type)
{
	if (manual_entities_max_entries <= 0 && !manual_entity_initialise_lists())
		return NULL;

	if (type == MANUAL_ENTITY_NONE || type < 0 || type >= manual_entities_max_entries)
		return NULL;

	/* Look up the tag name.
	 *
	 * WARNING: This relies on the array indices matching the values of
	 * the entity enum entries.
	 */

	return manual_entity_names[type].name;
}

/**
 * Given an entity type, return the Unicode codepoint.
 *
 * \param type		The entity type to look up.
 * \return		The Unicode codepoint, or -1 if not recognised.
 */

int manual_entity_find_codepoint(enum manual_entity_type type)
{
	if (manual_entities_max_entries <= 0 && !manual_entity_initialise_lists())
		return MANUAL_ENTITY_NO_CODEPOINT;

	if (type == MANUAL_ENTITY_NONE || type < 0 || type >= manual_entities_max_entries)
		return MANUAL_ENTITY_NO_CODEPOINT;

	/* Look up the tag name.
	 *
	 * WARNING: This relies on the array indices matching the values of
	 * the entity enum entries.
	 */

	return manual_entity_names[type].unicode;
}

/**
 * Given a unicode code point, return an appropriate entity name
 * if one exists.
 *
 * \param type		The entity type to look up.
 * \return		Pointer to the entity's textual name, or to "" if
 *			the type was not recognised.
 */

const char *manual_entity_find_name_from_codepoint(int codepoint)
{
	int first = 0, last, middle;

	if (manual_entities_max_entries <= 0 && !manual_entity_initialise_lists())
		return NULL;

	if (codepoint < 0)
		return NULL;

	/* Find the character in the current encoding. */

	last = manual_entities_max_entries;

	while (first <= last) {
		middle = (first + last) / 2;

		if (manual_entity_names[middle].unicode == codepoint)
			return manual_entity_names[middle].name;
		else if (manual_entity_names[middle].unicode < codepoint)
			first = middle + 1;
		else
			last = middle - 1;
	}

	/* Look up the tag name.
	 *
	 * WARNING: This relies on the array indices matching the values of
	 * the entity enum entries.
	 */

	return NULL;
}

/**
 * Initialise the parse tree and entity list.
 * 
 * \return		True if successful; False on failure.
 */

static bool manual_entity_initialise_lists(void)
{
	int i, current_code = -1;

	if (manual_entity_search_tree != NULL || manual_entities_max_entries > 0)
		return false;

	/* Create a new search tree. */

	manual_entity_search_tree = search_tree_create();
	if (manual_entity_search_tree == NULL)
		return false;

	/* Add the element tags to the search tree. */

	for (i = 0; manual_entity_names[i].type != MANUAL_ENTITY_NONE; i++) {
		if (!search_tree_add_entry(manual_entity_search_tree, manual_entity_names[i].name, &(manual_entity_names[i])))
			return false;

		/* Check that the values are in the correct index slots, to allow for
		 * quick lookups.
		 */

		if (manual_entity_names[i].type != i || manual_entity_names[i].unicode < current_code) {
			msg_report(MSG_ENTITY_OUT_OF_SEQ);
			return false;
		}

		current_code = manual_entity_names[i].unicode;
	}

	manual_entities_max_entries = i;

	return true;
}
