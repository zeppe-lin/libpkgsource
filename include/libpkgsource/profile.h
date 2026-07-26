// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file profile.h
 *  \brief Sealed authoritative requirement profiles and expansion.
 */
#pragma once

#include <vector>

#include <libpkgsource/model.h>

namespace pkgsource {

/*! \brief One direct package or nested-profile member declaration. */
class profile_member_declaration final {
public:
  profile_member_declaration(requirement_subject subject,
                             declaration_provenance provenance);
  [[nodiscard]] const requirement_subject& subject() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;
private:
  requirement_subject subject_;
  declaration_provenance provenance_;
};

/*! \brief Parser-neutral input for one profile definition. */
class profile_declaration final {
public:
  profile_declaration(profile_reference name,
                      declaration_provenance provenance,
                      std::vector<profile_member_declaration> members);
  [[nodiscard]] const profile_reference& name() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;
  [[nodiscard]] const std::vector<profile_member_declaration>& members() const noexcept;
private:
  profile_reference name_;
  declaration_provenance provenance_;
  std::vector<profile_member_declaration> members_;
};

/*! \brief One retained edge in a transitive profile expansion path. */
class profile_expansion_step final {
public:
  profile_expansion_step(profile_reference profile,
                         requirement_subject member,
                         declaration_provenance provenance);
  [[nodiscard]] const profile_reference& profile() const noexcept;
  [[nodiscard]] const requirement_subject& member() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;
  friend bool operator==(const profile_expansion_step& lhs,
                         const profile_expansion_step& rhs) noexcept;
  friend bool operator!=(const profile_expansion_step& lhs,
                         const profile_expansion_step& rhs) noexcept;
  friend bool operator<(const profile_expansion_step& lhs,
                        const profile_expansion_step& rhs) noexcept;
private:
  profile_reference profile_;
  requirement_subject member_;
  declaration_provenance provenance_;
};

/*! \brief One exact route from a selected profile to a package member. */
class profile_expansion_path final {
public:
  profile_expansion_path(package_reference package,
                         std::vector<profile_expansion_step> steps);
  [[nodiscard]] const package_reference& package() const noexcept;
  [[nodiscard]] const std::vector<profile_expansion_step>& steps() const noexcept;
  friend bool operator==(const profile_expansion_path& lhs,
                         const profile_expansion_path& rhs) noexcept;
  friend bool operator!=(const profile_expansion_path& lhs,
                         const profile_expansion_path& rhs) noexcept;
  friend bool operator<(const profile_expansion_path& lhs,
                        const profile_expansion_path& rhs) noexcept;
private:
  package_reference package_;
  std::vector<profile_expansion_step> steps_;
};

/*! \brief Immutable authoritative profile value. */
class sealed_profile final {
public:
  sealed_profile(profile_reference name, profile_identity identity,
                 declaration_provenance provenance,
                 std::vector<profile_member_declaration> direct_members,
                 std::vector<profile_expansion_path> expansion);
  [[nodiscard]] const profile_reference& name() const noexcept;
  [[nodiscard]] const profile_identity& identity() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;
  [[nodiscard]] const std::vector<profile_member_declaration>& direct_members() const noexcept;
  [[nodiscard]] const std::vector<profile_expansion_path>& expansion() const noexcept;
private:
  profile_reference name_;
  profile_identity identity_;
  declaration_provenance provenance_;
  std::vector<profile_member_declaration> direct_members_;
  std::vector<profile_expansion_path> expansion_;
};

/*! \brief Deterministically sealed profile authority. */
class profile_catalog final {
public:
  [[nodiscard]] static profile_catalog seal(
      std::vector<profile_declaration> declarations);
  [[nodiscard]] const std::vector<sealed_profile>& profiles() const noexcept;
  [[nodiscard]] const sealed_profile& require(
      const profile_reference& profile) const;
private:
  explicit profile_catalog(std::vector<sealed_profile> profiles);
  std::vector<sealed_profile> profiles_;
};

/*! \brief Provenance for one exact expanded package requirement. */
class requirement_origin final {
public:
  requirement_origin(declaration_provenance declaration,
                     std::vector<profile_expansion_step> expansion);
  [[nodiscard]] const declaration_provenance& declaration() const noexcept;
  [[nodiscard]] const std::vector<profile_expansion_step>& expansion() const noexcept;
  friend bool operator==(const requirement_origin& lhs,
                         const requirement_origin& rhs) noexcept;
  friend bool operator!=(const requirement_origin& lhs,
                         const requirement_origin& rhs) noexcept;
  friend bool operator<(const requirement_origin& lhs,
                        const requirement_origin& rhs) noexcept;
private:
  declaration_provenance declaration_;
  std::vector<profile_expansion_step> expansion_;
};

/*! \brief One exact package requirement after profile expansion. */
class resolved_requirement final {
public:
  resolved_requirement(requirement_scope scope, package_reference package,
                       std::vector<requirement_origin> origins);
  [[nodiscard]] const requirement_scope& scope() const noexcept;
  [[nodiscard]] const package_reference& package() const noexcept;
  [[nodiscard]] const std::vector<requirement_origin>& origins() const noexcept;
  friend bool operator==(const resolved_requirement& lhs,
                         const resolved_requirement& rhs) noexcept;
  friend bool operator!=(const resolved_requirement& lhs,
                         const resolved_requirement& rhs) noexcept;
  friend bool operator<(const resolved_requirement& lhs,
                        const resolved_requirement& rhs) noexcept;
private:
  requirement_scope scope_;
  package_reference package_;
  std::vector<requirement_origin> origins_;
};

/*! \brief One selected build-profile root and all issuing declarations. */
class selected_profile final {
public:
  selected_profile(profile_reference profile, profile_identity identity,
                   std::vector<declaration_provenance> declarations);
  [[nodiscard]] const profile_reference& profile() const noexcept;
  [[nodiscard]] const profile_identity& identity() const noexcept;
  [[nodiscard]] const std::vector<declaration_provenance>& declarations() const noexcept;
private:
  profile_reference profile_;
  profile_identity identity_;
  std::vector<declaration_provenance> declarations_;
};

/*! \brief Complete resolved requirement authority for one recipe. */
class sealed_requirement_set final {
public:
  [[nodiscard]] static sealed_requirement_set seal(
      std::vector<requirement_declaration> declarations,
      const profile_catalog& profiles);
  [[nodiscard]] const std::vector<resolved_requirement>& requirements() const noexcept;
  [[nodiscard]] std::vector<resolved_requirement> for_scope(
      const requirement_scope& scope) const;
  [[nodiscard]] const std::vector<selected_profile>& selected_build_profiles() const noexcept;
  [[nodiscard]] const std::vector<sealed_profile>& profile_closure() const noexcept;
private:
  sealed_requirement_set(std::vector<resolved_requirement> requirements,
                         std::vector<selected_profile> selected_build_profiles,
                         std::vector<sealed_profile> profile_closure);
  std::vector<resolved_requirement> requirements_;
  std::vector<selected_profile> selected_build_profiles_;
  std::vector<sealed_profile> profile_closure_;
};

} // namespace pkgsource
