version=$(shell cat VERSION)
pkgname=neosphere-$(version)
os=$(shell uname)

ifndef prefix
ifeq ($(os), Darwin)
prefix=/usr/local
else
prefix=/usr
endif
endif
installdir=$(DESTDIR)$(prefix)

ifndef CC
CC=cc
endif

ifndef CFLAGS
CFLAGS=-O3
endif

engine_sources=src/neosphere/main.c \
   vendor/dyad/dyad.c \
   vendor/md5/md5.c \
   vendor/wildmatch/wildmatch.c \
   src/shared/api.c \
   src/shared/compress.c \
   src/shared/console.c \
   src/shared/dyad.c \
   src/shared/encoding.c \
   src/shared/jsal.c \
   src/shared/ki.c \
   src/shared/lstring.c \
   src/shared/md5.c \
   src/shared/path.c \
   src/shared/sockets.c \
   src/shared/unicode.c \
   src/shared/vector.c \
   src/shared/wildmatch.c \
   src/shared/xoroshiro.c \
   src/neosphere/animation.c \
   src/neosphere/atlas.c \
   src/neosphere/audio.c \
   src/neosphere/blend_op.c \
   src/neosphere/byte_array.c \
   src/neosphere/color.c \
   src/neosphere/debugger.c \
   src/neosphere/dispatch.c \
   src/neosphere/event_loop.c \
   src/neosphere/font.c \
   src/neosphere/galileo.c \
   src/neosphere/game.c \
   src/neosphere/geometry.c \
   src/neosphere/image.c \
   src/neosphere/input.c \
   src/neosphere/kev_file.c \
   src/neosphere/legacy.c \
   src/neosphere/logger.c \
   src/neosphere/map_engine.c \
   src/neosphere/module.c \
   src/neosphere/obstruction.c \
   src/neosphere/package.c \
   src/neosphere/pegasus.c \
   src/neosphere/profiler.c \
   src/neosphere/screen.c \
   src/neosphere/script.c \
   src/neosphere/source_map.c \
   src/neosphere/spriteset.c \
   src/neosphere/table.c \
   src/neosphere/tileset.c \
   src/neosphere/transform.c \
   src/neosphere/utility.c \
   src/neosphere/vanilla.c \
   src/neosphere/windowstyle.c
engine_libs= \
   -lallegro_acodec \
   -lallegro_audio \
   -lallegro_color \
   -lallegro_dialog \
   -lallegro_font \
   -lallegro_image \
   -lallegro_main \
   -lallegro_memfile \
   -lallegro_primitives \
   -lallegro_ttf \
   -lallegro \
   -lChakraCore \
   -lz \
   -lm

cell_sources=src/cell/main.c \
   vendor/wildmatch/wildmatch.c \
   src/shared/api.c \
   src/shared/compress.c \
   src/shared/encoding.c \
   src/shared/jsal.c \
   src/shared/lstring.c \
   src/shared/path.c \
   src/shared/unicode.c \
   src/shared/vector.c \
   src/shared/wildmatch.c \
   src/shared/xoroshiro.c \
   src/cell/build.c \
   src/cell/fs.c \
   src/cell/image.c \
   src/cell/module.c \
   src/cell/spk_writer.c \
   src/cell/target.c \
   src/cell/tileset.c \
   src/cell/tool.c \
   src/cell/utility.c \
   src/cell/visor.c
cell_libs= \
   -lChakraCore \
   -lpng \
   -lz \
   -lm

ssj_sources=src/ssj/main.c \
   vendor/dyad/dyad.c \
   src/shared/console.c \
   src/shared/dyad.c \
   src/shared/ki.c \
   src/shared/path.c \
   src/shared/sockets.c \
   src/shared/vector.c \
   src/shared/xoroshiro.c \
   src/ssj/backtrace.c \
   src/ssj/help.c \
   src/ssj/inferior.c \
   src/ssj/listing.c \
   src/ssj/objview.c \
   src/ssj/parser.c \
   src/ssj/session.c

ifeq ($(os), Darwin)
LINKER_ARGS=-Wl,-rpath,\$$ORIGIN
CHAKRACORE_URL=https://aka.ms/chakracore/cc_osx_x64_1_11_15
else
LINKER_ARGS=-Wl,-rpath=\$$ORIGIN
OPTIONS=-DNEOSPHERE_MNG_SUPPORT
engine_libs+=-lmng
CHAKRACORE_URL=https://aka.ms/chakracore/cc_linux_x64_1_11_15
endif

.PHONY: all
all: neosphere spherun cell ssj

.PHONY: deps
deps:
	mkdir -p dep
	wget -O dep/libChakraCore.tar.gz $(CHAKRACORE_URL)
	cd dep && tar xzf libChakraCore.tar.gz --strip-components=1 ChakraCoreFiles/include ChakraCoreFiles/lib

.PHONY: installdeps
installdeps:
	cp dep/lib/* $(installdir)/lib

.PHONY: neosphere
neosphere: bin/neosphere

.PHONY: spherun
spherun: bin/neosphere bin/spherun

.PHONY: cell
cell: bin/cell

.PHONY: ssj
ssj: bin/ssj

.PHONY: dist
dist: all
	mkdir -p dist/$(pkgname)
	cp -r assets desktop docs license manpages src dist/$(pkgname)
	cp Makefile VERSION dist/$(pkgname)
	cp CHANGELOG.md LICENSE.txt README.md dist/$(pkgname)
	cd dist && tar czf $(pkgname)-src.tar.gz $(pkgname) && rm -rf dist/$(pkgname)

.PHONY: install
install: all
	mkdir -p $(installdir)/bin
	mkdir -p $(installdir)/lib
	mkdir -p $(installdir)/share/sphere
	mkdir -p $(installdir)/share/applications
	mkdir -p $(installdir)/share/doc/sphere
	mkdir -p $(installdir)/share/icons/hicolor/scalable/mimetypes
	mkdir -p $(installdir)/share/mime/packages
	mkdir -p $(installdir)/share/man/man1
	mkdir -p $(installdir)/share/pixmaps
	cp bin/neosphere bin/spherun bin/cell bin/ssj $(installdir)/bin
	cp -r bin/system $(installdir)/share/sphere
	gzip docs/sphere2-core-api.txt -c > $(installdir)/share/doc/sphere/sphere2-core-api.gz
	gzip docs/sphere2-hl-api.txt -c > $(installdir)/share/doc/sphere/sphere2-hl-api.gz
	gzip docs/cellscript-api.txt -c > $(installdir)/share/doc/sphere/cellscript-api.gz
	gzip manpages/neosphere.1 -c > $(installdir)/share/man/man1/neosphere.1.gz
	gzip manpages/spherun.1 -c > $(installdir)/share/man/man1/spherun.1.gz
	gzip manpages/cell.1 -c > $(installdir)/share/man/man1/cell.1.gz
	gzip manpages/ssj.1 -c > $(installdir)/share/man/man1/ssj.1.gz
	cp desktop/neosphere.desktop $(installdir)/share/applications
	cp desktop/sphere-icon.svg $(installdir)/share/pixmaps
	cp desktop/mimetypes/neosphere.xml $(installdir)/share/mime/packages
	cp desktop/mimetypes/*.svg $(installdir)/share/icons/hicolor/scalable/mimetypes

.PHONY: clean
clean:
	rm -rf bin
	rm -rf dist

bin/neosphere:
	mkdir -p bin
	$(CC) -o bin/neosphere $(CFLAGS) \
	      -fno-omit-frame-pointer \
	      -Isrc/shared -Idep/include -Ivendor/dyad -Ivendor/md5 -Ivendor/tinydir -I vendor/wildmatch \
	      -Ldep/lib \
	      $(LINKER_ARGS) \
	      $(OPTIONS) \
	      $(engine_sources) $(engine_libs)
	cp -r assets/system bin

bin/spherun:
	mkdir -p bin
	$(CC) -o bin/spherun $(CFLAGS) \
	      -fno-omit-frame-pointer \
	      -Isrc/shared -Idep/include -Ivendor/dyad -Ivendor/md5 -Ivendor/tinydir -I vendor/wildmatch \
	      -Ldep/lib \
	      $(LINKER_ARGS) \
	      $(OPTIONS) \
	      -DNEOSPHERE_SPHERUN \
	      $(engine_sources) $(engine_libs)

bin/cell:
	mkdir -p bin
	$(CC) -o bin/cell $(CFLAGS) \
	      -fno-omit-frame-pointer \
	      -Isrc/shared -Idep/include -Ivendor/tinydir -Ivendor/wildmatch \
	      -Ldep/lib \
	      $(LINKER_ARGS) \
	      $(cell_sources) $(cell_libs)

bin/ssj:
	mkdir -p bin
	$(CC) -o bin/ssj $(CFLAGS) \
	      -Isrc/shared -Ivendor/dyad \
	      $(ssj_sources)
