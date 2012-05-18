#include "Sampler.h"
#include <memory.h>
#include <malloc.h>
#include <stdio.h>

Sampler::Sampler(void) {
}

Sampler::~Sampler(void) {
	SetBufferSize(0);
	SetResBufferSize(0);
	if(resampler) delete resampler;

	SamplerList * curr;

	for(curr = head; curr; curr=curr->next)
		if(curr->sampler = this) {
			SamplerList * n = curr->next;
			SamplerList * p = curr->prev;
			if(p) p->next = n;
			if(n) n->prev = p;
			curr->sampler = (Sampler *) 0;
			delete curr;
			break;
		}
}

// one sampler per device type:chans:ssr:dsr
Sampler::Sampler(bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel) :
	src_samplerate(ssr),
	dst_samplerate(dsr),
	_asioChannel(asioChannel),
	bufferLen(0), 
	resBufferLen(0),
	needFloat(inFloat),
	needShort(!inFloat),
	inChans(2 - (chans % 2)),
	needResample(ssr != dsr),
	refcount(0),
	usecount(0)
{
	if(inFloat) {
		if(inChans == 1) {
			src = SRC_MONOFLOAT;
		}
		else {
			src = SRC_STEREOFLOAT;
		}
	}
	else {
		if(inChans == 1) {
			src = SRC_MONOSHORT;
		}
		else {
			src = SRC_STEREOSHORT;
		}
	}
	makeUniq();
	SetBufferSize(bufferLen);
	SetResBufferSize(resBufferLen);
	if(needResample) {
		needFloat = true;
		resampler = new Resampler(inChans, dsr, ssr);
	}
	addToList();
}

int 
Sampler::getMonoShort(short *buff)
{
	if(inChans == 1 ) MonoMonoShort(buff);
	else StereoMonoShort(buff);
	return usedBufferLen;
}

int 
Sampler::getStereoShort(short * buff)
{
	if(inChans == 1 ) MonoStereoShort(buff);
	else StereoStereoShort(buff);
	return usedBufferLen;
}

int 
Sampler::getMonoFloat(float * buff)
{
	if(inChans == 1 ) MonoMonoFloat(buff);
	else StereoMonoFloat(buff);
	return usedBufferLen;
}

int 
Sampler::getStereoFloat(float * buff)
{
	if(inChans == 1 ) MonoStereoFloat(buff);
	else StereoStereoFloat(buff);
	return usedBufferLen;
}


void
Sampler::SetBufferSize(unsigned int n)
{
	if(shortBuffer) { free(shortBuffer); shortBuffer = (short *) 0; }
	if(floatBuffer) { free(floatBuffer); floatBuffer = (float *) 0; }
	
	if(n && needShort) shortBuffer = (short *) calloc(n * inChans, sizeof(short));
	if(n && needFloat) floatBuffer = (float *) calloc(n * inChans, sizeof(float));

	bufferLen = n;
}

void
Sampler::SetResBufferSize(unsigned int n)
{
	if(resampleBuffer) free(resampleBuffer);

	if(n) resampleBuffer = (float *) calloc(n * inChans, sizeof(float));
	resBufferLen = n;
}

void
Sampler::fillBuffer(short *s, unsigned int len)
{
	if(!usecount) return;
	SRC_TYPE srcType = src;
	if(needResample) {
		needFloat = true;
		if(len > resBufferLen) {
			SetResBufferSize(len);
		}
		unsigned int rLen = resampler->res_push_check(len);
		if(rLen > bufferLen) {
			SetBufferSize(rLen);
		}
		switch(src){
			case SRC_MONOSHORT:
				srcType = SRC_MONOFLOAT;
				ShortToFloat(s, resampleBuffer, len);
				len = resampler->res_push_interleaved(floatBuffer, resampleBuffer, len);
				return _fillBuffer(floatBuffer, len, SRC_MONOFLOAT);
			case SRC_STEREOSHORT:
				srcType = SRC_STEREOFLOAT;
				ShortToFloat((short *)s, resampleBuffer, len*2);
				len = resampler->res_push_interleaved(floatBuffer, resampleBuffer, len);
				return _fillBuffer(floatBuffer, len, SRC_STEREOFLOAT);
			default:
				return;
		}
	}
	_fillBuffer(s, len, srcType);
}

void 
Sampler::fillBuffer(float *s, unsigned int len)
{
	if(!usecount) return;
	_fillBuffer(s, len, src);
}

void 
Sampler::_fillBuffer(short *s, unsigned int len, SRC_TYPE typ)
{
	short * sb;
	if(len > bufferLen)
		SetBufferSize(len);
	usedBufferLen = len;
	sb = shortBuffer;
	len *= inChans;
	while(len--) 
		*(sb++) = *(s++);
	if(needFloat || needResample)
		ShortToFloat();
}

void 
Sampler::_fillBuffer(float *s, unsigned int len, SRC_TYPE typ)
{
	float * fb;
	if(len > bufferLen)
		SetBufferSize(len);
	usedBufferLen = len;
	fb = floatBuffer;
	len *= inChans;
	while(len--) 
		*(fb++) = *(s++);
	if(needShort)
		FloatToShort();
}

void 
Sampler::MonoMonoShort(short *buff)
{
	short *s=shortBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = *(s++);
	}
}

void
Sampler::MonoStereoShort(short * buff)
{
	short *s=shortBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = *(s);
		*(buff++) = *(s++);
	}
}

void 
Sampler::StereoMonoShort(short * buff)
{
	short *s=shortBuffer;
	int t;
	unsigned int len = usedBufferLen;
	while(len--) {
		t = (*(s++) + *(s++))>>1;
		*(buff++) = (short) t;
	}
}

void 
Sampler::StereoStereoShort(short * buff)
{
	short *s=shortBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = *(s++);
		*(buff++) = *(s++);
	}
}

void 
Sampler::MonoMonoFloat(float * buff)
{
	float *s=floatBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = *(s++);
	}
}

void 
Sampler::MonoStereoFloat(float * buff)
{
	float *s=floatBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = *(s);
		*(buff++) = *(s++);
	}
}

void 
Sampler::StereoMonoFloat(float * buff)
{
	float *s=floatBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = (*(s++) + *(s++)) / 2;
	}
}

void
Sampler::StereoStereoFloat(float * buff)
{
	float *s=floatBuffer;
	unsigned int len = usedBufferLen;
	while(len--) {
		*(buff++) = *(s++);
		*(buff++) = *(s++);
	}
}

void 
Sampler::FloatToShort()
{
	for(unsigned int i = 0; i < usedBufferLen; i++) {
		shortBuffer[i] = (short)(floatBuffer[i] * 32767.0);
	}
}

void 
Sampler::ShortToFloat()
{
	for(unsigned int i = 0; i < usedBufferLen; i++) {
		floatBuffer[i] = (float)(shortBuffer[i] / 32768.0);
	}
}

void 
Sampler::ShortToFloat(short * s, float * d, unsigned int n)
{
	while(n--) {
		*(d++) = (float)(*(s++) / 32768.0);
	}
}

void
Sampler::makeUniq(const char * code, bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel)
{
	sprintf((char *) code, "%cC%ldS%ldD%ldA%d", inFloat?'F':'I', chans, ssr, dsr, asioChannel);
}

void
Sampler::makeUniq(void)
{
	makeUniq(uniq, needFloat, inChans, src_samplerate, dst_samplerate, _asioChannel);
}

Sampler *
Sampler::getSampler(bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel)
{
	// code needs to be a string buffer
	char code[256];
	makeUniq(code, inFloat, chans, ssr, dsr, asioChannel);
	SamplerList * curr;
	for(curr = head; curr; curr=curr->next)
		if(curr->sampler->isUniq(code))
			break;
	if(curr) return curr->sampler;
	return new Sampler(inFloat, chans, ssr, dsr, asioChannel);
}

void 
Sampler::addToList(void) {
	SamplerList *me = new SamplerList();
	me->next = head;
	me->sampler = this;
	me->prev = (SamplerList *) 0;
	if(head) {
		head->next->prev = me;
	}
	head = me;
}
void Sampler::addRef(void) { refcount++; }
void Sampler::removeRef(void) { refcount--; }
int Sampler::getRef(void) { return refcount; }
void Sampler::addUse(void) { usecount++; }
void Sampler::removeUse(void) { usecount--; }
int Sampler::getUse(void) { return usecount; }
