# ELF ABI policy

## Public libraries

The repository publishes two independent ELF ABIs:

```text
libpkgsource.so.4
libpkgsource-codec.so.2
```

The core does not import the codec. The codec imports the matching core ABI.

## Canonical manifests

Both libraries compile with hidden visibility. Public declarations carry an
explicit export macro. The reviewed manifests are:

```text
abi/libpkgsource.exports
abi/libpkgsource-codec.exports
```

Meson generates anonymous GNU ld scripts from those manifests. The generated
script is mechanism; the manifest is policy. Namespace wildcards and automatic
manifest regeneration are forbidden because compiler output cannot decide what
the project intends to promise.

## Versioning

The manifests control symbol visibility; they do not create premature named
symbol-version nodes. SONAMEs express the current binary compatibility
boundaries.

An additive or incompatible public change requires explicit review of headers,
object lifetimes, exception types, mangled symbols, pkg-config closure, and the
appropriate SONAME. A semantic or record protocol change may require a new
identity or record version even when the C++ ABI itself remains compatible.

## Qualification

Shared builds compare the exact dynamic symbol set with the corresponding
manifest under GCC and Clang. Tests also inspect SONAMEs and `NEEDED` entries,
compile installed consumers from each umbrella header, and reject private
provider, framing, codec, or STL-instantiation exports.
