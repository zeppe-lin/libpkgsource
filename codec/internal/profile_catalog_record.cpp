// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgsource-codec/codec.h>

#include "record_io.h"
#include "value_codec.h"

#include <array>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace pkgsource::codec {
namespace {

constexpr std::array<std::uint8_t, 8> profile_catalog_magic = {
    'Z',
    'L',
    'P',
    'S',
    'P',
    'C',
    'A',
    'T',
};

struct decoded_profile final {
  profile_declaration declaration;
  std::string identity;
};

void encode_profile_declaration(internal::record_writer& output,
                                const sealed_profile& profile)
{
  output.text(profile.name().name());
  output.text(profile.identity().hex());
  internal::encode_provenance(output, profile.provenance());
  output.u32(internal::record_count(profile.direct_members().size()));
  for (const auto& member : profile.direct_members()) {
    internal::encode_subject(output, member.subject());
    internal::encode_provenance(output, member.provenance());
  }
}

[[nodiscard]] decoded_profile
decode_profile_declaration(internal::record_reader& input)
{
  auto name = input.text();
  auto identity = input.text();
  auto provenance = internal::decode_provenance(input);
  std::vector<profile_member_declaration> members;
  const auto member_count = internal::read_record_count(input);
  members.reserve(member_count);
  for (std::uint32_t index = 0; index != member_count; ++index) {
    auto subject = internal::decode_subject(input);
    auto member_provenance = internal::decode_provenance(input);
    members.emplace_back(std::move(subject), std::move(member_provenance));
  }
  return decoded_profile{
      profile_declaration(profile_reference(std::move(name)),
                          std::move(provenance),
                          std::move(members)),
      std::move(identity),
  };
}

} // namespace

profile_catalog_encoding encode_profile_catalog(const profile_catalog& catalog)
{
  internal::record_writer output(maximum_profile_catalog_encoding_size);
  output.raw(profile_catalog_magic);
  output.u16(profile_catalog_encoding_version);
  output.u32(internal::record_count(catalog.profiles().size()));
  for (const auto& profile : catalog.profiles()) {
    encode_profile_declaration(output, profile);
  }
  return output.finish();
}

profile_catalog decode_profile_catalog(const profile_catalog_encoding& encoding)
{
  try {
    internal::record_reader input(encoding,
                                  maximum_profile_catalog_encoding_size);
    input.expect(profile_catalog_magic, "profile catalog");
    if (input.u16() != profile_catalog_encoding_version) {
      internal::fail(codec_error_code::unsupported_version,
                     "unsupported profile catalog encoding version");
    }

    std::vector<decoded_profile> decoded;
    const auto profile_count = internal::read_record_count(input);
    decoded.reserve(profile_count);
    for (std::uint32_t index = 0; index != profile_count; ++index) {
      decoded.push_back(decode_profile_declaration(input));
    }
    input.finish();

    std::vector<profile_declaration> declarations;
    declarations.reserve(decoded.size());
    for (auto& value : decoded) {
      declarations.push_back(std::move(value.declaration));
    }

    profile_catalog result = profile_catalog::seal(std::move(declarations));
    if (result.profiles().size() != decoded.size()) {
      internal::fail(codec_error_code::identity_mismatch,
                     "profile catalog shape changed during sealing");
    }
    for (std::size_t index = 0; index != decoded.size(); ++index) {
      if (result.profiles()[index].identity().hex() !=
          decoded[index].identity) {
        internal::fail(codec_error_code::identity_mismatch,
                       "profile identity changed during decoding");
      }
    }
    if (encode_profile_catalog(result) != encoding) {
      internal::fail(codec_error_code::noncanonical,
                     "profile catalog record is not canonical");
    }
    return result;
  } catch (const codec_error&) {
    throw;
  } catch (const error& failure) {
    internal::translate_model_failure(failure);
  } catch (const std::exception& failure) {
    internal::fail(codec_error_code::invalid_record,
                   std::string("invalid profile catalog record: ") +
                       failure.what());
  }
}

} // namespace pkgsource::codec
