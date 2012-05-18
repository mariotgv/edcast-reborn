#pragma once
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

typedef float SAMPLE;

typedef enum
{
	RES_END,
	RES_GAIN,	/* (double)1.0 */
	RES_CUTOFF,	/* (double)0.80 */ 
	RES_TAPS,	/* (int)45 */
	RES_BETA	/* (double)16.0 */
} res_parameter;

class Resampler
{
public:
	Resampler(void);
	Resampler(int channels, int outfreq, int infreq);
	~Resampler(void);

/*
 * Configure *state to manage a data stream with the specified parameters.  The
 * string 'params' is currently unspecified, but will configure the parameters
 * of the filter.
 *
 * This function allocates memory, and requires that res_clear() be called when
 * the buffer is no longer needed.
 *
 *
 * All counts/lengths used in the following functions consider only the data in
 * a single channel, and in numbers of samples rather than bytes, even though
 * functionality will be mirrored across as many channels as specified here.
 */
	bool res_init(int channels, int outfreq, int infreq, res_parameter op1, ...);


/*
 *  Returns the maximum number of input elements that may be provided without
 *  risk of flooding an output buffer of size maxoutput.  maxoutput is
 *  specified in counts of elements, NOT in bytes.
 */
	int res_push_max_input(size_t maxoutput);


/*
 * Returns the number of elements that will be returned if the given srclen
 * is used in the next call to res_push().
 */
	int res_push_check(size_t srclen);


/*
 * Pushes srclen samples into the front end of the filter, and returns the
 * number of resulting samples.
 *
 * res_push(): srclist and dstlist point to lists of pointers, each of which
 * indicates the beginning of a list of samples.
 *
 * res_push_interleaved(): source and dest point to the beginning of a list of
 * interleaved samples.
 */
	int res_push(SAMPLE **dstlist, SAMPLE const **srclist, size_t srclen);
	int res_push_interleaved(SAMPLE *dest, SAMPLE const *source, size_t srclen);


/*
 * Recover the remaining elements by flushing the internal pool with 0 values,
 * and storing the resulting samples.
 *
 * After either of these functions are called, *state should only re-used in a
 * final call to res_clear().
 */
	int res_drain(SAMPLE **dstlist);
	int res_drain_interleaved(SAMPLE *dest);


/*
 * Free allocated buffers, etc.
 */
	void res_clear(void);

private:
	//res_state state;
	unsigned int __channels, __infreq, __outfreq, __taps;
	float *__table;
	SAMPLE *__pool;

	/* dynamic bits */
	int __poolfill;
	int __offset;
	void filt_sinc(double fc, double gain);
	void win_kaiser(double alpha);

	int push(SAMPLE *pool, int * const poolfill, int * const offset, SAMPLE *dest, int dststep, SAMPLE const *source, int srcstep, size_t srclen);

};
