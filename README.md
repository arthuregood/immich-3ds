# immich-3ds

**English** · [Português](README.pt-BR.md)

Send the photos from your Nintendo 3DS straight to your [Immich](https://immich.app) server — no PC in the middle.

![Demo: browsing the gallery and uploading the photos of one day](docs/demo.gif)

## What it does

- Scans `sdmc:/DCIM`, grouped by day, newest first
- Full photo on the top screen, thumbnail grid on the bottom one
- Upload one photo or every photo from that day, with a progress popup
- **No typing on the console**: first run opens a setup page in your phone's browser
- Interface language follows the console (Portuguese or English)
- Own icon and banner in the Home Menu

| Gallery | Actions menu |
|---|---|
| ![Gallery](docs/screenshot-gallery.png) | ![Menu](docs/screenshot-menu.png) |

## Why not AirDrop

AirDrop is not possible on a 3DS, and it is not about encryption: discovery runs over **AWDL**, a
radio protocol that needs monitor mode and raw frame injection — things the 3DS closed firmware
does not expose, and the iPhone's share sheet only lists peers that speak AWDL. Open
reimplementations like [OpenDrop](https://github.com/seemoo-lab/opendrop) only work on Wi-Fi cards
with active monitor mode.

Immich is the path that does work: the 3DS uploads, and the iPhone sees it in the Immich app. The
other direction comes for free.

## Requirements

- 3DS/2DS with [Luma3DS](https://github.com/LumaTeam/Luma3DS) and FBI
- An Immich server reachable from the console (same network, HTTP or HTTPS)
- An Immich API key (*Account Settings → API Keys*)

## Install

Grab `immich-3ds.cia` from the [releases](../../releases) page, copy it to your SD card
(for example `sdmc:/cias/`) and install it with **FBI**. It shows up in the Home Menu.

Alternatively, build from source and use the `.3dsx` through the Homebrew Launcher.

## Setup

### First run: from your phone or PC

With no configuration, the app starts a small web server and shows the address on the top screen:

![Setup page](docs/screenshot-web.png)

Open that address, fill in the server, the API key and the timezone, and hit **Send to 3DS**. The app
saves it to `sdmc:/immich-3ds/.env`, tests the connection and goes back to the gallery.

The same mode is available later: **SELECT → Setup in browser**.

### Or: a `.env` file on the SD card

Create `sdmc:/immich-3ds/.env`:

```
IMMICH_SERVER=http://192.168.0.10:2283
IMMICH_API_KEY=your_api_key
IMMICH_TZ=-03:00
lang=pt
```

Only `IMMICH_SERVER` and `IMMICH_API_KEY` are required. `lang` (`pt`/`en`) forces the language.
The app rewrites this same file whenever you configure it from the screen or the browser.

### Or: the 3DS keyboard

**SELECT → Setup with 3DS keyboard**. The key is ~60 characters, so this is handy for fixing a field,
not for a first setup.

## Using it

| Input | Action |
|---|---|
| D-pad ← → ↑ ↓ | move through the photos |
| A / touch | open the menu (send this photo, send all from this day) |
| SELECT | settings |
| B | exit |

The footer shows the server and its status: `online v3.4.1` or `offline` (checked on boot and after
saving the settings).

## Limitations

- Upload only; downloading from Immich to the 3DS does not exist yet
- Reads only `sdmc:/DCIM` (camera photos and games that save there)
- `.MPO` (3D photos) are uploaded as JPEG: Immich shows the first frame and keeps the original
- TLS 1.2 is the ceiling of the bundled mbedTLS — a server that only accepts TLS 1.3 will not connect
- The API key sits in plain text on the SD card. Use a dedicated key so you can revoke it alone

## Building

```sh
./build.sh          # builds .3dsx and .cia with Docker (no devkitPro needed)
```

With devkitPro installed natively, `make` uses `$DEVKITARM`; the `.cia` step needs
[makerom](https://github.com/3DSGuy/Project_CTR).

Optional: copy `.env.example` to `.env` to bake the server and key into the binary at build time
(handy for distributing a pre-configured build — but then the `.cia` contains your key).

## Regenerating the demo assets

The GIF in this README was recorded with a build flag that drives the UI by itself and fakes the
upload (no server needed):

```sh
make CFLAGS_EXTRA=-DDEMO
```

## Tests

```sh
./tests/run.sh
```

Covers the EXIF date parser and the urlencoded parser used by the setup page. Both are compiled for
the host, no emulator needed.

## Credits and legal notes

- Built with [devkitPro](https://devkitpro.org) / libctru, [curl](https://curl.se) +
  [mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/), [libjpeg-turbo](https://libjpeg-turbo.org),
  and [makerom](https://github.com/3DSGuy/Project_CTR).
- The Immich name and logo belong to the [Immich](https://immich.app) project. This app is **not
  official** nor affiliated with it; the logo is here only to identify the service you connect to.
- The sample images in `docs/` are fan memes used as **test data only**. Characters and artwork are
  trademarks of **Nintendo**; this project is not affiliated with, endorsed or sponsored by Nintendo.
  If you fork this, swap those images for something you own.

## License

MIT — see [LICENSE](LICENSE).

By [arthuregood](https://github.com/arthuregood).
