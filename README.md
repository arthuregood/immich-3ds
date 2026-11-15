# immich-3ds

[![build](https://github.com/arthuregood/immich-3ds/actions/workflows/build.yml/badge.svg)](https://github.com/arthuregood/immich-3ds/actions/workflows/build.yml)

**English** · [Português](README.pt-BR.md)

Uploads the photos on your 3DS SD card to [Immich](https://immich.app). It runs on the console and talks to the Immich API directly, so there is no PC in the middle.

![Browsing the gallery and sending the photos of one day](docs/demo.gif)

## How it works

The app lists what is in `sdmc:/DCIM`, grouped by day, newest first. You can upload one photo or everything from the same date.

Dates come from the JPEG's EXIF, not from the file timestamp. 3DS clocks drift; the date inside the photo does not.

| Gallery | Actions |
|---|---|
| ![Gallery](docs/screenshot-gallery.png) | ![Menu](docs/screenshot-menu.png) |

In Immich they arrive as normal photos, with the camera reported as Nintendo 3DS:

| Album | Photo details |
|---|---|
| ![Album](docs/screenshot-immich-album.png) | ![Details](docs/screenshot-immich-info.png) |

## What you need

- A 3DS or 2DS with [Luma3DS](https://github.com/LumaTeam/Luma3DS) and FBI
- An Immich server reachable from the console
- An API key from *User Settings → API Keys*

## Install

Download `immich-3ds.cia` from [releases](../../releases), copy it to the SD card (e.g. `sdmc:/cias/`) and install it with FBI. The same release has a `.3dsx` if you prefer the Homebrew Launcher.

To update later, install the newer `.cia` the same way. It replaces the app and keeps the settings that are on the SD card.

### Without the SD card

FBI can install straight from the console. Open **Remote Install → Scan QR Code** and point the camera
at this:

![QR code with the direct download link](docs/install-qr.png)

Hosted outside GitHub, so the link may expire; if it does, use the `.cia` from the release.

## First setup

There is nothing to type on the console. On first launch the app starts a small web server and shows the address on the top screen. Open it on your phone, fill in the server URL and the API key, send.

![Setup page](docs/screenshot-web.png)

Nothing leaves your network here: the page is served by the console itself, so the key goes straight from your browser to the 3DS, no cloud in between.

It has no password, and it shows the key you already saved, so press B on the console when you are done. That closes the server.

If you would rather not use the browser, write the file yourself:

```
sdmc:/immich-3ds/.env

IMMICH_SERVER=http://192.168.0.10:2283
IMMICH_API_KEY=your_key
IMMICH_TZ=-03:00          # optional, defaults to -03:00; your offset from UTC
lang=en                   # optional; without it the app follows the console language
```

Only the server and the key are required. Both routes are also under **SELECT → setup**, together with the 3DS keyboard for when you just need to fix one field.

## If it does not work

- **The setup page does not open.** That address only works from the same network. You can find the console IP in *System Settings → Internet Settings → Connection Test*.
- **The footer says `offline`.** Wi-Fi is up but Immich did not answer. Test the same URL in your phone's browser.
- **`could not open the port (wifi off?)`.** Port 8080 is taken or Wi-Fi dropped. Try **SELECT → setup → browser** again.
- **`no photos in sdmc:/DCIM`.** The photos are not where the app looks; see the folder rule below.

## Controls

| Input | Action |
|---|---|
| D-pad | move through the photos |
| A / touch | open the menu |
| SELECT | setup |
| B | exit |

The footer shows the server and whether it answered: `online` plus the server version, or `offline`. It checks on boot and after you save the settings.

## Good to know

- Upload only; there is no download from Immich to the 3DS yet
- Reads `sdmc:/DCIM` one folder deep, so photos must sit in a subfolder like `sdmc:/DCIM/100NIN03/`
- Up to 512 photos are listed, the rest is ignored
- `.MPO` (3D photos) uploads as JPEG; Immich keeps the original
- The certificate is **not verified** unless you put your CA at `sdmc:/immich-3ds/ca.pem` (for Let's Encrypt, the ISRG Root X1 file)
- TLS 1.2 is the limit of the bundled mbedTLS
- The API key is plain text on the SD card, so use one you can revoke alone

## Credits

Built with [devkitPro](https://devkitpro.org) / libctru, [curl](https://curl.se) + [mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/) and [libjpeg-turbo](https://libjpeg-turbo.org).

The Immich name and logo belong to the Immich project. This app is not official and not affiliated with it.

The sample images in `docs/` are fan memes used as test data. Characters are trademarks of Nintendo, and this project is not affiliated with, endorsed or sponsored by Nintendo.

## License

MIT, see [LICENSE](LICENSE). Build instructions, tests and the release process are in [CONTRIBUTING.md](CONTRIBUTING.md).

By [arthuregood](https://github.com/arthuregood).
