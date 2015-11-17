# usage:
# make - builds target with default compiler (clang)
# COMPILER=gcc make - builds target with selected compiler

ifndef COMPILER
    COMPILER=clang
endif

ifeq ($(COMPILER),clang)
    C=clang
    CPP=clang++
else
    C=gcc
    CPP=g++
endif

NAME1:=run-shell
CPPFLAGS+=-g -std=c++11
LDFLAGS+=-g

CPPFLAGS+=$(shell PKG_CONFIG_PATH='/usr/local/lib/pkgconfig/:/usr/local/lib64/pkgconfig/' pkg-config --cflags liblxcpp libLogger)
LDFLAGS+=$(shell PKG_CONFIG_PATH='/usr/local/lib/pkgconfig/:/usr/local/lib64/pkgconfig/' pkg-config --libs liblxcpp libLogger)

OBJECTS1=$(NAME1).o
HEADERS=

all: $(NAME1)

%.o: %.cpp $(HEADERS)
	$(CPP) -c -o $@ $< $(CPPFLAGS)

$(NAME1): $(OBJECTS1)
	$(CPP) $(OBJECTS1) $(LDFLAGS) -o $(NAME1)

clean:
	rm -f $(OBJECTS1) $(NAME1)
