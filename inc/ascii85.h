#ifndef ASCII85_H
#define ASCII85_H

typedef struct { const char *data; int n; } block;
char *toAscii85 (block b);
block fromAscii85 (const char *in);

#endif // ASCII85_H
