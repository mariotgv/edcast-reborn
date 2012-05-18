#include "audioConverter.h"

//ShortInt
inline void makeShortIntMono(float * samples, short int * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (short int)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeShortIntMonoMono(float * samplesLeft, float * samplesRight, short int * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (short int)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(dst++) = (short int)(*(samplesRight++) * 32767.0);
	}
}

inline void makeShortIntStereo(float * samples, short int * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (short int)(*(samples++) * 32767.0);
		*(dst++) = (short int)(*(samples++) * 32767.0);
	}
}

void makeShortInt(Limiters * limiter, short int * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeShortIntMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeShortIntStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeShortIntMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeShortIntMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}
// LongInt
inline void makeLongIntMono(float * samples, long * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (long)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeLongIntMonoMono(float * samplesLeft, float * samplesRight, long * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (long)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(dst++) = (long)(*(samplesRight++) * 32767.0);
	}
}

inline void makeLongIntStereo(float * samples, long * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (long)(*(samples++) * 32767.0);
		*(dst++) = (long)(*(samples++) * 32767.0);
	}
}

void makeLongInt(Limiters * limiter, long * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeLongIntMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeLongIntStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeLongIntMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeLongIntMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}
//  faacFloat
inline void makeFaacFloatMono(float * samples, float * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (float)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeFaacFloatMonoMono(float * samplesLeft, float * samplesRight, float * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (float)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(dst++) = (float)(*(samplesRight++) * 32767.0);
	}
}

inline void makeFaacFloatStereo(float * samples, float * dst, int numSamples)
{
	while(numSamples--)
	{
		*(dst++) = (float)(*(samples++) * 32767.0);
		*(dst++) = (float)(*(samples++) * 32767.0);
	}
}

void makeFaacFloat(Limiters * limiter, float * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeFaacFloatMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeFaacFloatStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeFaacFloatMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeFaacFloatMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}

//  flacFloat
inline void makeFlacFloatMono(float * samples, float * * ppdst, int numSamples)
{
	float * dst = ppdst[0];
	while(numSamples--)
	{
		*(dst++) = (float)(*(samples++) * 32767.0);
		samples++;
	}
}

inline void makeFlacFloatMonoMono(float * samplesLeft, float * samplesRight, float * * ppdst, int numSamples)
{
	float * left = ppdst[0];
	float * right = ppdst[1];

	while(numSamples--)
	{
		*(left++) = (float)(*(samplesLeft++) * 32767.0);
		samplesLeft++;
		samplesRight++;
		*(right++) = (float)(*(samplesRight++) * 32767.0);
	}
}

inline void makeFlacFloatStereo(float * samples, float * * ppdst, int numSamples)
{
	float * left = ppdst[0];
	float * right = ppdst[1];

	while(numSamples--)
	{
		*(left++) = (float)(*(samples++) * 32767.0);
		*(right++) = (float)(*(samples++) * 32767.0);
	}
}

void makeFlacFloat(Limiters * limiter, float * * dstSamples, int numSamples, int dstChannels, int leftChannel, int rightChannel)
{
	int srcChannels = limiter->channels;

	if(srcChannels < 3)
	{
		if(dstChannels == 1) // 1:1
		{
			makeFlacFloatMono(limiter->outputMono, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			makeFlacFloatStereo(limiter->outputStereo, dstSamples, numSamples);
		}
	}
	else // channel x[y] -> mono[stereo]
	{
		if(dstChannels == 1) // 1:1
		{
			int srcoffsetM = limiter->outputSize * 2 * leftChannel;
			float * srcM = (limiter->outputMono) + srcoffsetM;

			makeFlacFloatMono(srcM, dstSamples, numSamples);
		}
		else // if(dstChannels == 2) // 2:2
		{
			int srcoffsetL = limiter->outputSize * 2 * leftChannel;
			float * srcL = (limiter->outputStereo) + srcoffsetL;

			int srcoffsetR = limiter->outputSize * 2 * rightChannel;
			float * srcR = (limiter->outputStereo) + srcoffsetR;

			makeFlacFloatMonoMono(srcL, srcR, dstSamples, numSamples);
		}
	}
}

