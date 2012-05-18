
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ProcessAudio.h"

ProcessAudio::ProcessAudio() : 
	audio(NULL),
	outputMono(NULL),
	outputStereo(NULL),
	outputSize(0),
	previousChannels(0)
{
}

ProcessAudio::~ProcessAudio()
{
	if(audio)
	{
		delete audio;
	}
}

void ProcessAudio::process(float * samples, int len, int chans, int inSampleRate, FLOAT inPreEmphasis, FLOAT inLimitdB, int doDeEmphasis)
{
	if(chans != previousChannels)
	{
		if(audio)
		{
			delete audio;
			audio = NULL;
		}
	}

	if(audio = NULL)
	{
		if(chans == 1)
		{
			audio = new audioMono();
		}
		else //if (chans == 2)
		{
			audio = new audioStereo();
		}
		/*
		{
			audio = new audioMulti(chans);
		}
		*/
		previousChannels = chans;
	}
	if(inLimitdB > 19.9)
	{
		audio->process(samples, len);
	}
	else if(inPreEmphasis <= 0.0)
	{
		audio->process(samples, len, inLimitdB);
	}
	else if(doDeEmphasis)
	{
		audio->process(samples, len, inSampleRate, inPreEmphasis, inLimitdB, doDeEmphasis);
	}
	else
	{
		audio->process(samples, len, inSampleRate, inPreEmphasis, inLimitdB);
	}
	outputMono = audio->outputMono;
	outputStereo = audio->outputStereo;
	outputSize = audio->outputSize;
}
