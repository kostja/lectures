#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sort.h"
#define CACHELINE_SIZE 64

/** struct funnel - represents a top-level funnel,
 * a funnel leaf and a binary merger.
 */
struct funnel {
	/** left and right funnels of the binary mergers */
	struct funnel  *lr[2];
	/** merger output */
	void *out;
	/** merger input - for a leaf funnel.  */
	void *in;
	/** element size */
	int size;
	/** the total number of elements */
	int nmemb;
	/** if the funnel is populated, the position in output,
	 * */
	int pos;
	cmp_t cmp;
	bool exhausted;
};


struct funnel *
funnel_create(void *in, void *out, size_t nmemb, size_t size, cmp_t cmp)
{
	struct funnel *funnel = (struct funnel *)
		malloc(sizeof(struct funnel));
	funnel->in = in;
	funnel->out = out;
	funnel->nmemb = nmemb;
	funnel->size = size;
	funnel->cmp = cmp;

	/*
	 * Todo: here we must stop at sqrt(nmemb), not
	 * CACHELINE_SIZE, and instead of a "leaf" funnel,
	 * create a sqrt(sqrt(nmemb))-way sub-funnel for each
	 * leaf, and so on, until we reach CACHELINE_SIZE.
	 */
	if (size * nmemb > K) {
		size_t nmemb_left = nmemb/2;
		size_t nmemb_right = nmemb - nmemb_left;
		void *out = malloc(K * lg(nmemb) * size);
		funnel->lr[0] = funnel_create(funnel->in, out, nmemb_left,
					      size, cmp);
		funnel->lr[1] = funnel_create(funnel->in + nmemb_left * size,
					      out + nmemb_left * size,
					      nmemb_right, size, cmp);
	} else if (size * nmemb > CACHELINE_SIZE) {
		K = pow(nmemb, 1/3.0)
	} else {
		funnel->lr[0] = funnel->lr[1] = 0;
	}
	return funnel;
}

void *
funnel_pop(struct funnel *funnel)
{
	if (funnel->exhausted)
		return NULL;
	if (funnel->pos >= funnel->nmemb) {
		if (funnel->lr[0] == NULL && funnel->lr[1] == NULL) {
			/* leaf funnel */
			funnel->exhausted = true;
			return NULL;
		}
		if (funnel->lr[0]->exhausted && funnel->lr[1]->exhausted) {
			funnel->exhausted = true;
			return NULL;
		}
		if (funnel->lr[0]->exhausted || funnel->lr[1]->exhausted)
			return funnel_pop(funnel->lr[!funnel->lr[0]->exhausted]);
		funnel_fill(funnel);
	}
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
	 * Case 1: this is a leaf.
	 * Sort the output.
	 */
	if (funnel->lr[0] == NULL) {
		qsort(funnel->in, funnel->nmemb, funnel->size, funnel->cmp);
		funnel->out = funnel->in;
		return;
	}
	/*
	 * Case 2: this is a top merger of some sub-funnel.
	 * Nothing needs to be done, it will be re-filled on
	 * demand in funnel_pop().
	 */
	/*
	 * Case 3: this is an intermediate funnel.
	 * Fill the children and merge them.
	 */
	funnel_fill(funnel->lr[0]);
	funnel_fill(funnel->lr[1]);

	void *lr[2] = { NULL, NULL };
	int res = 0, i = 0;
	int size = funnel->size;

	lr[res] = funnel_pop(funnel->lr[res]);
	assert(lr[res]);
	res = !res;
	while (i < funnel->nmemb) {
		lr[res] = funnel_pop(funnel->lr[res]);
		if (lr[res] == NULL) {
			/*
			 * The sort buffer of one of the
			 * funnels is exhausted. @todo:
			 * here we will need to refill it.
			 */
			res = !res;
			/*
			 * Copy out the tail of the
			 * "bigger" sub-funnel.
			 */
			do {
				memcpy(funnel->out + size * i++, lr[res], size);
				lr[res] = funnel_pop(funnel->lr[res]);
			} while (lr[res]);
			assert(i == funnel->nmemb);
			return;
		}
		res = funnel->cmp(lr[0], lr[1]) > 0 ? 1 : 0;
		memcpy(funnel->out + size * i++, lr[res], size);
		lr[res] = NULL;
	}
	funnel->pos = 0;
}

void
sort(void *ptr, size_t nmemb, size_t size, cmp_t cmp)
{
	void *out = malloc(size * nmemb);
	struct funnel *funnel = funnel_create(ptr, out, nmemb, size, cmp);
	while (! funnel->exhausted) {
		funnel_fill(funnel);
		memcpy(out, funnel->out, funnel->nmemb * funnel->size);
	}
	memcpy(ptr, out, size * nmemb);
}

