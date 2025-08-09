#!/bin/sh
for file in *; do
sha256sum ${file} > ${file}.sha256
sha512sum ${file} > ${file}.sha512
gpg --detach-sign ${file}
done
