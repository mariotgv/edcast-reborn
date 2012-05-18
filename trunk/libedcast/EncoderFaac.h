#pragma once
#if HAVE_FAAC
#include "encoder.h"

class EncoderFaac :
	public Encoder
{
public:
	EncoderFaac(void);
	~EncoderFaac(void);
	EncoderFaac(STD_PARMS) { setEncoder(STD_PARMS_PASS); }
	virtual int PrepareEncode() { return 0; }
	virtual int EncodeBuffer() { return 0; }
	virtual int FinishEncode() { return 0; }
};
#endif