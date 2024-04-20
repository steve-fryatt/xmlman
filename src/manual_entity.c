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
	enum manual_entity_type	type;			/**< The type of entity.			*/
	int			unicode;		/**< The unicode code point for the entity.	*/
	const char		*name;			/**< The name of the entity	.		*/
	char			**alternatives;		/**< A list of alternative names, or NULL.	*/
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

	{ MANUAL_ENTITY_SMILEYFACE,	MANUAL_ENTITY_NO_CODEPOINT,	"smileyface",	NULL },
	{ MANUAL_ENTITY_SADFACE,	MANUAL_ENTITY_NO_CODEPOINT,	"sadface",	NULL },
	{ MANUAL_ENTITY_MSEP,		MANUAL_ENTITY_NO_CODEPOINT,	"msep",		NULL },

	/* Basic Latin */

	{ MANUAL_ENTITY_TAB,			9,	"Tab",				NULL },						// 0x09
	{ MANUAL_ENTITY_NEWLINE,		10,	"NewLine",			NULL },						// 0x0a
	{ MANUAL_ENTITY_EXCL,			33,	"excl",				NULL },						// 0x21
	{ MANUAL_ENTITY_QUOT,			34,	"quot",				(char*[]) { "QUOT", NULL } },			// 0x22
	{ MANUAL_ENTITY_NUM,			35,	"num",				NULL },						// 0x23
	{ MANUAL_ENTITY_DOLLAR,			36,	"dollar",			NULL },						// 0x24
	{ MANUAL_ENTITY_PERCNT,			37,	"percnt",			NULL },						// 0x25
	{ MANUAL_ENTITY_AMP,			38,	"amp",				(char*[]) { "AMP", NULL } },			// 0x26
	{ MANUAL_ENTITY_APOS,			39,	"apos",				NULL },						// 0x27
	{ MANUAL_ENTITY_LPAR,			40,	"lpar",				NULL },						// 0x28
	{ MANUAL_ENTITY_RPAR,			41,	"rpar",				NULL },						// 0x29
	{ MANUAL_ENTITY_AST,			42,	"ast",				(char*[]) { "midast", NULL } },			// 0x2a
	{ MANUAL_ENTITY_PLUS,			43,	"plus",				NULL },						// 0x2b
	{ MANUAL_ENTITY_COMMA,			44,	"comma",			NULL },						// 0x2c
	{ MANUAL_ENTITY_PERIOD,			46,	"period",			NULL },						// 0x2e
	{ MANUAL_ENTITY_SOL,			47,	"sol",				NULL },						// 0x2f
	{ MANUAL_ENTITY_COLON,			58,	"colon",			NULL },						// 0x3a
	{ MANUAL_ENTITY_SEMI,			59,	"semi",				NULL },						// 0x3b
	{ MANUAL_ENTITY_LT,			60,	"lt",				(char*[]) { "LT", NULL } },			// 0x3c
	{ MANUAL_ENTITY_EQUALS,			61,	"equals",			NULL },						// 0x3d
	{ MANUAL_ENTITY_GT,			62,	"gt",				(char*[]) { "GT", NULL } },			// 0x3e
	{ MANUAL_ENTITY_QUEST,			63,	"quest",			NULL },						// 0x3f
	{ MANUAL_ENTITY_COMMAT,			64,	"commat",			NULL },						// 0x40
	{ MANUAL_ENTITY_LBRACK,			91,	"lbrack",			(char*[]) { "lsqb", NULL } },			// 0x5b
	{ MANUAL_ENTITY_BSOL,			92,	"bsol",				NULL },						// 0x5c
	{ MANUAL_ENTITY_RBRACK,			93,	"rbrack",			(char*[]) { "rsqb", NULL } },			// 0x5d
	{ MANUAL_ENTITY_HAT,			94,	"Hat",				NULL },						// 0x5e
	{ MANUAL_ENTITY_UNDERBAR,		95,	"lowbar",			(char*[]) { "UnderBar", NULL } },		// 0x5f
	{ MANUAL_ENTITY_DIACRITICALGRAVE,	96,	"grave",			(char*[]) { "DiacriticalGrave", NULL } },	// 0x60
	{ MANUAL_ENTITY_LBRACE,			123,	"lbrace",			(char*[]) { "lcub", NULL } },			// 0x7b
	{ MANUAL_ENTITY_VERTICALLINE,		124,	"verbar",			(char*[]) { "VerticalLine", "vert", NULL } },	// 0x7c
	{ MANUAL_ENTITY_RBRACE,			125,	"rbrace",			(char*[]) { "rcub", NULL } },			// 0x7d

	/* Latin-1 Supplement */

	{ MANUAL_ENTITY_NBSP,			160,	"nbsp",				(char*[]) { "NonBreakingSpace", NULL } },	// 0xa0
	{ MANUAL_ENTITY_IEXCL,			161,	"iexcl",			NULL },						// 0xa1
	{ MANUAL_ENTITY_CENT,			162,	"cent",				NULL },						// 0xa2
	{ MANUAL_ENTITY_POUND,			163,	"pound",			NULL },						// 0xa3
	{ MANUAL_ENTITY_CURREN,			164,	"curren",			NULL },						// 0xa4
	{ MANUAL_ENTITY_YEN,			165,	"yen",				NULL },						// 0xa5
	{ MANUAL_ENTITY_BRVBAR,			166,	"brvbar",			NULL },						// 0xa6
	{ MANUAL_ENTITY_SECT,			167,	"sect",				NULL },						// 0xa7
	{ MANUAL_ENTITY_UML,			168,	"uml",				(char*[]) { "Dot", "DoubleDot", "die", NULL } },	// 0xa8
	{ MANUAL_ENTITY_COPY,			169,	"copy",				(char*[]) { "COPY", NULL } },			// 0xa9
	{ MANUAL_ENTITY_ORDF_L,			170,	"ordf",				NULL },						// 0xaa
	{ MANUAL_ENTITY_LAQUO,			171,	"laquo",			NULL },						// 0xab
	{ MANUAL_ENTITY_NOT,			172,	"not",				NULL },						// 0xac
	{ MANUAL_ENTITY_SHY,			173,	"shy",				NULL },						// 0xad
	{ MANUAL_ENTITY_REG,			174,	"reg",				(char*[]) { "REG", "circledR", NULL } },	// 0xae
	{ MANUAL_ENTITY_MACR,			175,	"macr",				(char*[]) { "strns", NULL } },			// 0xaf
	{ MANUAL_ENTITY_DEG,			176,	"deg",				NULL },						// 0xb0
	{ MANUAL_ENTITY_PLUSMN,			177,	"plusmn",			(char*[]) { "PlusMinus", "pm", NULL } },	// 0xb1
	{ MANUAL_ENTITY_SUP2,			178,	"sup2",				NULL },						// 0xb2
	{ MANUAL_ENTITY_SUP3,			179,	"sup3",				NULL },						// 0xb3
	{ MANUAL_ENTITY_ACUTE,			180,	"acute",			(char*[]) { "DiacriticalAcute", NULL } },	// 0xb4
	{ MANUAL_ENTITY_MICRO_L,		181,	"micro",			NULL },						// 0xb5
	{ MANUAL_ENTITY_PARA,			182,	"para",				NULL },						// 0xb6
	{ MANUAL_ENTITY_MIDDOT,			183,	"middot",			(char*[]) { "CenterDot", "centerdot", NULL } },	// 0xb7
	{ MANUAL_ENTITY_CEDIL,			184,	"cedil",			(char*[]) { "Cedilla", NULL } },		// 0xb8
	{ MANUAL_ENTITY_SUP1,			185,	"sup1",				NULL },						// 0xb9
	{ MANUAL_ENTITY_ORDM_L,			186,	"ordm",				NULL },						// 0xba
	{ MANUAL_ENTITY_RAQUO,			187,	"raquo",			NULL },						// 0xbb
	{ MANUAL_ENTITY_FRAC14,			188,	"frac14",			NULL },						// 0xbc
	{ MANUAL_ENTITY_FRAC12,			189,	"frac12",			(char*[]) { "half", NULL } },			// 0xbd
	{ MANUAL_ENTITY_FRAC34,			190,	"frac34",			NULL },						// 0xbe
	{ MANUAL_ENTITY_IQUEST,			191,	"iquest",			NULL },						// 0xbf
	{ MANUAL_ENTITY_AGRAVE_U,		192,	"Agrave",			NULL },						// 0xc0
	{ MANUAL_ENTITY_AACUTE_U,		193,	"Aacute",			NULL },						// 0xc1
	{ MANUAL_ENTITY_ACIRC_U,		194,	"Acirc",			NULL },						// 0xc2
	{ MANUAL_ENTITY_ATILDE_U,		195,	"Atilde",			NULL },						// 0xc3
	{ MANUAL_ENTITY_AUML_U,			196,	"Auml",				NULL },						// 0xc4
	{ MANUAL_ENTITY_ARING_U,		197,	"Aring",			(char*[]) { "angst", NULL } },			// 0xc5
	{ MANUAL_ENTITY_AELIG_U,		198,	"AElig",			NULL },						// 0xc6
	{ MANUAL_ENTITY_CCEDIL_U,		199,	"Ccedil",			NULL },						// 0xc7
	{ MANUAL_ENTITY_EGRAVE_U,		200,	"Egrave",			NULL },						// 0xc8
	{ MANUAL_ENTITY_EACUTE_U,		201,	"Eacute",			NULL },						// 0xc9
	{ MANUAL_ENTITY_ECIRC_U,		202,	"Ecirc",			NULL },						// 0xca
	{ MANUAL_ENTITY_EUML_U,			203,	"Euml",				NULL },						// 0xcb
	{ MANUAL_ENTITY_IGRAVE_U,		204,	"Igrave",			NULL },						// 0xcc
	{ MANUAL_ENTITY_IACUTE_U,		205,	"Iacute",			NULL },						// 0xcd
	{ MANUAL_ENTITY_ICIRC_U,		206,	"Icirc",			NULL },						// 0xce
	{ MANUAL_ENTITY_IUML_U,			207,	"Iuml",				NULL },						// 0xcf
	{ MANUAL_ENTITY_ETH_U,			208,	"ETH",				NULL },						// 0xd0
	{ MANUAL_ENTITY_NTILDE_U,		209,	"Ntilde",			NULL },						// 0xd1
	{ MANUAL_ENTITY_OGRAVE_U,		210,	"Ograve",			NULL },						// 0xd2
	{ MANUAL_ENTITY_OACUTE_U,		211,	"Oacute",			NULL },						// 0xd3
	{ MANUAL_ENTITY_OCIRC_U,		212,	"Ocirc",			NULL },						// 0xd4
	{ MANUAL_ENTITY_OTILDE_U,		213,	"Otilde",			NULL },						// 0xd5
	{ MANUAL_ENTITY_OUML_U,			214,	"Ouml",				NULL },						// 0xd6
	{ MANUAL_ENTITY_TIMES,			215,	"times",			NULL },						// 0xd7
	{ MANUAL_ENTITY_OSLASH_U,		216,	"Oslash",			NULL },						// 0xd8
	{ MANUAL_ENTITY_UGRAVE_U,		217,	"Ugrave",			NULL },						// 0xd9
	{ MANUAL_ENTITY_UACUTE_U,		218,	"Uacute",			NULL },						// 0xda
	{ MANUAL_ENTITY_UCIRC_U,		219,	"Ucirc",			NULL },						// 0xdb
	{ MANUAL_ENTITY_UUML_U,			220,	"Uuml",				NULL },						// 0xdc
	{ MANUAL_ENTITY_YACUTE_U,		221,	"Yacute",			NULL },						// 0xdd
	{ MANUAL_ENTITY_THORN_U,		222,	"THORN",			NULL },						// 0xde
	{ MANUAL_ENTITY_SZLIG_L,		223,	"szlig",			NULL },						// 0xdf
	{ MANUAL_ENTITY_AGRAVE_L,		224,	"agrave",			NULL },						// 0xe0
	{ MANUAL_ENTITY_AACUTE_L,		225,	"aacute",			NULL },						// 0xe1
	{ MANUAL_ENTITY_ACIRC_L,		226,	"acirc",			NULL },						// 0xe2
	{ MANUAL_ENTITY_ATILDE_L,		227,	"atilde",			NULL },						// 0xe3
	{ MANUAL_ENTITY_AUML_L,			228,	"auml",				NULL },						// 0xe4
	{ MANUAL_ENTITY_ARING_L,		229,	"aring",			NULL },						// 0xe5
	{ MANUAL_ENTITY_AELIG_L,		230,	"aelig",			NULL },						// 0xe6
	{ MANUAL_ENTITY_CCEDIL_L,		231,	"ccedil",			NULL },						// 0xe7
	{ MANUAL_ENTITY_EGRAVE_L,		232,	"egrave",			NULL },						// 0xe8
	{ MANUAL_ENTITY_EACUTE_L,		233,	"eacute",			NULL },						// 0xe9
	{ MANUAL_ENTITY_ECIRC_L,		234,	"ecirc",			NULL },						// 0xea
	{ MANUAL_ENTITY_EUML_L,			235,	"euml",				NULL },						// 0xeb
	{ MANUAL_ENTITY_IGRAVE_L,		236,	"igrave",			NULL },						// 0xec
	{ MANUAL_ENTITY_IACUTE_L,		237,	"iacute",			NULL },						// 0xed
	{ MANUAL_ENTITY_ICIRC_L,		238,	"icirc",			NULL },						// 0xee
	{ MANUAL_ENTITY_IUML_L,			239,	"iuml",				NULL },						// 0xef
	{ MANUAL_ENTITY_ETH_L,			240,	"eth",				NULL },						// 0xf0
	{ MANUAL_ENTITY_NTILDE_L,		241,	"ntilde",			NULL },						// 0xf1
	{ MANUAL_ENTITY_OGRAVE_L,		242,	"ograve",			NULL },						// 0xf2
	{ MANUAL_ENTITY_OACUTE_L,		243,	"oacute",			NULL },						// 0xf3
	{ MANUAL_ENTITY_OCIRC_L,		244,	"ocirc",			NULL },						// 0xf4
	{ MANUAL_ENTITY_OTILDE_L,		245,	"otilde",			NULL },						// 0xf5
	{ MANUAL_ENTITY_OUML_L,			246,	"ouml",				NULL },						// 0xf6
	{ MANUAL_ENTITY_DIVIDE,			247,	"divide",			(char*[]) { "div", NULL } },			// 0xf7
	{ MANUAL_ENTITY_OSLASH_L,		248,	"oslash",			NULL },						// 0xf8
	{ MANUAL_ENTITY_UGRAVE_L,		249,	"ugrave",			NULL },						// 0xf9
	{ MANUAL_ENTITY_UACUTE_L,		250,	"uacute",			NULL },						// 0xfa
	{ MANUAL_ENTITY_UCIRC_L,		251,	"ucirc",			NULL },						// 0xfb
	{ MANUAL_ENTITY_UUML_L,			252,	"uuml",				NULL },						// 0xfc
	{ MANUAL_ENTITY_YACUTE_L,		253,	"yacute",			NULL },						// 0xfd
	{ MANUAL_ENTITY_THORN_L,		254,	"thorn",			NULL },						// 0xfe
	{ MANUAL_ENTITY_YUML_L,			255,	"yuml",				NULL },						// 0xff

	/* Latin Extended A */

	{ MANUAL_ENTITY_AMACR_U,		256,	"Amacr",			NULL },						// 0x100
	{ MANUAL_ENTITY_AMACR_L,		257,	"amacr",			NULL },						// 0x101
	{ MANUAL_ENTITY_ABREVE_U,		258,	"Abreve",			NULL },						// 0x102
	{ MANUAL_ENTITY_ABREVE_L,		259,	"abreve",			NULL },						// 0x103
	{ MANUAL_ENTITY_AOGON_U,		260,	"Aogon",			NULL },						// 0x104
	{ MANUAL_ENTITY_AOGON_L,		261,	"aogon",			NULL },						// 0x105
	{ MANUAL_ENTITY_CACUTE_U,		262,	"Cacute",			NULL },						// 0x106
	{ MANUAL_ENTITY_CACUTE_L,		263,	"cacute",			NULL },						// 0x107
	{ MANUAL_ENTITY_CCIRC_U,		264,	"Ccirc",			NULL },						// 0x108
	{ MANUAL_ENTITY_CCIRC_L,		265,	"ccirc",			NULL },						// 0x109
	{ MANUAL_ENTITY_CDOT_U,			266,	"Cdot",				NULL },						// 0x10a
	{ MANUAL_ENTITY_CDOT_L,			267,	"cdot",				NULL },						// 0x10b
	{ MANUAL_ENTITY_CCARON_U,		268,	"Ccaron",			NULL },						// 0x10c
	{ MANUAL_ENTITY_CCARON_L,		269,	"ccaron",			NULL },						// 0x10d
	{ MANUAL_ENTITY_DCARON_U,		270,	"Dcaron",			NULL },						// 0x10e
	{ MANUAL_ENTITY_DCARON_L,		271,	"dcaron",			NULL },						// 0x10f
	{ MANUAL_ENTITY_DSTROK_U,		272,	"Dstrok",			NULL },						// 0x110
	{ MANUAL_ENTITY_DSTROK_L,		273,	"dstrok",			NULL },						// 0x111
	{ MANUAL_ENTITY_EMACR_U,		274,	"Emacr",			NULL },						// 0x112
	{ MANUAL_ENTITY_EMACR_L,		275,	"emacr",			NULL },						// 0x113
	{ MANUAL_ENTITY_EDOT_U,			278,	"Edot",				NULL },						// 0x116
	{ MANUAL_ENTITY_EDOT_L,			279,	"edot",				NULL },						// 0x117
	{ MANUAL_ENTITY_EOGON_U,		280,	"Eogon",			NULL },						// 0x118
	{ MANUAL_ENTITY_EOGON_L,		281,	"eogon",			NULL },						// 0x119
	{ MANUAL_ENTITY_ECARON_U,		282,	"Ecaron",			NULL },						// 0x11a
	{ MANUAL_ENTITY_ECARON_L,		283,	"ecaron",			NULL },						// 0x11b
	{ MANUAL_ENTITY_GCIRC_U,		284,	"Gcirc",			NULL },						// 0x11c
	{ MANUAL_ENTITY_GCIRC_L,		285,	"gcirc",			NULL },						// 0x11d
	{ MANUAL_ENTITY_GBREVE_U,		286,	"Gbreve",			NULL },						// 0x11e
	{ MANUAL_ENTITY_GBREVE_L,		287,	"gbreve",			NULL },						// 0x11f
	{ MANUAL_ENTITY_GDOT_U,			288,	"Gdot",				NULL },						// 0x120
	{ MANUAL_ENTITY_GDOT_L,			289,	"gdot",				NULL },						// 0x121
	{ MANUAL_ENTITY_GCEDIL_U,		290,	"Gcedil",			NULL },						// 0x122
	{ MANUAL_ENTITY_HCIRC_U,		292,	"Hcirc",			NULL },						// 0x124
	{ MANUAL_ENTITY_HCIRC_L,		293,	"hcirc",			NULL },						// 0x125
	{ MANUAL_ENTITY_HSTROK_U,		294,	"Hstrok",			NULL },						// 0x126
	{ MANUAL_ENTITY_HSTROK_L,		295,	"hstrok",			NULL },						// 0x127
	{ MANUAL_ENTITY_ITILDE_U,		296,	"Itilde",			NULL },						// 0x128
	{ MANUAL_ENTITY_ITILDE_L,		297,	"itilde",			NULL },						// 0x129
	{ MANUAL_ENTITY_IMACR_U,		298,	"Imacr",			NULL },						// 0x12a
	{ MANUAL_ENTITY_IMACR_L,		299,	"imacr",			NULL },						// 0x12b
	{ MANUAL_ENTITY_IOGON_U,		302,	"Iogon",			NULL },						// 0x12e
	{ MANUAL_ENTITY_IOGON_L,		303,	"iogon",			NULL },						// 0x12f
	{ MANUAL_ENTITY_IDOT_U,			304,	"Idot",				NULL },						// 0x130
	{ MANUAL_ENTITY_IMATH_L,		305,	"imath",			(char*[]) { "inodot", NULL } },			// 0x131
	{ MANUAL_ENTITY_IJLIG_U,		306,	"IJlig",			NULL },						// 0x132
	{ MANUAL_ENTITY_IJLIG_L,		307,	"ijlig",			NULL },						// 0x133
	{ MANUAL_ENTITY_JCIRC_U,		308,	"Jcirc",			NULL },						// 0x134
	{ MANUAL_ENTITY_JCIRC_L,		309,	"jcirc",			NULL },						// 0x135
	{ MANUAL_ENTITY_KCEDIL_U,		310,	"Kcedil",			NULL },						// 0x136
	{ MANUAL_ENTITY_KCEDIL_L,		311,	"kcedil",			NULL },						// 0x137
	{ MANUAL_ENTITY_KGREEN_L,		312,	"kgreen",			NULL },						// 0x138
	{ MANUAL_ENTITY_LACUTE_U,		313,	"Lacute",			NULL },						// 0x139
	{ MANUAL_ENTITY_LACUTE_L,		314,	"lacute",			NULL },						// 0x13a
	{ MANUAL_ENTITY_LCEDIL_U,		315,	"Lcedil",			NULL },						// 0x13b
	{ MANUAL_ENTITY_LCEDIL_L,		316,	"lcedil",			NULL },						// 0x13c
	{ MANUAL_ENTITY_LCARON_U,		317,	"Lcaron",			NULL },						// 0x13d
	{ MANUAL_ENTITY_LCARON_L,		318,	"lcaron",			NULL },						// 0x13e
	{ MANUAL_ENTITY_LMIDOT_U,		319,	"Lmidot",			NULL },						// 0x13f
	{ MANUAL_ENTITY_LMIDOT_L,		320,	"lmidot",			NULL },						// 0x140
	{ MANUAL_ENTITY_LSTROK_U,		321,	"Lstrok",			NULL },						// 0x141
	{ MANUAL_ENTITY_LSTROK_L,		322,	"lstrok",			NULL },						// 0x142
	{ MANUAL_ENTITY_NACUTE_U,		323,	"Nacute",			NULL },						// 0x143
	{ MANUAL_ENTITY_NACUTE_L,		324,	"nacute",			NULL },						// 0x144
	{ MANUAL_ENTITY_NCEDIL_U,		325,	"Ncedil",			NULL },						// 0x145
	{ MANUAL_ENTITY_NCEDIL_L,		326,	"ncedil",			NULL },						// 0x146
	{ MANUAL_ENTITY_NCARON_U,		327,	"Ncaron",			NULL },						// 0x147
	{ MANUAL_ENTITY_NCARON_L,		328,	"ncaron",			NULL },						// 0x148
	{ MANUAL_ENTITY_NAPOS_L,		329,	"napos",			NULL },						// 0x149
	{ MANUAL_ENTITY_ENG_U,			330,	"ENG",				NULL },						// 0x14a
	{ MANUAL_ENTITY_ENG_L,			331,	"eng",				NULL },						// 0x14b
	{ MANUAL_ENTITY_OMACR_U,		332,	"Omacr",			NULL },						// 0x14c
	{ MANUAL_ENTITY_OMACR_L,		333,	"omacr",			NULL },						// 0x14d
	{ MANUAL_ENTITY_ODBLAC_U,		336,	"Odblac",			NULL },						// 0x150
	{ MANUAL_ENTITY_ODBLAC_L,		337,	"odblac",			NULL },						// 0x151
	{ MANUAL_ENTITY_OELIG_U,		338,	"OElig",			NULL },						// 0x152
	{ MANUAL_ENTITY_OELIG_L,		339,	"oelig",			NULL },						// 0x153
	{ MANUAL_ENTITY_RACUTE_U,		340,	"Racute",			NULL },						// 0x154
	{ MANUAL_ENTITY_RACUTE_L,		341,	"racute",			NULL },						// 0x155
	{ MANUAL_ENTITY_RCEDIL_U,		342,	"Rcedil",			NULL },						// 0x156
	{ MANUAL_ENTITY_RCEDIL_L,		343,	"rcedil",			NULL },						// 0x157
	{ MANUAL_ENTITY_RCARON_U,		344,	"Rcaron",			NULL },						// 0x158
	{ MANUAL_ENTITY_RCARON_L,		345,	"rcaron",			NULL },						// 0x159
	{ MANUAL_ENTITY_SACUTE_U,		346,	"Sacute",			NULL },						// 0x15a
	{ MANUAL_ENTITY_SACUTE_L,		347,	"sacute",			NULL },						// 0x15b
	{ MANUAL_ENTITY_SCIRC_U,		348,	"Scirc",			NULL },						// 0x15c
	{ MANUAL_ENTITY_SCIRC_L,		349,	"scirc",			NULL },						// 0x15d
	{ MANUAL_ENTITY_SCEDIL_U,		350,	"Scedil",			NULL },						// 0x15e
	{ MANUAL_ENTITY_SCEDIL_L,		351,	"scedil",			NULL },						// 0x15f
	{ MANUAL_ENTITY_SCARON_U,		352,	"Scaron",			NULL },						// 0x160
	{ MANUAL_ENTITY_SCARON_L,		353,	"scaron",			NULL },						// 0x161
	{ MANUAL_ENTITY_TCEDIL_U,		354,	"Tcedil",			NULL },						// 0x162
	{ MANUAL_ENTITY_TCEDIL_L,		355,	"tcedil",			NULL },						// 0x163
	{ MANUAL_ENTITY_TCARON_U,		356,	"Tcaron",			NULL },						// 0x164
	{ MANUAL_ENTITY_TCARON_L,		357,	"tcaron",			NULL },						// 0x165
	{ MANUAL_ENTITY_TSTROK_U,		358,	"Tstrok",			NULL },						// 0x166
	{ MANUAL_ENTITY_TSTROK_L,		359,	"tstrok",			NULL },						// 0x167
	{ MANUAL_ENTITY_UTILDE_U,		360,	"Utilde",			NULL },						// 0x168
	{ MANUAL_ENTITY_UTILDE_L,		361,	"utilde",			NULL },						// 0x169
	{ MANUAL_ENTITY_UMACR_U,		362,	"Umacr",			NULL },						// 0x16a
	{ MANUAL_ENTITY_UMACR_L,		363,	"umacr",			NULL },						// 0x16b
	{ MANUAL_ENTITY_UBREVE_U,		364,	"Ubreve",			NULL },						// 0x16c
	{ MANUAL_ENTITY_UBREVE_L,		365,	"ubreve",			NULL },						// 0x16d
	{ MANUAL_ENTITY_URING_U,		366,	"Uring",			NULL },						// 0x16e
	{ MANUAL_ENTITY_URING_L,		367,	"uring",			NULL },						// 0x16f
	{ MANUAL_ENTITY_UDBLAC_U,		368,	"Udblac",			NULL },						// 0x170
	{ MANUAL_ENTITY_UDBLAC_L,		369,	"udblac",			NULL },						// 0x171
	{ MANUAL_ENTITY_UOGON_U,		370,	"Uogon",			NULL },						// 0x172
	{ MANUAL_ENTITY_UOGON_L,		371,	"uogon",			NULL },						// 0x173
	{ MANUAL_ENTITY_WCIRC_U,		372,	"Wcirc",			NULL },						// 0x174
	{ MANUAL_ENTITY_WCIRC_L,		373,	"wcirc",			NULL },						// 0x175
	{ MANUAL_ENTITY_YCIRC_U,		374,	"Ycirc",			NULL },						// 0x176
	{ MANUAL_ENTITY_YCIRC_L,		375,	"ycirc",			NULL },						// 0x177
	{ MANUAL_ENTITY_YUML_U,			376,	"Yuml",				NULL },						// 0x178
	{ MANUAL_ENTITY_ZACUTE_U,		377,	"Zacute",			NULL },						// 0x179
	{ MANUAL_ENTITY_ZACUTE_L,		378,	"zacute",			NULL },						// 0x17a
	{ MANUAL_ENTITY_ZDOT_U,			379,	"Zdot",				NULL },						// 0x17b
	{ MANUAL_ENTITY_ZDOT_L,			380,	"zdot",				NULL },						// 0x17c
	{ MANUAL_ENTITY_ZCARON_U,		381,	"Zcaron",			NULL },						// 0x17d
	{ MANUAL_ENTITY_ZCARON_L,		382,	"zcaron",			NULL },						// 0x17e

	/* Latin Extended B */

	{ MANUAL_ENTITY_FNOF_L,			402,	"fnof",				NULL },						// 0x192
	{ MANUAL_ENTITY_IMPED_U,		437,	"imped",			NULL },						// 0x1b5
	{ MANUAL_ENTITY_GACUTE_L,		501,	"gacute",			NULL },						// 0x1f5
	{ MANUAL_ENTITY_JMATH_L,		567,	"jmath",			NULL },						// 0x237

	/* Spacing Modifier Letter */

	{ MANUAL_ENTITY_CIRC,			710,	"circ",				NULL },						// 0x2c6
	{ MANUAL_ENTITY_HACEK,			711,	"Hacek",			(char*[]) { "caron", NULL } },			// 0x2c7
	{ MANUAL_ENTITY_BREVE,			728,	"breve",			(char*[]) { "Breve", NULL } },			// 0x2d8
	{ MANUAL_ENTITY_DIACRITICALDOT,		729,	"dot",				(char*[]) { "DiacriticalDot", NULL } },		// 0x2d9
	{ MANUAL_ENTITY_RING,			730,	"ring",				NULL },						// 0x2da
	{ MANUAL_ENTITY_OGON,			731,	"ogon",				NULL },						// 0x2db
	{ MANUAL_ENTITY_TILDE,			732,	"tilde",			(char*[]) { "DiacriticalTilde", NULL } },	// 0x2dc
	{ MANUAL_ENTITY_DIACRITICALDOUBLEACUTE,	733,	"dblac",			(char*[]) { "DiacriticalDoubleAcute", NULL } },	// 0x2dd

	/* Combining Diacritical Marks */

	{ MANUAL_ENTITY_DOWNBREVE,		785,	"DownBreve",			NULL },						// 0x311

	/* Greek and Coptic */

	{ MANUAL_ENTITY_ALPHA_U,		913,	"Alpha",			NULL },						// 0x391
	{ MANUAL_ENTITY_BETA_U,			914,	"Beta",				NULL },						// 0x392
	{ MANUAL_ENTITY_GAMMA_U,		915,	"Gamma",			NULL },						// 0x393
	{ MANUAL_ENTITY_DELTA_U,		916,	"Delta",			NULL },						// 0x394
	{ MANUAL_ENTITY_EPSILON_U,		917,	"Epsilon",			NULL },						// 0x395
	{ MANUAL_ENTITY_ZETA_U,			918,	"Zeta",				NULL },						// 0x396
	{ MANUAL_ENTITY_ETA_U,			919,	"Eta",				NULL },						// 0x397
	{ MANUAL_ENTITY_THETA_U,		920,	"Theta",			NULL },						// 0x398
	{ MANUAL_ENTITY_IOTA_U,			921,	"Iota",				NULL },						// 0x399
	{ MANUAL_ENTITY_KAPPA_U,		922,	"Kappa",			NULL },						// 0x39a
	{ MANUAL_ENTITY_LAMBDA_U,		923,	"Lambda",			NULL },						// 0x39b
	{ MANUAL_ENTITY_MU_U,			924,	"Mu",				NULL },						// 0x39c
	{ MANUAL_ENTITY_NU_U,			925,	"Nu",				NULL },						// 0x39d
	{ MANUAL_ENTITY_XI_U,			926,	"Xi",				NULL },						// 0x39e
	{ MANUAL_ENTITY_OMICRON_U,		927,	"Omicron",			NULL },						// 0x39f
	{ MANUAL_ENTITY_PI_U,			928,	"Pi",				NULL },						// 0x3a0
	{ MANUAL_ENTITY_RHO_U,			929,	"Rho",				NULL },						// 0x3a1
	{ MANUAL_ENTITY_SIGMA_U,		931,	"Sigma",			NULL },						// 0x3a3
	{ MANUAL_ENTITY_TAU_U,			932,	"Tau",				NULL },						// 0x3a4
	{ MANUAL_ENTITY_UPSILON_U,		933,	"Upsilon",			NULL },						// 0x3a5
	{ MANUAL_ENTITY_PHI_U,			934,	"Phi",				NULL },						// 0x3a6
	{ MANUAL_ENTITY_CHI_U,			935,	"Chi",				NULL },						// 0x3a7
	{ MANUAL_ENTITY_PSI_U,			936,	"Psi",				NULL },						// 0x3a8
	{ MANUAL_ENTITY_OMEGA_U,		937,	"Omega",			(char*[]) { "ohm", NULL } },			// 0x3a9
	{ MANUAL_ENTITY_ALPHA_L,		945,	"alpha",			NULL },						// 0x3b1
	{ MANUAL_ENTITY_BETA_L,			946,	"beta",				NULL },						// 0x3b2
	{ MANUAL_ENTITY_GAMMA_L,		947,	"gamma",			NULL },						// 0x3b3
	{ MANUAL_ENTITY_DELTA_L,		948,	"delta",			NULL },						// 0x3b4
	{ MANUAL_ENTITY_EPSILON_L,		949,	"epsilon",			(char*[]) { "epsi", NULL } },			// 0x3b5
	{ MANUAL_ENTITY_ZETA_L,			950,	"zeta",				NULL },						// 0x3b6
	{ MANUAL_ENTITY_ETA_L,			951,	"eta",				NULL },						// 0x3b7
	{ MANUAL_ENTITY_THETA_L,		952,	"theta",			NULL },						// 0x3b8
	{ MANUAL_ENTITY_IOTA_L,			953,	"iota",				NULL },						// 0x3b9
	{ MANUAL_ENTITY_KAPPA_L,		954,	"kappa",			NULL },						// 0x3ba
	{ MANUAL_ENTITY_LAMBDA_L,		955,	"lambda",			NULL },						// 0x3bb
	{ MANUAL_ENTITY_MU_L,			956,	"mu",				NULL },						// 0x3bc
	{ MANUAL_ENTITY_NU_L,			957,	"nu",				NULL },						// 0x3bd
	{ MANUAL_ENTITY_XI_L,			958,	"xi",				NULL },						// 0x3be
	{ MANUAL_ENTITY_OMICRON_L,		959,	"omicron",			NULL },						// 0x3bf
	{ MANUAL_ENTITY_PI_L,			960,	"pi",				NULL },						// 0x3c0
	{ MANUAL_ENTITY_RHO_L,			961,	"rho",				NULL },						// 0x3c1
	{ MANUAL_ENTITY_SIGMAF_L,		962,	"sigmaf",			(char*[]) { "sigmav", "varsigma", NULL } },	// 0x3c2
	{ MANUAL_ENTITY_SIGMA_L,		963,	"sigma",			NULL },						// 0x3c3
	{ MANUAL_ENTITY_TAU_L,			964,	"tau",				NULL },						// 0x3c4
	{ MANUAL_ENTITY_UPSILON_L,		965,	"upsilon",			(char*[]) { "upsi", NULL } },			// 0x3c5
	{ MANUAL_ENTITY_PHI_L,			966,	"phi",				NULL },						// 0x3c6
	{ MANUAL_ENTITY_CHI_L,			967,	"chi",				NULL },						// 0x3c7
	{ MANUAL_ENTITY_PSI_L,			968,	"psi",				NULL },						// 0x3c8
	{ MANUAL_ENTITY_OMEGA_L,		969,	"omega",			NULL },						// 0x3c9
	{ MANUAL_ENTITY_THETASYM_L,		977,	"thetasym",			(char*[]) { "thetav", "vartheta", NULL } },	// 0x3d1
	{ MANUAL_ENTITY_UPSIH_U,		978,	"upsih",			(char*[]) { "Upsi", NULL } },			// 0x3d2
	{ MANUAL_ENTITY_PHIV_L,			981,	"phiv",				(char*[]) { "straightphi", "varphi", NULL } },	// 0x3d5
	{ MANUAL_ENTITY_PIV_L,			982,	"piv",				(char*[]) { "varpi", NULL } },			// 0x3d6
	{ MANUAL_ENTITY_GAMMAD_U,		988,	"Gammad",			NULL },						// 0x3dc
	{ MANUAL_ENTITY_DIGAMMA_L,		989,	"digamma",			(char*[]) { "gammad", NULL } },			// 0x3dd
	{ MANUAL_ENTITY_KAPPAV_L,		1008,	"kappav",			(char*[]) { "varkappa", NULL } },		// 0x3f0
	{ MANUAL_ENTITY_RHOV_L,			1009,	"rhov",				(char*[]) { "varrho", NULL } },			// 0x3f1
	{ MANUAL_ENTITY_EPSIV_L,		1013,	"epsiv",			(char*[]) { "straightepsilon", "varepsilon", NULL } },	// 0x3f5
	{ MANUAL_ENTITY_BACKEPSILON,		1014,	"backepsilon",			(char*[]) { "bepsi", NULL } },			// 0x3f6

	/* Cyrillic */

	{ MANUAL_ENTITY_IOCY_U,			1025,	"IOcy",				NULL },	// 0x401
	{ MANUAL_ENTITY_DJCY_U,			1026,	"DJcy",				NULL },	// 0x402
	{ MANUAL_ENTITY_GJCY_U,			1027,	"GJcy",				NULL },	// 0x403
	{ MANUAL_ENTITY_JUKCY_U,		1028,	"Jukcy",			NULL },	// 0x404
	{ MANUAL_ENTITY_DSCY_U,			1029,	"DScy",				NULL },	// 0x405
	{ MANUAL_ENTITY_IUKCY_U,		1030,	"Iukcy",			NULL },	// 0x406
	{ MANUAL_ENTITY_YICY_U,			1031,	"YIcy",				NULL },	// 0x407
	{ MANUAL_ENTITY_JSERCY_U,		1032,	"Jsercy",			NULL },	// 0x408
	{ MANUAL_ENTITY_LJCY_U,			1033,	"LJcy",				NULL },	// 0x409
	{ MANUAL_ENTITY_NJCY_U,			1034,	"NJcy",				NULL },	// 0x40a
	{ MANUAL_ENTITY_TSHCY_U,		1035,	"TSHcy",			NULL },	// 0x40b
	{ MANUAL_ENTITY_KJCY_U,			1036,	"KJcy",				NULL },	// 0x40c
	{ MANUAL_ENTITY_UBRCY_U,		1038,	"Ubrcy",			NULL },	// 0x40e
	{ MANUAL_ENTITY_DZCY_U,			1039,	"DZcy",				NULL },	// 0x40f
	{ MANUAL_ENTITY_ACY_U,			1040,	"Acy",				NULL },	// 0x410
	{ MANUAL_ENTITY_BCY_U,			1041,	"Bcy",				NULL },	// 0x411
	{ MANUAL_ENTITY_VCY_U,			1042,	"Vcy",				NULL },	// 0x412
	{ MANUAL_ENTITY_GCY_U,			1043,	"Gcy",				NULL },	// 0x413
	{ MANUAL_ENTITY_DCY_U,			1044,	"Dcy",				NULL },	// 0x414
	{ MANUAL_ENTITY_IECY_U,			1045,	"IEcy",				NULL },	// 0x415
	{ MANUAL_ENTITY_ZHCY_U,			1046,	"ZHcy",				NULL },	// 0x416
	{ MANUAL_ENTITY_ZCY_U,			1047,	"Zcy",				NULL },	// 0x417
	{ MANUAL_ENTITY_ICY_U,			1048,	"Icy",				NULL },	// 0x418
	{ MANUAL_ENTITY_JCY_U,			1049,	"Jcy",				NULL },	// 0x419
	{ MANUAL_ENTITY_KCY_U,			1050,	"Kcy",				NULL },	// 0x41a
	{ MANUAL_ENTITY_LCY_U,			1051,	"Lcy",				NULL },	// 0x41b
	{ MANUAL_ENTITY_MCY_U,			1052,	"Mcy",				NULL },	// 0x41c
	{ MANUAL_ENTITY_NCY_U,			1053,	"Ncy",				NULL },	// 0x41d
	{ MANUAL_ENTITY_OCY_U,			1054,	"Ocy",				NULL },	// 0x41e
	{ MANUAL_ENTITY_PCY_U,			1055,	"Pcy",				NULL },	// 0x41f
	{ MANUAL_ENTITY_RCY_U,			1056,	"Rcy",				NULL },	// 0x420
	{ MANUAL_ENTITY_SCY_U,			1057,	"Scy",				NULL },	// 0x421
	{ MANUAL_ENTITY_TCY_U,			1058,	"Tcy",				NULL },	// 0x422
	{ MANUAL_ENTITY_UCY_U,			1059,	"Ucy",				NULL },	// 0x423
	{ MANUAL_ENTITY_FCY_U,			1060,	"Fcy",				NULL },	// 0x424
	{ MANUAL_ENTITY_KHCY_U,			1061,	"KHcy",				NULL },	// 0x425
	{ MANUAL_ENTITY_TSCY_U,			1062,	"TScy",				NULL },	// 0x426
	{ MANUAL_ENTITY_CHCY_U,			1063,	"CHcy",				NULL },	// 0x427
	{ MANUAL_ENTITY_SHCY_U,			1064,	"SHcy",				NULL },	// 0x428
	{ MANUAL_ENTITY_SHCHCY_U,		1065,	"SHCHcy",			NULL },	// 0x429
	{ MANUAL_ENTITY_HARDCY_U,		1066,	"HARDcy",			NULL },	// 0x42a
	{ MANUAL_ENTITY_YCY_U,			1067,	"Ycy",				NULL },	// 0x42b
	{ MANUAL_ENTITY_SOFTCY_U,		1068,	"SOFTcy",			NULL },	// 0x42c
	{ MANUAL_ENTITY_ECY_U,			1069,	"Ecy",				NULL },	// 0x42d
	{ MANUAL_ENTITY_YUCY_U,			1070,	"YUcy",				NULL },	// 0x42e
	{ MANUAL_ENTITY_YACY_U,			1071,	"YAcy",				NULL },	// 0x42f
	{ MANUAL_ENTITY_ACY_L,			1072,	"acy",				NULL },	// 0x430
	{ MANUAL_ENTITY_BCY_L,			1073,	"bcy",				NULL },	// 0x431
	{ MANUAL_ENTITY_VCY_L,			1074,	"vcy",				NULL },	// 0x432
	{ MANUAL_ENTITY_GCY_L,			1075,	"gcy",				NULL },	// 0x433
	{ MANUAL_ENTITY_DCY_L,			1076,	"dcy",				NULL },	// 0x434
	{ MANUAL_ENTITY_IECY_L,			1077,	"iecy",				NULL },	// 0x435
	{ MANUAL_ENTITY_ZHCY_L,			1078,	"zhcy",				NULL },	// 0x436
	{ MANUAL_ENTITY_ZCY_L,			1079,	"zcy",				NULL },	// 0x437
	{ MANUAL_ENTITY_ICY_L,			1080,	"icy",				NULL },	// 0x438
	{ MANUAL_ENTITY_JCY_L,			1081,	"jcy",				NULL },	// 0x439
	{ MANUAL_ENTITY_KCY_L,			1082,	"kcy",				NULL },	// 0x43a
	{ MANUAL_ENTITY_LCY_L,			1083,	"lcy",				NULL },	// 0x43b
	{ MANUAL_ENTITY_MCY_L,			1084,	"mcy",				NULL },	// 0x43c
	{ MANUAL_ENTITY_NCY_L,			1085,	"ncy",				NULL },	// 0x43d
	{ MANUAL_ENTITY_OCY_L,			1086,	"ocy",				NULL },	// 0x43e
	{ MANUAL_ENTITY_PCY_L,			1087,	"pcy",				NULL },	// 0x43f
	{ MANUAL_ENTITY_RCY_L,			1088,	"rcy",				NULL },	// 0x440
	{ MANUAL_ENTITY_SCY_L,			1089,	"scy",				NULL },	// 0x441
	{ MANUAL_ENTITY_TCY_L,			1090,	"tcy",				NULL },	// 0x442
	{ MANUAL_ENTITY_UCY_L,			1091,	"ucy",				NULL },	// 0x443
	{ MANUAL_ENTITY_FCY_L,			1092,	"fcy",				NULL },	// 0x444
	{ MANUAL_ENTITY_KHCY_L,			1093,	"khcy",				NULL },	// 0x445
	{ MANUAL_ENTITY_TSCY_L,			1094,	"tscy",				NULL },	// 0x446
	{ MANUAL_ENTITY_CHCY_L,			1095,	"chcy",				NULL },	// 0x447
	{ MANUAL_ENTITY_SHCY_L,			1096,	"shcy",				NULL },	// 0x448
	{ MANUAL_ENTITY_SHCHCY_L,		1097,	"shchcy",			NULL },	// 0x449
	{ MANUAL_ENTITY_HARDCY_L,		1098,	"hardcy",			NULL },	// 0x44a
	{ MANUAL_ENTITY_YCY_L,			1099,	"ycy",				NULL },	// 0x44b
	{ MANUAL_ENTITY_SOFTCY_L,		1100,	"softcy",			NULL },	// 0x44c
	{ MANUAL_ENTITY_ECY_L,			1101,	"ecy",				NULL },	// 0x44d
	{ MANUAL_ENTITY_YUCY_L,			1102,	"yucy",				NULL },	// 0x44e
	{ MANUAL_ENTITY_YACY_L,			1103,	"yacy",				NULL },	// 0x44f
	{ MANUAL_ENTITY_IOCY_L,			1105,	"iocy",				NULL },	// 0x451
	{ MANUAL_ENTITY_DJCY_L,			1106,	"djcy",				NULL },	// 0x452
	{ MANUAL_ENTITY_GJCY_L,			1107,	"gjcy",				NULL },	// 0x453
	{ MANUAL_ENTITY_JUKCY_L,		1108,	"jukcy",			NULL },	// 0x454
	{ MANUAL_ENTITY_DSCY_L,			1109,	"dscy",				NULL },	// 0x455
	{ MANUAL_ENTITY_IUKCY_L,		1110,	"iukcy",			NULL },	// 0x456
	{ MANUAL_ENTITY_YICY_L,			1111,	"yicy",				NULL },	// 0x457
	{ MANUAL_ENTITY_JSERCY_L,		1112,	"jsercy",			NULL },	// 0x458
	{ MANUAL_ENTITY_LJCY_L,			1113,	"ljcy",				NULL },	// 0x459
	{ MANUAL_ENTITY_NJCY_L,			1114,	"njcy",				NULL },	// 0x45a
	{ MANUAL_ENTITY_TSHCY_L,		1115,	"tshcy",			NULL },	// 0x45b
	{ MANUAL_ENTITY_KJCY_L,			1116,	"kjcy",				NULL },	// 0x45c
	{ MANUAL_ENTITY_UBRCY_L,		1118,	"ubrcy",			NULL },	// 0x45e
	{ MANUAL_ENTITY_DZCY_L,			1119,	"dzcy",				NULL },	// 0x45f

	/* General Punctuation */

	{ MANUAL_ENTITY_ENSP,			8194,	"ensp",				NULL },								// 0x2002
	{ MANUAL_ENTITY_EMSP,			8195,	"emsp",				NULL },								// 0x2003
	{ MANUAL_ENTITY_EMSP13,			8196,	"emsp13",			NULL },								// 0x2004
	{ MANUAL_ENTITY_EMSP14,			8197,	"emsp14",			NULL },								// 0x2005
	{ MANUAL_ENTITY_NUMSP,			8199,	"numsp",			NULL },								// 0x2007
	{ MANUAL_ENTITY_PUNCSP,			8200,	"puncsp",			NULL },								// 0x2008
	{ MANUAL_ENTITY_THINSP,			8201,	"thinsp",			(char*[]) { "ThinSpace", NULL } },				// 0x2009
	{ MANUAL_ENTITY_VERYTHINSPACE,		8202,	"hairsp",			(char*[]) { "VeryThinSpace", NULL } },				// 0x200a
	{ MANUAL_ENTITY_NEGATIVEMEDIUMSPACE,	8203,	"NegativeMediumSpace",		(char*[]) { "NegativeThickSpace", "NegativeThinSpace", "NegativeVeryThinSpace", "ZeroWidthSpace", NULL } },	// 0x200b
	{ MANUAL_ENTITY_ZWNJ,			8204,	"zwnj",				NULL },								// 0x200c
	{ MANUAL_ENTITY_ZWJ,			8205,	"zwj",				NULL },								// 0x200d
	{ MANUAL_ENTITY_LRM,			8206,	"lrm",				NULL },								// 0x200e
	{ MANUAL_ENTITY_RLM,			8207,	"rlm",				NULL },								// 0x200f
	{ MANUAL_ENTITY_DASH,			8208,	"dash",				(char*[]) { "hyphen", NULL } },					// 0x2010
	{ MANUAL_ENTITY_NDASH,			8211,	"ndash",			NULL },								// 0x2013
	{ MANUAL_ENTITY_MDASH,			8212,	"mdash",			NULL },								// 0x2014
	{ MANUAL_ENTITY_HORBAR,			8213,	"horbar",			NULL },								// 0x2015
	{ MANUAL_ENTITY_VERBAR,			8214,	"Verbar",			(char*[]) { "Vert", NULL } },					// 0x2016
	{ MANUAL_ENTITY_LSQUO,			8216,	"lsquo",			(char*[]) { "OpenCurlyQuote", NULL } },				// 0x2018
	{ MANUAL_ENTITY_RSQUO,			8217,	"rsquo",			(char*[]) { "CloseCurlyQuote", "rsquor", NULL } },		// 0x2019
	{ MANUAL_ENTITY_SBQUO,			8218,	"sbquo",			(char*[]) { "lsquor", NULL } },					// 0x201a
	{ MANUAL_ENTITY_LDQUO,			8220,	"ldquo",			(char*[]) { "OpenCurlyDoubleQuote", NULL } },			// 0x201c
	{ MANUAL_ENTITY_RDQUO,			8221,	"rdquo",			(char*[]) { "CloseCurlyDoubleQuote", "rdquor", NULL } },	// 0x201d
	{ MANUAL_ENTITY_BDQUO,			8222,	"bdquo",			(char*[]) { "ldquor", NULL } },					// 0x201e
	{ MANUAL_ENTITY_DAGGER,			8224,	"dagger",			NULL },								// 0x2020
	{ MANUAL_ENTITY_DDAGGER,		8225,	"ddagger",			(char*[]) { "Dagger", NULL } },					// 0x2021
	{ MANUAL_ENTITY_BULL,			8226,	"bull",				(char*[]) { "bullet", NULL } },					// 0x2022
	{ MANUAL_ENTITY_NLDR,			8229,	"nldr",				NULL },								// 0x2025
	{ MANUAL_ENTITY_HELLIP,			8230,	"hellip",			(char*[]) { "mldr", NULL } },					// 0x2026
	{ MANUAL_ENTITY_PERMIL,			8240,	"permil",			NULL },								// 0x2030
	{ MANUAL_ENTITY_PERTENK,		8241,	"pertenk",			NULL },								// 0x2031
	{ MANUAL_ENTITY_PRIME,			8242,	"prime",			NULL },								// 0x2032
	{ MANUAL_ENTITY_DPRIME,			8243,	"Prime",			NULL },								// 0x2033
	{ MANUAL_ENTITY_TPRIME,			8244,	"tprime",			NULL },								// 0x2034
	{ MANUAL_ENTITY_BACKPRIME,		8245,	"backprime",			(char*[]) { "bprime", NULL } },					// 0x2035
	{ MANUAL_ENTITY_LSAQUO,			8249,	"lsaquo",			NULL },								// 0x2039
	{ MANUAL_ENTITY_RSAQUO,			8250,	"rsaquo",			NULL },								// 0x203a
	{ MANUAL_ENTITY_OLINE,			8254,	"oline",			(char*[]) { "OverBar", NULL } },				// 0x203e
	{ MANUAL_ENTITY_CARET,			8257,	"caret",			NULL },								// 0x2041
	{ MANUAL_ENTITY_HYBULL,			8259,	"hybull",			NULL },								// 0x2043
	{ MANUAL_ENTITY_FRASL,			8260,	"frasl",			NULL },								// 0x2044
	{ MANUAL_ENTITY_BSEMI,			8271,	"bsemi",			NULL },								// 0x204f
	{ MANUAL_ENTITY_QPRIME,			8279,	"qprime",			NULL },								// 0x2057
	{ MANUAL_ENTITY_MEDIUMSPACE,		8287,	"MediumSpace",			NULL },								// 0x205f
	{ MANUAL_ENTITY_NOBREAK,		8288,	"NoBreak",			NULL },								// 0x2060
	{ MANUAL_ENTITY_APPLYFUNCTION,		8289,	"af",				(char*[]) { "ApplyFunction", NULL } },				// 0x2061
	{ MANUAL_ENTITY_INVISIBLETIMES,		8290,	"it",				(char*[]) { "InvisibleTimes", NULL } },				// 0x2062
	{ MANUAL_ENTITY_INVISIBLECOMMA,		8291,	"ic",				(char*[]) { "InvisibleComma", NULL } },				// 0x2063

	/* Currency Symbols */

	{ MANUAL_ENTITY_EURO,			8364,	"euro",				NULL },	// 0x20ac

	/* Combining Diacritical Marks for Symbols */

	{ MANUAL_ENTITY_TRIPLEDOT,		8411,	"tdot",				(char*[]) { "TripleDot", NULL } },			// 0x20db
	{ MANUAL_ENTITY_DOTDOT,			8412,	"DotDot",			NULL },							// 0x20dc

	/* Letterlike Symbols */

	{ MANUAL_ENTITY_COPF_U,			8450,	"complexes",			(char*[]) { "Copf", NULL } },				// 0x2102
	{ MANUAL_ENTITY_INCARE,			8453,	"incare",			NULL },							// 0x2105
	{ MANUAL_ENTITY_GSCR_L,			8458,	"gscr",				NULL },							// 0x210a
	{ MANUAL_ENTITY_HILBERTSPACE_U,		8459,	"hamilt",			(char*[]) { "HilbertSpace", "Hscr", NULL } },		// 0x210b
	{ MANUAL_ENTITY_HFR_U,			8460,	"Hfr",				(char*[]) { "Poincareplane", NULL } },			// 0x210c
	{ MANUAL_ENTITY_HOPF_U,			8461,	"Hopf",				(char*[]) { "quaternions", NULL } },			// 0x210d
	{ MANUAL_ENTITY_PLANCKH_L,		8462,	"planckh",			NULL },							// 0x210e
	{ MANUAL_ENTITY_HBAR_L,			8463,	"hbar",				(char*[]) { "hslash", "planck", "plankv", NULL } },	// 0x210f
	{ MANUAL_ENTITY_ISCR_U,			8464,	"Iscr",				(char*[]) { "imagline", NULL } },			// 0x2110
	{ MANUAL_ENTITY_IMAGE_U,		8465,	"image",			(char*[]) { "Ifr", "Im", "imagpart", NULL } },		// 0x2111
	{ MANUAL_ENTITY_LAPLACETRF_U,		8466,	"lagran",			(char*[]) { "Laplacetrf", "Lscr", NULL } },		// 0x2112
	{ MANUAL_ENTITY_ELL_L,			8467,	"ell",				NULL },							// 0x2113
	{ MANUAL_ENTITY_NOPF_U,			8469,	"Nopf",				(char*[]) { "naturals", NULL } },			// 0x2115
	{ MANUAL_ENTITY_NUMERO,			8470,	"numero",			NULL },							// 0x2116
	{ MANUAL_ENTITY_COPYSR,			8471,	"copysr",			NULL },							// 0x2117
	{ MANUAL_ENTITY_WEIERP,			8472,	"weierp",			(char*[]) { "wp", NULL } },				// 0x2118
	{ MANUAL_ENTITY_POPF_U,			8473,	"Popf",				(char*[]) { "primes", NULL } },				// 0x2119
	{ MANUAL_ENTITY_QOPF_U,			8474,	"Qopf",				(char*[]) { "rationals", NULL } },			// 0x211a
	{ MANUAL_ENTITY_RSCR_U,			8475,	"Rscr",				(char*[]) { "realine", NULL } },			// 0x211b
	{ MANUAL_ENTITY_REAL_U,			8476,	"real",				(char*[]) { "Re", "Rfr", "realpart", NULL } },		// 0x211c
	{ MANUAL_ENTITY_ROPF_U,			8477,	"Ropf",				(char*[]) { "reals", NULL } },				// 0x211d
	{ MANUAL_ENTITY_RX,			8478,	"rx",				NULL },							// 0x211e
	{ MANUAL_ENTITY_TRADE,			8482,	"trade",			(char*[]) { "TRADE", NULL } },				// 0x2122
	{ MANUAL_ENTITY_ZOPF_U,			8484,	"Zopf",				(char*[]) { "integers", NULL } },			// 0x2124
	{ MANUAL_ENTITY_MHO,			8487,	"mho",				NULL },							// 0x2127
	{ MANUAL_ENTITY_ZFR_U,			8488,	"Zfr",				(char*[]) { "zeetrf", NULL } },				// 0x2128
	{ MANUAL_ENTITY_IIOTA,			8489,	"iiota",			NULL },							// 0x2129
	{ MANUAL_ENTITY_BERNOULLIS_U,		8492,	"bernou",			(char*[]) { "Bscr", "Bernoullis", NULL } },		// 0x212c
	{ MANUAL_ENTITY_CAYLEYS_U,		8493,	"Cayleys",			(char*[]) { "Cfr", NULL } },				// 0x212d
	{ MANUAL_ENTITY_ESCR_L,			8495,	"escr",				NULL },							// 0x212f
	{ MANUAL_ENTITY_ESCR_U,			8496,	"Escr",				(char*[]) { "expectation", NULL } },			// 0x2130
	{ MANUAL_ENTITY_FOURIERTRF_U,		8497,	"Fouriertrf",			(char*[]) { "Fscr", NULL } },				// 0x2131
	{ MANUAL_ENTITY_MELLINTRF_U,		8499,	"Mellintrf",			(char*[]) { "Mscr", "phmmat", NULL } },			// 0x2133
	{ MANUAL_ENTITY_ORDER_L,		8500,	"order",			(char*[]) { "orderof", "oscr", NULL } },		// 0x2134
	{ MANUAL_ENTITY_ALEFSYM,		8501,	"alefsym",			(char*[]) { "aleph", NULL } },				// 0x2135
	{ MANUAL_ENTITY_BETH,			8502,	"beth",				NULL },							// 0x2136
	{ MANUAL_ENTITY_GIMEL,			8503,	"gimel",			NULL },							// 0x2137
	{ MANUAL_ENTITY_DALETH,			8504,	"daleth",			NULL },							// 0x2138
	{ MANUAL_ENTITY_DIFFERENTIALD_U,	8517,	"DD",				(char*[]) { "CapitalDifferentialD", NULL } },		// 0x2145
	{ MANUAL_ENTITY_DIFFERENTIALD_L,	8518,	"dd",				(char*[]) { "DifferentialD", NULL } },			// 0x2146
	{ MANUAL_ENTITY_EXPONENTIALE_L,		8519,	"ee",				(char*[]) { "ExponentialE", "exponentiale", NULL } },	// 0x2147
	{ MANUAL_ENTITY_IMAGINARYI_L,		8520,	"ii",				(char*[]) { "ImaginaryI", NULL } },			// 0x2148

	/* Number Forms */

	{ MANUAL_ENTITY_FRAC13,			8531,	"frac13",			NULL },	// 0x2153
	{ MANUAL_ENTITY_FRAC23,			8532,	"frac23",			NULL },	// 0x2154
	{ MANUAL_ENTITY_FRAC15,			8533,	"frac15",			NULL },	// 0x2155
	{ MANUAL_ENTITY_FRAC25,			8534,	"frac25",			NULL },	// 0x2156
	{ MANUAL_ENTITY_FRAC35,			8535,	"frac35",			NULL },	// 0x2157
	{ MANUAL_ENTITY_FRAC45,			8536,	"frac45",			NULL },	// 0x2158
	{ MANUAL_ENTITY_FRAC16,			8537,	"frac16",			NULL },	// 0x2159
	{ MANUAL_ENTITY_FRAC56,			8538,	"frac56",			NULL },	// 0x215a
	{ MANUAL_ENTITY_FRAC18,			8539,	"frac18",			NULL },	// 0x215b
	{ MANUAL_ENTITY_FRAC38,			8540,	"frac38",			NULL },	// 0x215c
	{ MANUAL_ENTITY_FRAC58,			8541,	"frac58",			NULL },	// 0x215d
	{ MANUAL_ENTITY_FRAC78,			8542,	"frac78",			NULL },	// 0x215e

	/* Arrows */

//	{ MANUAL_ENTITY_LARR,			8592,	"larr",				(char*[]) { "LeftArrow", "ShortLeftArrow", "leftarrow", "slarr", NULL } },	// 0x2190
//	{ MANUAL_ENTITY_UARR,			8593,	"uarr",				(char*[]) { "ShortUpArrow", "UpArrow", "uparrow", NULL } },	// 0x2191
//	{ MANUAL_ENTITY_RARR,			8594,	"rarr",				(char*[]) { "RightArrow", "ShortRightArrow", "rightarrow", "srarr", NULL } },	// 0x2192
//	{ MANUAL_ENTITY_DARR,			8595,	"darr",				(char*[]) { "DownArrow", "ShortDownArrow", "downarrow", NULL } },	// 0x2193
//	{ MANUAL_ENTITY_HARR,			8596,	"harr",				(char*[]) { "LeftRightArrow", "leftrightarrow", NULL } },	// 0x2194
	{ MANUAL_ENTITY_UPDOWNARROW,		8597,	"UpDownArrow",			(char*[]) { "updownarrow", "varr", NULL } },	// 0x2195
	{ MANUAL_ENTITY_UPPERLEFTARROW,		8598,	"UpperLeftArrow",		(char*[]) { "nwarr", "nwarrow", NULL } },	// 0x2196
	{ MANUAL_ENTITY_UPPERRIGHTARROW,	8599,	"UpperRightArrow",		(char*[]) { "nearr", "nearrow", NULL } },	// 0x2197
	{ MANUAL_ENTITY_LOWERRIGHTARROW,	8600,	"LowerRightArrow",		(char*[]) { "searr", "searrow", NULL } },	// 0x2198
	{ MANUAL_ENTITY_LOWERLEFTARROW,		8601,	"LowerLeftArrow",		(char*[]) { "swarr", "swarrow", NULL } },	// 0x2199
	{ MANUAL_ENTITY_NLARR,			8602,	"nlarr",			(char*[]) { "nleftarrow", NULL } },	// 0x219a
	{ MANUAL_ENTITY_NRARR,			8603,	"nrarr",			(char*[]) { "nrightarrow", NULL } },	// 0x219b
	{ MANUAL_ENTITY_RARRW,			8605,	"rarrw",			(char*[]) { "rightsquigarrow", NULL } },	// 0x219d
//	{ MANUAL_ENTITY_LARR,			8606,	"Larr",				(char*[]) { "twoheadleftarrow", NULL } },	// 0x219e
//	{ MANUAL_ENTITY_UARR,			8607,	"Uarr",				NULL },	// 0x219f
//	{ MANUAL_ENTITY_RARR,			8608,	"Rarr",				(char*[]) { "twoheadrightarrow", NULL } },	// 0x21a0
//	{ MANUAL_ENTITY_DARR,			8609,	"Darr",				NULL },	// 0x21a1
	{ MANUAL_ENTITY_LARRTL,			8610,	"larrtl",			(char*[]) { "leftarrowtail", NULL } },	// 0x21a2
	{ MANUAL_ENTITY_RARRTL,			8611,	"rarrtl",			(char*[]) { "rightarrowtail", NULL } },	// 0x21a3
	{ MANUAL_ENTITY_LEFTTEEARROW,		8612,	"LeftTeeArrow",			(char*[]) { "mapstoleft", NULL } },	// 0x21a4
	{ MANUAL_ENTITY_UPTEEARROW,		8613,	"UpTeeArrow",			(char*[]) { "mapstoup", NULL } },	// 0x21a5
	{ MANUAL_ENTITY_RIGHTTEEARROW,		8614,	"RightTeeArrow",		(char*[]) { "map", "mapsto", NULL } },	// 0x21a6
	{ MANUAL_ENTITY_DOWNTEEARROW,		8615,	"DownTeeArrow"		,	(char*[]) { "mapstodown", NULL } },	// 0x21a7
	{ MANUAL_ENTITY_HOOKLEFTARROW,		8617,	"hookleftarrow",		(char*[]) { "larrhk", NULL } },	// 0x21a9
	{ MANUAL_ENTITY_HOOKRIGHTARROW,		8618,	"hookrightarrow",		(char*[]) { "rarrhk", NULL } },	// 0x21aa
	{ MANUAL_ENTITY_LARRLP,			8619,	"larrlp",			(char*[]) { "looparrowleft", NULL } },	// 0x21ab
	{ MANUAL_ENTITY_LOOPARROWRIGHT,		8620,	"looparrowright",		(char*[]) { "rarrlp", NULL } },	// 0x21ac
	{ MANUAL_ENTITY_HARRW,			8621,	"harrw",			(char*[]) { "leftrightsquigarrow", NULL } },	// 0x21ad
	{ MANUAL_ENTITY_NHARR,			8622,	"nharr",			(char*[]) { "nleftrightarrow", NULL } },	// 0x21ae
	{ MANUAL_ENTITY_LSH,			8624,	"Lsh",				(char*[]) { "lsh", NULL } },	// 0x21b0
	{ MANUAL_ENTITY_RSH,			8625,	"Rsh",				(char*[]) { "rsh", NULL } },	// 0x21b1
	{ MANUAL_ENTITY_LDSH,			8626,	"ldsh",				NULL },	// 0x21b2
	{ MANUAL_ENTITY_RDSH,			8627,	"rdsh",				NULL },	// 0x21b3
	{ MANUAL_ENTITY_CRARR,			8629,	"crarr",			NULL },	// 0x21b5
	{ MANUAL_ENTITY_CULARR,			8630,	"cularr",			(char*[]) { "curvearrowleft", NULL } },	// 0x21b6
	{ MANUAL_ENTITY_CURARR,			8631,	"curarr",			(char*[]) { "curvearrowright", NULL } },	// 0x21b7
	{ MANUAL_ENTITY_CIRCLEARROWLEFT,	8634,	"circlearrowleft",		(char*[]) { "olarr", NULL } },	// 0x21ba
	{ MANUAL_ENTITY_CIRCLEARROWRIGHT,	8635,	"circlearrowright",		(char*[]) { "orarr", NULL } },	// 0x21bb
	{ MANUAL_ENTITY_LEFTVECTOR,		8636,	"LeftVector",			(char*[]) { "leftharpoonup", "lharu", NULL } },	// 0x21bc
	{ MANUAL_ENTITY_DOWNLEFTVECTOR,		8637,	"DownLeftVector",		(char*[]) { "leftharpoondown", "lhard", NULL } },	// 0x21bd
	{ MANUAL_ENTITY_RIGHTUPVECTOR,		8638,	"RightUpVector",		(char*[]) { "uharr", "upharpoonright", NULL } },	// 0x21be
	{ MANUAL_ENTITY_LEFTUPVECTOR,		8639,	"LeftUpVector",			(char*[]) { "uharl", "upharpoonleft", NULL } },	// 0x21bf
	{ MANUAL_ENTITY_RIGHTVECTOR,		8640,	"RightVector",			(char*[]) { "rharu", "rightharpoonup", NULL } },	// 0x21c0
	{ MANUAL_ENTITY_DOWNRIGHTVECTOR,	8641,	"DownRightVector",		(char*[]) { "rhard", "rightharpoondown", NULL } },	// 0x21c1
	{ MANUAL_ENTITY_RIGHTDOWNVECTOR,	8642,	"RightDownVector",		(char*[]) { "dharr", "downharpoonright", NULL } },	// 0x21c2
	{ MANUAL_ENTITY_LEFTDOWNVECTOR,		8643,	"LeftDownVector",		(char*[]) { "dharl", "downharpoonleft", NULL } },	// 0x21c3
	{ MANUAL_ENTITY_RIGHTARROWLEFTARROW,	8644,	"RightArrowLeftArrow",		(char*[]) { "rightleftarrows", "rlarr", NULL } },	// 0x21c4
	{ MANUAL_ENTITY_UPARROWDOWNARROW,	8645,	"UpArrowDownArrow",		(char*[]) { "udarr", NULL } },	// 0x21c5
	{ MANUAL_ENTITY_LEFTARROWRIGHTARROW,	8646,	"LeftArrowRightArrow",		(char*[]) { "leftrightarrows", "lrarr", NULL } },	// 0x21c6
	{ MANUAL_ENTITY_LEFTLEFTARROWS,		8647,	"leftleftarrows",		(char*[]) { "llarr", NULL } },	// 0x21c7
	{ MANUAL_ENTITY_UPUPARROWS,		8648,	"upuparrows",			(char*[]) { "uuarr", NULL } },	// 0x21c8
	{ MANUAL_ENTITY_RIGHTRIGHTARROWS,	8649,	"rightrightarrows",		(char*[]) { "rrarr", NULL } },	// 0x21c9
	{ MANUAL_ENTITY_DDARR,			8650,	"ddarr",			(char*[]) { "downdownarrows", NULL } },	// 0x21ca
	{ MANUAL_ENTITY_REVERSEEQUILIBRIUM,	8651,	"ReverseEquilibrium",		(char*[]) { "leftrightharpoons", "lrhar", NULL } },	// 0x21cb
	{ MANUAL_ENTITY_EQUILIBRIUM,		8652,	"Equilibrium",			(char*[]) { "rightleftharpoons", "rlhar", NULL } },	// 0x21cc
	{ MANUAL_ENTITY_NLEFTARROW,		8653,	"nLeftarrow",			(char*[]) { "nlArr", NULL } },	// 0x21cd
	{ MANUAL_ENTITY_NLEFTRIGHTARROW,	8654,	"nLeftrightarrow",		(char*[]) { "nhArr", NULL } },	// 0x21ce
	{ MANUAL_ENTITY_NRIGHTARROW,		8655,	"nRightarrow",			(char*[]) { "nrArr", NULL } },	// 0x21cf
//	{ MANUAL_ENTITY_LARR,			8656,	"lArr",				(char*[]) { "DoubleLeftArrow", "Leftarrow", NULL } },	// 0x21d0
//	{ MANUAL_ENTITY_UARR,			8657,	"uArr",				(char*[]) { "DoubleUpArrow", "Uparrow", NULL } },	// 0x21d1
//	{ MANUAL_ENTITY_RARR,			8658,	"rArr",				(char*[]) { "DoubleRightArrow", "Implies", "Rightarrow", NULL } },	// 0x21d2
//	{ MANUAL_ENTITY_DARR,			8659,	"dArr",				(char*[]) { "DoubleDownArrow", "Downarrow", NULL } },	// 0x21d3
//	{ MANUAL_ENTITY_HARR,			8660,	"hArr",				(char*[]) { "DoubleLeftRightArrow", "Leftrightarrow", "iff", NULL } },	// 0x21d4
	{ MANUAL_ENTITY_DOUBLEUPDOWNARROW,	8661,	"DoubleUpDownArrow",		(char*[]) { "Updownarrow", "vArr", NULL } },	// 0x21d5
	{ MANUAL_ENTITY_NWARR,			8662,	"nwArr",			NULL },	// 0x21d6
	{ MANUAL_ENTITY_NEARR,			8663,	"neArr",			NULL },	// 0x21d7
	{ MANUAL_ENTITY_SEARR,			8664,	"seArr",			NULL },	// 0x21d8
	{ MANUAL_ENTITY_SWARR,			8665,	"swArr",			NULL },	// 0x21d9
	{ MANUAL_ENTITY_LLEFTARROW,		8666,	"Lleftarrow",			(char*[]) { "lAarr", NULL } },	// 0x21da
	{ MANUAL_ENTITY_RRIGHTARROW,		8667,	"Rrightarrow",			(char*[]) { "rAarr", NULL } },	// 0x21db
	{ MANUAL_ENTITY_ZIGRARR,		8669,	"zigrarr",			NULL },	// 0x21dd
	{ MANUAL_ENTITY_LEFTARROWBAR,		8676,	"LeftArrowBar",			(char*[]) { "larrb", NULL } },	// 0x21e4
	{ MANUAL_ENTITY_RIGHTARROWBAR,		8677,	"RightArrowBar",		(char*[]) { "rarrb", NULL } },	// 0x21e5
	{ MANUAL_ENTITY_DOWNARROWUPARROW,	8693,	"DownArrowUpArrow",		(char*[]) { "duarr", NULL } },	// 0x21f5
	{ MANUAL_ENTITY_LOARR,			8701,	"loarr",			NULL },	// 0x21fd
	{ MANUAL_ENTITY_ROARR,			8702,	"roarr",			NULL },	// 0x21fe
	{ MANUAL_ENTITY_HOARR,			8703,	"hoarr",			NULL },	// 0x21ff

	/* Mathematical Operators */

	{ MANUAL_ENTITY_FORALL,			8704,	"forall",			(char*[]) { "ForAll", NULL } },	// 0x2200
	{ MANUAL_ENTITY_COMP,			8705,	"comp",				(char*[]) { "complement", NULL } },	// 0x2201
	{ MANUAL_ENTITY_PART,			8706,	"part",				(char*[]) { "PartialD", NULL } },	// 0x2202
	{ MANUAL_ENTITY_EXIST,			8707,	"exist",			(char*[]) { "Exists", NULL } },	// 0x2203
	{ MANUAL_ENTITY_NOTEXISTS,		8708,	"NotExists",			(char*[]) { "nexist", "nexists", NULL } },	// 0x2204
	{ MANUAL_ENTITY_EMPTY,			8709,	"empty",			(char*[]) { "emptyset", "emptyv", "varnothing", NULL } },	// 0x2205
	{ MANUAL_ENTITY_NABLA,			8711,	"nabla",			(char*[]) { "Del", NULL } },	// 0x2207
	{ MANUAL_ENTITY_ISIN,			8712,	"isin",				(char*[]) { "Element", "in", "isinv", NULL } },	// 0x2208
	{ MANUAL_ENTITY_NOTIN,			8713,	"notin",			(char*[]) { "NotElement", "notinva", NULL } },	// 0x2209
	{ MANUAL_ENTITY_NI,			8715,	"ni",				(char*[]) { "ReverseElement", "SuchThat", "niv", NULL } },	// 0x220b
	{ MANUAL_ENTITY_NOTREVERSEELEMENT,	8716,	"NotReverseElement",		(char*[]) { "notni", "notniva", NULL } },	// 0x220c
	{ MANUAL_ENTITY_PROD,			8719,	"prod",				(char*[]) { "Product", NULL } },	// 0x220f
	{ MANUAL_ENTITY_COPRODUCT,		8720,	"Coproduct",			(char*[]) { "coprod", NULL } },	// 0x2210
	{ MANUAL_ENTITY_SUM,			8721,	"sum",				(char*[]) { "Sum", NULL } },	// 0x2211
	{ MANUAL_ENTITY_MINUS,			8722,	"minus",			NULL },	// 0x2212
	{ MANUAL_ENTITY_MINUSPLUS,		8723,	"MinusPlus",			(char*[]) { "mnplus", "mp", NULL } },	// 0x2213
	{ MANUAL_ENTITY_DOTPLUS,		8724,	"dotplus",			(char*[]) { "plusdo", NULL } },	// 0x2214
	{ MANUAL_ENTITY_BACKSLASH,		8726,	"Backslash",			(char*[]) { "setminus", "setmn", "smallsetminus", "ssetmn", NULL } },	// 0x2216
	{ MANUAL_ENTITY_LOWAST,			8727,	"lowast",			NULL },	// 0x2217
	{ MANUAL_ENTITY_SMALLCIRCLE,		8728,	"SmallCircle",			(char*[]) { "compfn", NULL } },	// 0x2218
	{ MANUAL_ENTITY_RADIC,			8730,	"radic",			(char*[]) { "Sqrt", NULL } },	// 0x221a
	{ MANUAL_ENTITY_PROP,			8733,	"prop",				(char*[]) { "Proportional", "propto", "varpropto", "vprop", NULL } },	// 0x221d
	{ MANUAL_ENTITY_INFIN,			8734,	"infin",			NULL },	// 0x221e
	{ MANUAL_ENTITY_ANGRT,			8735,	"angrt",			NULL },	// 0x221f
	{ MANUAL_ENTITY_ANG,			8736,	"ang",				(char*[]) { "angle", NULL } },	// 0x2220
	{ MANUAL_ENTITY_ANGMSD,			8737,	"angmsd",			(char*[]) { "measuredangle", NULL } },	// 0x2221
	{ MANUAL_ENTITY_ANGSPH,			8738,	"angsph",			NULL },	// 0x2222
	{ MANUAL_ENTITY_VERTICALBAR,		8739,	"VerticalBar",			(char*[]) { "mid", "shortmid", "smid", NULL } },	// 0x2223
	{ MANUAL_ENTITY_NOTVERTICALBAR,		8740,	"NotVerticalBar",		(char*[]) { "nmid", "nshortmid", "nsmid", NULL } },	// 0x2224
	{ MANUAL_ENTITY_DOUBLEVERTICALBAR,	8741,	"DoubleVerticalBar",		(char*[]) { "par", "parallel", "shortparallel", "spar", NULL } },	// 0x2225
	{ MANUAL_ENTITY_NOTDOUBLEVERTICALBAR,	8742,	"NotDoubleVerticalBar",		(char*[]) { "npar", "nparallel", "nshortparallel", "nspar", NULL } },	// 0x2226
//	{ MANUAL_ENTITY_AND,			8743,	"and",				(char*[]) { "wedge", NULL } },	// 0x2227
//	{ MANUAL_ENTITY_OR,			8744,	"or",				(char*[]) { "vee", NULL } },	// 0x2228
//	{ MANUAL_ENTITY_CAP,			8745,	"cap",				NULL },	// 0x2229
//	{ MANUAL_ENTITY_CUP,			8746,	"cup",				NULL },	// 0x222a
//	{ MANUAL_ENTITY_INT,			8747,	"int",				(char*[]) { "Integral", NULL } },	// 0x222b
//	{ MANUAL_ENTITY_INT,			8748,	"Int",				NULL },	// 0x222c
	{ MANUAL_ENTITY_IIINT,			8749,	"iiint",			(char*[]) { "tint", NULL } },	// 0x222d
	{ MANUAL_ENTITY_CONTOURINTEGRAL,	8750,	"ContourIntegral",		(char*[]) { "conint", "oint", NULL } },	// 0x222e
	{ MANUAL_ENTITY_CONINT,			8751,	"Conint",			(char*[]) { "DoubleContourIntegral", NULL } },	// 0x222f
	{ MANUAL_ENTITY_CCONINT,		8752,	"Cconint",			NULL },	// 0x2230
	{ MANUAL_ENTITY_CWINT,			8753,	"cwint",			NULL },	// 0x2231
	{ MANUAL_ENTITY_CLOCKWISECONTOURINTEGRAL,	8754,	"ClockwiseContourIntegral",	(char*[]) { "cwconint", NULL } },	// 0x2232
	{ MANUAL_ENTITY_COUNTERCLOCKWISECONTOURINTEGRAL,	8755,	"CounterClockwiseContourIntegral",	(char*[]) { "awconint", NULL } },	// 0x2233
	{ MANUAL_ENTITY_THERE4,			8756,	"there4",			(char*[]) { "Therefore", "therefore", NULL } },	// 0x2234
	{ MANUAL_ENTITY_BECAUSE,		8757,	"Because",			(char*[]) { "becaus", "because", NULL } },	// 0x2235
	{ MANUAL_ENTITY_RATIO,			8758,	"ratio",			NULL },	// 0x2236
//	{ MANUAL_ENTITY_COLON,			8759,	"Colon",			(char*[]) { "Proportion", NULL } },	// 0x2237
	{ MANUAL_ENTITY_DOTMINUS,		8760,	"dotminus",			(char*[]) { "minusd", NULL } },	// 0x2238
	{ MANUAL_ENTITY_MDDOT,			8762,	"mDDot",			NULL },	// 0x223a
	{ MANUAL_ENTITY_HOMTHT,			8763,	"homtht",			NULL },	// 0x223b
	{ MANUAL_ENTITY_SIM,			8764,	"sim",				(char*[]) { "Tilde", "thicksim", "thksim", NULL } },	// 0x223c
	{ MANUAL_ENTITY_BACKSIM,		8765,	"backsim",			(char*[]) { "bsim", NULL } },	// 0x223d
	{ MANUAL_ENTITY_AC,			8766,	"ac",				(char*[]) { "mstpos", NULL } },	// 0x223e
	{ MANUAL_ENTITY_ACD,			8767,	"acd",				NULL },	// 0x223f
	{ MANUAL_ENTITY_VERTICALTILDE,		8768,	"VerticalTilde",		(char*[]) { "wr", "wreath", NULL } },	// 0x2240
	{ MANUAL_ENTITY_NOTTILDE,		8769,	"NotTilde",			(char*[]) { "nsim", NULL } },	// 0x2241
	{ MANUAL_ENTITY_EQUALTILDE,		8770,	"EqualTilde",			(char*[]) { "eqsim", "esim", NULL } },	// 0x2242
	{ MANUAL_ENTITY_TILDEEQUAL,		8771,	"TildeEqual",			(char*[]) { "sime", "simeq", NULL } },	// 0x2243
	{ MANUAL_ENTITY_NOTTILDEEQUAL,		8772,	"NotTildeEqual",		(char*[]) { "nsime", "nsimeq", NULL } },	// 0x2244
	{ MANUAL_ENTITY_CONG,			8773,	"cong",				(char*[]) { "TildeFullEqual", NULL } },	// 0x2245
	{ MANUAL_ENTITY_SIMNE,			8774,	"simne",			NULL },	// 0x2246
	{ MANUAL_ENTITY_NOTTILDEFULLEQUAL,	8775,	"NotTildeFullEqual",		(char*[]) { "ncong", NULL } },	// 0x2247
	{ MANUAL_ENTITY_ASYMP,			8776,	"asymp",			(char*[]) { "TildeTilde", "ap", "approx", "thickapprox", "thkap", NULL } },	// 0x2248
	{ MANUAL_ENTITY_NOTTILDETILDE,		8777,	"NotTildeTilde",		(char*[]) { "nap", "napprox", NULL } },	// 0x2249
//	{ MANUAL_ENTITY_APE,			8778,	"ape",				(char*[]) { "approxeq", NULL } },	// 0x224a
	{ MANUAL_ENTITY_APID,			8779,	"apid",				NULL },	// 0x224b
	{ MANUAL_ENTITY_BACKCONG,		8780,	"backcong",			(char*[]) { "bcong", NULL } },	// 0x224c
//	{ MANUAL_ENTITY_CUPCAP,			8781,	"CupCap",			(char*[]) { "asympeq", NULL } },	// 0x224d
	{ MANUAL_ENTITY_BUMPEQ,			8782,	"Bumpeq",			(char*[]) { "HumpDownHump", "bump", NULL } },	// 0x224e
	{ MANUAL_ENTITY_HUMPEQUAL,		8783,	"HumpEqual",			(char*[]) { "bumpe", "bumpeq", NULL } },	// 0x224f
	{ MANUAL_ENTITY_DOTEQUAL,		8784,	"DotEqual",			(char*[]) { "doteq", "esdot", NULL } },	// 0x2250
	{ MANUAL_ENTITY_DOTEQDOT,		8785,	"doteqdot",			(char*[]) { "eDot", NULL } },	// 0x2251
	{ MANUAL_ENTITY_EFDOT,			8786,	"efDot",			(char*[]) { "fallingdotseq", NULL } },	// 0x2252
	{ MANUAL_ENTITY_ERDOT,			8787,	"erDot",			(char*[]) { "risingdotseq", NULL } },	// 0x2253
	{ MANUAL_ENTITY_ASSIGN,			8788,	"Assign",			(char*[]) { "colone", "coloneq", NULL } },	// 0x2254
	{ MANUAL_ENTITY_ECOLON,			8789,	"ecolon",			(char*[]) { "eqcolon", NULL } },	// 0x2255
	{ MANUAL_ENTITY_ECIR,			8790,	"ecir",				(char*[]) { "eqcirc", NULL } },	// 0x2256
	{ MANUAL_ENTITY_CIRCEQ,			8791,	"circeq",			(char*[]) { "cire", NULL } },	// 0x2257
	{ MANUAL_ENTITY_WEDGEQ,			8793,	"wedgeq",			NULL },	// 0x2259
	{ MANUAL_ENTITY_VEEEQ,			8794,	"veeeq",			NULL },	// 0x225a
	{ MANUAL_ENTITY_TRIANGLEQ,		8796,	"triangleq",			(char*[]) { "trie", NULL } },	// 0x225c
	{ MANUAL_ENTITY_EQUEST,			8799,	"equest",			(char*[]) { "questeq", NULL } },	// 0x225f
	{ MANUAL_ENTITY_NE,			8800,	"ne",				(char*[]) { "NotEqual", NULL } },	// 0x2260
	{ MANUAL_ENTITY_EQUIV,			8801,	"equiv",			(char*[]) { "Congruent", NULL } },	// 0x2261
	{ MANUAL_ENTITY_NOTCONGRUENT,		8802,	"NotCongruent",			(char*[]) { "nequiv", NULL } },	// 0x2262
	{ MANUAL_ENTITY_LE,			8804,	"le",				(char*[]) { "leq", NULL } },	// 0x2264
	{ MANUAL_ENTITY_GE,			8805,	"ge",				(char*[]) { "GreaterEqual", "geq", NULL } },	// 0x2265
	{ MANUAL_ENTITY_LESSFULLEQUAL,		8806,	"LessFullEqual",		(char*[]) { "lE", "leqq", NULL } },	// 0x2266
	{ MANUAL_ENTITY_GREATERFULLEQUAL,	8807,	"GreaterFullEqual",		(char*[]) { "gE", "geqq", NULL } },	// 0x2267
//	{ MANUAL_ENTITY_LNE,			8808,	"lnE",				(char*[]) { "lneqq", NULL } },	// 0x2268
//	{ MANUAL_ENTITY_GNE,			8809,	"gnE",				(char*[]) { "gneqq", NULL } },	// 0x2269
//	{ MANUAL_ENTITY_LT,			8810,	"Lt",				(char*[]) { "NestedLessLess", "ll", NULL } },	// 0x226a
//	{ MANUAL_ENTITY_GT,			8811,	"Gt",				(char*[]) { "NestedGreaterGreater", "gg", NULL } },	// 0x226b
	{ MANUAL_ENTITY_BETWEEN,		8812,	"between",			(char*[]) { "twixt", NULL } },	// 0x226c
	{ MANUAL_ENTITY_NOTCUPCAP,		8813,	"NotCupCap",			NULL },	// 0x226d
	{ MANUAL_ENTITY_NOTLESS,		8814,	"NotLess",			(char*[]) { "nless", "nlt", NULL } },	// 0x226e
	{ MANUAL_ENTITY_NOTGREATER,		8815,	"NotGreater",			(char*[]) { "ngt", "ngtr", NULL } },	// 0x226f
	{ MANUAL_ENTITY_NOTLESSEQUAL,		8816,	"NotLessEqual",			(char*[]) { "nle", "nleq", NULL } },	// 0x2270
	{ MANUAL_ENTITY_NOTGREATEREQUAL,	8817,	"NotGreaterEqual",		(char*[]) { "nge", "ngeq", NULL } },	// 0x2271
	{ MANUAL_ENTITY_LESSTILDE,		8818,	"LessTilde",			(char*[]) { "lesssim", "lsim", NULL } },	// 0x2272
	{ MANUAL_ENTITY_GREATERTILDE,		8819,	"GreaterTilde",			(char*[]) { "gsim", "gtrsim", NULL } },	// 0x2273
	{ MANUAL_ENTITY_NOTLESSTILDE,		8820,	"NotLessTilde",			(char*[]) { "nlsim", NULL } },	// 0x2274
	{ MANUAL_ENTITY_NOTGREATERTILDE,	8821,	"NotGreaterTilde",		(char*[]) { "ngsim", NULL } },	// 0x2275
	{ MANUAL_ENTITY_LESSGREATER,		8822,	"LessGreater",			(char*[]) { "lessgtr", "lg", NULL } },	// 0x2276
	{ MANUAL_ENTITY_GREATERLESS,		8823,	"GreaterLess",			(char*[]) { "gl", "gtrless", NULL } },	// 0x2277
	{ MANUAL_ENTITY_NOTLESSGREATER,		8824,	"NotLessGreater",		(char*[]) { "ntlg", NULL } },	// 0x2278
	{ MANUAL_ENTITY_NOTGREATERLESS,		8825,	"NotGreaterLess",		(char*[]) { "ntgl", NULL } },	// 0x2279
	{ MANUAL_ENTITY_PRECEDES,		8826,	"Precedes",			(char*[]) { "pr", "prec", NULL } },	// 0x227a
	{ MANUAL_ENTITY_SUCCEEDS,		8827,	"Succeeds",			(char*[]) { "sc", "succ", NULL } },	// 0x227b
	{ MANUAL_ENTITY_PRECEDESSLANTEQUAL,	8828,	"PrecedesSlantEqual",		(char*[]) { "prcue", "preccurlyeq", NULL } },	// 0x227c
	{ MANUAL_ENTITY_SUCCEEDSSLANTEQUAL,	8829,	"SucceedsSlantEqual",		(char*[]) { "sccue", "succcurlyeq", NULL } },	// 0x227d
	{ MANUAL_ENTITY_PRECEDESTILDE,		8830,	"PrecedesTilde",		(char*[]) { "precsim", "prsim", NULL } },	// 0x227e
	{ MANUAL_ENTITY_SUCCEEDSTILDE,		8831,	"SucceedsTilde",		(char*[]) { "scsim", "succsim", NULL } },	// 0x227f
	{ MANUAL_ENTITY_NOTPRECEDES,		8832,	"NotPrecedes",			(char*[]) { "npr", "nprec", NULL } },	// 0x2280
	{ MANUAL_ENTITY_NOTSUCCEEDS,		8833,	"NotSucceeds",			(char*[]) { "nsc", "nsucc", NULL } },	// 0x2281
//	{ MANUAL_ENTITY_SUB,			8834,	"sub",				(char*[]) { "subset", NULL } },	// 0x2282
//	{ MANUAL_ENTITY_SUP,			8835,	"sup",				(char*[]) { "Superset", "supset", NULL } },	// 0x2283
	{ MANUAL_ENTITY_NSUB,			8836,	"nsub",				NULL },	// 0x2284
	{ MANUAL_ENTITY_NSUP,			8837,	"nsup",				NULL },	// 0x2285
//	{ MANUAL_ENTITY_SUBE,			8838,	"sube",				(char*[]) { "SubsetEqual", "subseteq", NULL } },	// 0x2286
//	{ MANUAL_ENTITY_SUPE,			8839,	"supe",				(char*[]) { "SupersetEqual", "supseteq", NULL } },	// 0x2287
	{ MANUAL_ENTITY_NOTSUBSETEQUAL,		8840,	"NotSubsetEqual",		(char*[]) { "nsube", "nsubseteq", NULL } },	// 0x2288
	{ MANUAL_ENTITY_NOTSUPERSETEQUAL,	8841,	"NotSupersetEqual",		(char*[]) { "nsupe", "nsupseteq", NULL } },	// 0x2289
//	{ MANUAL_ENTITY_SUBNE,			8842,	"subne",			(char*[]) { "subsetneq", NULL } },	// 0x228a
//	{ MANUAL_ENTITY_SUPNE,			8843,	"supne",			(char*[]) { "supsetneq", NULL } },	// 0x228b
	{ MANUAL_ENTITY_CUPDOT,			8845,	"cupdot",			NULL },	// 0x228d
	{ MANUAL_ENTITY_UNIONPLUS,		8846,	"UnionPlus",			(char*[]) { "uplus", NULL } },	// 0x228e
	{ MANUAL_ENTITY_SQUARESUBSET,		8847,	"SquareSubset",			(char*[]) { "sqsub", "sqsubset", NULL } },	// 0x228f
	{ MANUAL_ENTITY_SQUARESUPERSET,		8848,	"SquareSuperset",		(char*[]) { "sqsup", "sqsupset", NULL } },	// 0x2290
	{ MANUAL_ENTITY_SQUARESUBSETEQUAL,	8849,	"SquareSubsetEqual",		(char*[]) { "sqsube", "sqsubseteq", NULL } },	// 0x2291
	{ MANUAL_ENTITY_SQUARESUPERSETEQUAL,	8850,	"SquareSupersetEqual",		(char*[]) { "sqsupe", "sqsupseteq", NULL } },	// 0x2292
	{ MANUAL_ENTITY_SQUAREINTERSECTION,	8851,	"SquareIntersection",		(char*[]) { "sqcap", NULL } },	// 0x2293
	{ MANUAL_ENTITY_SQUAREUNION,		8852,	"SquareUnion",			(char*[]) { "sqcup", NULL } },	// 0x2294
	{ MANUAL_ENTITY_OPLUS,			8853,	"oplus",			(char*[]) { "CirclePlus", NULL } },	// 0x2295
	{ MANUAL_ENTITY_CIRCLEMINUS,		8854,	"CircleMinus",			(char*[]) { "ominus", NULL } },	// 0x2296
//	{ MANUAL_ENTITY_OTIMES,			8855,	"otimes",			(char*[]) { "CircleTimes", NULL } },	// 0x2297
	{ MANUAL_ENTITY_OSOL,			8856,	"osol",				NULL },	// 0x2298
	{ MANUAL_ENTITY_CIRCLEDOT,		8857,	"CircleDot",			(char*[]) { "odot", NULL } },	// 0x2299
	{ MANUAL_ENTITY_CIRCLEDCIRC,		8858,	"circledcirc",			(char*[]) { "ocir", NULL } },	// 0x229a
	{ MANUAL_ENTITY_CIRCLEDAST,		8859,	"circledast",			(char*[]) { "oast", NULL } },	// 0x229b
	{ MANUAL_ENTITY_CIRCLEDDASH,		8861,	"circleddash",			(char*[]) { "odash", NULL } },	// 0x229d
	{ MANUAL_ENTITY_BOXPLUS,		8862,	"boxplus",			(char*[]) { "plusb", NULL } },	// 0x229e
	{ MANUAL_ENTITY_BOXMINUS,		8863,	"boxminus",			(char*[]) { "minusb", NULL } },	// 0x229f
	{ MANUAL_ENTITY_BOXTIMES,		8864,	"boxtimes",			(char*[]) { "timesb", NULL } },	// 0x22a0
	{ MANUAL_ENTITY_DOTSQUARE,		8865,	"dotsquare",			(char*[]) { "sdotb", NULL } },	// 0x22a1
	{ MANUAL_ENTITY_RIGHTTEE,		8866,	"RightTee",			(char*[]) { "vdash", NULL } },	// 0x22a2
	{ MANUAL_ENTITY_LEFTTEE,		8867,	"LeftTee",			(char*[]) { "dashv", NULL } },	// 0x22a3
	{ MANUAL_ENTITY_DOWNTEE,		8868,	"DownTee",			(char*[]) { "top", NULL } },	// 0x22a4
	{ MANUAL_ENTITY_PERP,			8869,	"perp",				(char*[]) { "UpTee", "bot", "bottom", NULL } },	// 0x22a5
	{ MANUAL_ENTITY_MODELS,			8871,	"models",			NULL },	// 0x22a7
	{ MANUAL_ENTITY_DOUBLERIGHTTEE,		8872,	"DoubleRightTee",		(char*[]) { "vDash", NULL } },	// 0x22a8
//	{ MANUAL_ENTITY_VDASH,			8873,	"Vdash",			NULL },	// 0x22a9
	{ MANUAL_ENTITY_VVDASH,			8874,	"Vvdash",			NULL },	// 0x22aa
//	{ MANUAL_ENTITY_VDASH,			8875,	"VDash",			NULL },	// 0x22ab
//	{ MANUAL_ENTITY_NVDASH,			8876,	"nvdash",			NULL },	// 0x22ac
//	{ MANUAL_ENTITY_NVDASH,			8877,	"nvDash",			NULL },	// 0x22ad
//	{ MANUAL_ENTITY_NVDASH,			8878,	"nVdash",			NULL },	// 0x22ae
//	{ MANUAL_ENTITY_NVDASH,			8879,	"nVDash",			NULL },	// 0x22af
	{ MANUAL_ENTITY_PRUREL,			8880,	"prurel",			NULL },	// 0x22b0
	{ MANUAL_ENTITY_LEFTTRIANGLE,		8882,	"LeftTriangle",			(char*[]) { "vartriangleleft", "vltri", NULL } },	// 0x22b2
	{ MANUAL_ENTITY_RIGHTTRIANGLE,		8883,	"RightTriangle",		(char*[]) { "vartriangleright", "vrtri", NULL } },	// 0x22b3
	{ MANUAL_ENTITY_LEFTTRIANGLEEQUAL,	8884,	"LeftTriangleEqual",		(char*[]) { "ltrie", "trianglelefteq", NULL } },	// 0x22b4
	{ MANUAL_ENTITY_RIGHTTRIANGLEEQUAL,	8885,	"RightTriangleEqual",		(char*[]) { "rtrie", "trianglerighteq", NULL } },	// 0x22b5
	{ MANUAL_ENTITY_ORIGOF,			8886,	"origof",			NULL },	// 0x22b6
	{ MANUAL_ENTITY_IMOF,			8887,	"imof",				NULL },	// 0x22b7
	{ MANUAL_ENTITY_MULTIMAP,		8888,	"multimap",			(char*[]) { "mumap", NULL } },	// 0x22b8
	{ MANUAL_ENTITY_HERCON,			8889,	"hercon",			NULL },	// 0x22b9
	{ MANUAL_ENTITY_INTCAL,			8890,	"intcal",			(char*[]) { "intercal", NULL } },	// 0x22ba
	{ MANUAL_ENTITY_VEEBAR,			8891,	"veebar",			NULL },	// 0x22bb
	{ MANUAL_ENTITY_BARVEE,			8893,	"barvee",			NULL },	// 0x22bd
	{ MANUAL_ENTITY_ANGRTVB,		8894,	"angrtvb",			NULL },	// 0x22be
	{ MANUAL_ENTITY_LRTRI,			8895,	"lrtri",			NULL },	// 0x22bf
	{ MANUAL_ENTITY_WEDGE,			8896,	"Wedge",			(char*[]) { "bigwedge", "xwedge", NULL } },	// 0x22c0
	{ MANUAL_ENTITY_VEE,			8897,	"Vee",				(char*[]) { "bigvee", "xvee", NULL } },	// 0x22c1
	{ MANUAL_ENTITY_INTERSECTION,		8898,	"Intersection",			(char*[]) { "bigcap", "xcap", NULL } },	// 0x22c2
	{ MANUAL_ENTITY_UNION,			8899,	"Union",			(char*[]) { "bigcup", "xcup", NULL } },	// 0x22c3
	{ MANUAL_ENTITY_DIAMOND,		8900,	"Diamond",			(char*[]) { "diam", "diamond", NULL } },	// 0x22c4
	{ MANUAL_ENTITY_SDOT,			8901,	"sdot",				NULL },	// 0x22c5
//	{ MANUAL_ENTITY_STAR,			8902,	"Star",				(char*[]) { "sstarf", NULL } },	// 0x22c6
	{ MANUAL_ENTITY_DIVIDEONTIMES,		8903,	"divideontimes",		(char*[]) { "divonx", NULL } },	// 0x22c7
	{ MANUAL_ENTITY_BOWTIE,			8904,	"bowtie",			NULL },	// 0x22c8
	{ MANUAL_ENTITY_LTIMES,			8905,	"ltimes",			NULL },	// 0x22c9
	{ MANUAL_ENTITY_RTIMES,			8906,	"rtimes",			NULL },	// 0x22ca
	{ MANUAL_ENTITY_LEFTTHREETIMES,		8907,	"leftthreetimes",		(char*[]) { "lthree", NULL } },	// 0x22cb
	{ MANUAL_ENTITY_RIGHTTHREETIMES,	8908,	"rightthreetimes",		(char*[]) { "rthree", NULL } },	// 0x22cc
	{ MANUAL_ENTITY_BACKSIMEQ,		8909,	"backsimeq",			(char*[]) { "bsime", NULL } },	// 0x22cd
	{ MANUAL_ENTITY_CURLYVEE,		8910,	"curlyvee",			(char*[]) { "cuvee", NULL } },	// 0x22ce
	{ MANUAL_ENTITY_CURLYWEDGE,		8911,	"curlywedge",			(char*[]) { "cuwed", NULL } },	// 0x22cf
//	{ MANUAL_ENTITY_SUB,			8912,	"Sub",				(char*[]) { "Subset", NULL } },	// 0x22d0
//	{ MANUAL_ENTITY_SUP,			8913,	"Sup",				(char*[]) { "Supset", NULL } },	// 0x22d1
//	{ MANUAL_ENTITY_CAP,			8914,	"Cap",				NULL },	// 0x22d2
//	{ MANUAL_ENTITY_CUP,			8915,	"Cup",				NULL },	// 0x22d3
	{ MANUAL_ENTITY_FORK,			8916,	"fork",				(char*[]) { "pitchfork", NULL } },	// 0x22d4
	{ MANUAL_ENTITY_EPAR,			8917,	"epar",				NULL },	// 0x22d5
	{ MANUAL_ENTITY_LESSDOT,		8918,	"lessdot",			(char*[]) { "ltdot", NULL } },	// 0x22d6
	{ MANUAL_ENTITY_GTDOT,			8919,	"gtdot",			(char*[]) { "gtrdot", NULL } },	// 0x22d7
	{ MANUAL_ENTITY_LL,			8920,	"Ll",				NULL },	// 0x22d8
	{ MANUAL_ENTITY_GG,			8921,	"Gg",				(char*[]) { "ggg", NULL } },	// 0x22d9
	{ MANUAL_ENTITY_LESSEQUALGREATER,	8922,	"LessEqualGreater",		(char*[]) { "leg", "lesseqgtr", NULL } },	// 0x22da
	{ MANUAL_ENTITY_GREATEREQUALLESS,	8923,	"GreaterEqualLess",		(char*[]) { "gel", "gtreqless", NULL } },	// 0x22db
	{ MANUAL_ENTITY_CUEPR,			8926,	"cuepr",			(char*[]) { "curlyeqprec", NULL } },	// 0x22de
	{ MANUAL_ENTITY_CUESC,			8927,	"cuesc",			(char*[]) { "curlyeqsucc", NULL } },	// 0x22df
	{ MANUAL_ENTITY_NOTPRECEDESSLANTEQUAL,	8928,	"NotPrecedesSlantEqual",	(char*[]) { "nprcue", NULL } },	// 0x22e0
	{ MANUAL_ENTITY_NOTSUCCEEDSSLANTEQUAL,	8929,	"NotSucceedsSlantEqual",	(char*[]) { "nsccue", NULL } },	// 0x22e1
	{ MANUAL_ENTITY_NOTSQUARESUBSETEQUAL,	8930,	"NotSquareSubsetEqual",		(char*[]) { "nsqsube", NULL } },	// 0x22e2
	{ MANUAL_ENTITY_NOTSQUARESUPERSETEQUAL,	8931,	"NotSquareSupersetEqual",	(char*[]) { "nsqsupe", NULL } },	// 0x22e3
	{ MANUAL_ENTITY_LNSIM,			8934,	"lnsim",			NULL },	// 0x22e6
	{ MANUAL_ENTITY_GNSIM,			8935,	"gnsim",			NULL },	// 0x22e7
	{ MANUAL_ENTITY_PRECNSIM,		8936,	"precnsim",			(char*[]) { "prnsim", NULL } },	// 0x22e8
	{ MANUAL_ENTITY_SCNSIM,			8937,	"scnsim",			(char*[]) { "succnsim", NULL } },	// 0x22e9
	{ MANUAL_ENTITY_NOTLEFTTRIANGLE,	8938,	"NotLeftTriangle",		(char*[]) { "nltri", "ntriangleleft", NULL } },	// 0x22ea
	{ MANUAL_ENTITY_NOTRIGHTTRIANGLE,	8939,	"NotRightTriangle",		(char*[]) { "nrtri", "ntriangleright", NULL } },	// 0x22eb
	{ MANUAL_ENTITY_NOTLEFTTRIANGLEEQUAL,	8940,	"NotLeftTriangleEqual",		(char*[]) { "nltrie", "ntrianglelefteq", NULL } },	// 0x22ec
	{ MANUAL_ENTITY_NOTRIGHTTRIANGLEEQUAL,	8941,	"NotRightTriangleEqual",	(char*[]) { "nrtrie", "ntrianglerighteq", NULL } },	// 0x22ed
	{ MANUAL_ENTITY_VELLIP,			8942,	"vellip",			NULL },	// 0x22ee
	{ MANUAL_ENTITY_CTDOT,			8943,	"ctdot",			NULL },	// 0x22ef
	{ MANUAL_ENTITY_UTDOT,			8944,	"utdot",			NULL },	// 0x22f0
	{ MANUAL_ENTITY_DTDOT,			8945,	"dtdot",			NULL },	// 0x22f1
	{ MANUAL_ENTITY_DISIN,			8946,	"disin",			NULL },	// 0x22f2
	{ MANUAL_ENTITY_ISINSV,			8947,	"isinsv",			NULL },	// 0x22f3
	{ MANUAL_ENTITY_ISINS,			8948,	"isins",			NULL },	// 0x22f4
	{ MANUAL_ENTITY_ISINDOT,		8949,	"isindot",			NULL },	// 0x22f5
	{ MANUAL_ENTITY_NOTINVC,		8950,	"notinvc",			NULL },	// 0x22f6
	{ MANUAL_ENTITY_NOTINVB,		8951,	"notinvb",			NULL },	// 0x22f7
	{ MANUAL_ENTITY_ISINE,			8953,	"isinE",			NULL },	// 0x22f9
	{ MANUAL_ENTITY_NISD,			8954,	"nisd",				NULL },	// 0x22fa
	{ MANUAL_ENTITY_XNIS,			8955,	"xnis",				NULL },	// 0x22fb
	{ MANUAL_ENTITY_NIS,			8956,	"nis",				NULL },	// 0x22fc
	{ MANUAL_ENTITY_NOTNIVC,		8957,	"notnivc",			NULL },	// 0x22fd
	{ MANUAL_ENTITY_NOTNIVB,		8958,	"notnivb",			NULL },	// 0x22fe

	/* Miscellaneous Technical */

//	{ MANUAL_ENTITY_BARWED,			8965,	"barwed",			(char*[]) { "barwedge", NULL } },	// 0x2305
//	{ MANUAL_ENTITY_BARWED,			8966,	"Barwed",			(char*[]) { "doublebarwedge", NULL } },	// 0x2306
	{ MANUAL_ENTITY_LCEIL,			8968,	"lceil",			(char*[]) { "LeftCeiling", NULL } },	// 0x2308
	{ MANUAL_ENTITY_RCEIL,			8969,	"rceil",			(char*[]) { "RightCeiling", NULL } },	// 0x2309
	{ MANUAL_ENTITY_LFLOOR,			8970,	"lfloor",			(char*[]) { "LeftFloor", NULL } },	// 0x230a
	{ MANUAL_ENTITY_RFLOOR,			8971,	"rfloor",			(char*[]) { "RightFloor", NULL } },	// 0x230b
	{ MANUAL_ENTITY_DRCROP,			8972,	"drcrop",			NULL },	// 0x230c
	{ MANUAL_ENTITY_DLCROP,			8973,	"dlcrop",			NULL },	// 0x230d
	{ MANUAL_ENTITY_URCROP,			8974,	"urcrop",			NULL },	// 0x230e
	{ MANUAL_ENTITY_ULCROP,			8975,	"ulcrop",			NULL },	// 0x230f
//	{ MANUAL_ENTITY_BNOT,			8976,	"bnot",				NULL },	// 0x2310
	{ MANUAL_ENTITY_PROFLINE,		8978,	"profline",			NULL },	// 0x2312
	{ MANUAL_ENTITY_PROFSURF,		8979,	"profsurf",			NULL },	// 0x2313
	{ MANUAL_ENTITY_TELREC,			8981,	"telrec",			NULL },	// 0x2315
	{ MANUAL_ENTITY_TARGET,			8982,	"target",			NULL },	// 0x2316
	{ MANUAL_ENTITY_ULCORN,			8988,	"ulcorn",			(char*[]) { "ulcorner", NULL } },	// 0x231c
	{ MANUAL_ENTITY_URCORN,			8989,	"urcorn",			(char*[]) { "urcorner", NULL } },	// 0x231d
	{ MANUAL_ENTITY_DLCORN,			8990,	"dlcorn",			(char*[]) { "llcorner", NULL } },	// 0x231e
	{ MANUAL_ENTITY_DRCORN,			8991,	"drcorn",			(char*[]) { "lrcorner", NULL } },	// 0x231f
	{ MANUAL_ENTITY_FROWN,			8994,	"frown",			(char*[]) { "sfrown", NULL } },	// 0x2322
	{ MANUAL_ENTITY_SMILE,			8995,	"smile",			(char*[]) { "ssmile", NULL } },	// 0x2323
//	{ MANUAL_ENTITY_LANG,			9001,	"lang",				NULL },	// 0x2329
//	{ MANUAL_ENTITY_RANG,			9002,	"rang",				NULL },	// 0x232a
	{ MANUAL_ENTITY_CYLCTY,			9005,	"cylcty",			NULL },	// 0x232d
	{ MANUAL_ENTITY_PROFALAR,		9006,	"profalar",			NULL },	// 0x232e
	{ MANUAL_ENTITY_TOPBOT,			9014,	"topbot",			NULL },	// 0x2336
	{ MANUAL_ENTITY_OVBAR,			9021,	"ovbar",			NULL },	// 0x233d
	{ MANUAL_ENTITY_SOLBAR,			9023,	"solbar",			NULL },	// 0x233f
	{ MANUAL_ENTITY_ANGZARR,		9084,	"angzarr",			NULL },	// 0x237c
	{ MANUAL_ENTITY_LMOUST,			9136,	"lmoust",			(char*[]) { "lmoustache", NULL } },	// 0x23b0
	{ MANUAL_ENTITY_RMOUST,			9137,	"rmoust",			(char*[]) { "rmoustache", NULL } },	// 0x23b1
	{ MANUAL_ENTITY_OVERBRACKET,		9140,	"OverBracket",			(char*[]) { "tbrk", NULL } },	// 0x23b4
	{ MANUAL_ENTITY_UNDERBRACKET,		9141,	"UnderBracket",			(char*[]) { "bbrk", NULL } },	// 0x23b5
	{ MANUAL_ENTITY_BBRKTBRK,		9142,	"bbrktbrk",			NULL },	// 0x23b6
	{ MANUAL_ENTITY_OVERPARENTHESIS,	9180,	"OverParenthesis",		NULL },	// 0x23dc
	{ MANUAL_ENTITY_UNDERPARENTHESIS,	9181,	"UnderParenthesis",		NULL },	// 0x23dd
	{ MANUAL_ENTITY_OVERBRACE,		9182,	"OverBrace",			NULL },	// 0x23de
	{ MANUAL_ENTITY_UNDERBRACE,		9183,	"UnderBrace",			NULL },	// 0x23df
	{ MANUAL_ENTITY_TRPEZIUM,		9186,	"trpezium",			NULL },	// 0x23e2
	{ MANUAL_ENTITY_ELINTERS,		9191,	"elinters",			NULL },	// 0x23e7

	/* Control Pictures */

	{ MANUAL_ENTITY_BLANK,			9251,	"blank",			NULL },	// 0x2423

	/* Enclosed Alphanumerics */

	{ MANUAL_ENTITY_CIRCLEDS_U,		9416,	"circledS",			(char*[]) { "oS", NULL } },	// 0x24c8

	/* Box Drawing */

	{ MANUAL_ENTITY_HORIZONTALLINE,		9472,	"HorizontalLine",		(char*[]) { "boxh", NULL } },	// 0x2500
//	{ MANUAL_ENTITY_BOXV,			9474,	"boxv",				NULL },	// 0x2502
//	{ MANUAL_ENTITY_BOXDR,			9484,	"boxdr",			NULL },	// 0x250c
//	{ MANUAL_ENTITY_BOXDL,			9488,	"boxdl",			NULL },	// 0x2510
//	{ MANUAL_ENTITY_BOXUR,			9492,	"boxur",			NULL },	// 0x2514
//	{ MANUAL_ENTITY_BOXUL,			9496,	"boxul",			NULL },	// 0x2518
//	{ MANUAL_ENTITY_BOXVR,			9500,	"boxvr",			NULL },	// 0x251c
//	{ MANUAL_ENTITY_BOXVL,			9508,	"boxvl",			NULL },	// 0x2524
//	{ MANUAL_ENTITY_BOXHD,			9516,	"boxhd",			NULL },	// 0x252c
//	{ MANUAL_ENTITY_BOXHU,			9524,	"boxhu",			NULL },	// 0x2534
//	{ MANUAL_ENTITY_BOXVH,			9532,	"boxvh",			NULL },	// 0x253c
	{ MANUAL_ENTITY_BOXH,			9552,	"boxH",				NULL },	// 0x2550
//	{ MANUAL_ENTITY_BOXV,			9553,	"boxV",				NULL },	// 0x2551
//	{ MANUAL_ENTITY_BOXDR,			9554,	"boxdR",			NULL },	// 0x2552
//	{ MANUAL_ENTITY_BOXDR,			9555,	"boxDr",			NULL },	// 0x2553
//	{ MANUAL_ENTITY_BOXDR,			9556,	"boxDR",			NULL },	// 0x2554
//	{ MANUAL_ENTITY_BOXDL,			9557,	"boxdL",			NULL },	// 0x2555
//	{ MANUAL_ENTITY_BOXDL,			9558,	"boxDl",			NULL },	// 0x2556
//	{ MANUAL_ENTITY_BOXDL,			9559,	"boxDL",			NULL },	// 0x2557
//	{ MANUAL_ENTITY_BOXUR,			9560,	"boxuR",			NULL },	// 0x2558
//	{ MANUAL_ENTITY_BOXUR,			9561,	"boxUr",			NULL },	// 0x2559
//	{ MANUAL_ENTITY_BOXUR,			9562,	"boxUR",			NULL },	// 0x255a
//	{ MANUAL_ENTITY_BOXUL,			9563,	"boxuL",			NULL },	// 0x255b
//	{ MANUAL_ENTITY_BOXUL,			9564,	"boxUl",			NULL },	// 0x255c
//	{ MANUAL_ENTITY_BOXUL,			9565,	"boxUL",			NULL },	// 0x255d
//	{ MANUAL_ENTITY_BOXVR,			9566,	"boxvR",			NULL },	// 0x255e
//	{ MANUAL_ENTITY_BOXVR,			9567,	"boxVr",			NULL },	// 0x255f
//	{ MANUAL_ENTITY_BOXVR,			9568,	"boxVR",			NULL },	// 0x2560
//	{ MANUAL_ENTITY_BOXVL,			9569,	"boxvL",			NULL },	// 0x2561
//	{ MANUAL_ENTITY_BOXVL,			9570,	"boxVl",			NULL },	// 0x2562
//	{ MANUAL_ENTITY_BOXVL,			9571,	"boxVL",			NULL },	// 0x2563
//	{ MANUAL_ENTITY_BOXHD,			9572,	"boxHd",			NULL },	// 0x2564
//	{ MANUAL_ENTITY_BOXHD,			9573,	"boxhD",			NULL },	// 0x2565
//	{ MANUAL_ENTITY_BOXHD,			9574,	"boxHD",			NULL },	// 0x2566
//	{ MANUAL_ENTITY_BOXHU,			9575,	"boxHu",			NULL },	// 0x2567
//	{ MANUAL_ENTITY_BOXHU,			9576,	"boxhU",			NULL },	// 0x2568
//	{ MANUAL_ENTITY_BOXHU,			9577,	"boxHU",			NULL },	// 0x2569
//	{ MANUAL_ENTITY_BOXVH,			9578,	"boxvH",			NULL },	// 0x256a
//	{ MANUAL_ENTITY_BOXVH,			9579,	"boxVh",			NULL },	// 0x256b
//	{ MANUAL_ENTITY_BOXVH,			9580,	"boxVH",			NULL },	// 0x256c

	/* Block Elements */

	{ MANUAL_ENTITY_UHBLK,			9600,	"uhblk",			NULL },	// 0x2580
	{ MANUAL_ENTITY_LHBLK,			9604,	"lhblk",			NULL },	// 0x2584
	{ MANUAL_ENTITY_BLOCK,			9608,	"block",			NULL },	// 0x2588
	{ MANUAL_ENTITY_BLK14,			9617,	"blk14",			NULL },	// 0x2591
	{ MANUAL_ENTITY_BLK12,			9618,	"blk12",			NULL },	// 0x2592
	{ MANUAL_ENTITY_BLK34,			9619,	"blk34",			NULL },	// 0x2593

	/* Geometric Shapes */

	{ MANUAL_ENTITY_SQUARE,			9633,	"Square",			(char*[]) { "squ", "square", NULL } },	// 0x25a1
	{ MANUAL_ENTITY_FILLEDVERYSMALLSQUARE,	9642,	"FilledVerySmallSquare",	(char*[]) { "blacksquare", "squarf", "squf", NULL } },	// 0x25aa
	{ MANUAL_ENTITY_EMPTYVERYSMALLSQUARE,	9643,	"EmptyVerySmallSquare",		NULL },	// 0x25ab
	{ MANUAL_ENTITY_RECT,			9645,	"rect",				NULL },	// 0x25ad
	{ MANUAL_ENTITY_MARKER,			9646,	"marker",			NULL },	// 0x25ae
	{ MANUAL_ENTITY_FLTNS,			9649,	"fltns",			NULL },	// 0x25b1
	{ MANUAL_ENTITY_BIGTRIANGLEUP,		9651,	"bigtriangleup",		(char*[]) { "xutri", NULL } },	// 0x25b3
	{ MANUAL_ENTITY_BLACKTRIANGLE,		9652,	"blacktriangle",		(char*[]) { "utrif", NULL } },	// 0x25b4
	{ MANUAL_ENTITY_TRIANGLE,		9653,	"triangle",			(char*[]) { "utri", NULL } },	// 0x25b5
	{ MANUAL_ENTITY_BLACKTRIANGLERIGHT,	9656,	"blacktriangleright",		(char*[]) { "rtrif", NULL } },	// 0x25b8
	{ MANUAL_ENTITY_RTRI,			9657,	"rtri",				(char*[]) { "triangleright", NULL } },	// 0x25b9
	{ MANUAL_ENTITY_BIGTRIANGLEDOWN,	9661,	"bigtriangledown",		(char*[]) { "xdtri", NULL } },	// 0x25bd
	{ MANUAL_ENTITY_BLACKTRIANGLEDOWN,	9662,	"blacktriangledown",		(char*[]) { "dtrif", NULL } },	// 0x25be
	{ MANUAL_ENTITY_DTRI,			9663,	"dtri",				(char*[]) { "triangledown", NULL } },	// 0x25bf
	{ MANUAL_ENTITY_BLACKTRIANGLELEFT,	9666,	"blacktriangleleft",		(char*[]) { "ltrif", NULL } },	// 0x25c2
	{ MANUAL_ENTITY_LTRI,			9667,	"ltri",				(char*[]) { "triangleleft", NULL } },	// 0x25c3
	{ MANUAL_ENTITY_LOZ,			9674,	"loz",				(char*[]) { "lozenge", NULL } },	// 0x25ca
	{ MANUAL_ENTITY_CIR,			9675,	"cir",				NULL },	// 0x25cb
	{ MANUAL_ENTITY_TRIDOT,			9708,	"tridot",			NULL },	// 0x25ec
	{ MANUAL_ENTITY_BIGCIRC,		9711,	"bigcirc",			(char*[]) { "xcirc", NULL } },	// 0x25ef
	{ MANUAL_ENTITY_ULTRI,			9720,	"ultri",			NULL },	// 0x25f8
	{ MANUAL_ENTITY_URTRI,			9721,	"urtri",			NULL },	// 0x25f9
	{ MANUAL_ENTITY_LLTRI,			9722,	"lltri",			NULL },	// 0x25fa
	{ MANUAL_ENTITY_EMPTYSMALLSQUARE,	9723,	"EmptySmallSquare",		NULL },	// 0x25fb
	{ MANUAL_ENTITY_FILLEDSMALLSQUARE,	9724,	"FilledSmallSquare",		NULL },	// 0x25fc

	/* Miscellaneous Symbols */

	{ MANUAL_ENTITY_BIGSTAR,		9733,	"bigstar",			(char*[]) { "starf", NULL } },	// 0x2605
//	{ MANUAL_ENTITY_STAR,			9734,	"star",				NULL },	// 0x2606
	{ MANUAL_ENTITY_PHONE,			9742,	"phone",			NULL },	// 0x260e
	{ MANUAL_ENTITY_FEMALE,			9792,	"female",			NULL },	// 0x2640
	{ MANUAL_ENTITY_MALE,			9794,	"male",				NULL },	// 0x2642
	{ MANUAL_ENTITY_SPADES,			9824,	"spades",			(char*[]) { "spadesuit", NULL } },	// 0x2660
	{ MANUAL_ENTITY_CLUBS,			9827,	"clubs",			(char*[]) { "clubsuit", NULL } },	// 0x2663
	{ MANUAL_ENTITY_HEARTS,			9829,	"hearts",			(char*[]) { "heartsuit", NULL } },	// 0x2665
	{ MANUAL_ENTITY_DIAMS,			9830,	"diams",			(char*[]) { "diamondsuit", NULL } },	// 0x2666
	{ MANUAL_ENTITY_SUNG,			9834,	"sung",				NULL },	// 0x266a
	{ MANUAL_ENTITY_FLAT,			9837,	"flat",				NULL },	// 0x266d
	{ MANUAL_ENTITY_NATUR,			9838,	"natur",			(char*[]) { "natural", NULL } },	// 0x266e
	{ MANUAL_ENTITY_SHARP,			9839,	"sharp",			NULL },	// 0x266f

	/* Dingbats */

	{ MANUAL_ENTITY_CHECK,			10003,	"check",			(char*[]) { "checkmark", NULL } },	// 0x2713
//	{ MANUAL_ENTITY_CROSS,			10007,	"cross",			NULL },	// 0x2717
	{ MANUAL_ENTITY_MALT,			10016,	"malt",				(char*[]) { "maltese", NULL } },	// 0x2720
	{ MANUAL_ENTITY_SEXT,			10038,	"sext",				NULL },	// 0x2736
	{ MANUAL_ENTITY_VERTICALSEPARATOR,	10072,	"VerticalSeparator",		NULL },	// 0x2758
	{ MANUAL_ENTITY_LBBRK,			10098,	"lbbrk",			NULL },	// 0x2772
	{ MANUAL_ENTITY_RBBRK,			10099,	"rbbrk",			NULL },	// 0x2773

	/* Miscellaneous Mathematical Symbols-A */

	{ MANUAL_ENTITY_BSOLHSUB,		10184,	"bsolhsub",			NULL },	// 0x27c8
	{ MANUAL_ENTITY_SUPHSOL,		10185,	"suphsol",			NULL },	// 0x27c9
	{ MANUAL_ENTITY_LEFTDOUBLEBRACKET,	10214,	"LeftDoubleBracket",		(char*[]) { "lobrk", NULL } },	// 0x27e6
	{ MANUAL_ENTITY_RIGHTDOUBLEBRACKET,	10215,	"RightDoubleBracket",		(char*[]) { "robrk", NULL } },	// 0x27e7
	{ MANUAL_ENTITY_LEFTANGLEBRACKET,	10216,	"LeftAngleBracket",		(char*[]) { "lang", "langle", NULL } },	// 0x27e8
	{ MANUAL_ENTITY_RIGHTANGLEBRACKET,	10217,	"RightAngleBracket",		(char*[]) { "rang", "rangle", NULL } },	// 0x27e9
//	{ MANUAL_ENTITY_LANG,			10218,	"Lang",				NULL },	// 0x27ea
//	{ MANUAL_ENTITY_RANG,			10219,	"Rang",				NULL },	// 0x27eb
	{ MANUAL_ENTITY_LOANG,			10220,	"loang",			NULL },	// 0x27ec
	{ MANUAL_ENTITY_ROANG,			10221,	"roang",			NULL },	// 0x27ed

	/* Supplemental Arrows-A */

	{ MANUAL_ENTITY_LONGLEFTARROW,		10229,	"LongLeftArrow",		(char*[]) { "longleftarrow", "xlarr", NULL } },	// 0x27f5
	{ MANUAL_ENTITY_LONGRIGHTARROW,		10230,	"LongRightArrow",		(char*[]) { "longrightarrow", "xrarr", NULL } },	// 0x27f6
	{ MANUAL_ENTITY_LONGLEFTRIGHTARROW,	10231,	"LongLeftRightArrow",		(char*[]) { "longleftrightarrow", "xharr", NULL } },	// 0x27f7
	{ MANUAL_ENTITY_DOUBLELONGLEFTARROW,	10232,	"DoubleLongLeftArrow",		(char*[]) { "Longleftarrow", "xlArr", NULL } },	// 0x27f8
	{ MANUAL_ENTITY_DOUBLELONGRIGHTARROW,	10233,	"DoubleLongRightArrow",		(char*[]) { "Longrightarrow", "xrArr", NULL } },	// 0x27f9
	{ MANUAL_ENTITY_DOUBLELONGLEFTRIGHTARROW,	10234,	"DoubleLongLeftRightArrow",	(char*[]) { "Longleftrightarrow", "xhArr", NULL } },	// 0x27fa
	{ MANUAL_ENTITY_LONGMAPSTO,		10236,	"longmapsto",			(char*[]) { "xmap", NULL } },	// 0x27fc
	{ MANUAL_ENTITY_DZIGRARR,		10239,	"dzigrarr",			NULL },	// 0x27ff

	/* Supplemental Arrows-B */

	{ MANUAL_ENTITY_NVLARR,			10498,	"nvlArr",			NULL },	// 0x2902
	{ MANUAL_ENTITY_NVRARR,			10499,	"nvrArr",			NULL },	// 0x2903
	{ MANUAL_ENTITY_NVHARR,			10500,	"nvHarr",			NULL },	// 0x2904
	{ MANUAL_ENTITY_MAP,			10501,	"Map",				NULL },	// 0x2905
	{ MANUAL_ENTITY_LBARR,			10508,	"lbarr",			NULL },	// 0x290c
	{ MANUAL_ENTITY_BKAROW,			10509,	"bkarow",			(char*[]) { "rbarr", NULL } },	// 0x290d
	{ MANUAL_ENTITY_DLBARR,			10510,	"lBarr",			NULL },	// 0x290e
	{ MANUAL_ENTITY_DBKAROW,		10511,	"dbkarow",			(char*[]) { "rBarr", NULL } },	// 0x290f
	{ MANUAL_ENTITY_RBARR,			10512,	"drbkarow",			(char*[]) { "RBarr", NULL } },	// 0x2910
	{ MANUAL_ENTITY_DDOTRAHD,		10513,	"DDotrahd",			NULL },	// 0x2911
	{ MANUAL_ENTITY_UPARROWBAR,		10514,	"UpArrowBar",			NULL },	// 0x2912
	{ MANUAL_ENTITY_DOWNARROWBAR,		10515,	"DownArrowBar",			NULL },	// 0x2913
	{ MANUAL_ENTITY_RRARRTL,		10518,	"Rarrtl",			NULL },	// 0x2916
	{ MANUAL_ENTITY_LATAIL,			10521,	"latail",			NULL },	// 0x2919
	{ MANUAL_ENTITY_RATAIL,			10522,	"ratail",			NULL },	// 0x291a
	{ MANUAL_ENTITY_DLATAIL,		10523,	"lAtail",			NULL },	// 0x291b
	{ MANUAL_ENTITY_DRATAIL,		10524,	"rAtail",			NULL },	// 0x291c
	{ MANUAL_ENTITY_LARRFS,			10525,	"larrfs",			NULL },	// 0x291d
	{ MANUAL_ENTITY_RARRFS,			10526,	"rarrfs",			NULL },	// 0x291e
	{ MANUAL_ENTITY_LARRBFS,		10527,	"larrbfs",			NULL },	// 0x291f
	{ MANUAL_ENTITY_RARRBFS,		10528,	"rarrbfs",			NULL },	// 0x2920
	{ MANUAL_ENTITY_NWARHK,			10531,	"nwarhk",			NULL },	// 0x2923
	{ MANUAL_ENTITY_NEARHK,			10532,	"nearhk",			NULL },	// 0x2924
	{ MANUAL_ENTITY_HKSEAROW,		10533,	"hksearow",			(char*[]) { "searhk", NULL } },	// 0x2925
	{ MANUAL_ENTITY_HKSWAROW,		10534,	"hkswarow",			(char*[]) { "swarhk", NULL } },	// 0x2926
	{ MANUAL_ENTITY_NWNEAR,			10535,	"nwnear",			NULL },	// 0x2927
	{ MANUAL_ENTITY_NESEAR,			10536,	"nesear",			(char*[]) { "toea", NULL } },	// 0x2928
	{ MANUAL_ENTITY_SESWAR,			10537,	"seswar",			(char*[]) { "tosa", NULL } },	// 0x2929
	{ MANUAL_ENTITY_SWNWAR,			10538,	"swnwar",			NULL },	// 0x292a
	{ MANUAL_ENTITY_RARRC,			10547,	"rarrc",			NULL },	// 0x2933
	{ MANUAL_ENTITY_CUDARRR,		10549,	"cudarrr",			NULL },	// 0x2935
	{ MANUAL_ENTITY_LDCA,			10550,	"ldca",				NULL },	// 0x2936
	{ MANUAL_ENTITY_RDCA,			10551,	"rdca",				NULL },	// 0x2937
	{ MANUAL_ENTITY_CUDARRL,		10552,	"cudarrl",			NULL },	// 0x2938
	{ MANUAL_ENTITY_LARRPL,			10553,	"larrpl",			NULL },	// 0x2939
	{ MANUAL_ENTITY_CURARRM,		10556,	"curarrm",			NULL },	// 0x293c
	{ MANUAL_ENTITY_CULARRP,		10557,	"cularrp",			NULL },	// 0x293d
	{ MANUAL_ENTITY_RARRPL,			10565,	"rarrpl",			NULL },	// 0x2945
	{ MANUAL_ENTITY_HARRCIR,		10568,	"harrcir",			NULL },	// 0x2948
	{ MANUAL_ENTITY_UARROCIR,		10569,	"Uarrocir",			NULL },	// 0x2949
	{ MANUAL_ENTITY_LURDSHAR,		10570,	"lurdshar",			NULL },	// 0x294a
	{ MANUAL_ENTITY_LDRUSHAR,		10571,	"ldrushar",			NULL },	// 0x294b
	{ MANUAL_ENTITY_LEFTRIGHTVECTOR,	10574,	"LeftRightVector",		NULL },	// 0x294e
	{ MANUAL_ENTITY_RIGHTUPDOWNVECTOR,	10575,	"RightUpDownVector",		NULL },	// 0x294f
	{ MANUAL_ENTITY_DOWNLEFTRIGHTVECTOR,	10576,	"DownLeftRightVector",		NULL },	// 0x2950
	{ MANUAL_ENTITY_LEFTUPDOWNVECTOR,	10577,	"LeftUpDownVector",		NULL },	// 0x2951
	{ MANUAL_ENTITY_LEFTVECTORBAR,		10578,	"LeftVectorBar",		NULL },	// 0x2952
	{ MANUAL_ENTITY_RIGHTVECTORBAR,		10579,	"RightVectorBar",		NULL },	// 0x2953
	{ MANUAL_ENTITY_RIGHTUPVECTORBAR,	10580,	"RightUpVectorBar",		NULL },	// 0x2954
	{ MANUAL_ENTITY_RIGHTDOWNVECTORBAR,	10581,	"RightDownVectorBar",		NULL },	// 0x2955
	{ MANUAL_ENTITY_DOWNLEFTVECTORBAR,	10582,	"DownLeftVectorBar",		NULL },	// 0x2956
	{ MANUAL_ENTITY_DOWNRIGHTVECTORBAR,	10583,	"DownRightVectorBar",		NULL },	// 0x2957
	{ MANUAL_ENTITY_LEFTUPVECTORBAR,	10584,	"LeftUpVectorBar",		NULL },	// 0x2958
	{ MANUAL_ENTITY_LEFTDOWNVECTORBAR,	10585,	"LeftDownVectorBar",		NULL },	// 0x2959
	{ MANUAL_ENTITY_LEFTTEEVECTOR,		10586,	"LeftTeeVector",		NULL },	// 0x295a
	{ MANUAL_ENTITY_RIGHTTEEVECTOR,		10587,	"RightTeeVector",		NULL },	// 0x295b
	{ MANUAL_ENTITY_RIGHTUPTEEVECTOR,	10588,	"RightUpTeeVector",		NULL },	// 0x295c
	{ MANUAL_ENTITY_RIGHTDOWNTEEVECTOR,	10589,	"RightDownTeeVector",		NULL },	// 0x295d
	{ MANUAL_ENTITY_DOWNLEFTTEEVECTOR,	10590,	"DownLeftTeeVector",		NULL },	// 0x295e
	{ MANUAL_ENTITY_DOWNRIGHTTEEVECTOR,	10591,	"DownRightTeeVector",		NULL },	// 0x295f
	{ MANUAL_ENTITY_LEFTUPTEEVECTOR,	10592,	"LeftUpTeeVector",		NULL },	// 0x2960
	{ MANUAL_ENTITY_LEFTDOWNTEEVECTOR,	10593,	"LeftDownTeeVector",		NULL },	// 0x2961
	{ MANUAL_ENTITY_LHAR,			10594,	"lHar",				NULL },	// 0x2962
	{ MANUAL_ENTITY_UHAR,			10595,	"uHar",				NULL },	// 0x2963
	{ MANUAL_ENTITY_RHAR,			10596,	"rHar",				NULL },	// 0x2964
	{ MANUAL_ENTITY_DHAR,			10597,	"dHar",				NULL },	// 0x2965
	{ MANUAL_ENTITY_LURUHAR,		10598,	"luruhar",			NULL },	// 0x2966
	{ MANUAL_ENTITY_LDRDHAR,		10599,	"ldrdhar",			NULL },	// 0x2967
	{ MANUAL_ENTITY_RULUHAR,		10600,	"ruluhar",			NULL },	// 0x2968
	{ MANUAL_ENTITY_RDLDHAR,		10601,	"rdldhar",			NULL },	// 0x2969
	{ MANUAL_ENTITY_LHARUL,			10602,	"lharul",			NULL },	// 0x296a
	{ MANUAL_ENTITY_LLHARD,			10603,	"llhard",			NULL },	// 0x296b
	{ MANUAL_ENTITY_RHARUL,			10604,	"rharul",			NULL },	// 0x296c
	{ MANUAL_ENTITY_LRHARD,			10605,	"lrhard",			NULL },	// 0x296d
	{ MANUAL_ENTITY_UPEQUILIBRIUM,		10606,	"udhar",			(char*[]) { "UpEquilibrium", NULL } },	// 0x296e
	{ MANUAL_ENTITY_REVERSEUPEQUILIBRIUM,	10607,	"duhar",			(char*[]) { "ReverseUpEquilibrium", NULL } },	// 0x296f
	{ MANUAL_ENTITY_ROUNDIMPLIES,		10608,	"RoundImplies",			NULL },	// 0x2970
	{ MANUAL_ENTITY_ERARR,			10609,	"erarr",			NULL },	// 0x2971
	{ MANUAL_ENTITY_SIMRARR,		10610,	"simrarr",			NULL },	// 0x2972
	{ MANUAL_ENTITY_LARRSIM,		10611,	"larrsim",			NULL },	// 0x2973
	{ MANUAL_ENTITY_RARRSIM,		10612,	"rarrsim",			NULL },	// 0x2974
	{ MANUAL_ENTITY_RARRAP,			10613,	"rarrap",			NULL },	// 0x2975
	{ MANUAL_ENTITY_LTLARR,			10614,	"ltlarr",			NULL },	// 0x2976
	{ MANUAL_ENTITY_GTRARR,			10616,	"gtrarr",			NULL },	// 0x2978
	{ MANUAL_ENTITY_SUBRARR,		10617,	"subrarr",			NULL },	// 0x2979
	{ MANUAL_ENTITY_SUPLARR,		10619,	"suplarr",			NULL },	// 0x297b
	{ MANUAL_ENTITY_LFISHT,			10620,	"lfisht",			NULL },	// 0x297c
	{ MANUAL_ENTITY_RFISHT,			10621,	"rfisht",			NULL },	// 0x297d
	{ MANUAL_ENTITY_UFISHT,			10622,	"ufisht",			NULL },	// 0x297e
	{ MANUAL_ENTITY_DFISHT,			10623,	"dfisht",			NULL },	// 0x297f

	/* Miscellaneous Mathematical Symbols-B */

	{ MANUAL_ENTITY_LOPAR,			10629,	"lopar",			NULL },	// 0x2985
	{ MANUAL_ENTITY_ROPAR,			10630,	"ropar",			NULL },	// 0x2986
	{ MANUAL_ENTITY_LBRKE,			10635,	"lbrke",			NULL },	// 0x298b
	{ MANUAL_ENTITY_RBRKE,			10636,	"rbrke",			NULL },	// 0x298c
	{ MANUAL_ENTITY_LBRKSLU,		10637,	"lbrkslu",			NULL },	// 0x298d
	{ MANUAL_ENTITY_RBRKSLD,		10638,	"rbrksld",			NULL },	// 0x298e
	{ MANUAL_ENTITY_LBRKSLD,		10639,	"lbrksld",			NULL },	// 0x298f
	{ MANUAL_ENTITY_RBRKSLU,		10640,	"rbrkslu",			NULL },	// 0x2990
	{ MANUAL_ENTITY_LANGD,			10641,	"langd",			NULL },	// 0x2991
	{ MANUAL_ENTITY_RANGD,			10642,	"rangd",			NULL },	// 0x2992
	{ MANUAL_ENTITY_LPARLT,			10643,	"lparlt",			NULL },	// 0x2993
	{ MANUAL_ENTITY_RPARGT,			10644,	"rpargt",			NULL },	// 0x2994
	{ MANUAL_ENTITY_GTLPAR,			10645,	"gtlPar",			NULL },	// 0x2995
	{ MANUAL_ENTITY_LTRPAR,			10646,	"ltrPar",			NULL },	// 0x2996
	{ MANUAL_ENTITY_VZIGZAG,		10650,	"vzigzag",			NULL },	// 0x299a
	{ MANUAL_ENTITY_VANGRT,			10652,	"vangrt",			NULL },	// 0x299c
	{ MANUAL_ENTITY_ANGRTVBD,		10653,	"angrtvbd",			NULL },	// 0x299d
	{ MANUAL_ENTITY_ANGE,			10660,	"ange",				NULL },	// 0x29a4
	{ MANUAL_ENTITY_RANGE,			10661,	"range",			NULL },	// 0x29a5
	{ MANUAL_ENTITY_DWANGLE,		10662,	"dwangle",			NULL },	// 0x29a6
	{ MANUAL_ENTITY_UWANGLE,		10663,	"uwangle",			NULL },	// 0x29a7
	{ MANUAL_ENTITY_ANGMSDAA,		10664,	"angmsdaa",			NULL },	// 0x29a8
	{ MANUAL_ENTITY_ANGMSDAB,		10665,	"angmsdab",			NULL },	// 0x29a9
	{ MANUAL_ENTITY_ANGMSDAC,		10666,	"angmsdac",			NULL },	// 0x29aa
	{ MANUAL_ENTITY_ANGMSDAD,		10667,	"angmsdad",			NULL },	// 0x29ab
	{ MANUAL_ENTITY_ANGMSDAE,		10668,	"angmsdae",			NULL },	// 0x29ac
	{ MANUAL_ENTITY_ANGMSDAF,		10669,	"angmsdaf",			NULL },	// 0x29ad
	{ MANUAL_ENTITY_ANGMSDAG,		10670,	"angmsdag",			NULL },	// 0x29ae
	{ MANUAL_ENTITY_ANGMSDAH,		10671,	"angmsdah",			NULL },	// 0x29af
	{ MANUAL_ENTITY_BEMPTYV,		10672,	"bemptyv",			NULL },	// 0x29b0
	{ MANUAL_ENTITY_DEMPTYV,		10673,	"demptyv",			NULL },	// 0x29b1
	{ MANUAL_ENTITY_CEMPTYV,		10674,	"cemptyv",			NULL },	// 0x29b2
	{ MANUAL_ENTITY_RAEMPTYV,		10675,	"raemptyv",			NULL },	// 0x29b3
	{ MANUAL_ENTITY_LAEMPTYV,		10676,	"laemptyv",			NULL },	// 0x29b4
	{ MANUAL_ENTITY_OHBAR,			10677,	"ohbar",			NULL },	// 0x29b5
	{ MANUAL_ENTITY_OMID,			10678,	"omid",				NULL },	// 0x29b6
	{ MANUAL_ENTITY_OPAR,			10679,	"opar",				NULL },	// 0x29b7
	{ MANUAL_ENTITY_OPERP,			10681,	"operp",			NULL },	// 0x29b9
	{ MANUAL_ENTITY_OLCROSS,		10683,	"olcross",			NULL },	// 0x29bb
	{ MANUAL_ENTITY_ODSOLD,			10684,	"odsold",			NULL },	// 0x29bc
	{ MANUAL_ENTITY_OLCIR,			10686,	"olcir",			NULL },	// 0x29be
	{ MANUAL_ENTITY_OFCIR,			10687,	"ofcir",			NULL },	// 0x29bf
	{ MANUAL_ENTITY_OLT,			10688,	"olt",				NULL },	// 0x29c0
	{ MANUAL_ENTITY_OGT,			10689,	"ogt",				NULL },	// 0x29c1
	{ MANUAL_ENTITY_CIRSCIR,		10690,	"cirscir",			NULL },	// 0x29c2
	{ MANUAL_ENTITY_CIRE,			10691,	"cirE",				NULL },	// 0x29c3
	{ MANUAL_ENTITY_SOLB,			10692,	"solb",				NULL },	// 0x29c4
	{ MANUAL_ENTITY_BSOLB,			10693,	"bsolb",			NULL },	// 0x29c5
	{ MANUAL_ENTITY_BOXBOX,			10697,	"boxbox",			NULL },	// 0x29c9
	{ MANUAL_ENTITY_TRISB,			10701,	"trisb",			NULL },	// 0x29cd
	{ MANUAL_ENTITY_RTRILTRI,		10702,	"rtriltri",			NULL },	// 0x29ce
	{ MANUAL_ENTITY_LEFTTRIANGLEBAR,	10703,	"LeftTriangleBar",		NULL },	// 0x29cf
	{ MANUAL_ENTITY_RIGHTTRIANGLEBAR,	10704,	"RightTriangleBar",		NULL },	// 0x29d0
	{ MANUAL_ENTITY_IINFIN,			10716,	"iinfin",			NULL },	// 0x29dc
	{ MANUAL_ENTITY_INFINTIE,		10717,	"infintie",			NULL },	// 0x29dd
	{ MANUAL_ENTITY_NVINFIN,		10718,	"nvinfin",			NULL },	// 0x29de
	{ MANUAL_ENTITY_EPARSL,			10723,	"eparsl",			NULL },	// 0x29e3
	{ MANUAL_ENTITY_SMEPARSL,		10724,	"smeparsl",			NULL },	// 0x29e4
	{ MANUAL_ENTITY_EQVPARSL,		10725,	"eqvparsl",			NULL },	// 0x29e5
	{ MANUAL_ENTITY_BLACKLOZENGE,		10731,	"blacklozenge",			(char*[]) { "lozf", NULL } },	// 0x29eb
	{ MANUAL_ENTITY_RULEDELAYED,		10740,	"RuleDelayed",			NULL },	// 0x29f4
	{ MANUAL_ENTITY_DSOL,			10742,	"dsol",				NULL },	// 0x29f6

	/* Supplemental Mathematical Operators */

	{ MANUAL_ENTITY_BIGODOT,		10752,	"bigodot",			(char*[]) { "xodot", NULL } },	// 0x2a00
	{ MANUAL_ENTITY_BIGOPLUS,		10753,	"bigoplus",			(char*[]) { "xoplus", NULL } },	// 0x2a01
	{ MANUAL_ENTITY_BIGOTIMES,		10754,	"bigotimes",			(char*[]) { "xotime", NULL } },	// 0x2a02
	{ MANUAL_ENTITY_BIGUPLUS,		10756,	"biguplus",			(char*[]) { "xuplus", NULL } },	// 0x2a04
	{ MANUAL_ENTITY_BIGSQCUP,		10758,	"bigsqcup",			(char*[]) { "xsqcup", NULL } },	// 0x2a06
	{ MANUAL_ENTITY_IIIINT,			10764,	"iiiint",			(char*[]) { "qint", NULL } },	// 0x2a0c
	{ MANUAL_ENTITY_FPARTINT,		10765,	"fpartint",			NULL },	// 0x2a0d
	{ MANUAL_ENTITY_CIRFNINT,		10768,	"cirfnint",			NULL },	// 0x2a10
	{ MANUAL_ENTITY_AWINT,			10769,	"awint",			NULL },	// 0x2a11
	{ MANUAL_ENTITY_RPPOLINT,		10770,	"rppolint",			NULL },	// 0x2a12
	{ MANUAL_ENTITY_SCPOLINT,		10771,	"scpolint",			NULL },	// 0x2a13
	{ MANUAL_ENTITY_NPOLINT,		10772,	"npolint",			NULL },	// 0x2a14
	{ MANUAL_ENTITY_POINTINT,		10773,	"pointint",			NULL },	// 0x2a15
	{ MANUAL_ENTITY_QUATINT,		10774,	"quatint",			NULL },	// 0x2a16
	{ MANUAL_ENTITY_INTLARHK,		10775,	"intlarhk",			NULL },	// 0x2a17
	{ MANUAL_ENTITY_PLUSCIR,		10786,	"pluscir",			NULL },	// 0x2a22
	{ MANUAL_ENTITY_PLUSACIR,		10787,	"plusacir",			NULL },	// 0x2a23
	{ MANUAL_ENTITY_SIMPLUS,		10788,	"simplus",			NULL },	// 0x2a24
	{ MANUAL_ENTITY_PLUSDU,			10789,	"plusdu",			NULL },	// 0x2a25
	{ MANUAL_ENTITY_PLUSSIM,		10790,	"plussim",			NULL },	// 0x2a26
	{ MANUAL_ENTITY_PLUSTWO,		10791,	"plustwo",			NULL },	// 0x2a27
	{ MANUAL_ENTITY_MCOMMA,			10793,	"mcomma",			NULL },	// 0x2a29
	{ MANUAL_ENTITY_MINUSDU,		10794,	"minusdu",			NULL },	// 0x2a2a
	{ MANUAL_ENTITY_LOPLUS,			10797,	"loplus",			NULL },	// 0x2a2d
	{ MANUAL_ENTITY_ROPLUS,			10798,	"roplus",			NULL },	// 0x2a2e
//	{ MANUAL_ENTITY_CROSS,			10799,	"Cross",			NULL },	// 0x2a2f
	{ MANUAL_ENTITY_TIMESD,			10800,	"timesd",			NULL },	// 0x2a30
	{ MANUAL_ENTITY_TIMESBAR,		10801,	"timesbar",			NULL },	// 0x2a31
	{ MANUAL_ENTITY_SMASHP,			10803,	"smashp",			NULL },	// 0x2a33
	{ MANUAL_ENTITY_LOTIMES,		10804,	"lotimes",			NULL },	// 0x2a34
	{ MANUAL_ENTITY_ROTIMES,		10805,	"rotimes",			NULL },	// 0x2a35
	{ MANUAL_ENTITY_OTIMESAS,		10806,	"otimesas",			NULL },	// 0x2a36
//	{ MANUAL_ENTITY_OTIMES,			10807,	"Otimes",			NULL },	// 0x2a37
	{ MANUAL_ENTITY_ODIV,			10808,	"odiv",				NULL },	// 0x2a38
	{ MANUAL_ENTITY_TRIPLUS,		10809,	"triplus",			NULL },	// 0x2a39
	{ MANUAL_ENTITY_TRIMINUS,		10810,	"triminus",			NULL },	// 0x2a3a
	{ MANUAL_ENTITY_TRITIME,		10811,	"tritime",			NULL },	// 0x2a3b
	{ MANUAL_ENTITY_INTPROD,		10812,	"intprod",			(char*[]) { "iprod", NULL } },	// 0x2a3c
	{ MANUAL_ENTITY_AMALG,			10815,	"amalg",			NULL },	// 0x2a3f
	{ MANUAL_ENTITY_CAPDOT,			10816,	"capdot",			NULL },	// 0x2a40
	{ MANUAL_ENTITY_NCUP,			10818,	"ncup",				NULL },	// 0x2a42
	{ MANUAL_ENTITY_NCAP,			10819,	"ncap",				NULL },	// 0x2a43
	{ MANUAL_ENTITY_CAPAND,			10820,	"capand",			NULL },	// 0x2a44
	{ MANUAL_ENTITY_CUPOR,			10821,	"cupor",			NULL },	// 0x2a45
//	{ MANUAL_ENTITY_CUPCAP,			10822,	"cupcap",			NULL },	// 0x2a46
	{ MANUAL_ENTITY_CAPCUP,			10823,	"capcup",			NULL },	// 0x2a47
	{ MANUAL_ENTITY_CUPBRCAP,		10824,	"cupbrcap",			NULL },	// 0x2a48
	{ MANUAL_ENTITY_CAPBRCUP,		10825,	"capbrcup",			NULL },	// 0x2a49
	{ MANUAL_ENTITY_CUPCUP,			10826,	"cupcup",			NULL },	// 0x2a4a
	{ MANUAL_ENTITY_CAPCAP,			10827,	"capcap",			NULL },	// 0x2a4b
	{ MANUAL_ENTITY_CCUPS,			10828,	"ccups",			NULL },	// 0x2a4c
	{ MANUAL_ENTITY_CCAPS,			10829,	"ccaps",			NULL },	// 0x2a4d
	{ MANUAL_ENTITY_CCUPSSM,		10832,	"ccupssm",			NULL },	// 0x2a50
//	{ MANUAL_ENTITY_AND,			10835,	"And",				NULL },	// 0x2a53
//	{ MANUAL_ENTITY_OR,			10836,	"Or",				NULL },	// 0x2a54
	{ MANUAL_ENTITY_ANDAND,			10837,	"andand",			NULL },	// 0x2a55
	{ MANUAL_ENTITY_OROR,			10838,	"oror",				NULL },	// 0x2a56
	{ MANUAL_ENTITY_ORSLOPE,		10839,	"orslope",			NULL },	// 0x2a57
	{ MANUAL_ENTITY_ANDSLOPE,		10840,	"andslope",			NULL },	// 0x2a58
	{ MANUAL_ENTITY_ANDV,			10842,	"andv",				NULL },	// 0x2a5a
	{ MANUAL_ENTITY_ORV,			10843,	"orv",				NULL },	// 0x2a5b
	{ MANUAL_ENTITY_ANDD,			10844,	"andd",				NULL },	// 0x2a5c
	{ MANUAL_ENTITY_ORD,			10845,	"ord",				NULL },	// 0x2a5d
	{ MANUAL_ENTITY_WEDBAR,			10847,	"wedbar",			NULL },	// 0x2a5f
	{ MANUAL_ENTITY_SDOTE,			10854,	"sdote",			NULL },	// 0x2a66
	{ MANUAL_ENTITY_SIMDOT,			10858,	"simdot",			NULL },	// 0x2a6a
	{ MANUAL_ENTITY_CONGDOT,		10861,	"congdot",			NULL },	// 0x2a6d
	{ MANUAL_ENTITY_EASTER,			10862,	"easter",			NULL },	// 0x2a6e
	{ MANUAL_ENTITY_APACIR,			10863,	"apacir",			NULL },	// 0x2a6f
//	{ MANUAL_ENTITY_APE,			10864,	"apE",				NULL },	// 0x2a70
	{ MANUAL_ENTITY_EPLUS,			10865,	"eplus",			NULL },	// 0x2a71
	{ MANUAL_ENTITY_PLUSE,			10866,	"pluse",			NULL },	// 0x2a72
	{ MANUAL_ENTITY_ESIM,			10867,	"Esim",				NULL },	// 0x2a73
	{ MANUAL_ENTITY_COLONE,			10868,	"Colone",			NULL },	// 0x2a74
	{ MANUAL_ENTITY_EQUAL,			10869,	"Equal",			NULL },	// 0x2a75
	{ MANUAL_ENTITY_DDOTSEQ,		10871,	"ddotseq",			(char*[]) { "eDDot", NULL } },	// 0x2a77
	{ MANUAL_ENTITY_EQUIVDD,		10872,	"equivDD",			NULL },	// 0x2a78
	{ MANUAL_ENTITY_LTCIR,			10873,	"ltcir",			NULL },	// 0x2a79
	{ MANUAL_ENTITY_GTCIR,			10874,	"gtcir",			NULL },	// 0x2a7a
	{ MANUAL_ENTITY_LTQUEST,		10875,	"ltquest",			NULL },	// 0x2a7b
	{ MANUAL_ENTITY_GTQUEST,		10876,	"gtquest",			NULL },	// 0x2a7c
	{ MANUAL_ENTITY_LESSSLANTEQUAL,		10877,	"LessSlantEqual",		(char*[]) { "leqslant", "les", NULL } },	// 0x2a7d
	{ MANUAL_ENTITY_GREATERSLANTEQUAL,	10878,	"GreaterSlantEqual",		(char*[]) { "geqslant", "ges", NULL } },	// 0x2a7e
	{ MANUAL_ENTITY_LESDOT,			10879,	"lesdot",			NULL },	// 0x2a7f
	{ MANUAL_ENTITY_GESDOT,			10880,	"gesdot",			NULL },	// 0x2a80
	{ MANUAL_ENTITY_LESDOTO,		10881,	"lesdoto",			NULL },	// 0x2a81
	{ MANUAL_ENTITY_GESDOTO,		10882,	"gesdoto",			NULL },	// 0x2a82
	{ MANUAL_ENTITY_LESDOTOR,		10883,	"lesdotor",			NULL },	// 0x2a83
	{ MANUAL_ENTITY_GESDOTOL,		10884,	"gesdotol",			NULL },	// 0x2a84
	{ MANUAL_ENTITY_LAP,			10885,	"lap",				(char*[]) { "lessapprox", NULL } },	// 0x2a85
	{ MANUAL_ENTITY_GAP,			10886,	"gap",				(char*[]) { "gtrapprox", NULL } },	// 0x2a86
//	{ MANUAL_ENTITY_LNE,			10887,	"lne",				(char*[]) { "lneq", NULL } },	// 0x2a87
//	{ MANUAL_ENTITY_GNE,			10888,	"gne",				(char*[]) { "gneq", NULL } },	// 0x2a88
	{ MANUAL_ENTITY_LNAP,			10889,	"lnap",				(char*[]) { "lnapprox", NULL } },	// 0x2a89
	{ MANUAL_ENTITY_GNAP,			10890,	"gnap",				(char*[]) { "gnapprox", NULL } },	// 0x2a8a
	{ MANUAL_ENTITY_LEG,			10891,	"lEg",				(char*[]) { "lesseqqgtr", NULL } },	// 0x2a8b
	{ MANUAL_ENTITY_GEL,			10892,	"gEl",				(char*[]) { "gtreqqless", NULL } },	// 0x2a8c
	{ MANUAL_ENTITY_LSIME,			10893,	"lsime",			NULL },	// 0x2a8d
	{ MANUAL_ENTITY_GSIME,			10894,	"gsime",			NULL },	// 0x2a8e
	{ MANUAL_ENTITY_LSIMG,			10895,	"lsimg",			NULL },	// 0x2a8f
	{ MANUAL_ENTITY_GSIML,			10896,	"gsiml",			NULL },	// 0x2a90
	{ MANUAL_ENTITY_LGE,			10897,	"lgE",				NULL },	// 0x2a91
	{ MANUAL_ENTITY_GLE,			10898,	"glE",				NULL },	// 0x2a92
	{ MANUAL_ENTITY_LESGES,			10899,	"lesges",			NULL },	// 0x2a93
	{ MANUAL_ENTITY_GESLES,			10900,	"gesles",			NULL },	// 0x2a94
	{ MANUAL_ENTITY_ELS,			10901,	"els",				(char*[]) { "eqslantless", NULL } },	// 0x2a95
	{ MANUAL_ENTITY_EGS,			10902,	"egs",				(char*[]) { "eqslantgtr", NULL } },	// 0x2a96
	{ MANUAL_ENTITY_ELSDOT,			10903,	"elsdot",			NULL },	// 0x2a97
	{ MANUAL_ENTITY_EGSDOT,			10904,	"egsdot",			NULL },	// 0x2a98
	{ MANUAL_ENTITY_EL,			10905,	"el",				NULL },	// 0x2a99
	{ MANUAL_ENTITY_EG,			10906,	"eg",				NULL },	// 0x2a9a
	{ MANUAL_ENTITY_SIML,			10909,	"siml",				NULL },	// 0x2a9d
	{ MANUAL_ENTITY_SIMG,			10910,	"simg",				NULL },	// 0x2a9e
	{ MANUAL_ENTITY_SIMLE,			10911,	"simlE",			NULL },	// 0x2a9f
	{ MANUAL_ENTITY_SIMGE,			10912,	"simgE",			NULL },	// 0x2aa0
	{ MANUAL_ENTITY_LESSLESS,		10913,	"LessLess",			NULL },	// 0x2aa1
	{ MANUAL_ENTITY_GREATERGREATER,		10914,	"GreaterGreater",		NULL },	// 0x2aa2
	{ MANUAL_ENTITY_GLJ,			10916,	"glj",				NULL },	// 0x2aa4
	{ MANUAL_ENTITY_GLA,			10917,	"gla",				NULL },	// 0x2aa5
	{ MANUAL_ENTITY_LTCC,			10918,	"ltcc",				NULL },	// 0x2aa6
	{ MANUAL_ENTITY_GTCC,			10919,	"gtcc",				NULL },	// 0x2aa7
	{ MANUAL_ENTITY_LESCC,			10920,	"lescc",			NULL },	// 0x2aa8
	{ MANUAL_ENTITY_GESCC,			10921,	"gescc",			NULL },	// 0x2aa9
	{ MANUAL_ENTITY_SMT,			10922,	"smt",				NULL },	// 0x2aaa
	{ MANUAL_ENTITY_LAT,			10923,	"lat",				NULL },	// 0x2aab
	{ MANUAL_ENTITY_SMTE,			10924,	"smte",				NULL },	// 0x2aac
	{ MANUAL_ENTITY_LATE,			10925,	"late",				NULL },	// 0x2aad
	{ MANUAL_ENTITY_BUMPE,			10926,	"bumpE",			NULL },	// 0x2aae
	{ MANUAL_ENTITY_PRECEDESEQUAL,		10927,	"PrecedesEqual",		(char*[]) { "pre", "preceq", NULL } },	// 0x2aaf
	{ MANUAL_ENTITY_SUCCEEDSEQUAL,		10928,	"SucceedsEqual",		(char*[]) { "sce", "succeq", NULL } },	// 0x2ab0
	{ MANUAL_ENTITY_PRE,			10931,	"prE",				NULL },	// 0x2ab3
	{ MANUAL_ENTITY_SCE,			10932,	"scE",				NULL },	// 0x2ab4
	{ MANUAL_ENTITY_PRECNEQQ,		10933,	"precneqq",			(char*[]) { "prnE", NULL } },	// 0x2ab5
	{ MANUAL_ENTITY_SCNE,			10934,	"scnE",				(char*[]) { "succneqq", NULL } },	// 0x2ab6
	{ MANUAL_ENTITY_PRAP,			10935,	"prap",				(char*[]) { "precapprox", NULL } },	// 0x2ab7
	{ MANUAL_ENTITY_SCAP,			10936,	"scap",				(char*[]) { "succapprox", NULL } },	// 0x2ab8
	{ MANUAL_ENTITY_PRECNAPPROX,		10937,	"precnapprox",			(char*[]) { "prnap", NULL } },	// 0x2ab9
	{ MANUAL_ENTITY_SCNAP,			10938,	"scnap",			(char*[]) { "succnapprox", NULL } },	// 0x2aba
	{ MANUAL_ENTITY_PR,			10939,	"Pr",				NULL },	// 0x2abb
	{ MANUAL_ENTITY_SC,			10940,	"Sc",				NULL },	// 0x2abc
	{ MANUAL_ENTITY_SUBDOT,			10941,	"subdot",			NULL },	// 0x2abd
	{ MANUAL_ENTITY_SUPDOT,			10942,	"supdot",			NULL },	// 0x2abe
	{ MANUAL_ENTITY_SUBPLUS,		10943,	"subplus",			NULL },	// 0x2abf
	{ MANUAL_ENTITY_SUPPLUS,		10944,	"supplus",			NULL },	// 0x2ac0
	{ MANUAL_ENTITY_SUBMULT,		10945,	"submult",			NULL },	// 0x2ac1
	{ MANUAL_ENTITY_SUPMULT,		10946,	"supmult",			NULL },	// 0x2ac2
	{ MANUAL_ENTITY_SUBEDOT,		10947,	"subedot",			NULL },	// 0x2ac3
	{ MANUAL_ENTITY_SUPEDOT,		10948,	"supedot",			NULL },	// 0x2ac4
//	{ MANUAL_ENTITY_SUBE,			10949,	"subE",				(char*[]) { "subseteqq", NULL } },	// 0x2ac5
//	{ MANUAL_ENTITY_SUPE,			10950,	"supE",				(char*[]) { "supseteqq", NULL } },	// 0x2ac6
	{ MANUAL_ENTITY_SUBSIM,			10951,	"subsim",			NULL },	// 0x2ac7
	{ MANUAL_ENTITY_SUPSIM,			10952,	"supsim",			NULL },	// 0x2ac8
//	{ MANUAL_ENTITY_SUBNE,			10955,	"subnE",			(char*[]) { "subsetneqq", NULL } },	// 0x2acb
//	{ MANUAL_ENTITY_SUPNE,			10956,	"supnE",			(char*[]) { "supsetneqq", NULL } },	// 0x2acc
	{ MANUAL_ENTITY_CSUB,			10959,	"csub",				NULL },	// 0x2acf
	{ MANUAL_ENTITY_CSUP,			10960,	"csup",				NULL },	// 0x2ad0
	{ MANUAL_ENTITY_CSUBE,			10961,	"csube",			NULL },	// 0x2ad1
	{ MANUAL_ENTITY_CSUPE,			10962,	"csupe",			NULL },	// 0x2ad2
	{ MANUAL_ENTITY_SUBSUP,			10963,	"subsup",			NULL },	// 0x2ad3
	{ MANUAL_ENTITY_SUPSUB,			10964,	"supsub",			NULL },	// 0x2ad4
	{ MANUAL_ENTITY_SUBSUB,			10965,	"subsub",			NULL },	// 0x2ad5
	{ MANUAL_ENTITY_SUPSUP,			10966,	"supsup",			NULL },	// 0x2ad6
	{ MANUAL_ENTITY_SUPHSUB,		10967,	"suphsub",			NULL },	// 0x2ad7
	{ MANUAL_ENTITY_SUPDSUB,		10968,	"supdsub",			NULL },	// 0x2ad8
	{ MANUAL_ENTITY_FORKV,			10969,	"forkv",			NULL },	// 0x2ad9
	{ MANUAL_ENTITY_TOPFORK,		10970,	"topfork",			NULL },	// 0x2ada
	{ MANUAL_ENTITY_MLCP,			10971,	"mlcp",				NULL },	// 0x2adb
	{ MANUAL_ENTITY_DASHV,			10980,	"Dashv",			(char*[]) { "DoubleLeftTee", NULL } },	// 0x2ae4
	{ MANUAL_ENTITY_VDASHL,			10982,	"Vdashl",			NULL },	// 0x2ae6
	{ MANUAL_ENTITY_BARV,			10983,	"Barv",				NULL },	// 0x2ae7
//	{ MANUAL_ENTITY_VBAR,			10984,	"vBar",				NULL },	// 0x2ae8
	{ MANUAL_ENTITY_VBARV,			10985,	"vBarv",			NULL },	// 0x2ae9
//	{ MANUAL_ENTITY_VBAR,			10987,	"Vbar",				NULL },	// 0x2aeb
//	{ MANUAL_ENTITY_NOT,			10988,	"Not",				NULL },	// 0x2aec
//	{ MANUAL_ENTITY_BNOT,			10989,	"bNot",				NULL },	// 0x2aed
	{ MANUAL_ENTITY_RNMID,			10990,	"rnmid",			NULL },	// 0x2aee
	{ MANUAL_ENTITY_CIRMID,			10991,	"cirmid",			NULL },	// 0x2aef
	{ MANUAL_ENTITY_MIDCIR,			10992,	"midcir",			NULL },	// 0x2af0
	{ MANUAL_ENTITY_TOPCIR,			10993,	"topcir",			NULL },	// 0x2af1
	{ MANUAL_ENTITY_NHPAR,			10994,	"nhpar",			NULL },	// 0x2af2
	{ MANUAL_ENTITY_PARSIM,			10995,	"parsim",			NULL },	// 0x2af3
	{ MANUAL_ENTITY_PARSL,			11005,	"parsl",			NULL },	// 0x2afd

	/* Alphabetic Presentation Forms */

	{ MANUAL_ENTITY_FFLIG_L,		64256,	"fflig",			NULL },	// 0xfb00
	{ MANUAL_ENTITY_FILIG_L,		64257,	"filig",			NULL },	// 0xfb01
	{ MANUAL_ENTITY_FLLIG_L,		64258,	"fllig",			NULL },	// 0xfb02
	{ MANUAL_ENTITY_FFILIG_L,		64259,	"ffilig",			NULL },	// 0xfb03
	{ MANUAL_ENTITY_FFLLIG_L,		64260,	"ffllig",			NULL },	// 0xfb04

	/* Mathematical Alphanumeric Symbols */

	{ MANUAL_ENTITY_ASCR_U,			119964,	"Ascr",				NULL },	// 0x1d49c
	{ MANUAL_ENTITY_CSCR_U,			119966,	"Cscr",				NULL },	// 0x1d49e
	{ MANUAL_ENTITY_DSCR_U,			119967,	"Dscr",				NULL },	// 0x1d49f
	{ MANUAL_ENTITY_GSCR_U,			119970,	"Gscr",				NULL },	// 0x1d4a2
	{ MANUAL_ENTITY_JSCR_U,			119973,	"Jscr",				NULL },	// 0x1d4a5
	{ MANUAL_ENTITY_KSCR_U,			119974,	"Kscr",				NULL },	// 0x1d4a6
	{ MANUAL_ENTITY_NSCR_U,			119977,	"Nscr",				NULL },	// 0x1d4a9
	{ MANUAL_ENTITY_OSCR_U,			119978,	"Oscr",				NULL },	// 0x1d4aa
	{ MANUAL_ENTITY_PSCR_U,			119979,	"Pscr",				NULL },	// 0x1d4ab
	{ MANUAL_ENTITY_QSCR_U,			119980,	"Qscr",				NULL },	// 0x1d4ac
	{ MANUAL_ENTITY_SSCR_U,			119982,	"Sscr",				NULL },	// 0x1d4ae
	{ MANUAL_ENTITY_TSCR_U,			119983,	"Tscr",				NULL },	// 0x1d4af
	{ MANUAL_ENTITY_USCR_U,			119984,	"Uscr",				NULL },	// 0x1d4b0
	{ MANUAL_ENTITY_VSCR_U,			119985,	"Vscr",				NULL },	// 0x1d4b1
	{ MANUAL_ENTITY_WSCR_U,			119986,	"Wscr",				NULL },	// 0x1d4b2
	{ MANUAL_ENTITY_XSCR_U,			119987,	"Xscr",				NULL },	// 0x1d4b3
	{ MANUAL_ENTITY_YSCR_U,			119988,	"Yscr",				NULL },	// 0x1d4b4
	{ MANUAL_ENTITY_ZSCR_U,			119989,	"Zscr",				NULL },	// 0x1d4b5
	{ MANUAL_ENTITY_ASCR_L,			119990,	"ascr",				NULL },	// 0x1d4b6
	{ MANUAL_ENTITY_BSCR_L,			119991,	"bscr",				NULL },	// 0x1d4b7
	{ MANUAL_ENTITY_CSCR_L,			119992,	"cscr",				NULL },	// 0x1d4b8
	{ MANUAL_ENTITY_DSCR_L,			119993,	"dscr",				NULL },	// 0x1d4b9
	{ MANUAL_ENTITY_FSCR_L,			119995,	"fscr",				NULL },	// 0x1d4bb
	{ MANUAL_ENTITY_HSCR_L,			119997,	"hscr",				NULL },	// 0x1d4bd
	{ MANUAL_ENTITY_ISCR_L,			119998,	"iscr",				NULL },	// 0x1d4be
	{ MANUAL_ENTITY_JSCR_L,			119999,	"jscr",				NULL },	// 0x1d4bf
	{ MANUAL_ENTITY_KSCR_L,			120000,	"kscr",				NULL },	// 0x1d4c0
	{ MANUAL_ENTITY_LSCR_L,			120001,	"lscr",				NULL },	// 0x1d4c1
	{ MANUAL_ENTITY_MSCR_L,			120002,	"mscr",				NULL },	// 0x1d4c2
	{ MANUAL_ENTITY_NSCR_L,			120003,	"nscr",				NULL },	// 0x1d4c3
	{ MANUAL_ENTITY_PSCR_L,			120005,	"pscr",				NULL },	// 0x1d4c5
	{ MANUAL_ENTITY_QSCR_L,			120006,	"qscr",				NULL },	// 0x1d4c6
	{ MANUAL_ENTITY_RSCR_L,			120007,	"rscr",				NULL },	// 0x1d4c7
	{ MANUAL_ENTITY_SSCR_L,			120008,	"sscr",				NULL },	// 0x1d4c8
	{ MANUAL_ENTITY_TSCR_L,			120009,	"tscr",				NULL },	// 0x1d4c9
	{ MANUAL_ENTITY_USCR_L,			120010,	"uscr",				NULL },	// 0x1d4ca
	{ MANUAL_ENTITY_VSCR_L,			120011,	"vscr",				NULL },	// 0x1d4cb
	{ MANUAL_ENTITY_WSCR_L,			120012,	"wscr",				NULL },	// 0x1d4cc
	{ MANUAL_ENTITY_XSCR_L,			120013,	"xscr",				NULL },	// 0x1d4cd
	{ MANUAL_ENTITY_YSCR_L,			120014,	"yscr",				NULL },	// 0x1d4ce
	{ MANUAL_ENTITY_ZSCR_L,			120015,	"zscr",				NULL },	// 0x1d4cf
	{ MANUAL_ENTITY_AFR_U,			120068,	"Afr",				NULL },	// 0x1d504
	{ MANUAL_ENTITY_BFR_U,			120069,	"Bfr",				NULL },	// 0x1d505
	{ MANUAL_ENTITY_DFR_U,			120071,	"Dfr",				NULL },	// 0x1d507
	{ MANUAL_ENTITY_EFR_U,			120072,	"Efr",				NULL },	// 0x1d508
	{ MANUAL_ENTITY_FFR_U,			120073,	"Ffr",				NULL },	// 0x1d509
	{ MANUAL_ENTITY_GFR_U,			120074,	"Gfr",				NULL },	// 0x1d50a
	{ MANUAL_ENTITY_JFR_U,			120077,	"Jfr",				NULL },	// 0x1d50d
	{ MANUAL_ENTITY_KFR_U,			120078,	"Kfr",				NULL },	// 0x1d50e
	{ MANUAL_ENTITY_LFR_U,			120079,	"Lfr",				NULL },	// 0x1d50f
	{ MANUAL_ENTITY_MFR_U,			120080,	"Mfr",				NULL },	// 0x1d510
	{ MANUAL_ENTITY_NFR_U,			120081,	"Nfr",				NULL },	// 0x1d511
	{ MANUAL_ENTITY_OFR_U,			120082,	"Ofr",				NULL },	// 0x1d512
	{ MANUAL_ENTITY_PFR_U,			120083,	"Pfr",				NULL },	// 0x1d513
	{ MANUAL_ENTITY_QFR_U,			120084,	"Qfr",				NULL },	// 0x1d514
	{ MANUAL_ENTITY_SFR_U,			120086,	"Sfr",				NULL },	// 0x1d516
	{ MANUAL_ENTITY_TFR_U,			120087,	"Tfr",				NULL },	// 0x1d517
	{ MANUAL_ENTITY_UFR_U,			120088,	"Ufr",				NULL },	// 0x1d518
	{ MANUAL_ENTITY_VFR_U,			120089,	"Vfr",				NULL },	// 0x1d519
	{ MANUAL_ENTITY_WFR_U,			120090,	"Wfr",				NULL },	// 0x1d51a
	{ MANUAL_ENTITY_XFR_U,			120091,	"Xfr",				NULL },	// 0x1d51b
	{ MANUAL_ENTITY_YFR_U,			120092,	"Yfr",				NULL },	// 0x1d51c
	{ MANUAL_ENTITY_AFR_L,			120094,	"afr",				NULL },	// 0x1d51e
	{ MANUAL_ENTITY_BFR_L,			120095,	"bfr",				NULL },	// 0x1d51f
	{ MANUAL_ENTITY_CFR_L,			120096,	"cfr",				NULL },	// 0x1d520
	{ MANUAL_ENTITY_DFR_L,			120097,	"dfr",				NULL },	// 0x1d521
	{ MANUAL_ENTITY_EFR_L,			120098,	"efr",				NULL },	// 0x1d522
	{ MANUAL_ENTITY_FFR_L,			120099,	"ffr",				NULL },	// 0x1d523
	{ MANUAL_ENTITY_GFR_L,			120100,	"gfr",				NULL },	// 0x1d524
	{ MANUAL_ENTITY_HFR_L,			120101,	"hfr",				NULL },	// 0x1d525
	{ MANUAL_ENTITY_IFR_L,			120102,	"ifr",				NULL },	// 0x1d526
	{ MANUAL_ENTITY_JFR_L,			120103,	"jfr",				NULL },	// 0x1d527
	{ MANUAL_ENTITY_KFR_L,			120104,	"kfr",				NULL },	// 0x1d528
	{ MANUAL_ENTITY_LFR_L,			120105,	"lfr",				NULL },	// 0x1d529
	{ MANUAL_ENTITY_MFR_L,			120106,	"mfr",				NULL },	// 0x1d52a
	{ MANUAL_ENTITY_NFR_L,			120107,	"nfr",				NULL },	// 0x1d52b
	{ MANUAL_ENTITY_OFR_L,			120108,	"ofr",				NULL },	// 0x1d52c
	{ MANUAL_ENTITY_PFR_L,			120109,	"pfr",				NULL },	// 0x1d52d
	{ MANUAL_ENTITY_QFR_L,			120110,	"qfr",				NULL },	// 0x1d52e
	{ MANUAL_ENTITY_RFR_L,			120111,	"rfr",				NULL },	// 0x1d52f
	{ MANUAL_ENTITY_SFR_L,			120112,	"sfr",				NULL },	// 0x1d530
	{ MANUAL_ENTITY_TFR_L,			120113,	"tfr",				NULL },	// 0x1d531
	{ MANUAL_ENTITY_UFR_L,			120114,	"ufr",				NULL },	// 0x1d532
	{ MANUAL_ENTITY_VFR_L,			120115,	"vfr",				NULL },	// 0x1d533
	{ MANUAL_ENTITY_WFR_L,			120116,	"wfr",				NULL },	// 0x1d534
	{ MANUAL_ENTITY_XFR_L,			120117,	"xfr",				NULL },	// 0x1d535
	{ MANUAL_ENTITY_YFR_L,			120118,	"yfr",				NULL },	// 0x1d536
	{ MANUAL_ENTITY_ZFR_L,			120119,	"zfr",				NULL },	// 0x1d537
	{ MANUAL_ENTITY_AOPF_U,			120120,	"Aopf",				NULL },	// 0x1d538
	{ MANUAL_ENTITY_BOPF_U,			120121,	"Bopf",				NULL },	// 0x1d539
	{ MANUAL_ENTITY_DOPF_U,			120123,	"Dopf",				NULL },	// 0x1d53b
	{ MANUAL_ENTITY_EOPF_U,			120124,	"Eopf",				NULL },	// 0x1d53c
	{ MANUAL_ENTITY_FOPF_U,			120125,	"Fopf",				NULL },	// 0x1d53d
	{ MANUAL_ENTITY_GOPF_U,			120126,	"Gopf",				NULL },	// 0x1d53e
	{ MANUAL_ENTITY_IOPF_U,			120128,	"Iopf",				NULL },	// 0x1d540
	{ MANUAL_ENTITY_JOPF_U,			120129,	"Jopf",				NULL },	// 0x1d541
	{ MANUAL_ENTITY_KOPF_U,			120130,	"Kopf",				NULL },	// 0x1d542
	{ MANUAL_ENTITY_LOPF_U,			120131,	"Lopf",				NULL },	// 0x1d543
	{ MANUAL_ENTITY_MOPF_U,			120132,	"Mopf",				NULL },	// 0x1d544
	{ MANUAL_ENTITY_OOPF_U,			120134,	"Oopf",				NULL },	// 0x1d546
	{ MANUAL_ENTITY_SOPF_U,			120138,	"Sopf",				NULL },	// 0x1d54a
	{ MANUAL_ENTITY_TOPF_U,			120139,	"Topf",				NULL },	// 0x1d54b
	{ MANUAL_ENTITY_UOPF_U,			120140,	"Uopf",				NULL },	// 0x1d54c
	{ MANUAL_ENTITY_VOPF_U,			120141,	"Vopf",				NULL },	// 0x1d54d
	{ MANUAL_ENTITY_WOPF_U,			120142,	"Wopf",				NULL },	// 0x1d54e
	{ MANUAL_ENTITY_XOPF_U,			120143,	"Xopf",				NULL },	// 0x1d54f
	{ MANUAL_ENTITY_YOPF_U,			120144,	"Yopf",				NULL },	// 0x1d550
	{ MANUAL_ENTITY_AOPF_L,			120146,	"aopf",				NULL },	// 0x1d552
	{ MANUAL_ENTITY_BOPF_L,			120147,	"bopf",				NULL },	// 0x1d553
	{ MANUAL_ENTITY_COPF_L,			120148,	"copf",				NULL },	// 0x1d554
	{ MANUAL_ENTITY_DOPF_L,			120149,	"dopf",				NULL },	// 0x1d555
	{ MANUAL_ENTITY_EOPF_L,			120150,	"eopf",				NULL },	// 0x1d556
	{ MANUAL_ENTITY_FOPF_L,			120151,	"fopf",				NULL },	// 0x1d557
	{ MANUAL_ENTITY_GOPF_L,			120152,	"gopf",				NULL },	// 0x1d558
	{ MANUAL_ENTITY_HOPF_L,			120153,	"hopf",				NULL },	// 0x1d559
	{ MANUAL_ENTITY_IOPF_L,			120154,	"iopf",				NULL },	// 0x1d55a
	{ MANUAL_ENTITY_JOPF_L,			120155,	"jopf",				NULL },	// 0x1d55b
	{ MANUAL_ENTITY_KOPF_L,			120156,	"kopf",				NULL },	// 0x1d55c
	{ MANUAL_ENTITY_LOPF_L,			120157,	"lopf",				NULL },	// 0x1d55d
	{ MANUAL_ENTITY_MOPF_L,			120158,	"mopf",				NULL },	// 0x1d55e
	{ MANUAL_ENTITY_NOPF_L,			120159,	"nopf",				NULL },	// 0x1d55f
	{ MANUAL_ENTITY_OOPF_L,			120160,	"oopf",				NULL },	// 0x1d560
	{ MANUAL_ENTITY_POPF_L,			120161,	"popf",				NULL },	// 0x1d561
	{ MANUAL_ENTITY_QOPF_L,			120162,	"qopf",				NULL },	// 0x1d562
	{ MANUAL_ENTITY_ROPF_L,			120163,	"ropf",				NULL },	// 0x1d563
	{ MANUAL_ENTITY_SOPF_L,			120164,	"sopf",				NULL },	// 0x1d564
	{ MANUAL_ENTITY_TOPF_L,			120165,	"topf",				NULL },	// 0x1d565
	{ MANUAL_ENTITY_UOPF_L,			120166,	"uopf",				NULL },	// 0x1d566
	{ MANUAL_ENTITY_VOPF_L,			120167,	"vopf",				NULL },	// 0x1d567
	{ MANUAL_ENTITY_WOPF_L,			120168,	"wopf",				NULL },	// 0x1d568
	{ MANUAL_ENTITY_XOPF_L,			120169,	"xopf",				NULL },	// 0x1d569
	{ MANUAL_ENTITY_YOPF_L,			120170,	"yopf",				NULL },	// 0x1d56a
	{ MANUAL_ENTITY_ZOPF_L,			120171,	"zopf",				NULL },	// 0x1d56b

	/* End of List */

	{ MANUAL_ENTITY_NONE,			0x7fffffff,	"",			NULL }
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
	char **alternative;
	int i, j, current_code = -1;

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

		alternative = manual_entity_names[i].alternatives;

		if (alternative != NULL) {
			for (j = 0; alternative[j] != NULL; j++) {
				if (!search_tree_add_entry(manual_entity_search_tree, alternative[j], &(manual_entity_names[i])))
					return false;
			}
		}

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
