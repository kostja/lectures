#include <stdlib.h>
#include <string.h>
#include "sort.h"
#define CACHELINE_SIZE 64

struct funnel {
	struct funnel  *lr[2];
	void *out;
	void *in;
	int size;
	int nmemb;
	int pos;
	cmp_t cmp;
};


struct funnel *
funnel_create(void *out, size_t nmemb, size_t size, cmp_t cmp)
{
	struct funnel *funnel = (struct funnel *)
		malloc(sizeof(struct funnel));
	funnel->out = out;
	funnel->nmemb = nmemb;
	funnel->size = size;
	funnel->pos = 0;
	funnel->cmp = cmp;
	funnel->in = NULL;

	/*
	 * Todo: here we must stop at sqrt(nmemb), not
	 * CACHELINE_SIZE, and instead of a "leaf" funnel,
	 * create a sqrt(sqrt(nmemb))-way sub-funnel for each
	 * leaf, and so on, until we reach CACHELINE_SIZE.
	 */
	if (size * nmemb > CACHELINE_SIZE) {
		size_t nmemb_left = nmemb/2;
		size_t nmemb_right = nmemb - nmemb_left;
		funnel->in = malloc(nmemb * size);
		funnel->lr[0] = funnel_create(funnel->in, nmemb_left,
					      size, cmp);
		funnel->lr[1] = funnel_create(funnel->in + nmemb_left * size,
					      nmemb_right, size, cmp);
	} else {
		funnel->lr[0] = funnel->lr[1] = 0;
	}
	return funnel;
}

void *
funnel_pop(struct funnel *funnel)
{
	if (funnel->pos >= funnel->nmemb)
		return NULL;
	return funnel->out + funnel->pos++ * funnel->size;
}

void
funnel_fill(struct funnel *funnel)
{
	/*
	 * @todo: add post-processing for output (swapping out)
	 * and pre-processing for input (swapping in.)
	 */
	/*
	 * Case 1: this is a leaf funnel.
	 * Sort the output.
	 * @todo: (re-)fill the leaf funnel on demand in funnel_pop()
	 * instead.
	 */
	if (funnel->lr[0] == NULL) {
		qsort(funnel->out, funnel->nmemb, funnel->size, funnel->cmp);
		return;
	}
	/*
	 * Case 2: this is an intermediate funnel.
	 * Sort the children and merge them.
	 */
	funnel_fill(funnel->lr[0]);
	funnel_fill(funnel->lr[1]);

	void *lr[2] = { NULL, NULL };
	int res = 0, i = 0;

	while (i < funnel->nmemb) {
		while (lr[res] == NULL) {
			lr[res] = funnel_pop(funnel->lr[res]);
			if (lr[res] == NULL) {
				/*
				 * The sort buffer of one of the
				 * funnels is exhausted. @todo:
				 * here we will need to refill it.
				 * Copy out the tail of the
				 * "bigger" sub-funnel.
				 */
				res = !res;
				/* Copy out the tail. */
				while (i < funnel->nmemb) {
					void *pos = (funnel->out + i++ *
						     funnel->size);
					memcpy(pos,
					       funnel_pop(funnel->lr[res]),
					       funnel->size);
				}
				return;
			}
			res = !res;
		}
		res = funnel->cmp(lr[0], lr[1]) > 0 ? 1 : 0;
		memcpy(funnel->out + i * funnel->size, lr[res], funnel->size);
		lr[res] = NULL;
		i++;
	}
}

void
sort(void *ptr, size_t nmemb, size_t size, cmp_t cmp)
{
	struct funnel *funnel = funnel_create(ptr, nmemb, size, cmp);
	funnel_fill(funnel);
}

