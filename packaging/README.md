# Packaging

PlusWeb is not in the vcpkg or Conan registries yet. These files let you install it from
here today, and they are the same files that go upstream when it is.

## vcpkg, as an overlay port

```bash
git clone https://github.com/Amsozzer1/PlusWeb.git
vcpkg install plusweb --overlay-ports=PlusWeb/packaging/vcpkg
```

Then, as usual:

```cmake
find_package(PlusWeb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE PlusWeb::PlusWeb)
```

`portfile.cmake` pins the release tarball by SHA512. If you point `REF` at a different tag,
`vcpkg install` will print the hash it actually got, and you paste that in.

## Conan, from a local recipe

```bash
git clone https://github.com/Amsozzer1/PlusWeb.git
conan create PlusWeb/packaging/conan --build=missing
```

```python
def requirements(self):
    self.requires("plusweb/0.1.1")
```

This recipe builds from the checkout it lives in. The conan-center-index version fetches a
release tarball through `conandata.yml` instead; that is the only difference.

## Not Windows yet

Both manifests declare that PlusWeb does not support Windows. Nothing in the library is
POSIX-bound — every socket goes through libuv — but it has never been built or tested
there, and saying so is better than shipping a port that fails in somebody's CI. Lifting it
is a matter of building it on Windows once and finding out.

## Going upstream

- vcpkg: copy `vcpkg/plusweb/` into `ports/plusweb/` in a fork of
  [microsoft/vcpkg](https://github.com/microsoft/vcpkg), run
  `vcpkg x-add-version plusweb`, and open the PR.
- Conan: restructure `conan/conanfile.py` around `conandata.yml` and open a PR against
  [conan-io/conan-center-index](https://github.com/conan-io/conan-center-index) under
  `recipes/plusweb/`.
