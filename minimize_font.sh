#!/bin/sh
chars=$(grep -o . strings | LC_ALL=C sort -u | tr -d '\n')
dir=app/src/main/assets/fonts/

mkdir -p $dir
pyftsubset /usr/share/fonts/noto/NotoSerif-Regular.ttf \
    --output-file=$dir/NotoSerif-Regular-subset-nohinting-nofeatures.ttf \
    --text="$chars" \
    --no-hinting \
    --layout-features=''
