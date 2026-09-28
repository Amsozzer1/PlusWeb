# Packaging

PlusWeb is in the vcpkg registry as
[`amsozzer1-plusweb`](https://github.com/microsoft/vcpkg/tree/master/ports/amsozzer1-plusweb),
so nothing here is needed to install it:

```bash
vcpkg install amsozzer1-plusweb
```

The files in `vcpkg/amsozzer1-plusweb/` are byte-identical to the ones upstream. They stay
here so a port change can be tested as an overlay before it is submitted:

```bash
vcpkg install amsozzer1-plusweb --overlay-ports=PlusWeb/packaging/vcpkg
```

Then, as usual:

```cmake
find_package(PlusWeb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE PlusWeb::PlusWeb)
```

`portfile.cmake` pins the release tarball by SHA512. If you point `REF` at a different tag,
`vcpkg install` will print the hash it actually got, and you paste that in.

The port is `amsozzer1-plusweb` rather than `plusweb` because vcpkg falls back to
`Owner-Project` when a name is already taken upstream.

## Conan, from a local recipe

Conan is not upstream, so this one really does come from here:

```bash
git clone https://github.com/Amsozzer1/PlusWeb.git
conan create PlusWeb/packaging/conan --build=missing
```

```python
def requirements(self):
    self.requires("plusweb/0.1.2")
```

This recipe builds from the checkout it lives in. A conan-center-index version would fetch a
release tarball through `conandata.yml` instead; that is the only difference.

## Not Windows yet

Both manifests declare that PlusWeb does not support Windows. Nothing in the library is
POSIX-bound — every socket goes through libuv — but it has never been built or tested
there, and saying so is better than shipping a port that fails in somebody's CI. Lifting it
is a matter of building it on Windows once and finding out.

## Still to do

Conan: restructure `conan/conanfile.py` around `conandata.yml` and open a PR against
[conan-io/conan-center-index](https://github.com/conan-io/conan-center-index) under
`recipes/plusweb/`.
