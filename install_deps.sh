#!/bin/bash
set -ex

sudo apt update
sudo apt install -y libc++-18-dev libc++abi-18-dev \
                    clang cmake libunwind-dev ninja-build