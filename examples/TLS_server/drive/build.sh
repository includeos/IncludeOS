#!/bin/bash
openssl req -newkey rsa:4096 -nodes -sha512 -x509 -days 3650 -nodes -out test.pem -keyout test.key
openssl x509 -outform der -in test.pem -out test.der
openssl req -newkey rsa:4096 -nodes -sha512 -x509 -days 3650 -nodes -keyout server.key
