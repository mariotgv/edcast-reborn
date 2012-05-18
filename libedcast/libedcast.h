#pragma once

#ifdef WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <pthread.h>

#include "cbuffer.h"
#include "Limiters.h"

#include "libedcast_socket.h"
#ifdef HAVE_VORBIS
#include <vorbis/vorbisenc.h>
#endif

#include <stdio.h>
#include <time.h>

#ifdef _DMALLOC_
#include <dmalloc.h>
#endif

/*
#ifdef _UNICODE
#define char_t wchar_t
#define atoi _wtoi
#define LPCSTR LPCWSTR 
#define strcpy wcscpy
#define strcmp wcscmp
#define strlen wcslen
#define fopen _wfopen
#define strstr wcsstr
#define sprintf swprintf
#define strcat wcscat
#define fgets fgetws
#else
#define char_t char
#endif
*/

#define ED_CHAR char

#ifdef __cplusplus
extern "C" {
#endif
#include "libedcast_resample.h"
#ifdef __cplusplus
}
#endif

#ifdef WIN32
#include <BladeMP3EncDLL.h>
#else
#ifdef HAVE_LAME
#include <lame/lame.h>
#endif
#endif

#ifdef HAVE_FAAC
#undef MPEG2 // hack *cough* hack
typedef signed int  int32_t; 
#include <faac.h>
#endif

#include "enc_if.h"
typedef AudioCoder* (*CREATEAUDIO3TYPE)(int , int , int , unsigned int , unsigned int *, char *);
typedef unsigned int (*GETAUDIOTYPES3TYPE)(int, char *);

#define LM_FORCE 0
#define LM_ERROR 1
#define LM_INFO 2
#define LM_DEBUG 3
#ifdef WIN32
#define LOG_FORCE LM_FORCE, TEXT(__FILE__), __LINE__
#define LOG_ERROR LM_ERROR, TEXT(__FILE__), __LINE__
#define LOG_INFO LM_INFO, TEXT(__FILE__), __LINE__
#define LOG_DEBUG LM_DEBUG, TEXT(__FILE__), __LINE__
#define FILE_LINE TEXT(__FILE__), __LINE__
#else
#define LOG_FORCE LM_FORCE, __FILE__, __LINE__
#define LOG_ERROR LM_ERROR, __FILE__, __LINE__
#define LOG_INFO LM_INFO, __FILE__, __LINE__
#define LOG_DEBUG LM_DEBUG, __FILE__, __LINE__
#define FILE_LINE __FILE__, __LINE__
#endif


#ifdef HAVE_FLAC
#include <FLAC/stream_encoder.h>
extern "C" {
#include <FLAC/metadata.h>
}
#endif

#define FormatID 'fmt '   /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */
// For skin stuff
#define WINDOW_WIDTH		276
#define WINDOW_HEIGHT		150

#ifndef FALSE
#define FALSE false
#endif
#ifndef TRUE
#define TRUE true
#endif
#ifndef WIN32
#include <sys/ioctl.h>
#else
#include <mmsystem.h>
#endif
// Callbacks
#define	BYTES_PER_SECOND 1

#define FRONT_END_EDCAST_PLUGIN 1
#define FRONT_END_TRANSCODER 2

typedef struct tagLAMEOptions {
	int		cbrflag;
	int		out_samplerate;
	int		quality;
#ifdef WIN32
	int		mode;
#else
#ifdef HAVE_LAME
	MPEG_mode	mode;
#endif
#endif
	int		brate;
	int		copywrite;
	int		original;
	int		strict_ISO;
	int		disable_reservoir;
	char	VBR_mode[25];
	int		VBR_mean_bitrate_kbps;
	int		VBR_min_bitrate_kbps;
	int		VBR_max_bitrate_kbps;
	int		lowpassfreq;
	int		highpassfreq;
} LAMEOptions;

typedef struct _RIFFChunk {
	char	RIFF[4];
	long	chunkSize;
	char	WAVE[4];
} RIFFChunk;

typedef struct _FormatChunk {
  char		chunkID[4];
  long		chunkSize;

  short          wFormatTag;
  unsigned short wChannels;
  unsigned long  dwSamplesPerSec;
  unsigned long  dwAvgBytesPerSec;
  unsigned short wBlockAlign;
  unsigned short wBitsPerSample;

/* Note: there may be additional fields here, depending upon wFormatTag. */

} FormatChunk;

typedef struct _DataChunk {
	char	chunkID[4];
	long	chunkSize;
	short *	waveformData;
} DataChunk;

struct wavhead
{
	unsigned int  main_chunk;
	unsigned int  length;
	unsigned int  chunk_type;
	unsigned int  sub_chunk;
	unsigned int  sc_len;
	unsigned short  format;
	unsigned short  modus;
	unsigned int  sample_fq;
	unsigned int  byte_p_sec;
	unsigned short  byte_p_spl;
	unsigned short  bit_p_spl;
	unsigned int  data_chunk;
	unsigned int  data_length;
};


static struct wavhead   wav_header;

// Global variables....gotta love em...
typedef struct _edcastGlobals {
	long		currentSamplerate;
	int		currentBitrate;
	int		currentBitrateMin;
	int		currentBitrateMax;
	int		currentChannels;
	ED_CHAR	attenuation[30];
	double	dAttenuation;
	int		gSCSocket;
	int		gSCSocket2;
	int		gSCSocketControl;
	CMySocket	dataChannel;
	CMySocket	controlChannel;
	int		gSCFlag;
	int		gCountdown;
	int		gAutoCountdown;
	int		automaticconnect;
	ED_CHAR		gSourceURL[1024];
	ED_CHAR		gServer[256];
	ED_CHAR		gPort[10];
	ED_CHAR		gPassword[256];
	int		weareconnected;
	ED_CHAR		gIniFile[1024];
	ED_CHAR		gAppName[256];
	ED_CHAR		gCurrentSong[1024];
	int			gSongUpdateCounter;
	ED_CHAR		gMetadataUpdate[10];
	int			gPubServ;
	ED_CHAR		gServIRC[20];
	ED_CHAR		gServICQ[20];
	ED_CHAR		gServAIM[20];
	ED_CHAR		gServURL[1024];
	ED_CHAR		gServDesc[1024];
	ED_CHAR		gServName[1024];
	ED_CHAR		gServGenre[100];
	ED_CHAR		gMountpoint[100];
	ED_CHAR		gFrequency[10];
	ED_CHAR		gChannels[10];
	int			gAutoReconnect;
	int 		gReconnectSec;
	ED_CHAR		gAutoStart[10];
	ED_CHAR		gAutoStartSec[20];
	ED_CHAR		gQuality[5];
#ifndef WIN32
#ifdef HAVE_LAME
	lame_global_flags *gf;
#endif
#endif
	int		gCurrentlyEncoding;
	int		gFLACFlag;
	int		gAACFlag;
	int		gAACPFlag;
	int		gFHAACPFlag;
	int		gOggFlag;
	ED_CHAR	gIceFlag[10];
	int		gLAMEFlag;
	ED_CHAR	gOggQuality[25];
	int		gLiveRecordingFlag;
	int		gLimiter;
	int		gLimitdb;
	int		gGaindb;
	int		gLimitpre;
	int		gStartMinimized;
	int		gOggBitQualFlag;
	ED_CHAR	gOggBitQual[40];
	ED_CHAR	gEncodeType[25];
	int		gAdvancedRecording;
	int		gNOggchannels;
	char	gModes[4][255];
	int		gShoutcastFlag;
	int		gIcecastFlag;
	int		gIcecast2Flag;
	ED_CHAR	gSaveDirectory[1024];
	ED_CHAR	gLogFile[1024];
	int		gLogLevel;
	FILE	*logFilep;
	int		gSaveDirectoryFlag;
	int		gSaveAsWAV;

	int		gAsioSelectChannel;
	ED_CHAR	gAsioChannel[255];

	int gEnableEncoderScheduler;
	int gMondayEnable;
	int gMondayOnTime;
	int gMondayOffTime;
	int gTuesdayEnable;
	int gTuesdayOnTime;
	int gTuesdayOffTime;
	int gWednesdayEnable;
	int gWednesdayOnTime;
	int gWednesdayOffTime;
	int gThursdayEnable;
	int gThursdayOnTime;
	int gThursdayOffTime;
	int gFridayEnable;
	int gFridayOnTime;
	int gFridayOffTime;
	int gSaturdayEnable;
	int gSaturdayOnTime;
	int gSaturdayOffTime;
	int gSundayEnable;
	int gSundayOnTime;
	int gSundayOffTime;

	FILE		*gSaveFile;
	LAMEOptions	gLAMEOptions;
	int		gLAMEHighpassFlag;
	int		gLAMELowpassFlag;

	int		oggflag;
	int		serialno;

	ogg_sync_state  oy_stream;
	ogg_packet header_main_save;
	ogg_packet header_comments_save;
	ogg_packet header_codebooks_save;

	bool		ice2songChange;
	int			in_header;
	long		 written;
	int		vuShow;

	int		gLAMEpreset;
	ED_CHAR	gLAMEbasicpreset[255];
	ED_CHAR	gLAMEaltpreset[255];
	ED_CHAR   gSongTitle[1024];
	ED_CHAR   gManualSongTitle[1024];
	int		gLockSongTitle;
    int     gNumEncoders;

	res_state	resampler;
	int	initializedResampler;
	void (*sourceURLCallback)(void *, void *);
	void (*destURLCallback)(void *, void *);
	void (*serverStatusCallback)(void *, void *);
	void (*generalStatusCallback)(void *, void *);
	void (*writeBytesCallback)(void *, void *);
	void (*serverTypeCallback)(void *, void *);
	void (*serverNameCallback)(void *, void *);
	void (*streamTypeCallback)(void *, void *);
	void (*bitrateCallback)(void *, void *);
	void (*VUCallback)(double, double, double, double);
	long	startTime;
	long	endTime;
	ED_CHAR	sourceDescription[255];

	ED_CHAR	gServerType[25];
#ifdef WIN32
	WAVEFORMATEX waveFormat;
	HWAVEIN      inHandle;
	WAVEHDR                 WAVbuffer1;
	WAVEHDR                 WAVbuffer2;
#else
	int	inHandle; // for advanced recording
#endif
	unsigned long result;
	short int WAVsamplesbuffer1[1152*2];
	short int WAVsamplesbuffer2[1152*2];
	bool  areLiveRecording;
//	char_t	gAdvRecDevice[255];
#ifndef WIN32
	char_t	gAdvRecServerTitle[255];
#endif
//	int		gLiveInSamplerate;
#ifdef WIN32
	// These are for the LAME DLL
        BEINITSTREAM            beInitStream;
        BEENCODECHUNK           beEncodeChunk;
        BEDEINITSTREAM          beDeinitStream;
        BECLOSESTREAM           beCloseStream;
        BEVERSION               beVersion;
        BEWRITEVBRHEADER        beWriteVBRHeader;
        HINSTANCE       hDLL;
        HINSTANCE       hFAACDLL;
        HINSTANCE       hAACPDLL;
        HINSTANCE       hFHGAACPDLL;
        DWORD           dwSamples;
        DWORD           dwMP3Buffer;
        HBE_STREAM      hbeStream;
#endif
		ED_CHAR	gConfigFileName[255];
		ED_CHAR	gOggEncoderText[255];
		int		gForceStop;
		ED_CHAR	gCurrentRecordingName[1024];
		long	lastX;
		long	lastY;
//		long	lastDummyX;
//		long	lastDummyY;


		ogg_stream_state os;
		vorbis_dsp_state vd;
		vorbis_block     vb;
		vorbis_info      vi;


		int	frontEndType;
		int	ReconnectTrigger;

		CREATEAUDIO3TYPE	fhCreateAudio3;
		GETAUDIOTYPES3TYPE	fhGetAudioTypes3;

		AudioCoder *(*fhFinishAudio3)(char *fn, AudioCoder *c);
		void (*fhPrepareToFinish)(const char *filename, AudioCoder *coder);
		AudioCoder * fhaacpEncoder;

		CREATEAUDIO3TYPE	CreateAudio3;
		GETAUDIOTYPES3TYPE	GetAudioTypes3;

		AudioCoder *(*finishAudio3)(char *fn, AudioCoder *c);
		void (*PrepareToFinish)(const char *filename, AudioCoder *coder);
		AudioCoder * aacpEncoder;

		faacEncHandle aacEncoder;

		unsigned long samplesInput, maxBytesOutput;
		float   *faacFIFO;
		long     faacFIFOendpos;
		ED_CHAR		gAACQuality[25];
		ED_CHAR		gAACCutoff[25];
        int     encoderNumber;
        bool    forcedDisconnect;
        time_t     forcedDisconnectSecs;
        int    autoconnect;
        ED_CHAR    externalMetadata[255];
        ED_CHAR    externalURL[255];
        ED_CHAR    externalFile[255];
        ED_CHAR    externalInterval[25];
        ED_CHAR    *vorbisComments[30];
        int     numVorbisComments;
//        char_t    outputControl[255];
        ED_CHAR    metadataAppendString[255];
        ED_CHAR    metadataRemoveStringBefore[255];
        ED_CHAR    metadataRemoveStringAfter[255];
        ED_CHAR    metadataWindowClass[255];
        bool    metadataWindowClassInd;

		FLAC__StreamEncoder	*flacEncoder;
		FLAC__StreamMetadata *flacMetadata;
		int	flacFailure;

		ED_CHAR	*configVariables[255];
		int		numConfigVariables;
		pthread_mutex_t mutex;

		ED_CHAR	WindowsRecSubDevice[255];
		ED_CHAR	WindowsRecDevice[255];

		int		LAMEJointStereoFlag;
		int		gForceDSPrecording;
		int		gThreeHourBug;
		int		gSkipCloseWarning;
		int		gAsioRate;
//		CBUFFER	circularBuffer;
} edcastGlobals;

void addConfigVariable(edcastGlobals *g, ED_CHAR *variable);
int initializeencoder(edcastGlobals *g);
void getCurrentSongTitle(edcastGlobals *g, ED_CHAR *song, ED_CHAR *artist, ED_CHAR *full);
void initializeGlobals(edcastGlobals *g);
void ReplaceString(ED_CHAR *source, ED_CHAR *dest, ED_CHAR *from, ED_CHAR *to);
void config_read(edcastGlobals *g);
void config_write(edcastGlobals *g);
int connectToServer(edcastGlobals *g);
int disconnectFromServer(edcastGlobals *g);
int do_encoding(edcastGlobals *g, short int *samples, int numsamples, Limiters * limiter = NULL);
void URLize(ED_CHAR *input, ED_CHAR *output, int inputlen, int outputlen);
int updateSongTitle(edcastGlobals *g, int forceURL);
int setCurrentSongTitleURL(edcastGlobals *g, ED_CHAR *song);
void icecast2SendMetadata(edcastGlobals *g);
int ogg_encode_dataout(edcastGlobals *g);
int	trimVariable(ED_CHAR *variable);
int readConfigFile(edcastGlobals *g,int readOnly = 0);
int writeConfigFile(edcastGlobals *g);
//void    printConfigFileValues();
void ErrorMessage(ED_CHAR *title, ED_CHAR *fmt, ...);
int setCurrentSongTitle(edcastGlobals *g, ED_CHAR *song);
ED_CHAR*   getSourceURL(edcastGlobals *g);
void    setSourceURL(edcastGlobals *g,ED_CHAR *url);
long    getCurrentSamplerate(edcastGlobals *g);
int     getCurrentBitrate(edcastGlobals *g);
int     getCurrentChannels(edcastGlobals *g);
double getAttenuation(edcastGlobals *g);
int  ocConvertAudio(edcastGlobals *g,float *in_samples, float *out_samples, int num_in_samples, int num_out_samples);
int initializeResampler(edcastGlobals *g,long inSampleRate, long inNCH);
int handle_output(edcastGlobals *g, float *samples, int nsamples, int nchannels, int in_samplerate, int asioChannel = -1, int asioChannel2 = -1);
int handle_output_fast(edcastGlobals *g, Limiters *limiter, int dataoffset=0);
void setServerStatusCallback(edcastGlobals *g,void (*pCallback)(void *,void *));
void setGeneralStatusCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setWriteBytesCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setServerTypeCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setServerNameCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setStreamTypeCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setBitrateCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setVUCallback(edcastGlobals *g, void (*pCallback)(int, int));
void setSourceURLCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setDestURLCallback(edcastGlobals *g, void (*pCallback)(void *,void *));
void setSourceDescription(edcastGlobals *g, ED_CHAR *desc);
int  getOggFlag(edcastGlobals *g);
//bool  getLiveRecordingFlag(edcastGlobals *g);
//void setLiveRecordingFlag(edcastGlobals *g, bool flag);
void setDumpData(edcastGlobals *g, int dump);
void setConfigFileName(edcastGlobals *g, ED_CHAR *configFile);
ED_CHAR *getConfigFileName(edcastGlobals *g);
ED_CHAR*	getServerDesc(edcastGlobals *g);
int	getReconnectFlag(edcastGlobals *g);
int getReconnectSecs(edcastGlobals *g);
int getIsConnected(edcastGlobals *g);
int resetResampler(edcastGlobals *g);
void setOggEncoderText(edcastGlobals *g, ED_CHAR *text);
//int getLiveRecordingSetFlag(edcastGlobals *g);
ED_CHAR *getCurrentRecordingName(edcastGlobals *g);
void setCurrentRecordingName(edcastGlobals *g, ED_CHAR *name);
void setForceStop(edcastGlobals *g, int forceStop);
long	getLastXWindow(edcastGlobals *g);
long	getLastYWindow(edcastGlobals *g);
void	setLastXWindow(edcastGlobals *g, long x);
void	setLastYWindow(edcastGlobals *g, long y);
//long	getLastDummyXWindow(edcastGlobals *g);
//long	getLastDummyYWindow(edcastGlobals *g);
//void	setLastDummyXWindow(edcastGlobals *g, long x);
//void	setLastDummyYWindow(edcastGlobals *g, long y);
long	getVUShow(edcastGlobals *g);
void	setVUShow(edcastGlobals *g, long x);
int		getFrontEndType(edcastGlobals *g);
void	setFrontEndType(edcastGlobals *g, int x);
int		getReconnectTrigger(edcastGlobals *g);
void	setReconnectTrigger(edcastGlobals *g, int x);
ED_CHAR *getCurrentlyPlaying(edcastGlobals *g);
//long GetConfigVariableLong(char_t *appName, char_t *paramName, long defaultvalue, char_t *desc);
long GetConfigVariableLong(edcastGlobals *g, ED_CHAR *appName, ED_CHAR *paramName, long defaultvalue, ED_CHAR *desc);
ED_CHAR	*getLockedMetadata(edcastGlobals *g);
void	setLockedMetadata(edcastGlobals *g, ED_CHAR *buf);
int		getLockedMetadataFlag(edcastGlobals *g);
void	setLockedMetadataFlag(edcastGlobals *g, int flag);
void	setSaveDirectory(edcastGlobals *g, ED_CHAR *saveDir);
ED_CHAR *getSaveDirectory(edcastGlobals *g);
ED_CHAR *getgLogFile(edcastGlobals *g);
void	setgLogFile(edcastGlobals *g,ED_CHAR *logFile);
int getSaveAsWAV(edcastGlobals *g);
void setSaveAsWAV(edcastGlobals *g, int flag);
int getForceDSP(edcastGlobals *g);
void setForceDSP(edcastGlobals *g, int flag);
FILE *getSaveFileP(edcastGlobals *g);
long getWritten(edcastGlobals *g);
void setWritten(edcastGlobals *g, long writ);
int deleteConfigFile(edcastGlobals *g);
void setAutoConnect(edcastGlobals *g, int flag);
void setStartMinimizedFlag(edcastGlobals *g, int flag);
int getStartMinimizedFlag(edcastGlobals *g);
void setLimiterFlag(edcastGlobals *g, int flag);
void setLimiterValues(edcastGlobals *g, int db, int pre, int gain);
void addVorbisComment(edcastGlobals *g, ED_CHAR *comment);
void freeVorbisComments(edcastGlobals *g);
void addBasicEncoderSettings(edcastGlobals *g);
void addMultiEncoderSettings(edcastGlobals *g);
void addMultiStereoEncoderSettings(edcastGlobals *g);
void addDSPONLYsettings(edcastGlobals *g);
void addStandaloneONLYsettings(edcastGlobals *g);
void addBASSONLYsettings(edcastGlobals *g);
void addUISettings(edcastGlobals *g);
void addASIOUISettings(edcastGlobals *g);
void addASIOExtraSettings(edcastGlobals *g);
void addOtherUISettings(edcastGlobals *g);
void setDefaultLogFileName(ED_CHAR *filename);
void setConfigDir(ED_CHAR *dirname);
void LogMessage(edcastGlobals *g, int type, ED_CHAR *source, int line, ED_CHAR *fmt, ...);
ED_CHAR *getWindowsRecordingDevice(edcastGlobals *g);
void	setWindowsRecordingDevice(edcastGlobals *g, ED_CHAR *device);
ED_CHAR *getWindowsRecordingSubDevice(edcastGlobals *g);
void	setWindowsRecordingSubDevice(edcastGlobals *g, ED_CHAR *device);
int getLAMEJointStereoFlag(edcastGlobals *g);
void	setLAMEJointStereoFlag(edcastGlobals *g, int flag);
int triggerDisconnect(edcastGlobals *g);
int getAppdata(bool checkonly, int locn, DWORD flags, LPCSTR subdir, LPCSTR configname, LPSTR strdestn);
bool testLocal(LPCSTR dir, LPCSTR file);
