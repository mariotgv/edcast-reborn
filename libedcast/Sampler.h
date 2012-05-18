#pragma once
#include "Resampler.h"
#include <string.h>
enum _SRC_TYPE {
	SRC_MONOSHORT,
	SRC_STEREOSHORT,
	SRC_MONOFLOAT,
	SRC_STEREOFLOAT
};

class Sampler;
typedef struct _SamplerList {
	struct _SamplerList * next;
	struct _SamplerList * prev;
	Sampler * sampler;
} SamplerList;

typedef enum _SRC_TYPE SRC_TYPE;
/*
Usually, only a single sampler will ever be needed
One exception is the MultiAsio version, which will have up to N mono samplers, or N/2 stereos samplers
Where N = number of available ASIO channels
Currently, only a mono version is written

asioChannel = 0xRRLL
XX = 0x0000, not multi source 
XX = 0x00LL, LL-1 = Mono Channel
XX = 0xRRLL, RR-1 LL-1 = Stereo Asio Channels

In the case of Multi source, the data we receive is just the data we want, the flag here is for matching purposes
*/
class Sampler
{
public:
	Sampler(void);
	Sampler(bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel);
	~Sampler(void);
	int getMonoShort(short *buff);
	int getStereoShort(short * buff);
	int getMonoFloat(float * buff);
	int getStereoFloat(float * buff);
	void fillBuffer(short *s, unsigned int len);
	void fillBuffer(float *s, unsigned int len);
	const char * getUniq() { return uniq; }
	bool isUniq(char *code) { return strcmp(code, uniq) == 0; }
	static void makeUniq(const char * code, bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel);
	void makeUniq(void);
	static Sampler *getSampler(bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel);
	void addRef(void);
	void removeRef(void);
	int getRef(void);
	void addUse(void);
	void removeUse(void);
	int getUse(void);
private:
	void _fillBuffer(short *s, unsigned int len, SRC_TYPE typ);
	void _fillBuffer(float *s, unsigned int len, SRC_TYPE typ);
	Resampler * resampler;
	void MonoMonoShort(short *buff);
	void MonoStereoShort(short * buff);
	void StereoMonoShort(short * buff);
	void StereoStereoShort(short * buff);
	void MonoMonoFloat(float * buff);
	void MonoStereoFloat(float * buff);
	void StereoMonoFloat(float * buff);
	void StereoStereoFloat(float * buff);
	
	void FloatToShort(void);
	void ShortToFloat(void);
	void ShortToFloat(short * s, float * d, unsigned int n);
	void SetBufferSize(unsigned int n);
	void SetResBufferSize(unsigned int n);

	short * shortBuffer;
	float * floatBuffer;
	float * resampleBuffer;
	unsigned int bufferLen;
	unsigned int resBufferLen;
	unsigned int usedBufferLen;

	SRC_TYPE src;
	unsigned int src_samplerate;
	unsigned int dst_samplerate;
	bool needResample;
	bool needFloat;
	bool needShort;
	int inChans;
	int _asioChannel;

	int refcount;
	int usecount;
	char uniq[256];
	void addToList(void);
	static SamplerList *head;
};
