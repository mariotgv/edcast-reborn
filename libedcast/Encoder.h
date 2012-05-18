#pragma once
#include "Sampler.h"
#include <string.h>
class Encoder;

#define MAKE_CODEC(ch0, ch1, ch2, ch3)                              \
                ((unsigned long)(unsigned char)(ch0) | ((unsigned long)(unsigned char)(ch1) << 8) |   \
                ((unsigned long)(unsigned char)(ch2) << 16) | ((unsigned long)(unsigned char)(ch3) << 24 ))

#define C_LAME		MAKE_CODEC('L', 'A', 'M', 'E')
#define C_LAMEJS	MAKE_CODEC('L', 'A', 'M', 'J')

#define C_AACPHE	MAKE_CODEC('A', 'A', 'C', 'P')
#define C_AACPPS	MAKE_CODEC('A', 'A', 'C', 'S')
#define C_AACPHI	MAKE_CODEC('A', 'A', 'C', 'H')
#define C_AACPLC	MAKE_CODEC('A', 'A', 'C', 'r')

#define C_FAAC		MAKE_CODEC('F', 'A', 'A', 'C')
#define C_VORBIS	MAKE_CODEC('O', 'G', 'G', 'V')
#define C_FLAC		MAKE_CODEC('F', 'L', 'A', 'C')

#define NO_PARAMS	   0
#define STEREO_BR	   1
#define STEREO_Q	   2
#define MONO_BR		   4
#define MONO_Q		   8
#define BOTH_BR		STEREO_BR|MONO_BR
#define BOTH_Q		STEREO_Q|MONO_Q
#define ALL_PARAM	BOTH_BR|BOTH_Q
#define SHARED_		 256
#define SHARED_BR	SHARED_|STEREO_BR
#define SHARED_Q	SHARED_|STEREO_Q
#define SHARED_BOTH	SHARED_|SHARED_BR|SHARED_Q
#define STEREO_ONLY	 512
#define MONO_ONLY	1024

typedef struct _Encoders {
	unsigned long codec;
	char *name;
	int valid;
	// [1, lo, hi, 0] [2, lo, inc, hi, 0] or [br1, br2, ... brn, 0] or [0,0] - q only
	unsigned long sbs[50]; 
	// [1, div, lo, hi, 0] [2, div, lo, inc, hi, 0] or [div, q1, q2, ... qn, 0] or [0,0] - bitrate only
	unsigned long sqs[50]; 
	unsigned long mbs[50];
	unsigned long mqs[50];
	// any of the above can be 4,0 ... which means not used, eg HE-AAC+PS - no such thing as mono
} Encoders;

typedef struct _EncoderList {
	struct _EncoderList * next;
	struct _EncoderList * prev;
	Encoder * encoder;
} EncoderList;

#define STD_PARMS unsigned long codec, unsigned long br, unsigned long q, bool inFloat, \
				  unsigned int chans, unsigned long ssr, unsigned long dsr, \
				  int asioChannel

#define STD_PARMS_PASS codec, br, q, inFloat, chans, ssr, dsr, asioChannel

class Encoder
{
public:
	Encoder(void);
	~Encoder(void);
	Encoder(STD_PARMS);
	void setEncoder(STD_PARMS);
	void setSampler(bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel);
	void addRef(void);
	void removeRef(void);
	int getRef(void);
	void addUse(void);
	void removeUse(void);
	int getUse(void);
	const char * getUniq() { return uniq; } // const char *
	bool isUniq(char *code) { return strcmp(code, uniq) == 0; }
	static Encoder * getEncoder(STD_PARMS);
	static void makeUniq(const char *code, unsigned long codec, unsigned int chans, unsigned long dsr, unsigned long br, unsigned long q, int asioChannel);
	void makeUniq(void);
	virtual int PrepareEncode() = 0;
	virtual int EncodeBuffer() = 0;
	virtual int FinishEncode() = 0;
	static const Encoders * GetCodec(int index);
private:
	char iniName[33];
	Sampler * mySampler;
	int refcount;
	int usecount;
	unsigned long _codec;
	unsigned long _sampleRate;
	unsigned long _bitrate;
	unsigned long _q;
	unsigned int _chans;
	int _asioChannel;

	char uniq[256];
	void addToList(void);
	static EncoderList *head;
};
