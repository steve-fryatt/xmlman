# Copyright 2018-2024, Stephen Fryatt (info@stevefryatt.org.uk)
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

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

ARCHIVE := xmlman

ifeq ($(TARGET),riscos)
  RUNIMAGE := xmlman,ff8
else
  RUNIMAGE := xmlman
endif

OBJS := args.o			\
	encoding.o		\
	filename.o		\
	list_numbers.o		\
	manual.o		\
	manual_data.o		\
	manual_defines.o	\
	manual_entity.o		\
	manual_ids.o		\
	manual_queue.o		\
	modes.o			\
	msg.o			\
	output_debug.o		\
	output_html.o		\
	output_html_file.o	\
	output_strong.o		\
	output_strong_file.o	\
	output_text.o		\
	output_text_line.o	\
	parse.o			\
	parse_element.o		\
	parse_link.o		\
	parse_xml.o		\
	search_tree.o		\
	string.o		\
	xmlman.o

include $(SFTOOLS_MAKE)/Cross
