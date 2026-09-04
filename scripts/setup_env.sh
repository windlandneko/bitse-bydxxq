#!/usr/bin/env bash
set -euo pipefail

echo "==> 更新软件源"
sudo apt update

echo "==> 安装编译工具链 + Qt 6"
sudo apt-get install -y \
  build-essential \
  cmake \
  gdb \
  clang-format \
  clangd \
  qt6-base-dev \
  qt6-tools-dev-tools \
  qtcreator \
  designer-qt6 \
  libqt6svg6 \
  curl \
  ca-certificates \
  git

echo "==> 安装 Node.js latest LTS"
curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
sudo apt install -y nodejs

echo "==> 安装 uv (Python)"
curl -LsSf https://astral.sh/uv/install.sh | sh
