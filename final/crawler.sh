#!/bin/zsh
set -e

#校庆网
./cele_crawler/cele_crawler.sh
python3 ./cele_crawler/crawler.py

#公文通
./offi_crawler/offi_crawler.sh
python3 ./offi_crawler/html2txt.py

#编译运行c++程序构建
./final/make.sh
