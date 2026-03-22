# Makefile for libSXAO

CC     = g++
SDK_LI ?= licensedinterfaces
RM     = rm -f
STRIP  = strip

GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)

UNAME_S := $(shell uname -s)

ifneq (,$(findstring MINGW,$(UNAME_S)))
  # Windows — MSYS2 / MinGW64
  TARGET_LIB = libSXAO.dll
  OS_FLAG    = -DSB_WIN_BUILD
  LDFLAGS    = -shared
else ifeq ($(UNAME_S),Darwin)
  TARGET_LIB = libSXAO.dylib
  OS_FLAG    = -DSB_MACOSX_BUILD
  LDFLAGS    = -dynamiclib -lstdc++
else
  TARGET_LIB = libSXAO.so
  OS_FLAG    = -DSB_LINUX_BUILD
  LDFLAGS    = -shared -lstdc++
endif

CPPFLAGS = -fPIC -Wall -Wextra -O2 -g $(OS_FLAG) -std=gnu++11 \
           -I. -I$(SDK_LI) \
           -DX2_FLAT_INCLUDES \
           -DGIT_HASH=\"$(GIT_HASH)\"

SRCS = main.cpp sxao.cpp x2sxao.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: ${TARGET_LIB}

${TARGET_LIB}: $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^
	$(STRIP) $@ >/dev/null 2>&1 || true

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	${RM} libSXAO.so libSXAO.dylib libSXAO.dll ${OBJS}
