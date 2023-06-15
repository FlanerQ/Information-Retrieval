#!/bin/zsh
set -e

sudo cp a.html /usr/share/nginx/html
sudo systemctl restart nginx