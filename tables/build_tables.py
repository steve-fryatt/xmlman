#!/bin/python3

# Copyright 2025, Stephen Fryatt (info@stevefryatt.org.uk)
#
# This file is part of XmlMan:
#
#   http://www.stevefryatt.org.uk/risc-os
#
# Licensed under the EUPL, Version 1.2 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# Build Character Tables
# ----------------------
#
# Build the struct case_character table for case.c and the various
# struct encoding_map tables for encoding.c.
#
# Before running, the following source data files need to be obtained
# and saved in the same directory:
#
# - The UnicodeData File from the Unicode Character Database, found at
#   https://www.unicode.org/L2/L1999/UnicodeData.html and saved in the
#   project folder as UnicodeData.txt
#
# - The UCS Encoding tables from RISC OS, found at
#   https://gitlab.riscosopen.org/RiscOS/Sources/Internat/Inter/-/blob/master/s/UCSTables
#   and saved in the project folder as UCSTables
#
# The code also requires ColorLog: https://pypi.org/project/colorlog/
# or Debian package python3-colorlog
#
# On successful completion, the tables can be found in case_table.txt
# and encoding_tables.txt -- these can then be copied and pasted into the
# appropriate C source files.

import logging
import colorlog
from typing import Iterator, Optional
from dataclasses import dataclass, field

# Hold data relating to a single Unicode Code Point from the Unicode
# Character Database.

@dataclass
class CodePoint:
	CodePoint: int
	CharacterName: str
	UpperCase: int|None
	LowerCase: int|None
	TitleCase: int|None

# Hold data relating to an entry from a RISC OS Encoding.

@dataclass(order=True)
class EncodingEntry:
	CodePoint: int = field(compare=True)
	Character: int = field(compare=False)
	CharacterName: str

# Set up logging.

handler = colorlog.StreamHandler()
handler.setFormatter(colorlog.ColoredFormatter('%(log_color)s%(levelname)-8s %(message)s'))
logging.getLogger().addHandler(handler)
logging.getLogger().setLevel(logging.INFO)

# Class for handling tabulation of the output.

class Tabulate:
	def __init__(self, *, width: int = 8) -> None:
		"""
		Construct a new Tabulate instance.

		:param width: The tab width to use when aligning columns.
		"""

		self.tab_width = width

		self.rows: list[int|str] = []
		self.widths: list[int] = []

	def clear(self) -> None:
		"""
		Clear the table contents.
		"""

		self.rows.clear()
		self.widths.clear()

	def add_row(self, *line: list[str]) -> None:
		"""
		Add a tabular line to the output, whose fields are tabulated.

		:param *line: The columns to be added to the line. Each parameter
		will be output in its own tab-aligned column.
		"""

		# Tot up the field lengths.

		for i, field in enumerate(line):
			if type(field) is not str:
				raise TypeError("Fields must be strings")

			while i >= len(self.widths):
				self.widths.append(0)

			if len(field) > self.widths[i]:
				self.widths[i] = len(field)

		# Add the line to the output.

		self.rows.append(line)

	def add_line(self, line: str) -> None:
		"""
		Add a line to the output, which does not affect the tabulated columns.

		:param line: The line to be added.
		"""

		if type(line) is not str:
			raise TypeError("Fields must be strings")

		# Add the line to the output.

		self.rows.append(line)

	def add_space(self) -> None:
		"""
		Add a blank line to the output, if there are lines already present.
		Otherwise, do nothing.
		"""

		if len(self.rows) > 0:
			self.rows.append("")

	def lines(self) -> Iterator[str]:
		"""
		Return the tabulated lines as an iterator.

		:returns: An iterator for the tabulated lines.
		"""

		for line in self.rows:
			if type(line) is str:
				yield line
			elif type(line) is tuple:
				columns: list[str] = []

				for field, width in zip(line, self.widths, strict=True):
					column_tabs: int = (width // self.tab_width) + 1
					field_tabs: int = len(field) // self.tab_width

					columns.append(field)
					columns.append("\t" * (column_tabs - field_tabs))

				yield "".join(columns).rstrip()
			else:
				raise RuntimeError("We've found a row type that shouldn't exist")

# Load the source data and generate the tables.

def main():
	# The data from the Unicode Character Database, keyed by codepoint.
	unicode_data: Optional[dict[int, CodePoint]] = None

	# The data from the RISC OS UCS tables, keyed by table name.
	riscos_ucs_tables: Optional[dict[str, list[int]]] = None

	# Handle the tabulation of the output.
	table: Tabulate = Tabulate()

	# Load the Unicode Data.

	try:
		unicode_data = import_unicode_data('UnicodeData.txt')
		logging.info("UnicodeData file loaded.")
	except Exception as e:
		logging.error("Failed to load UnicodeData file: %s.", str(e))
		return

	# Write the case table, which only needs the Unicode data.

	table.clear()

	write_case_change_table(table, unicode_data, "case_table")

	try:

		with open(file="case_table.txt", mode='w', encoding='utf-8') as f:
			for line in table.lines():
				f.write(f"{line}\n")
		logging.info("Case Change Table written.")
	except Exception as e:
		logging.error("Failed to write Case Change Table: %s.", str(e))

	# Load the RISC OS UCSTables.

	try:
		riscos_ucs_tables = import_riscos_ucs_tables('UCSTables')
		logging.info("RISC OS UCSTables file loaded.")
	except Exception as e:
		logging.error("Failed to load RISC OS UCSTables file: %s.", str(e))
		return

	# Write the encoding tables.

	table.clear()

	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin1", "encoding_acorn_latin1", "RISC OS Latin 1")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin2", "encoding_acorn_latin2", "RISC OS Latin 2")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin3", "encoding_acorn_latin3", "RISC OS Latin 3")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin4", "encoding_acorn_latin4", "RISC OS Latin 4")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin5", "encoding_acorn_latin5", "RISC OS Latin 5")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin6", "encoding_acorn_latin6", "RISC OS Latin 6")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin7", "encoding_acorn_latin7", "RISC OS Latin 7")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin8", "encoding_acorn_latin8", "RISC OS Latin 8")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin9", "encoding_acorn_latin9", "RISC OS Latin 9")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Latin10", "encoding_acorn_latin10", "RISC OS Latin 10")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Cyrillic", "encoding_acorn_cyrillic", "RISC OS Cyrillic")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Welsh", "encoding_acorn_welsh", "RISC OS Welsh")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Greek", "encoding_acorn_greek", "RISC OS Greek")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Hebrew", "encoding_acorn_hebrew", "RISC OS Hebrew")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Cyrillic2", "encoding_acorn_cyrillic2", "RISC OS Cyrillic 2")
	write_encoding_table(table, unicode_data, riscos_ucs_tables, "Bfont", "encoding_acorn_bfont", "RISC OS BFont")

	try:
		with open(file="encoding_tables.txt", mode='w', encoding='utf-8') as f:
			for line in table.lines():
				f.write(f"{line}\n")
		logging.info("Encoding Tables written.")
	except Exception as e:
		logging.error("Failed to write Encoding Tables: %s.", str(e))


def write_case_change_table(
	table: Tabulate,
	unicode_data: dict[int, CodePoint],
	struct_name: str
) -> None:
	"""
	Write the case change table to a Tabulate instance.

	:param table: The Tabulate instance to write to.
	:param unicode_data: The Unicde Character Data from which to build
	the table.
	:param struct_name: The name of the structure to be written.
	"""

	table.add_space()
	table.add_line("/**")
	table.add_line(" * The list of known character case conversions.")
	table.add_line(" *")
	table.add_line(" * The order of this table is by ascending unicode point, with -1 at")
	table.add_line(" * the end as an end stop.")
	table.add_line(" *")
	table.add_line(" * The data in this table was derived from UnicodeData 17.0.0")
	table.add_line(" */")
	table.add_space()
	table.add_line(f"static struct case_character {struct_name}[] = {{")

	for codepoint, character in unicode_data.items():
		if character.UpperCase is None and character.LowerCase is None and character.TitleCase is None:
			continue

		upper_case = f"0x{character.UpperCase:x}" if character.UpperCase is not None else "0x0"
		lower_case = f"0x{character.LowerCase:x}" if character.LowerCase is not None else "0x0"
		title_case = f"0x{character.TitleCase:x}" if character.TitleCase is not None else upper_case

		table.add_row("", f"{{0x{codepoint:x},", f"{upper_case},", f"{lower_case},", f"{title_case}}},", f"// {character.CharacterName.title()}")

	table.add_row("", "{-1,", "0x0,", "0x0,", "0x0}", "// End of Table")
	table.add_line("};")

	logging.info("Generated the case conversion table.")


def write_encoding_table(
	table: Tabulate,
	unicode_data: dict[int, CodePoint],
	riscos_tables: dict[str, list[int]],
	riscos_table_name: str,
	struct_name: str,
	table_title: str
) -> None:
	"""
	Write an encoding table to a Tabulate instance.

	:param table: The Tabulate instance to write to.
	:param unicode_data: The Unicde Character Data from which to build
	the table.
	:param riscos_tables: The RISC OS Encoding tables from which to build
	the encoding table.
	:param riscos_table_name: The name of the table in the RISC OS
	Encoding data.
	:param struct_name: The name of the structure to be written.
	:param table_title: The name of the table for use in the table heading
	comment.
	"""

	encoding_data: list[EncodingEntry] = build_encoding_table(unicode_data, riscos_tables[riscos_table_name])

	table.add_space()
	table.add_line("/**")
	table.add_line(f" * UTF8 to {table_title}")
	table.add_line(" */")
	table.add_space()
	table.add_line(f"static struct encoding_map {struct_name}[] = {{")

	for character in encoding_data:
		if character.Character < 128:
			continue

		table.add_row("", f"{{{character.CodePoint},", f"'\\x{character.Character:02x}'}},", f"// {character.CharacterName.title()}")

	table.add_row("", "{0,", "'\\0'}", "// End of Table")
	table.add_line("};")

	logging.info("Generated the %s encoding table.", riscos_table_name)



def build_encoding_table(
	unicode_data: dict[int, CodePoint],
	riscos_table: list[int]
) -> list[EncodingEntry]:
	"""
	Build an encoding table as a list of Unicode code points, with their
	mappings to characters in the current encoding.

		:param unicode_data: The Unicde Character Data from which to build
	the table.
	:param riscos_tables: The RISC OS Encoding table from which to build
	the encoding table.
	"""

	encoding: list[EncodingEntry] = list()

	for character, codepoint in enumerate(riscos_table):
		if character < 32 or character == 127 or codepoint == 0xffffffff:
			continue

		encoding.append(EncodingEntry(codepoint, character, unicode_data[codepoint].CharacterName))

	encoding.sort()

	return encoding

def import_unicode_data(filename: str) -> dict[int, CodePoint]:
	"""
	Import the UnicodeData file from the Unicode Character Database,
	returning a dictionary of CodePoint instances keyed on codepoint.

	:param filename: The filename of the UnicodeData file
	:return: A dictionary of codepoints from the file.
	"""

	characters: dict[int, CodePoint] = dict()

	# See https://www.unicode.org/L2/L1999/UnicodeData.html for the file format.

	with open(file=filename, mode='r', encoding='utf-8') as f:
		for line in f:
			fields = line.rstrip().split(";")
			if len(fields) != 15:
				continue

			codepoint = int(fields[0], 16) if fields[0] != '' else None

			characters[codepoint] = CodePoint(
				CodePoint=codepoint,
				CharacterName=fields[1] if fields[1] != '' else None,
				UpperCase=int(fields[12], 16) if fields[12] != '' else None,
				LowerCase=int(fields[13], 16) if fields[13] != '' else None,
				TitleCase=int(fields[14], 16) if fields[14] != '' else None,
			)

	return characters


def import_riscos_ucs_tables(filename: str) -> dict[str, list[int]]:
	"""
	Import the UCSTables from the RISC OS Source, returning a dictionary
	of the individual encoding tables keyed on table name as used in the
	source file.

	:param filename: The filename of the UCSTables file
	:return: A dictionary of encoding tables.
	"""
	tables: dict[str, list[int]] = dict()

	# See https://gitlab.riscosopen.org/RiscOS/Sources/Internat/Inter/-/blob/master/s/UCSTables

	current_table: list|None = None

	with open(file=filename, mode='r', encoding='utf-8') as f:
		for line in f:
			line = line.strip()

			# Skip empty lines, comments or ASM conditionals

			if line == '' or line[0] in set(';[]'):
				continue

			if line[0] == '&':
				if current_table is None:
					continue

				if line[0:2] != "& ":
					logging.warning("Unexpected UCSTable line '%s'.", line)
					continue

				fields = line[2:].split(",")

				for field in fields:
					if field[0] != "&":
						logging.warning("Unexpected field '%s' in UCSTables.", field)
						continue

					current_table.append(int(field[1:], 16))

			else:
				# The line is a table containing a table name, so create a new
				# list for the table and store it in the dict.

				current_table = list()
				tables[line] = current_table

	return tables

if __name__ == "__main__":
	main()