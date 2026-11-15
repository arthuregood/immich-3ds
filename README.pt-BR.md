# immich-3ds

[![build](https://github.com/arthuregood/immich-3ds/actions/workflows/build.yml/badge.svg)](https://github.com/arthuregood/immich-3ds/actions/workflows/build.yml)

[English](README.md) · **Português**

Manda as fotos do seu Nintendo 3DS direto para o seu servidor [Immich](https://immich.app) — sem PC no meio.

![Demo: navegando pela galeria e enviando as fotos de um dia](docs/demo-pt.gif)

## O que ele faz

- Varre `sdmc:/DCIM`, agrupado por dia, do mais recente para o mais antigo
- Foto inteira na tela de cima, grade de miniaturas na de baixo
- Envia uma foto ou todas do dia, com popup de progresso
- **Sem digitar nada no console**: na primeira vez ele abre uma página de configuração no navegador do celular
- Idioma da interface segue o console (português ou inglês)
- Ícone e banner próprios no menu inicial

| Galeria | Menu de ações |
|---|---|
| ![Galeria](docs/screenshot-gallery-pt.png) | ![Menu](docs/screenshot-menu-pt.png) |

## Requisitos

- 3DS/2DS com [Luma3DS](https://github.com/LumaTeam/Luma3DS) e FBI
- Um servidor Immich acessível pelo console (mesma rede, HTTP ou HTTPS)
- Uma API key do Immich (*Account Settings → API Keys*)

## Instalar

Baixe `immich-3ds.cia` na página de [releases](../../releases), copie para o cartão SD
(por exemplo `sdmc:/cias/`) e instale com o **FBI**. Ele aparece no menu inicial.

## Configurar

### Primeira vez: pelo celular ou PC

Sem configuração, o app sobe um pequeno servidor web e mostra o endereço na tela de cima:

![Página de configuração](docs/screenshot-web-pt.png)

Abra esse endereço, preencha servidor, API key e fuso, e toque em **Enviar pro 3DS**. O app salva em
`sdmc:/immich-3ds/.env`, testa a conexão e volta para a galeria.

O mesmo modo existe depois de configurado: **SELECT → Configurar pelo navegador**.

### Ou: um arquivo `.env` no cartão

Crie `sdmc:/immich-3ds/.env`:

```
IMMICH_SERVER=http://192.168.0.10:2283
IMMICH_API_KEY=sua_api_key
IMMICH_TZ=-03:00
lang=pt
```

Só `IMMICH_SERVER` e `IMMICH_API_KEY` são obrigatórios. `lang` (`pt`/`en`) força o idioma.
O app reescreve esse mesmo arquivo quando você configura pela tela ou pelo navegador.

### Ou: o teclado do 3DS

**SELECT → Configurar no teclado**. A key tem ~60 caracteres, então isso serve para corrigir um
campo, não para a primeira configuração.

## Usando

| Tecla | Ação |
|---|---|
| D-pad ← → ↑ ↓ | navegar pelas fotos |
| A / toque | abrir o menu (enviar esta foto, enviar todas do dia) |
| SELECT | configuração |
| B | sair |

O rodapé mostra o servidor e o estado: `online v3.4.1` ou `offline` (checado no boot e ao salvar a
configuração).

## Limitações

- Só envia; baixar do Immich para o 3DS ainda não existe
- Lê apenas `sdmc:/DCIM` (fotos da câmera e jogos que salvam ali)
- `.MPO` (fotos 3D) sobem como JPEG: o Immich mostra o primeiro frame e preserva o original
- TLS 1.2 é o teto do mbedTLS embarcado — servidor que só aceita TLS 1.3 não conecta
- A API key fica em texto plano no cartão. Use uma key dedicada, para revogar só ela

## Créditos e notas legais

- Feito com [devkitPro](https://devkitpro.org) / libctru, [curl](https://curl.se) +
  [mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/) e
  [libjpeg-turbo](https://libjpeg-turbo.org).
- O nome e a logo da Immich pertencem ao projeto [Immich](https://immich.app). Este app **não é
  oficial** nem afiliado; a logo está aqui só para identificar o serviço que você conecta.
- As imagens de exemplo em `docs/` são memes de fã usados **apenas como dado de teste**. Personagens e
  artes são marcas da **Nintendo**; este projeto não é afiliado, endossado ou patrocinado pela Nintendo.
  Se você fizer um fork, troque essas imagens por algo seu.

## Licença e contribuição

MIT — veja [LICENSE](LICENSE). Compilar do código, testes e processo de release: [CONTRIBUTING.md](CONTRIBUTING.md).

Por [arthuregood](https://github.com/arthuregood).
