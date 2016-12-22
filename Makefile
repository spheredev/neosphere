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

engine_sources=src/engine/main.c \
   src/shared/api.c src/shared/duktape.c src/shared/duk_rubber.c \
   src/shared/dyad.c src/shared/encoding.c src/shared/lstring.c \
   src/shared/path.c src/shared/unicode.c src/shared/vector.c \
   src/shared/xoroshiro.c \
   src/engine/animation.c src/engine/async.c src/engine/atlas.c \
   src/engine/audio.c src/engine/bytearray.c src/engine/color.c \
   src/engine/console.c src/engine/debugger.c src/engine/font.c \
   src/engine/galileo.c src/engine/geometry.c src/engine/image.c \
   src/engine/input.c src/engine/kevfile.c src/engine/logger.c \
   src/engine/map_engine.c src/engine/matrix.c src/engine/obsmap.c \
   src/engine/pegasus.c src/engine/persons.c src/engine/screen.c \
   src/engine/script.c src/engine/shader.c src/engine/sockets.c \
   src/engine/spherefs.c src/engine/spk.c src/engine/spriteset.c \
   src/engine/tileset.c src/engine/utility.c src/engine/vanilla.c \
   src/engine/windowstyle.c
engine_libs= \
   -lallegro_acodec -lallegro_audio -lallegro_color -lallegro_dialog \
   -lallegro_image -lallegro_memfile -lallegro_primitives -lallegro \
   -lmng -lz -lm

cell_sources=src/compiler/main.c \
   src/shared/api.c src/shared/duktape.c src/shared/duk_rubber.c \
   src/shared/encoding.c src/shared/lstring.c src/shared/path.c \
   src/shared/unicode.c src/shared/vector.c \
   src/compiler/build.c src/compiler/script.c src/compiler/spk_writer.c \
   src/compiler/target.c src/compiler/tool.c src/compiler/utility.c
cell_libs= \
   -lz -lm

majin_sources=src/preptool/main.c

ssj_sources=src/debugger/main.c \
   src/shared/dyad.c src/shared/path.c src/shared/vector.c \
   src/debugger/backtrace.c src/debugger/dmessage.c src/debugger/dvalue.c \
   src/debugger/help.c src/debugger/inferior.c src/debugger/objview.c \
   src/debugger/parser.c src/debugger/session.c src/debugger/sockets.c \
   src/debugger/source.c

.PHONY: all
all: minisphere spherun cell majin ssj

.PHONY: minisphere
minisphere: bin/minisphere

.PHONY: spherun
spherun: bin/minisphere bin/spherun

.PHONY: cell
cell: bin/cell

.PHONY: majin
majin: bin/majin

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
	cp CHANGELOG.md LICENSE.txt README.md dist/$(pkgname)
	cd dist && tar cfz $(pkgname).tar.gz $(pkgname) && rm -rf dist/$(pkgname)

.PHONY: install
install: all
	mkdir -p $(installdir)/bin
	mkdir -p $(installdir)/share/minisphere
	mkdir -p $(installdir)/share/applications
	mkdir -p $(installdir)/share/doc/minisphere
	mkdir -p $(installdir)/share/icons/hicolor/scalable/mimetypes
	mkdir -p $(installdir)/share/mime/packages
	mkdir -p $(installdir)/share/man/man1
	mkdir -p $(installdir)/share/pixmaps
	cp bin/minisphere bin/spherun bin/cell bin/majin bin/ssj $(installdir)/bin
	cp -r bin/system $(installdir)/share/minisphere
	gzip docs/sphere2-api.txt -c > $(installdir)/share/doc/minisphere/sphere2-api.gz
	gzip docs/cellscript-api.txt -c > $(installdir)/share/doc/minisphere/cellscript-api.gz
	gzip docs/miniRT-api.txt -c > $(installdir)/share/doc/minisphere/miniRT-api.gz
	gzip manpages/minisphere.1 -c > $(installdir)/share/man/man1/minisphere.1.gz
	gzip manpages/spherun.1 -c > $(installdir)/share/man/man1/spherun.1.gz
	gzip manpages/cell.1 -c > $(installdir)/share/man/man1/cell.1.gz
	gzip manpages/ssj.1 -c > $(installdir)/share/man/man1/ssj.1.gz
	cp desktop/minisphere.desktop $(installdir)/share/applications
	cp desktop/sphere-icon.svg $(installdir)/share/pixmaps
	cp desktop/mimetypes/minisphere.xml $(installdir)/share/mime/packages
	cp desktop/mimetypes/*.svg $(installdir)/share/icons/hicolor/scalable/mimetypes

.PHONY: clean
clean:
	rm -rf bin
	rm -rf dist

bin/minisphere:
	mkdir -p bin
	$(CC) -o bin/minisphere $(CFLAGS) -Isrc/shared -Isrc/engine \
	      -DDUK_OPT_HAVE_CUSTOM_H \
	      $(engine_sources) $(engine_libs)
	cp -r assets/system bin

bin/spherun:
	mkdir -p bin
	$(CC) -o bin/spherun $(CFLAGS) -Isrc/shared -Isrc/engine \
	      -DDUK_OPT_HAVE_CUSTOM_H -DMINISPHERE_SPHERUN \
	      $(engine_sources) $(engine_libs)

bin/cell:
	mkdir -p bin
	$(CC) -o bin/cell $(CFLAGS) -Isrc/shared $(cell_sources) $(cell_libs)

bin/majin:
	mkdir -p bin
	$(CC) -o bin/majin $(CFLAGS) -Isrc/shared $(majin_sources)

bin/ssj:
	mkdir -p bin
	$(CC) -o bin/ssj $(CFLAGS) -Isrc/shared $(ssj_sources)
