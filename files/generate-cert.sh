#!/bin/sh

# Country
# State or Province
# Locality
# Organization
# Organizational Unit
# Common Name
# E-Mail address
echo "JP
Ehime
Matsuyama-shi
Mojyack Corp.
Development team
localhost
mojyack@gmail.com
" | openssl req -new -newkey rsa:4096 -days 36500 -nodes -x509 -keyout "localhost.key" -out "localhost.cert"
