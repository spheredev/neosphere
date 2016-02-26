version=$(shell cat VERSION)
pkgname=minisphere-$(version)

ifndef prefix
prefix=/usr
endif
installdir=$(DESTDIR)$(prefix)

ifndef CC
CC=cc
endif

ifndef CFLAGS
CFLAGS=-O3
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

cell_sources=src/cell/main.c \
   src/shared/duktape.c src/shared/path.c src/shared/vector.c \
   src/cell/assets.c src/cell/build.c src/cell/spk_writer.c src/cell/utility.c
cell_libs= \
   -lz -lm

ssj_sources=src/ssj/main.c \
   src/shared/dyad.c src/shared/path.c src/shared/vector.c \
   src/ssj/backtrace.c src/ssj/dvalue.c src/ssj/help.c src/ssj/inferior.c \
   src/ssj/message.c src/ssj/parser.c src/ssj/session.c src/ssj/sockets.c \
   src/ssj/source.c

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

.PHONY: deb
deb: dist
	cp dist/minisphere-$(version).tar.gz dist/minisphere_$(version).orig.tar.gz
	cd dist && tar xf minisphere_$(version).orig.tar.gz
	cp -r src/debian dist/minisphere-$(version)
	cd dist/minisphere-$(version) && debuild -S -sa

.PHONY: dist
dist:
	mkdir -p dist/$(pkgname)
	cp -r assets desktop docs manpages src dist/$(pkgname)
	cp Makefile VERSION dist/$(pkgname)
	cp CHANGELOG LICENSE.txt README.md dist/$(pkgname)
	cd dist && tar cfz $(pkgname).tar.gz $(pkgname) && rm -rf dist/$(pkgname)

.PHONY: install
install: all
	mkdir -p $(installdir)/bin
	mkdir -p $(installdir)/share/minisphere
	mkdir -p $(installdir)/share/man/man1
	mkdir -p $(installdir)/share/applications
	mkdir -p $(installdir)/share/pixmaps
	cp bin/minisphere bin/spherun bin/cell bin/ssj $(installdir)/bin
	cp -r bin/system $(installdir)/share/minisphere
	gzip manpages/minisphere.1 -c > $(installdir)/share/man/man1/minisphere.1.gz
	gzip manpages/spherun.1 -c > $(installdir)/share/man/man1/spherun.1.gz
	gzip manpages/cell.1 -c > $(installdir)/share/man/man1/cell.1.gz
	gzip manpages/ssj.1 -c > $(installdir)/share/man/man1/ssj.1.gz
	cp desktop/minisphere.desktop $(installdir)/share/applications
	cp desktop/sphere-icon.svg $(installdir)/share/pixmaps

.PHONY: clean
clean:
	rm -rf bin
	rm -rf dist

bin/minisphere:
	mkdir -p bin
	$(CC) -o bin/minisphere $(CFLAGS) -Isrc/shared -Isrc/minisphere \
	      -DDUK_OPT_HAVE_CUSTOM_H \
	      $(engine_sources) $(engine_libs)
	cp -r assets/system bin

bin/spherun:
	mkdir -p bin
	$(CC) -o bin/spherun $(CFLAGS) -Isrc/shared -Isrc/minisphere \
	      -DDUK_OPT_HAVE_CUSTOM_H -DMINISPHERE_SPHERUN \
	      $(engine_sources) $(engine_libs)

bin/cell:
	mkdir -p bin
	$(CC) -o bin/cell $(CFLAGS) -Isrc/shared $(cell_sources) $(cell_libs)

bin/ssj:
	mkdir -p bin
	$(CC) -o bin/ssj $(CFLAGS) -Isrc/shared $(ssj_sources)
