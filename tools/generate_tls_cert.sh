#!/usr/bin/env bash

set -e
set -u

FQDN="hackerhotel2024.ota.bodge.team"
CERT="hackerhotel2024.pem"
KEY="hackerhotel2024.key.pem"

openssl req -x509 -newkey rsa:4096 -keyout "$KEY" -out "$CERT" -days 36500 -nodes -subj "/CN=$FQDN"
