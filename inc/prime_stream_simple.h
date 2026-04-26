#ifndef PRIME_STREAM_SIMPLE_H
#define PRIME_STREAM_SIMPLE_H

struct prime_stream;

struct prime_stream *prime_stream_new(void);
unsigned prime_stream_next(struct prime_stream *ps);
void prime_stream_destroy(struct prime_stream *ps);

#endif
