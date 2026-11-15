#!/usr/bin/env bash
# Roda os testes de logica pura no host (sem devkitPro, sem 3DS).
set -euo pipefail
cd "$(dirname "$0")/.."

python3 tests/make_fixtures.py
cc -std=c11 -Wall -Wextra -O1 -Isource -o /tmp/test_exif tests/test_exif.c source/exif.c
/tmp/test_exif
cc -std=c11 -Wall -Wextra -O1 -Isource -o /tmp/test_form tests/test_form.c source/form.c
/tmp/test_form
cc -std=c11 -Wall -Wextra -O1 -Isource -o /tmp/render_pages tests/render_pages.c source/page.c
/tmp/render_pages
