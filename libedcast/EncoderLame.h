#pragma once
#if HAVE_LAME
#include "encoder.h"

class EncoderLame :
	public Encoder
{
public:
	EncoderLame(void);
	~EncoderLame(void);
	EncoderLame(STD_PARMS) { setEncoder(STD_PARMS_PASS); }
	virtual int PrepareEncode() { return 0; }
	virtual int EncodeBuffer() { return 0; }
	virtual int FinishEncode() { return 0; }
};
#endif