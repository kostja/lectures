#ifndef INCLUDES_FUNNELSORT_SORT_H
#define INCLUDES_FUNNELSORT_SORT_H
#include <stddef.h>

typedef int (*cmp_t)(const void *, const void *);
void
sort(void *ptr, size_t nmemb, size_t size, cmp_t cmp);

#endif /* INCLUDES_FUNNELSORT_SORT_H */
