#pragma once
#if HAVE_FLAC
#include "encoder.h"

class EncoderFlac :
	public Encoder
{
public:
	EncoderFlac(void);
	~EncoderFlac(void);
	EncoderFlac(STD_PARMS) { setEncoder(STD_PARMS_PASS); }
	virtual int PrepareEncode() { return 0; }
	virtual int EncodeBuffer() { return 0; }
	virtual int FinishEncode() { return 0; }
};
#endif