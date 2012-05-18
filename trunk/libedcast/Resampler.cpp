#include "Resampler.h"

Resampler::Resampler(void)
{
}

Resampler::Resampler(int channels, int outfreq, int infreq)
{
	res_init(channels, outfreq, infreq, RES_END);
}

Resampler::~Resampler(void)
{
	if(__table) free(__table);
	if(__pool) free(__pool);
}

#ifndef M_PI
#define M_PI       3.14159265358979323846 
#endif

static int hcf(int arg1, int arg2)
{
	int mult = 1;

	while (~(arg1 | arg2) & 1)
		arg1 >>= 1, arg2 >>= 1, mult <<= 1;

	while (arg1 > 0)
	{
		if (~(arg1 & arg2) & 1)
		{
			arg1 >>= (~arg1 & 1);
			arg2 >>= (~arg2 & 1);
		}
		else if (arg1 < arg2)
			arg2 = (arg2 - arg1) >> 1;
		else
			arg1 = (arg1 - arg2) >> 1;
	}

	return arg2 * mult;
}

void Resampler::filt_sinc(double fc, double gain)
{
	int N = __outfreq * __taps;
	float * dest = __table;
	double s = fc / __outfreq;
	int mid, x;
	float *endpoint = dest + N,
		*base = dest,
		*origdest = dest;
	
	assert((int)__taps <= N);

	if ((N & 1) == 0)
	{
		*dest = 0.0;
		dest += __taps;
		if (dest >= endpoint)
			dest = ++base;
		N--;
	}

	mid = N / 2;
	x = -mid;

	while (N--)
	{
		*dest = (float)(x ? sin(x * M_PI * s) / (float)(x * M_PI) * __outfreq : fc) * (float)gain;
		x++;
		dest += __taps;
		if (dest >= endpoint)
			dest = ++base;
	}
//	assert(dest == origdest + width);
}


static double I_zero(double x)
{
	int n = 0;
	double u = 1.0,
		s = 1.0,
		t;

	do
	{
		n += 2;
		t = x / n;
		u *= t * t;
		s += u;
	} while (u > 1e-21 * s);

	return s;
}


void Resampler::win_kaiser(double alpha)
{
	float *dest = __table;
	int N = __outfreq * __taps;
	int width = __taps;
	double I_alpha, midsq;
	int x;
	float *endpoint = dest + N,
		*base = dest,
		*origdest = dest;

	assert(width <= N);

	if ((N & 1) == 0)
	{
		*dest = 0.0;
		dest += width;
		if (dest >= endpoint)
			dest = ++base;
		N--;
	}

	x = -(N / 2);
	midsq = (double)(x - 1) * (double)(x - 1);
	I_alpha = I_zero(alpha);

	while (N--)
	{
		*dest *= (float)I_zero(alpha * sqrt(1.0 - ((double)x * (double)x) / midsq)) / (float)I_alpha;
		x++;
		dest += width;
		if (dest >= endpoint)
			dest = ++base;
	}
	assert(dest == origdest + width);
}

bool Resampler::res_init(int channels, int outfreq, int infreq, res_parameter op1, ...)
{
	double beta = 16.0,
		cutoff = 0.80,
		gain = 1.0;
	int taps = 45;

	int factor;

	assert(channels > 0);
	assert(outfreq > 0);
	assert(infreq > 0);
	assert(taps > 0);

	if (channels <= 0 || outfreq <= 0 || infreq <= 0 || taps <= 0)
		return false;

	if (op1 != RES_END)
	{
		va_list argp;
		va_start(argp, op1);
		do
		{
			switch (op1)
			{
			case RES_GAIN:
				gain = va_arg(argp, double);
				break;

			case RES_CUTOFF:
				cutoff = va_arg(argp, double);
				assert(cutoff > 0.01 && cutoff <= 1.0);
				break;

			case RES_TAPS:
				taps = va_arg(argp, int);
				assert(taps > 2 && taps < 1000);
				break;
				
			case RES_BETA:
				beta = va_arg(argp, double);
				assert(beta > 2.0);
				break;
			default:
				assert("arglist" == "valid");
				return false;
			}
			op1 = va_arg(argp, res_parameter);
		} while (op1 != RES_END);
		va_end(argp);
	}

	factor = hcf(infreq, outfreq);
	outfreq /= factor;
	infreq /= factor;

	/* adjust to rational values for downsampling */
	if (outfreq < infreq)
	{
		/* push the cutoff frequency down to the output frequency */
		cutoff = cutoff * outfreq / infreq; 

        /* compensate for the sharper roll-off requirement
         * by using a bigger hammer */
        taps = taps * infreq/outfreq;
	}

	assert(taps >= (infreq + outfreq - 1) / outfreq);

	if ((__table = (float *)calloc(outfreq * taps, sizeof(float))) == NULL)
		return false;
	if ((__pool = (SAMPLE *)calloc(channels * taps, sizeof(SAMPLE))) == NULL)
	{
		free(__table);
		__table = NULL;
		return false;
	}

	__poolfill = taps / 2 + 1;
	__channels = channels;
	__outfreq = outfreq;
	__infreq = infreq;
	__taps = taps;
	__offset = 0;

	filt_sinc(cutoff, gain);
	win_kaiser(beta);

	return true;
}


static SAMPLE sum(float const *scale, int count, SAMPLE const *source, SAMPLE const *trigger, SAMPLE const *reset, int srcstep)
{
	float total = 0.0;

	while (count--)
	{
		total += *source * *scale;

		if (source == trigger)
			source = reset, srcstep = 1;
		source -= srcstep;
		scale++;
	}

	return total;
}


int Resampler::push(SAMPLE *pool, int * const poolfill, int * const offset, SAMPLE *dest, int dststep, SAMPLE const *source, int srcstep, size_t srclen)
{
	SAMPLE	* const destbase = dest,
		*poolhead = pool + *poolfill,
		*poolend = pool + __taps,
		*newpool = pool;
	SAMPLE const *refill, *base, *endpoint;
	int	lencheck;


//	assert(state);
	assert(pool);
	assert(poolfill);
	assert(dest);
	assert(source);

	assert(__poolfill != -1);
	
	lencheck = res_push_check(srclen);

	/* fill the pool before diving in */
	while (poolhead < poolend && srclen > 0)
	{
		*poolhead++ = *source;
		source += srcstep;
		srclen--;
	}

	if (srclen <= 0)
		return 0;

	base = source;
	endpoint = source + srclen * srcstep;

	while (source < endpoint)
	{
		*dest = sum(__table + *offset * __taps, __taps, source, base, poolend, srcstep);
		dest += dststep;
		*offset += __infreq;
		while (*offset >= (int)__outfreq)
		{
			*offset -= __outfreq;
			source += srcstep;
		}
	}

	assert(dest == destbase + lencheck * dststep);

	/* pretend that source has that underrun data we're not going to get */
	srclen += (source - endpoint) / srcstep;

	/* if we didn't get enough to completely replace the pool, then shift things about a bit */
	if (srclen < __taps)
	{
		refill = pool + srclen;
		while (refill < poolend)
			*newpool++ = *refill++;

		refill = source - srclen * srcstep;
	}
	else
		refill = source - __taps * srcstep;

	/* pull in fresh pool data */
	while (refill < endpoint)
	{
		*newpool++ = *refill;
		refill += srcstep;
	}

	assert(newpool > pool);
	assert(newpool <= poolend);

	*poolfill = newpool - pool;

	return (dest - destbase) / dststep;
}


int Resampler::res_push_max_input(size_t maxoutput)
{
	return maxoutput * __infreq / __outfreq;
}


int Resampler::res_push_check(size_t srclen)
{
	if (__poolfill < (int)__taps)
		srclen -= __taps - __poolfill;

	return (srclen * __outfreq - __offset + __infreq - 1) / __infreq;
}


int Resampler::res_push(SAMPLE **dstlist, SAMPLE const **srclist, size_t srclen)
{
	int result = -1, poolfill = -1, offset = -1, i;

//	assert(state);
	assert(dstlist);
	assert(srclist);
	assert(__poolfill >= 0);

	for (i = 0; i < (int)__channels; i++)
	{
		poolfill = __poolfill;
		offset = __offset;
		result = push(__pool + i * __taps, &poolfill, &offset, dstlist[i], 1, srclist[i], 1, srclen);
	}
	__poolfill = poolfill;
	__offset = offset;

	return result;
}


int Resampler::res_push_interleaved(SAMPLE *dest, SAMPLE const *source, size_t srclen)
{
	int result = -1, poolfill = -1, offset = -1, i;
	
//	assert(state);
	assert(dest);
	assert(source);
	assert(__poolfill >= 0);

	for (i = 0; i < (int)__channels; i++)
	{
		poolfill = __poolfill;
		offset = __offset;
		result = push(__pool + i * __taps, &poolfill, &offset, dest + i, __channels, source + i, __channels, srclen);
	}
	__poolfill = poolfill;
	__offset = offset;

	return result;
}


int Resampler::res_drain(SAMPLE **dstlist)
{
	SAMPLE *tail;
	int result = -1, poolfill = -1, offset = -1, i;

//	assert(state);
	assert(dstlist);
	assert(__poolfill >= 0);

	if ((tail = (SAMPLE *)calloc(__taps, sizeof(SAMPLE))) == NULL)
		return -1;

	for (i = 0; i < (int)__channels; i++)
	{
		poolfill = __poolfill;
		offset = __offset;
		result = push(__pool + i * __taps, &poolfill, &offset, dstlist[i], 1, tail, 1, __taps / 2 - 1);
	}
		
	free(tail);

	__poolfill = -1;

	return result;
}


int Resampler::res_drain_interleaved(SAMPLE *dest)
{
	SAMPLE *tail;
	int result = -1, poolfill = -1, offset = -1, i;

//	assert(state);
	assert(dest);
	assert(__poolfill >= 0);

	if ((tail = (SAMPLE *)calloc(__taps, sizeof(SAMPLE))) == NULL)
		return -1;

	for (i = 0; i < (int)__channels; i++)
	{
		poolfill = __poolfill;
		offset = __offset;
		result = push(__pool + i * __taps, &poolfill, &offset, dest + i, __channels, tail, 1, __taps / 2 - 1);
	}
		
	free(tail);

	__poolfill = -1;

	return result;
}


void Resampler::res_clear()
{
	assert(__table);
	assert(__pool);

	free(__table);
	free(__pool);
}

