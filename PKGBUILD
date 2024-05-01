# Maintainer: Chris DeRose <chris@chrisderose.com>
pkgname=propertyservicesmonitor
pkgver=0.2
pkgrel=1
epoch=
pkgdesc="This is a lightweight tool to test services and servers on a property, and e-mail an address with the results of those tests."
arch=(x86_64 arm)
url="https://github.com/brighton36/property-services-monitor"
license=('unknown')
groups=()
depends=(fmt poco yaml-cpp)
makedepends=()
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=()
noextract=()
sha256sums=()
validpgpkeys=()

build() {
  cd $startdir
	make
}

package() {
  mkdir -p $pkgdir/usr/bin
  cp $startdir/build/bin/property-services-monitor $pkgdir/usr/bin/
  mkdir -p $pkgdir/usr/share/man/man1
  cp $startdir/build/bin/property-services-monitor.1.gz $pkgdir/usr/share/man/man1/
  mkdir -p $pkgdir/usr/share/property-services-monitor
  cp -R $startdir/views $pkgdir/usr/share/property-services-monitor/
}
