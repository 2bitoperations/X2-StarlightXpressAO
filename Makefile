# Makefile for libSXAO
#
# The X2 SDK licensed interface headers use relative includes of the form
# "../../licensedinterfaces/foo.h", which requires the build to occur from
# a directory that is two levels below a directory containing licensedinterfaces/.
# We satisfy this by symlinking ../licensedinterfaces -> ../X2-Examples/licensedinterfaces
# (i.e. appinstall/licensedinterfaces -> appinstall/X2-Examples/licensedinterfaces)
# and adding that path to the flat-include search path.

CC      = g++
SDK_LI  = ../X2-Examples/licensedinterfaces
RM      = rm -f
STRIP   = strip

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  TARGET_LIB = libSXAO.dylib
  OS_FLAG    = -DSB_MACOSX_BUILD
  LDFLAGS    = -dynamiclib -lstdc++
else
  TARGET_LIB = libSXAO.so
  OS_FLAG    = -DSB_LINUX_BUILD
  LDFLAGS    = -shared -lstdc++
endif

CPPFLAGS = -fPIC -Wall -Wextra -O2 -g $(OS_FLAG) -std=gnu++11 \
           -I. -I$(SDK_LI)

SRCS = main.cpp sxao.cpp x2sxao.cpp
OBJS = $(SRCS:.cpp=.o)

# Symlink appinstall/licensedinterfaces -> X2-Examples/licensedinterfaces so that
# the SDK headers' internal "../../licensedinterfaces/foo.h" includes resolve.
SYMLINK_TARGET = ../licensedinterfaces

.PHONY: all clean symlink

all: symlink ${TARGET_LIB}

symlink:
	@if [ ! -e $(SYMLINK_TARGET) ]; then \
		echo "Creating SDK include symlink: $(SYMLINK_TARGET)"; \
		ln -s X2-Examples/licensedinterfaces $(SYMLINK_TARGET); \
	fi

${TARGET_LIB}: $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^
	$(STRIP) $@ >/dev/null 2>&1 || true

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	${RM} libSXAO.so libSXAO.dylib ${OBJS}
