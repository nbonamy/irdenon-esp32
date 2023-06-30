#!/bin/sh
cd data
for file in *; do echo Uploading ${file}; curl -F "file=@${file}" irdenon.local/api/upload; done
echo Done!