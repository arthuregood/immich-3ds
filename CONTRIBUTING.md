# Contributing

This is a small homebrew project. Issues and pull requests are welcome; keep in mind that **releases
are published only by the maintainer** — please do not tag or publish.

## Building

Quickest path, no toolchain installed (needs Docker):

```sh
./build.sh          # produces immich-3ds.3dsx and immich-3ds.cia
```

With devkitPro installed natively you can use `make` directly; the `.cia` step needs
[makerom](https://github.com/3DSGuy/Project_CTR) (the Docker path in `build.sh` does both).

Optional: copy `.env.example` to `.env` to bake the server and API key into the binary at build time.
Handy for a pre-configured build; remember the resulting `.cia` then contains your key, so never share
it.

## Tests

```sh
./tests/run.sh
```

Runs on the host, no emulator needed. It covers the two pieces of logic that are worth testing in
isolation: the EXIF date parser (`source/exif.c`) and the urlencoded parser behind the setup page
(`source/form.c`). The same script also regenerates the setup page as HTML in `/tmp`, which is how the
screenshot in the README is made.

## Regenerating the demo assets

The GIFs and screenshots in `docs/` come from a build flag that drives the UI by itself and fakes the
upload (no server needed):

```sh
make CFLAGS_EXTRA=-DDEMO
```

Run it in an emulator, record the window, and crop. The demo walks through: moving the cursor, opening
the menu, sending the three photos of a day, and showing the result.

## Releasing (maintainer only)

Push a tag; CI runs the tests, compiles inside the devkitPro container, packages the `.cia` and
publishes both binaries in a GitHub release.

```sh
git tag v1.0.1 && git push --tags
```

Every push and pull request also builds the artifacts without publishing, so a broken build shows up as
a red check. The workflow also fails if any `.env`-style file is committed.
