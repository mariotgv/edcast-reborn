#pragma once
#include "Encoder.h"

/* 
 basecode - derived server types: icecast, icecast2, shoutcast, ultravox2.1=shoutcast2, ultravox2, ultravox3 and my own outcast
 ultravox(2,2.1 and 3) will be a later addition
 outcast is same as shoutcast in many ways, just a convenience for entering the following:
 application
 instance
 stream_id
 path

 outcast streaming protocol, as mentioned, similar to shoutcast.

 shoutcast:
	password\r\n
	Content-type: ...\r\n
	icy-*: ...\r\n
	\r\n

 etc, outcast:
	cast \t app \t instance \t stream_id \t password\r\n
	Content-type: ...\r\n
	icy-*: ...\r\n
	\r\n
 - or -
	application/instance:stream_id:password\r\n
	Content-type: ...\r\n
	icy-*: ...\r\n
	\r\n

 outcast will return which port accepts metadata, and possibly what the path is
 outcast metadata update
 
 admin.cgi?pass=...&app=...&inst=...&stream=...&mode=updinfo&....
 or
 admin.cgi?pass=application/instance:stream_id:password&mode=updinfo&....
 app, inst and id can NOT have / or : in them anyway, so there should be no issues with deciphering the second format


 the config files contains one section per server

 protocol specific things like host, port, password, icy values
 encoder specific things like dest sample rate, bit rate/quality, encoder to use, number of channels to encode
 source specific things like source sample rate, number of channels to read, float/int samples, and asio channel(s) to use

 source sample rate, number of channels, float/int determined "intelligently"

 source sample rate = max of ALL encoder sample rates
 number of channels = max of ALL encoder channels to encode
 use int unless there's a resample required

 VU meter needs int, therefore if a resample is required, we will need both float and int buffers
[SERVER_ID]
host=
port=
password=
icy-*=
application= *outcast
instance= *outcast
streamid= *outcast
path= *outcast
mount= *icecast

samplerate=generally 48000,44100 or 22050 - have seen 32000
bitrate=as per encoder tables
quality=as per encoder tables
encoder=LAME,AACP,AACS,AASH,AACr,FAAC,OGGV,FLAC
channels=
LAMEjointStereo=

asiochannels=

the future:

video=camera device
framerate=fps
devwidth=
devheight=
srctop=
srcleft=
srcwidth=
srcheight=
outwidth=
outheight=
videocodec=VP6 (only one supported to begin with)
syncrate=min,max
audiodelay=
flip= set to 1/true if streaming to outcast with shoutcast clients disabled - this will set icy-videoinverted: 1 which WILL
		be passed on to the flash player to indicate that the stream does not need inversion (VP6 only)
the actual video codec config will be stored in a separate .ini as per nsvcap, since we can use enc_vp6 config dialog directly
I will be adding more functionality to outcast- there will be a connect time parameter to enable/disable shout/flash clients, 
	metadata that will be passed on to flash clients only, that will enable call ins (video or audio) - however,
	this will require an external (even standalone) flash player to play the incoming media. would like to do it using
	flash peer 2 peer, possible for audio, but video frame grabbing in flash is not likely to be enabled on p2p connections
	so it may be that I'll have to do it via the outcast server - but that has to be a little more stable before I try that
eventually, video= will disappear as bravoswitch/bravomix will be integrated directly - eliminating the need for bravocam device
will be replaced by 

[device name]
enabled=
devwidth=
devheight=
the following are defaults only - they can be changed during streaming, but only saved when specifically told to
srctop=
srcleft=
srcwidth=
srcheight=
outtop=
outleft=
outwidth=
outheight=
encwidth=
encheight=
zindex=
alpha=
the above means that for each camera we define the actual capture size, which part of it we want to capture, and the dimensions
of the final capture image

dev* not changeable once we are capturing
src*,out*,zindex and alpha will allow for almost any imaginable (basic) effects/fades/wipes etc
src* MUST be within 0,0->devwidth,devheight
out* can stray outside 0,0->encwidth,encheight - though, any effect can be achieved by manipulation of src* and out* without
												either straying outside of their respective "containers", I just have to
												figure out the logistics of it - actually, I think I already did that with my
												two way crop routine - I just have to remember it!
*/ 

class Server
{
public:
	Server(void);
	~Server(void);
	void startEncoder(void);
	void stopEncoder(void);
	void setEncoder(STD_PARMS);

private:
	Encoder * myEncoder;
	char iniName[33];
};
