// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgsource/libpkgsource.h>

#include "record_io.h"

namespace pkgsource::codec::internal {

void encode_provenance(record_writer& output,
                       const declaration_provenance& value);
[[nodiscard]] declaration_provenance decode_provenance(record_reader& input);

void encode_subject(record_writer& output, const requirement_subject& value);
[[nodiscard]] requirement_subject decode_subject(record_reader& input);

void encode_scope(record_writer& output, const requirement_scope& value);
[[nodiscard]] requirement_scope decode_scope(record_reader& input);

void encode_program(record_writer& output, const program& value);
[[nodiscard]] program decode_program(record_reader& input);

void encode_metadata(record_writer& output, const package_metadata& value);
[[nodiscard]] package_metadata decode_metadata(record_reader& input);

void encode_source_input(record_writer& output, const source_input& value);
[[nodiscard]] source_input decode_source_input(record_reader& input);

void encode_lifecycle_program(record_writer& output,
                              const lifecycle_program& value);
[[nodiscard]] lifecycle_program decode_lifecycle_program(record_reader& input);

void encode_architectures(record_writer& output,
                          const architecture_requirements& value);
[[nodiscard]] architecture_requirements
decode_architectures(record_reader& input);

} // namespace pkgsource::codec::internal
