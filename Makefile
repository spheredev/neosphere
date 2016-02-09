PKG_NAME=minisphere-$(shell cat VERSION)

ifndef prefix
prefix=/usr
endif
installdir=$(DESTDIR)$(prefix)

ifndef CC
CC=cc
endif

engine_sources=src/minisphere/main.c \
   src/shared/duktape.c src/shared/dyad.c src/shared/mt19937ar.c \
   src/shared/lstring.c src/shared/path.c src/shared/unicode.c \
   src/shared/vector.c \
   src/minisphere/animation.c src/minisphere/api.c \
   src/minisphere/async.c src/minisphere/atlas.c \
   src/minisphere/audialis.c src/minisphere/bytearray.c \
   src/minisphere/color.c src/minisphere/console.c \
   src/minisphere/debugger.c src/minisphere/file.c \
   src/minisphere/font.c src/minisphere/galileo.c \
   src/minisphere/geometry.c src/minisphere/image.c \
   src/minisphere/input.c src/minisphere/logger.c \
   src/minisphere/map_engine.c src/minisphere/obsmap.c \
   src/minisphere/persons.c src/minisphere/physics.c \
   src/minisphere/primitives.c src/minisphere/rng.c \
   src/minisphere/script.c src/minisphere/shader.c \
   src/minisphere/sockets.c src/minisphere/spherefs.c \
   src/minisphere/spk.c src/minisphere/spriteset.c \
   src/minisphere/surface.c src/minisphere/tileset.c \
   src/minisphere/utility.c src/minisphere/windowstyle.c
engine_libs= \
   -lallegro_acodec -lallegro_audio -lallegro_color -lallegro_dialog \
   -lallegro_image -lallegro_memfile -lallegro_primitives -lallegro \
   -lmng -lz -lm

.PHONY: all
all: minisphere spherun cell ssj

.PHONY: minisphere
minisphere: bin/minisphere

.PHONY: spherun
spherun: bin/minisphere bin/spherun

.PHONY: cell
cell: bin/cell

.PHONY: ssj
ssj: bin/ssj

.PHONY: debian
debian: dist
	cp dist/minisphere-3.0a0.tar.gz dist/minisphere_3.0a0.orig.tar.gz
	cd dist && tar xf minisphere_3.0a0.orig.tar.gz
	cd dist/minisphere-3.0a0 && debuild -us -uc

.PHONY: dist
dist:
	mkdir -p dist/$(PKG_NAME)
	cp -r assets debian desktops docs man-pages src dist/$(PKG_NAME)
	cp Makefile VERSION dist/$(PKG_NAME)
	cp CHANGELOG LICENSE.txt README.md dist/$(PKG_NAME)
	cd dist && tar cfz $(PKG_NAME).tar.gz $(PKG_NAME) && rm -rf $(PKG_NAME)

.PHONY: install
install: all
	mkdir -p $(installdir)/bin
	mkdir -p $(installdir)/share/minisphere
	mkdir -p $(installdir)/share/man/man1
	mkdir -p $(installdir)/share/applications
	cp bin/minisphere bin/spherun bin/cell bin/ssj $(installdir)/bin
	cp -r bin/system $(installdir)/share/minisphere
	gzip man-pages/minisphere.1 -c > $(installdir)/share/man/man1/minisphere.1.gz
	gzip man-pages/spherun.1 -c > $(installdir)/share/man/man1/spherun.1.gz
	gzip man-pages/cell.1 -c > $(installdir)/share/man/man1/cell.1.gz
	gzip man-pages/ssj.1 -c > $(installdir)/share/man/man1/ssj.1.gz
	cp desktops/minisphere.desktop $(installdir)/share/applications
	cp desktops/sphere-icon.svg $(installdir)/share/minisphere

.PHONY: clean
clean:
	rm -rf bin

bin/minisphere:
	mkdir -p bin
	$(CC) -o bin/minisphere -O3 -Isrc/shared -Isrc/minisphere \
	   -DDUK_OPT_HAVE_CUSTOM_H \
	   $(engine_sources) $(engine_libs)
	cp -r assets/system bin

bin/spherun:
	mkdir -p bin
	$(CC) -o bin/spherun -O3 -Isrc/shared -Isrc/minisphere \
	   -DMINISPHERE_SPHERUN -DDUK_OPT_HAVE_CUSTOM_H \
	   $(engine_sources) $(engine_libs)

bin/cell:
	mkdir -p bin
	$(CC) -o bin/cell -O3 -Isrc/shared \
	   src/cell/main.c \
	   src/shared/duktape.c src/shared/path.c src/shared/vector.c \
	   src/cell/assets.c src/cell/build.c src/cell/spk_writer.c \
	   src/cell/utility.c \
	   -lz -lm

bin/ssj:
	mkdir -p bin
	$(CC) -o bin/ssj -O3 -Isrc/shared \
	   src/ssj/main.c \
	   src/shared/dyad.c src/shared/lstring.c src/shared/path.c \
	   src/shared/unicode.c src/shared/vector.c \
	   src/ssj/remote.c src/ssj/session.c src/ssj/source.c
