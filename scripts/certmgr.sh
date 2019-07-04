#!/bin/bash

PARAMS=""
HOST=localhost
CREATE=
NAME=cert

usage() {
    echo "usage: $0 -c [--host localhost --name cert]"
}

while (( $# )); do
    key=$1
    case $key in
        -c|--create)
            CREATE=1
            shift
            ;;
        -h|--host)
            HOST=$2
            shift
            shift
            ;;
        -n|--name)
            NAME=$2
            shift
            shift
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

if [ "$CREATE" ]; then
    echo "Creating key: ${NAME}.{key,crt}"
    openssl req -x509 -newkey rsa:2048 -nodes \
        -subj "/C=US/ST=San Francisco/L=San Francisco/O=nspawn self-signed/CN=$HOST" \
        --keyout ${NAME}.key \
        --out ${NAME}.crt
fi

