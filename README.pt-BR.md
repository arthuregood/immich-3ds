# immich-3ds

[![build](https://github.com/arthuregood/immich-3ds/actions/workflows/build.yml/badge.svg)](https://github.com/arthuregood/immich-3ds/actions/workflows/build.yml)

[English](README.md) · **Português**

Manda as fotos do cartão SD do seu 3DS para o [Immich](https://immich.app). Roda no console e fala direto com a API do Immich — sem PC, sem leitor de cartão.

![Navegando pela galeria e enviando as fotos de um dia](docs/demo-pt.gif)

## Como funciona

O app lista o que está em `sdmc:/DCIM`, agrupado por dia, do mais recente para o mais antigo. A tela de cima mostra a foto selecionada, a de baixo é a grade de miniaturas. Aperte A para enviar aquela foto, ou escolha "enviar todas deste dia".

A data vem do EXIF do JPEG, não do timestamp do arquivo — o relógio do 3DS atrasa, a data da foto não.

| Galeria | Ações |
|---|---|
| ![Galeria](docs/screenshot-gallery-pt.png) | ![Menu](docs/screenshot-menu-pt.png) |

No Immich elas chegam como qualquer outra foto, com a câmera identificada como Nintendo 3DS:

| Álbum | Detalhes da foto |
|---|---|
| ![Álbum](docs/screenshot-immich-album.png) | ![Detalhes](docs/screenshot-immich-info.png) |

## O que você precisa

- Um 3DS ou 2DS com [Luma3DS](https://github.com/LumaTeam/Luma3DS) e FBI
- Um servidor Immich que o console alcance na rede
- Uma API key de *Account Settings → API Keys*

## Instalar

Baixe `immich-3ds.cia` nas [releases](../../releases), copie para o cartão SD (ex: `sdmc:/cias/`) e instale pelo FBI. Ele aparece no menu inicial.

## Primeira configuração

Não precisa digitar nada no console. Na primeira vez o app sobe um pequeno servidor web e mostra o endereço na tela de cima — abra no celular, preencha a URL do servidor e a API key, envie.

![Página de configuração](docs/screenshot-web-pt.png)

Se preferir não usar o navegador, escreva o arquivo na mão:

```
sdmc:/immich-3ds/.env

IMMICH_SERVER=http://192.168.0.10:2283
IMMICH_API_KEY=sua_key
IMMICH_TZ=-03:00
lang=pt
```

Só as duas primeiras são obrigatórias. Os dois caminhos também estão em **SELECT → configurar**, junto com o teclado do 3DS para quando você só quer corrigir um campo.

## Controles

| Tecla | Ação |
|---|---|
| D-pad | navegar pelas fotos |
| A / toque | abrir o menu |
| SELECT | configuração |
| B | sair |

O rodapé mostra o servidor e se ele respondeu: `online v3.4.1` ou `offline`. A checagem roda no boot e depois de salvar a configuração.

## Bom saber

- Por enquanto só envia; baixar do Immich para o 3DS não existe
- Lê apenas `sdmc:/DCIM`
- `.MPO` (fotos 3D) sobe como JPEG — o Immich guarda o arquivo original
- TLS 1.2 é o limite do mbedTLS embarcado
- A API key fica em texto plano no cartão, então use uma que você possa revogar sozinha

## Créditos

Feito com [devkitPro](https://devkitpro.org) / libctru, [curl](https://curl.se) + [mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/) e [libjpeg-turbo](https://libjpeg-turbo.org).

O nome e a logo da Immich pertencem ao projeto Immich; este app não é oficial nem afiliado.

As imagens de exemplo em `docs/` são memes de fã usados como dado de teste. Os personagens são marcas da Nintendo, e este projeto não é afiliado, endossado ou patrocinado pela Nintendo.

## Licença

MIT — veja [LICENSE](LICENSE). Instruções de build, testes e processo de release estão no [CONTRIBUTING.md](CONTRIBUTING.md).

Por [arthuregood](https://github.com/arthuregood).
