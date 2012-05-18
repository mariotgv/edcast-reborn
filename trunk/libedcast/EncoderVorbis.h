#pragma once
#if HAVE_VORBIS
#include "encoder.h"

class EncoderVorbis :
	public Encoder
{
public:
	EncoderVorbis(void);
	~EncoderVorbis(void);
	EncoderVorbis(STD_PARMS) { setEncoder(STD_PARMS_PASS); }
	virtual int PrepareEncode() { return 0; }
	virtual int EncodeBuffer() { return 0; }
	virtual int FinishEncode() { return 0; }
};
#endif