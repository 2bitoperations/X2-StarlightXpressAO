# Makefile for libSXAO

CC    = g++
RM    = rm -f
STRIP = strip

GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)

UNAME_S := $(shell uname -s)

ifneq (,$(findstring MINGW,$(UNAME_S)))
  # Windows — MSYS2 / MinGW64
  TARGET_LIB  = libSXAO.dll
  OS_FLAG     = -DSB_WIN_BUILD
  # Link runtimes statically so the DLL has no MinGW .dll dependencies.
  LDFLAGS     = -shared -static-libstdc++ -static-libgcc
  STRIP_FLAGS =
else ifeq ($(UNAME_S),Darwin)
  TARGET_LIB  = libSXAO.dylib
  OS_FLAG     = -DSB_MACOSX_BUILD
  LDFLAGS     = -dynamiclib -lstdc++
  # -x: strip local/debug symbols only; keep exported symbols so TSX can load the plugin.
  STRIP_FLAGS = -x
else
  TARGET_LIB  = libSXAO.so
  OS_FLAG     = -DSB_LINUX_BUILD
  LDFLAGS     = -shared -lstdc++
  STRIP_FLAGS = --strip-unneeded
endif

CPPFLAGS = -fPIC -Wall -Wextra -O2 $(OS_FLAG) -std=gnu++11 \
           -I. -Ilicensedinterfaces \
           -DX2_FLAT_INCLUDES \
           -DGIT_HASH=\"$(GIT_HASH)\"

# BUILD_NUMBER is passed from CI: make BUILD_NUMBER=42
# Locally it is omitted and version.h falls back to the bare major version.
ifdef BUILD_NUMBER
  CPPFLAGS += -DBUILD_NUMBER=$(BUILD_NUMBER)
endif

SRCS = main.cpp sxao.cpp x2sxao.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: ${TARGET_LIB}

${TARGET_LIB}: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $(STRIP_FLAGS) $@ >/dev/null 2>&1 || true

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	$(RM) libSXAO.so libSXAO.dylib libSXAO.dll $(OBJS)
