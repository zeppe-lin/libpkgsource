// SPDX-FileCopyrightText: 2026 Alexandr Savca <alexandr.savca89@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgsource/error.h>
#include <libpkgsource/profile.h>

#include "internal/identity_writer.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace pkgsource {
namespace {

template <typename T>
void sort_unique(std::vector<T>& values, std::string_view field)
{
  std::sort(values.begin(), values.end());
  if (std::adjacent_find(values.begin(), values.end()) != values.end()) {
    throw error(
        error_code::duplicate_declaration,
        "duplicate " + std::string(field));
  }
}

struct declaration_record final {
  profile_reference name;
  declaration_provenance provenance;
  std::vector<profile_member_declaration> members;
};

struct computed_profile final {
  profile_identity identity;
  std::vector<profile_expansion_path> expansion;
};

struct reconstructed_profile final {
  profile_reference name;
  std::map<requirement_subject, declaration_provenance> members;
};

class sealed_profile_validator final {
public:
  sealed_profile_validator(
      const profile_reference& name,
      const std::vector<profile_member_declaration>& direct_members,
      const std::vector<profile_expansion_path>& expansion)
      : root_(name.name())
  {
    records_.emplace(root_, reconstructed_profile{name, {}});
    reconstructed_profile& root = records_.at(root_);
    for (const profile_member_declaration& member : direct_members) {
      const auto inserted =
          root.members.emplace(member.subject(), member.provenance());
      if (!inserted.second) {
        throw error(
            error_code::duplicate_declaration,
            "duplicate sealed profile member: " + member.subject().text());
      }
    }

    for (const profile_expansion_path& path : expansion) {
      std::string expected = root_;
      const auto& steps = path.steps();
      for (std::size_t index = 0; index < steps.size(); ++index) {
        const profile_expansion_step& step = steps[index];
        if (step.profile().name() != expected) {
          throw error(
              error_code::invalid_profile,
              "sealed profile expansion is not contiguous");
        }

        auto found = records_.find(expected);
        if (found == records_.end()) {
          found = records_.emplace(
              expected, reconstructed_profile{step.profile(), {}}).first;
        }
        const auto inserted =
            found->second.members.emplace(step.member(), step.provenance());
        if (!inserted.second &&
            inserted.first->second != step.provenance()) {
          throw error(
              error_code::invalid_profile,
              "sealed profile expansion has contradictory provenance");
        }

        if (step.member().kind() == requirement_subject_kind::profile) {
          if (index + 1 == steps.size()) {
            throw error(
                error_code::invalid_profile,
                "sealed profile expansion terminates at a profile");
          }
          expected = step.member().profile().name();
          continue;
        }
        if (index + 1 != steps.size() ||
            step.member().package() != path.package()) {
          throw error(
              error_code::invalid_profile,
              "sealed profile expansion terminates at the wrong package");
        }
      }
    }

    if (!matches_direct_members(records_.at(root_), direct_members)) {
      throw error(
          error_code::invalid_profile,
          "sealed profile direct members are not canonical");
    }
  }

  void verify(
      const profile_identity& identity,
      const std::vector<profile_expansion_path>& expansion)
  {
    const computed_profile& computed = compute(root_);
    if (computed.identity != identity) {
      throw error(
          error_code::invalid_identity,
          "sealed profile identity does not match its members");
    }
    if (computed.expansion != expansion) {
      throw error(
          error_code::invalid_profile,
          "sealed profile expansion is not canonical");
    }
  }

private:
  static bool matches_direct_members(
      const reconstructed_profile& record,
      const std::vector<profile_member_declaration>& members)
  {
    if (record.members.size() != members.size()) {
      return false;
    }
    auto expected = record.members.begin();
    for (const profile_member_declaration& member : members) {
      if (expected->first != member.subject() ||
          expected->second != member.provenance()) {
        return false;
      }
      ++expected;
    }
    return true;
  }

  const computed_profile& compute(const std::string& name)
  {
    const auto existing = computed_.find(name);
    if (existing != computed_.end()) {
      return existing->second;
    }
    if (!active_.insert(name).second) {
      throw error(
          error_code::profile_cycle,
          "sealed profile expansion contains a cycle");
    }

    const auto record = records_.find(name);
    if (record == records_.end() || record->second.members.empty()) {
      throw error(
          error_code::invalid_profile,
          "sealed profile expansion omits a referenced profile");
    }

    detail::identity_writer writer;
    writer.text("libpkgsource/profile/v1");
    writer.text(record->second.name.name());
    writer.number(record->second.members.size());
    std::vector<profile_expansion_path> expansion;

    for (const auto& entry : record->second.members) {
      writer.text(to_string(entry.first.kind()));
      writer.text(entry.first.text());
      const profile_expansion_step root_step(
          record->second.name, entry.first, entry.second);
      if (entry.first.kind() == requirement_subject_kind::package) {
        expansion.emplace_back(
            entry.first.package(),
            std::vector<profile_expansion_step>{root_step});
        continue;
      }

      const computed_profile& nested =
          compute(entry.first.profile().name());
      writer.text(nested.identity.hex());
      for (const profile_expansion_path& nested_path : nested.expansion) {
        std::vector<profile_expansion_step> steps;
        steps.reserve(nested_path.steps().size() + 1);
        steps.push_back(root_step);
        steps.insert(steps.end(),
                     nested_path.steps().begin(),
                     nested_path.steps().end());
        expansion.emplace_back(nested_path.package(), std::move(steps));
      }
    }

    sort_unique(expansion, "sealed profile expansion path");
    active_.erase(name);
    auto inserted = computed_.emplace(
        name,
        computed_profile{profile_identity::from_sha256(writer.finish()),
                         std::move(expansion)});
    return inserted.first->second;
  }

  std::string root_;
  std::map<std::string, reconstructed_profile> records_;
  std::map<std::string, computed_profile> computed_;
  std::set<std::string> active_;
};

class catalog_builder final {
public:
  explicit catalog_builder(std::vector<profile_declaration> declarations)
  {
    std::sort(
        declarations.begin(),
        declarations.end(),
        [](const profile_declaration& lhs, const profile_declaration& rhs) {
          return lhs.name() < rhs.name();
        });
    for (std::size_t i = 1; i < declarations.size(); ++i) {
      if (declarations[i - 1].name() == declarations[i].name()) {
        throw error(
            error_code::duplicate_declaration,
            "duplicate profile definition: " + declarations[i].name().name());
      }
    }

    for (profile_declaration& declaration : declarations) {
      std::vector<profile_member_declaration> members = declaration.members();
      std::sort(members.begin(),
                members.end(),
                [](const profile_member_declaration& lhs,
                   const profile_member_declaration& rhs) {
                  return lhs.subject() < rhs.subject();
                });
      for (std::size_t i = 1; i < members.size(); ++i) {
        if (members[i - 1].subject() == members[i].subject()) {
          throw error(
              error_code::duplicate_declaration,
              "duplicate member in " + declaration.name().name() +
                  ": " + members[i].subject().text());
        }
      }
      records_.emplace(
          declaration.name().name(),
          declaration_record{declaration.name(),
              declaration.provenance(), std::move(members)});
    }

    for (const auto& entry : records_) {
      for (const profile_member_declaration& member : entry.second.members) {
        if (member.subject().kind() == requirement_subject_kind::profile &&
            records_.find(member.subject().profile().name()) ==
                records_.end()) {
          throw error(
              error_code::unknown_profile,
              "unknown profile " + member.subject().profile().name() +
                  " in " + entry.first);
        }
      }
    }
  }

  std::vector<sealed_profile> build()
  {
    std::vector<sealed_profile> result;
    result.reserve(records_.size());
    for (const auto& entry : records_) {
      const computed_profile& computed = compute(entry.first);
      result.emplace_back(
          entry.second.name,
          computed.identity,
          entry.second.provenance,
          entry.second.members,
          computed.expansion);
    }
    return result;
  }

private:
  const computed_profile& compute(const std::string& name)
  {
    const auto existing = computed_.find(name);
    if (existing != computed_.end()) {
      return existing->second;
    }
    if (!active_.insert(name).second) {
      std::string chain;
      for (const std::string& item : stack_) {
        if (!chain.empty()) {
          chain += " -> ";
        }
        chain += item;
      }
      if (!chain.empty()) {
        chain += " -> ";
      }
      chain += name;
      throw error(error_code::profile_cycle, "profile cycle: " + chain);
    }
    stack_.push_back(name);

    const declaration_record& record = records_.at(name);
    std::vector<profile_expansion_path> expansion;
    detail::identity_writer writer;
    writer.text("libpkgsource/profile/v1");
    writer.text(record.name.name());
    writer.number(record.members.size());

    for (const profile_member_declaration& member : record.members) {
      writer.text(to_string(member.subject().kind()));
      writer.text(member.subject().text());
      const profile_expansion_step root_step(
          record.name, member.subject(), member.provenance());
      if (member.subject().kind() == requirement_subject_kind::package) {
        expansion.emplace_back(
            member.subject().package(),
            std::vector<profile_expansion_step>{root_step});
      } else {
        const computed_profile& nested =
            compute(member.subject().profile().name());
        writer.text(nested.identity.hex());
        for (const profile_expansion_path& nested_path : nested.expansion) {
          std::vector<profile_expansion_step> steps;
          steps.reserve(nested_path.steps().size() + 1);
          steps.push_back(root_step);
          steps.insert(
              steps.end(),
              nested_path.steps().begin(),
              nested_path.steps().end());
          expansion.emplace_back(nested_path.package(), std::move(steps));
        }
      }
    }

    sort_unique(expansion, "profile expansion path");
    stack_.pop_back();
    active_.erase(name);
    auto inserted = computed_.emplace(
        name,
        computed_profile{profile_identity::from_sha256(writer.finish()),
                         std::move(expansion)});
    return inserted.first->second;
  }

  std::map<std::string, declaration_record> records_;
  std::map<std::string, computed_profile> computed_;
  std::set<std::string> active_;
  std::vector<std::string> stack_;
};

void collect_profile_closure(
    const sealed_profile& profile,
    const profile_catalog& catalog,
    std::map<std::string, sealed_profile>& closure)
{
  if (!closure.emplace(profile.name().name(), profile).second) {
    return;
  }
  for (const profile_member_declaration& member : profile.direct_members()) {
    if (member.subject().kind() == requirement_subject_kind::profile) {
      collect_profile_closure(
          catalog.require(member.subject().profile()), catalog, closure);
    }
  }
}

} // namespace

profile_member_declaration::profile_member_declaration(
    requirement_subject subject, declaration_provenance provenance)
    : subject_(std::move(subject)), provenance_(std::move(provenance))
{
}
const requirement_subject& profile_member_declaration::subject() const noexcept
{
  return subject_;
}
const declaration_provenance&
profile_member_declaration::provenance() const noexcept
{
  return provenance_;
}

profile_declaration::profile_declaration(
    profile_reference name,
    declaration_provenance provenance,
    std::vector<profile_member_declaration> members)
    : name_(std::move(name)), provenance_(std::move(provenance)),
      members_(std::move(members))
{
  if (members_.empty()) {
    throw error(
        error_code::invalid_profile,
        "profile has no members: " + name_.name());
  }
}
const profile_reference& profile_declaration::name() const noexcept
{
  return name_;
}
const declaration_provenance& profile_declaration::provenance() const noexcept
{
  return provenance_;
}
const std::vector<profile_member_declaration>&
profile_declaration::members() const noexcept
{
  return members_;
}

profile_expansion_step::profile_expansion_step(
    profile_reference profile,
    requirement_subject member,
    declaration_provenance provenance)
    : profile_(std::move(profile)), member_(std::move(member)),
      provenance_(std::move(provenance))
{
}
const profile_reference& profile_expansion_step::profile() const noexcept
{
  return profile_;
}
const requirement_subject& profile_expansion_step::member() const noexcept
{
  return member_;
}
const declaration_provenance&
profile_expansion_step::provenance() const noexcept
{
  return provenance_;
}
bool operator==(const profile_expansion_step& lhs,
                const profile_expansion_step& rhs) noexcept
{
  return std::tie(lhs.profile_, lhs.member_, lhs.provenance_) ==
         std::tie(rhs.profile_, rhs.member_, rhs.provenance_);
}
bool operator!=(const profile_expansion_step& lhs,
                const profile_expansion_step& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const profile_expansion_step& lhs,
               const profile_expansion_step& rhs) noexcept
{
  return std::tie(lhs.profile_, lhs.member_, lhs.provenance_) <
         std::tie(rhs.profile_, rhs.member_, rhs.provenance_);
}

profile_expansion_path::profile_expansion_path(
    package_reference package, std::vector<profile_expansion_step> steps)
    : package_(std::move(package)), steps_(std::move(steps))
{
  if (steps_.empty()) {
    throw error(error_code::invalid_profile, "empty profile expansion path");
  }
  const requirement_subject& leaf = steps_.back().member();
  if (leaf.kind() != requirement_subject_kind::package ||
      leaf.package() != package_) {
    throw error(error_code::invalid_profile,
                "profile expansion path does not terminate at its package");
  }
}
const package_reference& profile_expansion_path::package() const noexcept
{
  return package_;
}
const std::vector<profile_expansion_step>&
profile_expansion_path::steps() const noexcept
{
  return steps_;
}
bool operator==(const profile_expansion_path& lhs,
                const profile_expansion_path& rhs) noexcept
{
  return std::tie(lhs.package_, lhs.steps_) ==
         std::tie(rhs.package_, rhs.steps_);
}
bool operator!=(const profile_expansion_path& lhs,
                const profile_expansion_path& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const profile_expansion_path& lhs,
               const profile_expansion_path& rhs) noexcept
{
  return std::tie(lhs.package_, lhs.steps_) <
         std::tie(rhs.package_, rhs.steps_);
}

sealed_profile::sealed_profile(
    profile_reference name,
    profile_identity identity,
    declaration_provenance provenance,
    std::vector<profile_member_declaration> direct_members,
    std::vector<profile_expansion_path> expansion)
    : name_(std::move(name)), identity_(std::move(identity)),
      provenance_(std::move(provenance)),
      direct_members_(std::move(direct_members)),
      expansion_(std::move(expansion))
{
  sealed_profile_validator validator(name_, direct_members_, expansion_);
  validator.verify(identity_, expansion_);
}
const profile_reference& sealed_profile::name() const noexcept
{
  return name_;
}
const profile_identity& sealed_profile::identity() const noexcept
{
  return identity_;
}
const declaration_provenance& sealed_profile::provenance() const noexcept
{
  return provenance_;
}
const std::vector<profile_member_declaration>&
sealed_profile::direct_members() const noexcept
{
  return direct_members_;
}
const std::vector<profile_expansion_path>&
sealed_profile::expansion() const noexcept
{
  return expansion_;
}

profile_catalog::profile_catalog(std::vector<sealed_profile> profiles)
    : profiles_(std::move(profiles))
{
}
profile_catalog
profile_catalog::seal(std::vector<profile_declaration> declarations)
{
  return profile_catalog(catalog_builder(std::move(declarations)).build());
}
const std::vector<sealed_profile>& profile_catalog::profiles() const noexcept
{
  return profiles_;
}
const sealed_profile&
profile_catalog::require(const profile_reference& profile) const
{
  const auto found = std::lower_bound(
      profiles_.begin(),
      profiles_.end(),
      profile,
      [](const sealed_profile& value, const profile_reference& key) {
        return value.name() < key;
      });
  if (found == profiles_.end() || found->name() != profile) {
    throw error(error_code::unknown_profile,
                "unknown profile: " + profile.name());
  }
  return *found;
}

requirement_origin::requirement_origin(
    declaration_provenance declaration,
    std::vector<profile_expansion_step> expansion)
    : declaration_(std::move(declaration)), expansion_(std::move(expansion))
{
}
const declaration_provenance& requirement_origin::declaration() const noexcept
{
  return declaration_;
}
const std::vector<profile_expansion_step>&
requirement_origin::expansion() const noexcept
{
  return expansion_;
}
bool operator==(const requirement_origin& lhs,
                const requirement_origin& rhs) noexcept
{
  return std::tie(lhs.declaration_, lhs.expansion_) ==
         std::tie(rhs.declaration_, rhs.expansion_);
}
bool operator!=(const requirement_origin& lhs,
                const requirement_origin& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const requirement_origin& lhs,
               const requirement_origin& rhs) noexcept
{
  return std::tie(lhs.declaration_, lhs.expansion_) <
         std::tie(rhs.declaration_, rhs.expansion_);
}

resolved_requirement::resolved_requirement(
    requirement_scope scope,
    package_reference package,
    std::vector<requirement_origin> origins)
    : scope_(std::move(scope)), package_(std::move(package)),
      origins_(std::move(origins))
{
  if (origins_.empty()) {
    throw error(error_code::invalid_requirement,
                "resolved requirement has no provenance");
  }
  sort_unique(origins_, "requirement origin");
}
const requirement_scope& resolved_requirement::scope() const noexcept
{
  return scope_;
}
const package_reference& resolved_requirement::package() const noexcept
{
  return package_;
}
const std::vector<requirement_origin>&
resolved_requirement::origins() const noexcept
{
  return origins_;
}
bool operator==(const resolved_requirement& lhs,
                const resolved_requirement& rhs) noexcept
{
  return std::tie(lhs.scope_, lhs.package_, lhs.origins_) ==
         std::tie(rhs.scope_, rhs.package_, rhs.origins_);
}
bool operator!=(const resolved_requirement& lhs,
                const resolved_requirement& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const resolved_requirement& lhs,
               const resolved_requirement& rhs) noexcept
{
  return std::tie(lhs.scope_, lhs.package_, lhs.origins_) <
         std::tie(rhs.scope_, rhs.package_, rhs.origins_);
}

selected_profile::selected_profile(
    profile_reference profile,
    profile_identity identity,
    std::vector<declaration_provenance> declarations)
    : profile_(std::move(profile)), identity_(std::move(identity)),
      declarations_(std::move(declarations))
{
  if (declarations_.empty()) {
    throw error(error_code::invalid_requirement,
                "selected profile has no declaration provenance");
  }
  sort_unique(declarations_, "selected profile declaration");
}
const profile_reference& selected_profile::profile() const noexcept
{
  return profile_;
}
const profile_identity& selected_profile::identity() const noexcept
{
  return identity_;
}
const std::vector<declaration_provenance>&
selected_profile::declarations() const noexcept
{
  return declarations_;
}

sealed_requirement_set::sealed_requirement_set(
    std::vector<resolved_requirement> requirements,
    std::vector<selected_profile> selected_build_profiles,
    std::vector<sealed_profile> profile_closure)
    : requirements_(std::move(requirements)),
      selected_build_profiles_(std::move(selected_build_profiles)),
      profile_closure_(std::move(profile_closure))
{
}
sealed_requirement_set
sealed_requirement_set::seal(std::vector<requirement_declaration> declarations,
                             const profile_catalog& profiles)
{
  using key_type = std::pair<requirement_scope, package_reference>;
  std::map<key_type, std::vector<requirement_origin>> requirements;
  std::map<std::string, std::vector<declaration_provenance>> selected;
  std::map<std::string, sealed_profile> closure;

  std::sort(declarations.begin(),
            declarations.end(),
            [](const requirement_declaration& lhs,
               const requirement_declaration& rhs) {
              return std::tie(lhs.scope(), lhs.subject(), lhs.provenance()) <
                     std::tie(rhs.scope(), rhs.subject(), rhs.provenance());
            });
  for (std::size_t i = 1; i < declarations.size(); ++i) {
    if (declarations[i - 1].scope() == declarations[i].scope() &&
        declarations[i - 1].subject() == declarations[i].subject()) {
      throw error(error_code::duplicate_declaration,
                  "duplicate requirement declaration: " +
                      declarations[i].subject().text());
    }
  }

  for (const requirement_declaration& declaration : declarations) {
    if (declaration.subject().kind() == requirement_subject_kind::package) {
      requirements[{declaration.scope(), declaration.subject().package()}]
          .emplace_back(declaration.provenance(),
                        std::vector<profile_expansion_step>{});
      continue;
    }

    const sealed_profile& profile =
        profiles.require(declaration.subject().profile());
    collect_profile_closure(profile, profiles, closure);
    if (declaration.scope().kind() == requirement_scope_kind::build) {
      selected[profile.name().name()].push_back(declaration.provenance());
    }
    for (const profile_expansion_path& path : profile.expansion()) {
      requirements[{declaration.scope(), path.package()}].emplace_back(
          declaration.provenance(), path.steps());
    }
  }

  std::vector<resolved_requirement> resolved;
  resolved.reserve(requirements.size());
  for (auto& entry : requirements) {
    resolved.emplace_back(
        entry.first.first, entry.first.second, std::move(entry.second));
  }

  std::vector<selected_profile> selected_profiles;
  selected_profiles.reserve(selected.size());
  for (auto& entry : selected) {
    const sealed_profile& profile =
        profiles.require(profile_reference(entry.first));
    selected_profiles.emplace_back(
        profile.name(), profile.identity(), std::move(entry.second));
  }

  std::vector<sealed_profile> used_profiles;
  used_profiles.reserve(closure.size());
  for (auto& entry : closure) {
    used_profiles.push_back(std::move(entry.second));
  }

  return sealed_requirement_set(std::move(resolved),
                                std::move(selected_profiles),
                                std::move(used_profiles));
}
const std::vector<resolved_requirement>&
sealed_requirement_set::requirements() const noexcept
{
  return requirements_;
}
std::vector<resolved_requirement>
sealed_requirement_set::for_scope(const requirement_scope& scope) const
{
  std::vector<resolved_requirement> result;
  for (const resolved_requirement& requirement : requirements_) {
    if (requirement.scope() == scope) {
      result.push_back(requirement);
    }
  }
  return result;
}
const std::vector<selected_profile>&
sealed_requirement_set::selected_build_profiles() const noexcept
{
  return selected_build_profiles_;
}
const std::vector<sealed_profile>&
sealed_requirement_set::profile_closure() const noexcept
{
  return profile_closure_;
}

} // namespace pkgsource
