# Contributing

Issues and pull requests are welcome. Releases, merges and workflow runs are maintainer-only, so a
pull request always needs approval before it lands.

## Building

Needs Docker running (no toolchain to install):

```sh
./build.sh          # produces immich-3ds.3dsx and immich-3ds.cia
```

The script cleans before building, so the artifact always matches the source. With devkitPro installed
natively, export `DEVKITARM` and run `make`; the `.cia` step needs
[makerom](https://github.com/3DSGuy/Project_CTR), which `build.sh` downloads by itself.

Optional: copy `.env.example` to `.env` to bake the server and API key into the binary. The resulting
`.cia` then contains your key, so do not share it.

## Tests

```sh
./tests/run.sh
```

Runs on the host, no emulator. Covers the EXIF date parser and the urlencoded parser behind the setup
page. It also writes the setup page to `/tmp` as HTML, which is how the screenshot in the README is
made.

## Demo assets

The GIFs and screenshots in `docs/` come from a build that drives the UI itself and fakes the upload:

```sh
make CFLAGS_EXTRA=-DDEMO
```

It walks through moving the cursor, opening the menu, sending the three photos of a day and showing the
result. Record it in an emulator and crop.

## Releasing

Maintainer only. Push a tag and CI runs the tests, compiles in the devkitPro container, packages the
`.cia` and publishes both binaries in a release:

```sh
git tag v1.0.1 && git push --tags
```

Every push and pull request builds the artifacts without publishing, so a broken build shows up as a
red check. The workflow also fails if a `.env`-style file is committed.
