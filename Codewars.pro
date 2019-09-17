TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += include \
  /opt/intel/compilers_and_libraries/linux/mkl/include

QMAKE_CFLAGS += -msse4.2
LIBS += -lgmp

SOURCES += \
    src/bignum.c \
    src/central_attention.c \
    src/main.c \
    src/brainfuck.c \
    src/search_string.c \
    src/roman.c \
    src/int_partitions.c \
    src/ascii85.c \
    src/bignum_mul.c \
    src/pointer_monster.c \
    src/commons.c \
    src/mul.s \
    src/skyscrapers.c \
    src/triangle.c

HEADERS += \
    include/bignum.h \
    include/brainfuck.h \
    include/central_attention.h \
    include/search_string.h \
    include/roman.h \
    include/int_partitions.h \
    include/ascii85.h \
    include/bignum_mul.h \
    include/pointer_monster.h \
    include/commons.h \
    include/skyscrapers.h \
    include/triangle.h
