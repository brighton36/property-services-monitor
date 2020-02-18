#!/bin/bash

mkdir -p build/dpkg
tar --exclude-vcs --exclude='*.yml' --exclude='*.swp' --exclude='*.txt' --exclude "build" --exclude '*.txt' --transform "s,^.,property-services-monitor," -czvf build/dpkg/propertyservicesmonitor-0.1.tar.gz ./
cd build/dpkg
debmake -a propertyservicesmonitor-0.1.tar.gz
cd propertyservicesmonitor-0.1
# Edit debian/control 
# * Edit the Build-Depends: debhelper-compat (= 12), libfmt-dev, libpoco-dev, libyaml-cpp-dev
# * section to be "admin"
# * Perhaps some of the meta fields: description, Vcs-Git, Vcs-browser, Homepage
dpkg-buildpackage -us -uc

cd ..
# TODO this should work too now
sbuild  --host=armhf -d buster propsmon-0.1

