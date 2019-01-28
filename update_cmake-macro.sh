#!/usr/bin/env bash

mkdir -p cmake-macro
cd cmake-macro
echo ${PWD}

echo "downloading cmake-macros..."
curl -LJO https://raw.githubusercontent.com/museghost/cmake-macro/master/FindCustomLibrary.cmake
curl -LJO https://raw.githubusercontent.com/museghost/cmake-macro/master/TargetPrintInfo.cmake
curl -LJO https://raw.githubusercontent.com/museghost/cmake-macro/master/FindCustomCURL.cmake
curl -LJO https://raw.githubusercontent.com/museghost/cmake-macro/master/FindCustomNghttp2.cmake
curl -LJO https://raw.githubusercontent.com/museghost/cmake-macro/master/FindCustomOpenSSL.cmake
cd ..

echo "done"
