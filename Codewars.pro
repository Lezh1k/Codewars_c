TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CFLAGS += -std=gnu17
DEFINES += __STDC_WANT_LIB_EXT1__=1 \
    SECONDS_TO_00=$$system(./seconds_to_00.sh)
LIBS += -lm -lpthread


INCLUDEPATH += include \
    /usr/include/ImageMagick-7/ \

SOURCES += \
    src/bignum.c \
    src/bmp.c \
    src/central_attention.c \
    src/coins.c \
    src/fft.c \
    src/gol.c \
    src/lcs.c \
    src/main.c \
    src/brainfuck.c \
    src/math_expression.c \
    src/merge_intervals.c \
    src/next_smaller_number.c \
    src/num_sum_without_va.c \
    src/ocr.c \
    src/ocr_obsolete.c \
    src/queue.c \
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
    src/sourcemappings.c \
    src/spiral.c \
    src/triangle.c \
    src/voronoi.c

HEADERS += \
    include/bignum.h \
    include/bmp.h \
    include/brainfuck.h \
    include/central_attention.h \
    include/gol.h \
    include/lcs.h \
    include/math_expression.h \
    include/merge_intervals.h \
    include/next_smaller_number.h \
    include/num_sum_without_va.h \
    include/ocr.h \
    include/queue.h \
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
    include/sourcemappings.h \
    include/spiral.h \
    include/triangle.h \
    include/voronoi.h
