#ifndef SOME_FUCKING_SHIT_PROCESSAUDIO
#define SOME_FUCKING_SHIT_PROCESSAUDIO

#if _MSC_VER > 1000
#pragma once
#endif 

#include "Audio.h"
#include "AudioMono.h"
#include "AudioStereo.h"
#include "AudioMulti.h"
#include "AudioMultiMono.h"
#include "AudioMultiStereo.h"

class ProcessAudio
{
public:
	ProcessAudio();
	~ProcessAudio();
	Audio * audio;
	float * outputStereo;
	float * outputMono;
	int outputSize;
	void process(float * samples, int len, int chans, int inSampleRate, FLOAT inPreEmphasis, FLOAT inLimitdB, int doDeEmphasis);
private:
	int previousChannels;
};
#endif