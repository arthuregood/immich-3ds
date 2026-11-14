#!/bin/sh
# Gera source/config_build.h a partir do .env (opcional).
# Sem .env, gera defaults vazios e o app pede a config no teclado do 3DS.
# uso: gen_build_config.sh [.env] [saida.h]
set -eu

env_file=${1:-.env}
out=${2:-source/config_build.h}

server=""
apikey=""
tz="-03:00"

if [ -f "$env_file" ]; then
    while IFS= read -r line || [ -n "$line" ]; do
        # tira CR (arquivo salvo no Windows) e espacos das pontas
        line=$(printf '%s' "$line" | tr -d '\r' | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')
        case "$line" in
            ''|'#'*) continue ;;
        esac
        key=${line%%=*}
        val=${line#*=}
        # remove aspas envolventes
        case "$val" in
            \"*\") val=${val#\"}; val=${val%\"} ;;
            \'*\') val=${val#\'}; val=${val%\'} ;;
        esac
        case "$key" in
            IMMICH_SERVER)  server=$val ;;
            IMMICH_API_KEY) apikey=$val ;;
            IMMICH_TZ)      [ -n "$val" ] && tz=$val ;;
        esac
    done < "$env_file"
fi

# escapa aspas/barra que quebrariam o literal C
esc() { printf '%s' "$1" | sed -e 's/[\\"]/\\&/g'; }

mkdir -p "$(dirname "$out")"
cat > "$out" <<EOF
// gerado por tools/gen_build_config.sh -- nao edite, nao versione
#pragma once
#define BUILD_SERVER "$(esc "$server")"
#define BUILD_APIKEY "$(esc "$apikey")"
#define BUILD_TZ     "$(esc "$tz")"
EOF

if [ -n "$server" ] && [ -n "$apikey" ]; then
    echo "config do build: $server (chave embutida, ${#apikey} chars)"
else
    echo "config do build: vazia (o app vai pedir no 3DS)"
fi
