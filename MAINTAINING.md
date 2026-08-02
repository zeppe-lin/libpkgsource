# Maintaining

## Release authority

A release is defined by one signed repository tag. `libpkgsource` and
`libpkgsource-codec` share the project version because the durable codec knows
the exact owner model. Their SONAMEs and pkg-config modules remain independent.

The release candidate must pass GCC and Clang shared and static builds, warnings
as errors, ASan and UBSan, generated metadata checks, installed consumers,
manual regeneration and lint, Doxygen, HTML validation, staged installation,
shared-object closure inspection, exact ABI manifests, and independent
`git am` replay.

## Identity discipline

Package-release, profile, and source-snapshot identities are owner protocols.
Changing participating fields, normalization, tag values, domain strings, or
framing requires protocol review and fixed-vector updates. Never silently
reinterpret an existing identity domain.

The SHA-256 implementation is private. A new provider must pass the same fixed
vectors and state-machine tests before selection becomes configurable. Replacing
SHA-256 itself requires new identity and record protocol versions.

## Durable records

`docs/protocols/source-records-v1.md` is normative. Schema 1 accepts no unknown
or trailing fields. Any codec change must either prove that schema-one bytes are
unchanged or introduce a new outer schema with new vectors and migration policy.

The intrinsic checksum, owner resealing, stored-identity comparison, and
canonical re-encoding are independent checks. A future content-addressed store
does not replace any of them.

## ABI maintenance

The reviewed manifests are:

```text
abi/libpkgsource.exports
abi/libpkgsource-codec.exports
```

Never regenerate them automatically from compiler output. Compare intended
exports under both GCC and Clang. Additive or incompatible public changes
require an explicit SONAME decision; private templates and provider symbols must
remain hidden.

## Documentation publication

Canonical project documents install under `share/doc/libpkgsource`. Generated
HTML installs under `share/htmldocs/libpkgsource/<version>` only when explicitly
enabled. The site publishes that versioned tree without rebuilding or
reinterpreting owner documentation.

## Release checklist

1. run both compiler and linkage matrices;
2. run sanitizer matrices under both compilers;
3. verify core and codec metadata and installed consumers;
4. compare both dynamic symbol sets with their manifests;
5. inspect SONAME and `NEEDED` closure;
6. regenerate and lint all manuals;
7. build and validate Doxygen and HTML output;
8. stage installation through `DESTDIR` and run install contracts;
9. replay the complete patch series onto the recorded base;
10. tag only after every owner and downstream acceptance gate is green.
