#!/bin/bash
export WEBRTC_TARGET_OS="android"
export WEBRTC_TARGET_CPU="arm" #arm arm64 x86 x64
python3 build.py
read -s -n 1 key
