#!/bin/sh
cd ..
mkdir tmp_hd
sudo mount -o loop,offset=10240 hd_oldlinux.img tmp_hd
