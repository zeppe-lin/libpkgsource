pkgsource_profiles_yaml(5)

# NAME

pkgsource_profiles_yaml - native profiles.yml/1 input syntax

# DESCRIPTION

A *profiles.yml/1* document declares named authoritative requirement profiles
for later sealing by *libpkgsource*.

The root mapping contains exactly:

```
format: zeppe-lin.profiles/1
profiles:
  toolchain:
    members:
      - package: binutils
      - profile: "@compiler"
```

*format* and *profiles* are required.  Profile definition keys omit the leading
*@*.  Every profile contains one non-empty *members* sequence.  Each member is
an explicit one-key *package* or *profile* mapping; scalar shorthand is invalid.

The parser retains declaration positions.  The native profile sealer rejects
non-canonical names, duplicate definitions or members, unknown nested profiles,
and cycles.  It computes deterministic expansion and semantic identities.

# AUTHORITY

The YAML document is input syntax.  Only a sealed *profile_catalog* is
authority.  File discovery, collection precedence, and aggregation of multiple
profile documents are outside this format.

# SEE ALSO

*pkgsource_yaml*(3), *pkgsource_profile*(3), *pkgsource_recipe_yaml*(5)
