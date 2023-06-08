#!/bin/bash
SOURCE="urls/$1.txt"
OUT_DIR="out/$1"

mkdir -p $OUT_DIR
while read id; do
    echo $id
    curl -b "" \
    "https://www1.szu.edu.cn/board/view.asp?id=$id" \
    | iconv -f gbk -t utf-8 > "$OUT_DIR/$id.html"
done < $SOURCE

