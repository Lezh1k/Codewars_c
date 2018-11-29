#ifndef SEARCH_STRING_H
#define SEARCH_STRING_H

#include <stdbool.h>

int search_substr(const char *full_text, const char *search_text, bool allow_overlap);

#endif // SEARCH_STRING_H
