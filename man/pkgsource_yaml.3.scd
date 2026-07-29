pkgsource_yaml(3)

# NAME

pkgsource_yaml - strict native profiles.yml/1 and recipe YAML frontend

# SYNOPSIS

*#include <libpkgsource-yaml/libpkgsource-yaml.h>*

*pkgsource::yaml_adapter::parsed_profile_document*
*parse_profiles_yaml_v1(std::string_view bytes,*
*    pkgsource::source_origin origin);*

*pkgsource::yaml_adapter::parsed_recipe_document*
*parse_recipe_yaml_v1(std::string_view bytes,*
*    pkgsource::source_origin origin);*

*pkgsource::yaml_adapter::parsed_recipe_document*
*parse_recipe_yaml_v2(std::string_view bytes,*
*    pkgsource::source_origin origin);*

# DESCRIPTION

*libpkgsource-yaml* parses one strict native YAML document from caller-owned
bytes.  It produces parser-neutral *profile_declaration* or
*recipe_declaration* values.  Parsed documents are not authority.

*seal_profiles_yaml_v1()* invokes *profile_catalog::seal()*.
*seal_recipe_yaml_v1()* and *seal_recipe_yaml_v2()* invoke *seal_source()* with
an already sealed profile catalog.  Only those sealed native values cross the
semantic authority boundary.

Version one preserves the published no-check-program protocol.  Version two
adds one optional exact check program using the same *language* and *script*
shape as other programs.  Version-two check requirements without that program
are semantic errors from *seal_source()*.

The adapter does not open paths, discover collections, combine profile
documents, choose precedence, resolve requirements, fetch sources, or execute
programs.

# STRICTNESS

The adapter rejects multiple documents, YAML directives, duplicate or unknown
mapping keys, non-scalar mapping keys, anchors, aliases, merge keys, custom or
kind-incompatible tags, scalar requirement shorthand, and schema/type drift.

Standard string, integer, sequence, and mapping tags are accepted only where
their node kind matches the schema.  YAML scalar typing does not infer package
semantics.

# ERRORS

Syntax and schema failures throw *yaml_error*.  Branch on *yaml_error_code*.
The error retains the source document, schema path, and one-based line and
column.

Values rejected while constructing parser-neutral declarations are reported as
*yaml_error* with code *invalid_value* at the exact input location.  Failures
from *profile_catalog::seal()* or *seal_source()* remain *pkgsource::error*
values; the adapter does not translate sealer error codes.

# ABI

*libpkgsource-yaml.so.2* is an optional library.  Its public headers do not
expose libyaml types.  The core *libpkgsource.so.2* has no libyaml dependency.

# SEE ALSO

*libpkgsource*(3), *pkgsource_profile*(3), *pkgsource_recipe*(3),
*pkgsource_profiles_yaml*(5), *pkgsource_recipe_yaml*(5)
