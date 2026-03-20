# Makefile for libSXAO
#
# The X2 SDK licensed interface headers use relative includes of the form
# "../../licensedinterfaces/foo.h", which requires the build to occur from
# a directory that is two levels below a directory containing licensedinterfaces/.
# We satisfy this by symlinking ../licensedinterfaces -> ../X2-Examples/licensedinterfaces
# (i.e. appinstall/licensedinterfaces -> appinstall/X2-Examples/licensedinterfaces)
# and adding that path to the flat-include search path.
#
# On macOS, replace -DSB_LINUX_BUILD with -DSB_MACOSX_BUILD and change the
# target lib extension to .dylib.

CC       = gcc
SDK_LI   = ../X2-Examples/licensedinterfaces
CPPFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -std=gnu++11 \
           -I. -I$(SDK_LI)
LDFLAGS  = -shared -lstdc++
RM       = rm -f
STRIP    = strip

TARGET_LIB = libSXAO.so

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
	${RM} ${TARGET_LIB} ${OBJS}
