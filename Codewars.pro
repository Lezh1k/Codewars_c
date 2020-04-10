TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += include \
  /opt/intel/compilers_and_libraries/linux/mkl/include

DEFINES += __x86_64__
QMAKE_CFLAGS += -msse4.2 -msse4.1
LIBS += -lgmp -lstemmer

SOURCES += \
    src/bignum.c \
    src/central_attention.c \
    src/main.c \
    src/brainfuck.c \
    src/merge_intervals.c \
    src/num_sum_without_va.c \
    src/regexp.c \
    src/search_string.c \
    src/roman.c \
    src/int_partitions.c \
    src/ascii85.c \
    src/bignum_mul.c \
    src/pointer_monster.c \
    src/commons.c \
    src/mul.s \
    src/skyscrapers.c \
    src/snail.c \
    src/triangle.c \
    src/voronoi.c

HEADERS += \
    include/bignum.h \
    include/brainfuck.h \
    include/central_attention.h \
    include/merge_intervals.h \
    include/num_sum_without_va.h \
    include/regexp.h \
    include/search_string.h \
    include/roman.h \
    include/int_partitions.h \
    include/ascii85.h \
    include/bignum_mul.h \
    include/pointer_monster.h \
    include/commons.h \
    include/skyscrapers.h \
    include/snail.h \
    include/triangle.h \
    include/voronoi.h
