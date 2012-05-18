#pragma once

#include "Limiters.h"

void makeShortInt(Limiters * limiter, short int * dstSamples, int numSamples, int dstChannels, int leftChannel = -1, int rightChannel = -1);
void makeLongInt(Limiters * limiter, long * dstSamples, int numSamples, int dstChannels, int leftChannel = -1, int rightChannel = -1);
void makeFaacFloat(Limiters * limiter, float * dstSamples, int numSamples, int dstChannels, int leftChannel = -1, int rightChannel = -1);
void makeFlacFloat(Limiters * limiter, float ** dstSamples, int numSamples, int dstChannels, int leftChannel = -1, int rightChannel = -1);
