#pragma once
#if HAVE_AACP
#include "encoder.h"

class EncoderAacp :
	public Encoder
{
public:
	EncoderAacp(void);
	~EncoderAacp(void);
	EncoderAacp(STD_PARMS) { setEncoder(STD_PARMS_PASS); }
	virtual int PrepareEncode() { return 0; }
	virtual int EncodeBuffer() { return 0; }
	virtual int FinishEncode() { return 0; }
};
#endif