#!/bin/bash

ROOT=$(dirname $(realpath $0))
BUILD_DIR="$ROOT/build/armhf"

info () {
  GREEN='\033[0;32m'
  NC='\033[0m' # No Color
  echo -e "• $GREEN$1$NC"
}

info "Downloading and Cross Compiling OpenSSL..."

OPENSSL_INSTALL_DIR="$BUILD_DIR/cross-openssl/compiled-openssl"
mkdir -p "$OPENSSL_INSTALL_DIR"
cd "$BUILD_DIR/cross-openssl"
curl https://www.openssl.org/source/openssl-1.1.1d.tar.gz -O
tar -xzvf openssl-1.1.1d.tar.gz
cd openssl-1.1.1d
./Configure linux-generic32 shared \
  --prefix="$OPENSSL_INSTALL_DIR" --openssldir="$OPENSSL_INSTALL_DIR/openssl" \
  --cross-compile-prefix=arm-linux-gnueabihf-
make depend
make
make install

info "Downloading and Cross Compiling POCO..."

POCO_INSTALL_DIR="$BUILD_DIR/cross-poco/compiled-poco"
mkdir -p "$POCO_INSTALL_DIR"
cd "$BUILD_DIR/cross-poco"
curl https://pocoproject.org/releases/poco-1.9.4/poco-1.9.4-all.tar.gz -O
tar -xzvf poco-1.9.4-all.tar.gz
cd poco-1.9.4-all
echo "TOOL               = arm-linux-gnueabihf" > build/config/RaspberryPI
cat build/config/ARM-Linux >> build/config/RaspberryPI
echo 'STATICOPT_CC = -fPIC' >> build/config/RaspberryPI
echo 'STATICOPT_CXX = -fPIC' >> build/config/RaspberryPI
./configure --static --config=RaspberryPI --no-samples --no-tests --omit=CppUnit,CppUnit/WinTestRunner,Data,Data/SQLite,Data/ODBCData/MySQL,MongoDB,PageCompiler,PageCompiler/File2Page --include-path=$OPENSSL_INSTALL_DIR/include --library-path=$OPENSSL_INSTALL_DIR/openssl/lib --prefix="$POCO_INSTALL_DIR"
make
make install

info "Downloading and Cross Compiling yaml-cpp..."

YAMLCPP_INSTALL_DIR="$BUILD_DIR/cross-yamlcpp/compiled-yamlcpp"
mkdir -p "$YAMLCPP_INSTALL_DIR"
cd "$BUILD_DIR/cross-yamlcpp"
curl -L https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.3.tar.gz -O
tar -xzvf yaml-cpp-0.6.3.tar.gz
cd yaml-cpp-yaml-cpp-0.6.3
cmake -D CMAKE_SYSTEM_NAME=Linux -D CMAKE_SYSTEM_PROCESSOR=arm \
  -D CMAKE_C_COMPILER="/usr/bin/arm-linux-gnueabihf-gcc" \
  -D CMAKE_CXX_COMPILER="/usr/bin/arm-linux-gnueabihf-g++" \
  -D CMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -D CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY -D YAML_CPP_BUILD_TESTS=OFF \
  -D YAML_CPP_BUILD_TOOLS=OFF -D YAML_CPP_BUILD_CONTRIB=OFF \
  -D CMAKE_INSTALL_PREFIX="$YAMLCPP_INSTALL_DIR" ./
make
make install

info "Downloading and Cross Compiling fmt..."

FMT_INSTALL_DIR="$BUILD_DIR/cross-fmt/compiled-fmt"
mkdir -p "$FMT_INSTALL_DIR"
cd "$BUILD_DIR/cross-fmt"
curl -L https://github.com/fmtlib/fmt/releases/download/6.1.2/fmt-6.1.2.zip -O
unzip fmt-6.1.2.zip
cd fmt-6.1.2
cmake -D CMAKE_SYSTEM_NAME=Linux -D CMAKE_SYSTEM_PROCESSOR=arm \
  -D CMAKE_CXX_COMPILER="/usr/bin/arm-linux-gnueabihf-g++" \
  -D CMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
  -D CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
  -D CMAKE_INSTALL_PREFIX="$FMT_INSTALL_DIR" ./

info "Cross Compiling property-services-manager..."

cd "$ROOT"
make ARCH=armhf
