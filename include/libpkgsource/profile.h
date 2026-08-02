// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/** @file profile.h
 *  @brief Deterministic requirement-profile sealing and expansion authority.
 */
#pragma once

#include <libpkgsource/export.h>
#include <libpkgsource/model.h>

#include <vector>

namespace pkgsource {

/** @brief One direct package or nested-profile member declaration. */
class PKGSOURCE_API profile_member_declaration final {
public:
  /** Construct a profile member declaration.
   * @param subject Direct package or nested-profile subject.
   * @param provenance Exact member declaration site.
   */
  profile_member_declaration(requirement_subject subject,
                             declaration_provenance provenance);

  /** Return the declared member subject.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const requirement_subject& subject() const noexcept;

  /** Return member declaration provenance.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

private:
  requirement_subject subject_;
  declaration_provenance provenance_;
};

/** @brief Parser-neutral input for one named profile definition. */
class PKGSOURCE_API profile_declaration final {
public:
  /** Construct a profile declaration.
   * @param name Canonical profile name.
   * @param provenance Exact profile declaration site.
   * @param members Direct package and nested-profile members.
   */
  profile_declaration(profile_reference name,
                      declaration_provenance provenance,
                      std::vector<profile_member_declaration> members);

  /** Return the profile name.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const profile_reference& name() const noexcept;

  /** Return profile declaration provenance.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

  /** Return direct member declarations in input order.
   * @return Reference valid for the lifetime of this declaration.
   */
  [[nodiscard]] const std::vector<profile_member_declaration>&
  members() const noexcept;

private:
  profile_reference name_;
  declaration_provenance provenance_;
  std::vector<profile_member_declaration> members_;
};

/** @brief One retained edge in a transitive profile expansion path. */
class PKGSOURCE_API profile_expansion_step final {
public:
  /** Construct one expansion edge.
   * @param profile Profile whose direct member was traversed.
   * @param member Direct package or nested-profile member.
   * @param provenance Exact direct-member declaration site.
   */
  profile_expansion_step(profile_reference profile,
                         requirement_subject member,
                         declaration_provenance provenance);

  /** Return the issuing profile.
   * @return Reference valid for the lifetime of this step.
   */
  [[nodiscard]] const profile_reference& profile() const noexcept;

  /** Return the traversed direct member.
   * @return Reference valid for the lifetime of this step.
   */
  [[nodiscard]] const requirement_subject& member() const noexcept;

  /** Return direct-member declaration provenance.
   * @return Reference valid for the lifetime of this step.
   */
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

  /** Compare expansion steps for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const profile_expansion_step& lhs,
             const profile_expansion_step& rhs) noexcept;
  /** Compare expansion steps for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const profile_expansion_step& lhs,
             const profile_expansion_step& rhs) noexcept;
  /** Order expansion steps by profile, member, and provenance.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool
  operator<(const profile_expansion_step& lhs,
            const profile_expansion_step& rhs) noexcept;

private:
  profile_reference profile_;
  requirement_subject member_;
  declaration_provenance provenance_;
};

/** @brief One exact route from a selected profile to a package member. */
class PKGSOURCE_API profile_expansion_path final {
public:
  /** Construct a package expansion path.
   * @param package Package reached by the path.
   * @param steps Non-empty ordered traversal from selected root to package.
   * @throws error with error_code::invalid_profile for an invalid path.
   */
  profile_expansion_path(package_reference package,
                         std::vector<profile_expansion_step> steps);

  /** Return the reached package.
   * @return Reference valid for the lifetime of this path.
   */
  [[nodiscard]] const package_reference& package() const noexcept;

  /** Return ordered traversal steps.
   * @return Reference valid for the lifetime of this path.
   */
  [[nodiscard]] const std::vector<profile_expansion_step>&
  steps() const noexcept;

  /** Compare expansion paths for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const profile_expansion_path& lhs,
             const profile_expansion_path& rhs) noexcept;
  /** Compare expansion paths for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const profile_expansion_path& lhs,
             const profile_expansion_path& rhs) noexcept;
  /** Order expansion paths by package and traversal steps.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool
  operator<(const profile_expansion_path& lhs,
            const profile_expansion_path& rhs) noexcept;

private:
  package_reference package_;
  std::vector<profile_expansion_step> steps_;
};

/** @brief Immutable deterministically sealed profile authority. */
class PKGSOURCE_API sealed_profile final {
public:
  /** Construct a sealed profile value.
   *
   * This constructor exists for owner reconstruction. Ordinary callers should
   * use profile_catalog::seal().
   *
   * @param name Canonical profile name.
   * @param identity Verified semantic profile identity.
   * @param provenance Exact profile declaration site.
   * @param direct_members Canonical direct member declarations.
   * @param expansion Canonical transitive package expansion.
   */
  sealed_profile(profile_reference name,
                 profile_identity identity,
                 declaration_provenance provenance,
                 std::vector<profile_member_declaration> direct_members,
                 std::vector<profile_expansion_path> expansion);

  /** Return the profile name.
   * @return Reference valid for the lifetime of this profile.
   */
  [[nodiscard]] const profile_reference& name() const noexcept;

  /** Return the semantic profile identity.
   * @return Reference valid for the lifetime of this profile.
   */
  [[nodiscard]] const profile_identity& identity() const noexcept;

  /** Return profile declaration provenance.
   * @return Reference valid for the lifetime of this profile.
   */
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

  /** Return canonical direct members.
   * @return Reference valid for the lifetime of this profile.
   */
  [[nodiscard]] const std::vector<profile_member_declaration>&
  direct_members() const noexcept;

  /** Return canonical transitive package expansion.
   * @return Reference valid for the lifetime of this profile.
   */
  [[nodiscard]] const std::vector<profile_expansion_path>&
  expansion() const noexcept;

private:
  profile_reference name_;
  profile_identity identity_;
  declaration_provenance provenance_;
  std::vector<profile_member_declaration> direct_members_;
  std::vector<profile_expansion_path> expansion_;
};

/** @brief Complete deterministically sealed profile universe. */
class PKGSOURCE_API profile_catalog final {
public:
  /** Seal a complete profile declaration universe.
   * @param declarations All profile declarations for one authority universe.
   * @return Canonical catalog sorted by profile name.
   * @throws error for duplicate profiles, duplicate normalized members,
   *         unknown nested profiles, cycles, or otherwise invalid expansion.
   */
  [[nodiscard]] static profile_catalog
  seal(std::vector<profile_declaration> declarations);

  /** Return all sealed profiles in canonical order.
   * @return Reference valid for the lifetime of this catalog.
   */
  [[nodiscard]] const std::vector<sealed_profile>& profiles() const noexcept;

  /** Require one profile by exact reference.
   * @param profile Exact profile reference.
   * @return Sealed profile reference valid for the lifetime of this catalog.
   * @throws error with error_code::unknown_profile when absent.
   */
  [[nodiscard]] const sealed_profile&
  require(const profile_reference& profile) const;

private:
  explicit profile_catalog(std::vector<sealed_profile> profiles);
  std::vector<sealed_profile> profiles_;
};

/** @brief Provenance for one exact expanded package requirement. */
class PKGSOURCE_API requirement_origin final {
public:
  /** Construct one requirement origin.
   * @param declaration Original requirement declaration site.
   * @param expansion Empty for a direct package requirement, otherwise the
   *        exact selected-profile traversal that reached the package.
   */
  requirement_origin(declaration_provenance declaration,
                     std::vector<profile_expansion_step> expansion);

  /** Return original declaration provenance.
   * @return Reference valid for the lifetime of this origin.
   */
  [[nodiscard]] const declaration_provenance& declaration() const noexcept;

  /** Return the optional profile expansion path.
   * @return Reference valid for the lifetime of this origin.
   */
  [[nodiscard]] const std::vector<profile_expansion_step>&
  expansion() const noexcept;

  /** Compare requirement origins for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool operator==(const requirement_origin& lhs,
                                       const requirement_origin& rhs) noexcept;
  /** Compare requirement origins for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool operator!=(const requirement_origin& lhs,
                                       const requirement_origin& rhs) noexcept;
  /** Order requirement origins by declaration and expansion.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const requirement_origin& lhs,
                                      const requirement_origin& rhs) noexcept;

private:
  declaration_provenance declaration_;
  std::vector<profile_expansion_step> expansion_;
};

/** @brief One exact package requirement after profile expansion. */
class PKGSOURCE_API resolved_requirement final {
public:
  /** Construct one canonical resolved requirement.
   * @param scope Exact requirement scope.
   * @param package Exact required package.
   * @param origins All direct and profile-expanded issuing declarations.
   * @throws error with error_code::invalid_requirement when origins are empty.
   */
  resolved_requirement(requirement_scope scope,
                       package_reference package,
                       std::vector<requirement_origin> origins);

  /** Return the exact requirement scope.
   * @return Reference valid for the lifetime of this requirement.
   */
  [[nodiscard]] const requirement_scope& scope() const noexcept;

  /** Return the exact required package.
   * @return Reference valid for the lifetime of this requirement.
   */
  [[nodiscard]] const package_reference& package() const noexcept;

  /** Return all issuing origins in canonical order.
   * @return Reference valid for the lifetime of this requirement.
   */
  [[nodiscard]] const std::vector<requirement_origin>& origins() const noexcept;

  /** Compare resolved requirements for equality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when both values are equal.
   */
  friend PKGSOURCE_API bool
  operator==(const resolved_requirement& lhs,
             const resolved_requirement& rhs) noexcept;
  /** Compare resolved requirements for inequality.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when the values differ.
   */
  friend PKGSOURCE_API bool
  operator!=(const resolved_requirement& lhs,
             const resolved_requirement& rhs) noexcept;
  /** Order resolved requirements by scope, package, and origins.
   * @param lhs Left comparison operand.
   * @param rhs Right comparison operand.
   * @return `true` when @p lhs precedes @p rhs in canonical order.
   */
  friend PKGSOURCE_API bool operator<(const resolved_requirement& lhs,
                                      const resolved_requirement& rhs) noexcept;

private:
  requirement_scope scope_;
  package_reference package_;
  std::vector<requirement_origin> origins_;
};

/** @brief One selected build-profile root and its issuing declarations. */
class PKGSOURCE_API selected_profile final {
public:
  /** Construct a selected profile record.
   * @param profile Selected profile root.
   * @param identity Identity of that sealed profile.
   * @param declarations All recipe declarations that selected the profile.
   */
  selected_profile(profile_reference profile,
                   profile_identity identity,
                   std::vector<declaration_provenance> declarations);

  /** Return the selected profile reference.
   * @return Reference valid for the lifetime of this record.
   */
  [[nodiscard]] const profile_reference& profile() const noexcept;

  /** Return the selected profile identity.
   * @return Reference valid for the lifetime of this record.
   */
  [[nodiscard]] const profile_identity& identity() const noexcept;

  /** Return all issuing declarations in canonical order.
   * @return Reference valid for the lifetime of this record.
   */
  [[nodiscard]] const std::vector<declaration_provenance>&
  declarations() const noexcept;

private:
  profile_reference profile_;
  profile_identity identity_;
  std::vector<declaration_provenance> declarations_;
};

/** @brief Complete resolved requirement authority for one recipe. */
class PKGSOURCE_API sealed_requirement_set final {
public:
  /** Expand, normalize, and seal requirement declarations.
   * @param declarations Direct package and profile requirements.
   * @param profiles Complete sealed profile universe used for expansion.
   * @return Canonical resolved requirement authority.
   * @throws error for unknown profiles, duplicate normalized authority, or
   *         invalid requirements.
   */
  [[nodiscard]] static sealed_requirement_set
  seal(std::vector<requirement_declaration> declarations,
       const profile_catalog& profiles);

  /** Return all resolved package requirements in canonical order.
   * @return Reference valid for the lifetime of this set.
   */
  [[nodiscard]] const std::vector<resolved_requirement>&
  requirements() const noexcept;

  /** Select resolved requirements for one exact scope.
   * @param scope Scope to select.
   * @return Copy of matching requirements in canonical order.
   */
  [[nodiscard]] std::vector<resolved_requirement>
  for_scope(const requirement_scope& scope) const;

  /** Return selected build-profile roots.
   * @return Reference valid for the lifetime of this set.
   */
  [[nodiscard]] const std::vector<selected_profile>&
  selected_build_profiles() const noexcept;

  /** Return the exact sealed profile closure retained by this set.
   * @return Reference valid for the lifetime of this set.
   */
  [[nodiscard]] const std::vector<sealed_profile>&
  profile_closure() const noexcept;

private:
  sealed_requirement_set(std::vector<resolved_requirement> requirements,
                         std::vector<selected_profile> selected_build_profiles,
                         std::vector<sealed_profile> profile_closure);
  std::vector<resolved_requirement> requirements_;
  std::vector<selected_profile> selected_build_profiles_;
  std::vector<sealed_profile> profile_closure_;
};

} // namespace pkgsource
