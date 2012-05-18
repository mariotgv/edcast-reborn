#include "Encoder.h"
#include "EncoderLame.h"
#include "EncoderVorbis.h"
#include "EncoderFlac.h"
#include "EncoderFaac.h"
#include "EncoderAacp.h"
#include <stdio.h>
Encoder::Encoder(void)
{
}

Encoder::~Encoder(void)
{
	if(mySampler) mySampler->removeRef();
}

Encoder::Encoder(STD_PARMS) :
	_codec(codec),
	_bitrate(br),
	_q(q),
	_chans(chans),
	_sampleRate(ssr),
	_asioChannel(asioChannel)
{
	setSampler(inFloat, chans, ssr, dsr, asioChannel);
}

void
Encoder::setEncoder(STD_PARMS)
{
	_codec=codec;
	_bitrate = br;
	_q = q;
	_chans = chans;
	_sampleRate = ssr;
	makeUniq();
	setSampler(inFloat, chans, ssr, dsr, asioChannel);
}

void 
Encoder::setSampler(bool inFloat, unsigned int chans, unsigned long ssr, unsigned long dsr, int asioChannel) { 
	if(mySampler) mySampler->removeRef();
	mySampler = Sampler::getSampler(inFloat, chans, ssr, dsr, asioChannel);
	mySampler->addRef();
}

void
Encoder::makeUniq(const char * code, unsigned long codec, unsigned int chans, unsigned long dsr, unsigned long br, unsigned long q, int asioChannel)
{
	sprintf((char *) code, "E%luC%ldS%ldB%ldQ%ldA%d", codec, chans, dsr, br, q, asioChannel);
}

void
Encoder::makeUniq(void)
{
	makeUniq(uniq, _codec, _chans, _sampleRate, _bitrate, _q, _asioChannel);
}
Encoder *
Encoder::getEncoder(STD_PARMS)
{
	char code[256];
	makeUniq(code, codec, chans, dsr, br, q, asioChannel);
	EncoderList * curr;
	for(curr = head; curr; curr=curr->next)
		if(curr->encoder->isUniq(code))
			break;
	if(curr) return curr->encoder;
	switch (codec) {
#if HAVE_LAME
		case C_LAME: 
		case C_LAMEJS: 
			return (Encoder *) new EncoderLame(STD_PARMS_PASS);
#endif
#if HAVE_AACP
		case C_AACPHE:
		case C_AACPPS:
		case C_AACPHI:
		case C_AACPLC:
			return (Encoder *) new EncoderAacp(STD_PARMS_PASS);
#endif
#if HAVE_FAAC
		case C_FAAC:
			return (Encoder *) new EncoderFaac(STD_PARMS_PASS);
#endif
#if HAVE_VORBIS
		case C_VORBIS:
			return (Encoder *) new EncoderVorbis(STD_PARMS_PASS);
#endif
#if HAVE_FLAC
		case C_FLAC:
			return (Encoder *) new EncoderFlac(STD_PARMS_PASS);
#endif
		default:
			return (Encoder *) 0;
	}
}

void 
Encoder::addToList(void) {
	EncoderList *me = new EncoderList();
	me->next = head;
	me->encoder = this;
	me->prev = (EncoderList *) 0;
	if(head) {
		head->next->prev = me;
	}
	head = me;
}

void Encoder::addRef(void) { refcount++; }
void Encoder::removeRef(void) { refcount--; }
int Encoder::getRef(void) { return refcount; }
void Encoder::addUse(void) { usecount++; mySampler->addUse(); }
void Encoder::removeUse(void) { mySampler->removeUse(); usecount--;}
int Encoder::getUse(void) { return usecount; }

static Encoders encoders[] = {
	// treat Joint Stereo as a different codec
#if HAVE_LAME
	{C_LAME, "LAME MP3",				SHARED_BOTH,	{1, 8, 320}, 
														{2, 1, 0, 1, 9}},
	{C_LAMEJS, "LAME MP3 Joint Stereo",	SHARED_BOTH,	{1, 8, 320}, 
														{2, 1, 0, 1, 9}},
#endif
#if HAVE_VORBIS
	{C_VORBIS, "Ogg Vorbis",			SHARED_BOTH,	{1, 8, 320}, 
														{1, 100, 10, 100}},
#endif
#if HAVE_FLAC
	{C_FLAC, "Ogg Flac",				NO_PARAMS },// no q or br, it's a lossless codec after all
#endif
#if HAVE_FAAC
	{C_FAAC, "FAAC",					SHARED_Q,		{0,0}, 
														{1,100,10,100}},
#endif
	// treat AACP v1 and v2 as separate codecs, because at many given bitrates, one can have PS or STEREO
	// PS is what makes v2 v2 - also, PS only has stereo options, due to it's nature
#if HAVE_AACP
	{C_AACPHE, "HE-AAC v1 (LC+SBR)", BOTH_BR,			{12,16,20,24,28,32,40,48,56,64,80,96,112,128},
														{0,0},
														{8,10,12,16,20,24,28,32,40,48,56,64},
														{0,0}},
	{C_AACPPS, "HE-AAC v2 (LC+SBR+PS)",	STEREO_BR,		{12,16,20,24,28,32,40,48,56},
														{0,0}},
	{C_AACPHI, "HE-AAC High Bitrate",	BOTH_BR,		{96,112,128,160,192,224,256,288,320},
														{0,0},
														{64,80,96,112,128,160},
														{0,0}},
	{C_AACPLC, "LC-AAC",				BOTH_BR,		{16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224,256,288,320},
														{0,0},
														{8,10,12,16,20,24,28,32,40,48,56,64,80,96,112,128,160},
														{0,0}},
#endif
	{0}
};

const Encoders * 
Encoder::GetCodec(int index)
{
	if(index < sizeof(encoders) / sizeof(Encoders))
		return(&encoders[index]);
	return 0;
}
