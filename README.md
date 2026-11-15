# immich-3ds

Por [arthuregood](https://github.com/arthuregood).

Envia as fotos do Nintendo 3DS direto para o seu servidor [Immich](https://immich.app) — sem PC no meio, sem cartão SD na mão.

Um app de homebrew (`.cia` / `.3dsx`) que lê `sdmc:/DCIM`, mostra as fotos na tela do 3DS, e sobe as marcadas para o Immich usando a API oficial.

```
 3DS  ──POST /api/assets──▶  Immich  ──▶  app do iPhone/Android (backup normal)
```

## Por que não AirDrop

AirDrop não é possível no 3DS, e não é questão de criptografia: a descoberta roda sobre **AWDL**, um protocolo de rádio que exige monitor mode e injeção de frames 802.11 — coisas que o firmware fechado do 3DS não expõe, e que o menu do iPhone só aceita de quem fala AWDL. Reimplementações abertas como [OpenDrop](https://github.com/seemoo-lab/opendrop) e [OWL](https://github.com/seemoo-lab/owl) só existem em placas Wi-Fi com monitor mode ativo.

O caminho que funciona é o Immich: o 3DS sobe as fotos, e o iPhone as vê pelo app do Immich. O "vice-versa" já vem de graça.

## Status

- ✅ Envio das fotos do SD para o Immich (`.cia` e `.3dsx` compilando)
- ✅ Galeria de miniaturas por data na tela de baixo, foto inteira na de cima
- ✅ Menu de acoes (enviar esta / enviar todas do dia) e popup de progresso
- ✅ Banner e icone proprios no menu inicial
- ⬜ Navegar a biblioteca do Immich no 3DS e baixar para o SD
- ⬜ Seleção múltipla por data, álbum por origem

## Como funciona

1. Varre `sdmc:/DCIM/*/` procurando `.JPG` e `.MPO`
2. Lê o `DateTime` do EXIF de cada arquivo (o relógio do 3DS costuma estar errado; o EXIF é a fonte confiável)
3. `POST /api/assets` multipart com `assetData` + `fileCreatedAt` + `fileModifiedAt` + `filename`, autenticado por `x-api-key`

O upload é feito em streaming direto do SD (`curl_mime`), então não carrega a foto na RAM.

A grade da tela de baixo decodifica os JPEG com `libjpeg-turbo` em escala reduzida (1/4 via DCT scaling, não 640x480 inteiro) e guarda 6 thumbs em cache de 106x80.

## Compilar

### Com Docker (não precisa instalar nada)

```sh
./build.sh
# gera immich-3ds.cia e immich-3ds.3dsx
```

O script compila no container `devkitpro/devkitarm` e monta o `.cia` com o makerom num `ubuntu:24.04` (a imagem do devkitpro tem glibc antiga demais pro makerom atual). Em Mac com Apple Silicon usa `--platform linux/amd64`.

### Com devkitPro instalado

```sh
export DEVKITARM=/opt/devkitpro/devkitARM
export DEVKITPRO=/opt/devkitpro
make -j$(nproc)
```

Pacotes necessários: `3ds-dev` (libctru), `3ds-curl`, `3ds-mbedtls`, `3ds-zlib`.

Depois, o `.cia`:

```sh
makerom -f cia -o immich-3ds.cia -elf immich-3ds.elf -icon immich-3ds.smdh \
        -desc app:2 -rsf app.rsf -target t
```

## Instalar

- **`.cia`**: copie para o SD (ex: `sdmc:/cias/`) e instale com o **FBI**. Aparece no menu inicial.
- **`.3dsx`**: copie para `sdmc:/3ds/immich-3ds/` e abra pelo Homebrew Launcher.

## Configurar

Tres formas, da mais confortavel para a mais manual. A primeira vez que o app abre sem
configuracao, ele ja cai no modo navegador.

### 1. Pelo navegador (sem digitar nada no console)

Se o app nao achar configuracao, ele sobe um servidor HTTP e mostra o endereco na tela de
cima:

```
Set up in your browser
http://192.168.1.254:8080
```

Abra esse endereco no celular ou no PC (mesma rede), preencha servidor, API key e fuso, e
toque em **Enviar pro 3DS**. O app salva em `sdmc:/immich-3ds/config.txt`, testa a conexao
e volta para a galeria.

O mesmo modo existe depois de configurado: **SELECT → Setup in browser**.

### 2. Arquivo no cartao SD

Crie `sdmc:/immich-3ds/config.txt` (ou `.env`, os dois funcionam) com:

```
server=http://192.168.0.10:2283
apikey=sua_api_key
tz=-03:00
lang=pt
```

Os nomes `IMMICH_SERVER`, `IMMICH_API_KEY` e `IMMICH_TZ` tambem valem, se voce preferir o
estilo `.env`. Só `server` e `apikey` sao obrigatorios. `lang` (`pt`/`en`) forca o idioma.

### 3. Teclado do 3DS

**SELECT → Setup with 3DS keyboard**. A key tem ~60 caracteres, entao isso e util para
corrigir um campo, nao para a primeira configuracao.

## Teclas

| Tecla | Ação |
|---|---|
| D-pad ↑ ↓ ← → | navegar na lista |
| A / toque | abrir o menu (enviar / configurar) |
| Y | marcar todas |
| X | limpar marcação |
| START | enviar marcadas |
| SELECT | configurar |
| B | sair |

## Limitações conhecidas

- Só **envia**; baixar do Immich para o 3DS ainda não existe.
- Lê apenas `sdmc:/DCIM` (fotos da câmera e de jogos que salvam ali).
- `.MPO` (foto 3D) sobe como JPEG: o Immich mostra o primeiro frame e preserva o original.
- TLS 1.2 é o teto do mbedtls 2.x — um servidor exigindo só TLS 1.3 não vai conectar.
- A API key fica em texto plano no SD (ou compilada no binário, se você usou `.env`). Use uma key dedicada a este app, para poder revogar sem afetar os outros clientes.

## Icone e banner

O menu inicial mostra o icone do app (SMDH) e, ao selecionar, um banner estatico na tela
de cima. Nao ha modelo 3D: isso exige um banner CGfx/CWAV que o homebrew nao usa — o
banner aqui e uma imagem 256x128.

Os arquivos ficam em `meta/` e ja estao versionados (`banner.bnr` pronto), entao o build
nao precisa de ferramenta extra. Para regerar depois de trocar `banner.png`:

```sh
bannertool makebanner -i meta/banner.png -a meta/silence.wav -o meta/banner.bnr
```

O `bannertool` exige uma faixa de audio (o formato do banner inclui um CWAV), por isso o
`silence.wav` de 0,1 s — resulta em silencio, sem musica. Se o menu inicial travar ao
selecionar o app, o banner e o culpado: rode `build.sh` sem `-banner` em uma linha.

A logo da Immich pertence ao projeto [Immich](https://immich.app). Este app **nao e
oficial** nem afiliado; a logo e usada apenas para identificar o servico de destino.

## Testes

A lógica pura (parser EXIF, formatação de data) roda no host:

```sh
./tests/run.sh
```

## Créditos

[devkitPro](https://devkitpro.org) / libctru, [curl](https://curl.se) + [mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/), [makerom](https://github.com/3DSGuy/Project_CTR), e a equipe do [Immich](https://immich.app) pela API.

## Licença

MIT — veja [LICENSE](LICENSE).

Autor: [arthuregood](https://github.com/arthuregood).
