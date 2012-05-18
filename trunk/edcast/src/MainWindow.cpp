/*$T MainWindow.cpp GC 1.140 10/23/05 09:48:26 */

/*
 * MainWindow.cpp : implementation file ;
 */
#include "stdafx.h"
#include "edcast.h"
#include "MainWindow.h"
#include "libedcast.h"
#include <process.h>

#include <math.h>
#include <afxinet.h>

#include "About.h"
#include "SystemTray.h"
#include <Limiters.h>

#define WM_MY_MESSAGE	WM_USER+998
Limiters *				limiter = NULL;
Limiters *				dsplimiter = NULL;

CMainWindow *			pWindow;

int						edcast_init(edcastGlobals *g);

unsigned int			edcastThreadTitsOnBull = 0;

edcastGlobals *			g[MAX_ENCODERS];
edcastGlobals			gMain;

int						m_AUDIOOpen = 0;

bool					gLiveRecording = false;

HDC						specdc = 0;
HBITMAP					specbmp = 0;

extern char 			logPrefix[255];
char					currentConfigDir[MAX_PATH] = "";

// common
extern "C"
{
	int startedcastThread(void *obj) {
		CMainWindow *pWindow = (CMainWindow *) obj;
		pWindow->startedcast(-1);
		//_endthread();
		return(1);
	}
}

extern "C"
{
	int startSpecificedcastThread(void *obj) {
		int		enc = (int) obj;
		/*
		 * CMainWindow *pWindow = (CMainWindow *)obj;
		 */
		int		ret = pWindow->startedcast(enc);
		g[enc]->forcedDisconnectSecs = time(NULL);
		//_endthread();
		return(1);
	}
}

VOID CALLBACK ReconnectTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	time_t	currentTime;
	currentTime = time(NULL);
	for(int i = 0; i < gMain.gNumEncoders; i++) {
		if(g[i]->forcedDisconnect) {
			int timeout = getReconnectSecs(g[i]);
			time_t timediff = currentTime - g[i]->forcedDisconnectSecs;
			if(timediff > timeout) {
				g[i]->forcedDisconnect = false;
				_beginthreadex(NULL,
							   0,
							   (unsigned(_stdcall *) (void *)) startSpecificedcastThread,
							   (void *) i,
							   0,
							   &edcastThreadTitsOnBull);
			}
			else {
				char	buf[255] = "";
				wsprintf(buf, "Connecting in %d seconds", timeout - timediff);
				pWindow->outputStatusCallback(i + 1, buf, FILE_LINE);
			}
		}
	}
}

VOID CALLBACK MetadataTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if(!strcmp(gMain.externalMetadata, "FILE")) {
		FILE	*filep = fopen(gMain.externalFile, "r");
		if(!filep) {
			char	buf[1024] = "";
			wsprintf(buf, "Cannot open metadata file (%s)", gMain.externalFile);
			pWindow->generalStatusCallback(buf, FILE_LINE);
		}
		else {
			char	buffer[1024];
			memset(buffer, '\000', sizeof(buffer));
			fgets(buffer, sizeof(buffer) - 1, filep);

			char	*p1;
			p1 = strstr(buffer, "\r\n");
			if(p1) {
				*p1 = '\000';
			}

			p1 = strstr(buffer, "\n");
			if(p1) {
				*p1 = '\000';
			}

			fclose(filep);
			setMetadata(buffer);
		}
	}

	if(!strcmp(gMain.externalMetadata, "URL")) {
		char	szCause[255];

		TRY
		{
			CInternetSession	session("edcast");
			CStdioFile			*file = NULL;
			file = session.OpenURL(gMain.externalURL, 1, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE);
			if(file == NULL) {
				char	buf[1024] = "";
				wsprintf(buf, "Cannot open metadata URL (%s)", gMain.externalURL);
				pWindow->generalStatusCallback(buf, FILE_LINE);
			}
			else {
				CString metadata;
				file->ReadString(metadata);
				setMetadata((char *) LPCSTR(metadata));
				file->Close();
				delete file;
			}

			session.Close();
			delete session;
		}

		CATCH_ALL(error) {
			error->GetErrorMessage(szCause, 254, NULL);
			pWindow->generalStatusCallback(szCause, FILE_LINE);
		}

		END_CATCH_ALL;
	}
}

char	lastWindowTitleString[4096] = "";

#define UNICODE
VOID CALLBACK MetadataCheckTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	pWindow->KillTimer(5);
	if(gMain.metadataWindowClassInd) {
		if(strlen(gMain.metadataWindowClass) > 0) {
			HWND	winHandle = FindWindow(gMain.metadataWindowClass, NULL);
			if(winHandle) {
				char	buf[1024] = "";
				GetWindowText(winHandle, buf, sizeof(buf) - 1);
				if(!strcmp(buf, lastWindowTitleString)) {
					;
				}
				else {
					setMetadata(buf);
					strcpy(lastWindowTitleString, buf);
				}
			}
		}
	}
	pWindow->SetTimer(5, 1000, (TIMERPROC) MetadataCheckTimer);
}

#undef UNICODE
VOID CALLBACK AutoConnectTimer(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	pWindow->DoConnect();
}

int handleAllOutput(float *samples, int nsamples, int nchannels, int in_samplerate)
{
	if(nchannels > 2)
	{
		if(limiter == NULL)
		{
			limiter = (Limiters *) new limitMultiMono(nchannels);
		}
		if(pWindow->m_Limiter)
		{
			limiter->multiLimit(samples, nsamples, in_samplerate, NULL, NULL, NULL);//0.55, -3.0);
		}
		else
		{
			limiter->multiLimit(samples, nsamples, in_samplerate, NULL, NULL, NULL);//0.0, 20.0);
		}
	}
	else
	{
		if(limiter != NULL)
		{
			if((nchannels == 1 && limiter->sourceIsStereo) || (nchannels == 2 && !limiter->sourceIsStereo))
			{
				delete limiter;
				limiter = NULL;
			}
		}
		if(limiter == NULL)
		{
			if(nchannels == 1)
			{
				limiter = (Limiters *) new limitMonoToStereoMono();
			}
			else
			{
				limiter = (Limiters *) new limitStereoToStereoMono();
			}
		}

		if(pWindow->m_Limiter)
		{
			limiter->limit(samples, nsamples, in_samplerate, pWindow->m_limitpre, pWindow->m_limitdb, pWindow->m_gaindb);
		}
		else
		{
			limiter->limit(samples, nsamples, in_samplerate, 0.0, 20.0);
		}
	}

	if(gMain.vuShow == 2)
	{
		UpdatePeak((int) limiter->PeakL + 60, (int) limiter->PeakR + 60, 0, 0);
	}
	else
	{
		UpdatePeak((int) limiter->RmsL + 60, (int) limiter->RmsR + 60, (int) limiter->PeakL + 60, (int) limiter->PeakR + 60);
	}
	
	for(int i = 0; i < gMain.gNumEncoders; i++) 
	{
		handleOut(g[i], limiter); // sub::
	}
	return 1;
}
//common
void UpdatePeak(int rmsL, int rmsR, int peakL, int peakR)
{
	pWindow->flexmeters.GetMeterInfoObject(0)->value = rmsL;
	pWindow->flexmeters.GetMeterInfoObject(1)->value = rmsR;
	pWindow->flexmeters.GetMeterInfoObject(0)->peak = peakL;
	pWindow->flexmeters.GetMeterInfoObject(1)->peak = peakR;
}

bool LiveRecordingCheck()
{
	return gLiveRecording;
}

bool HaveEncoderAlwaysDSP() // DSP
{
	for(int i = 0; i < gMain.gNumEncoders; i++) 
	{
		if(g[i]->gForceDSPrecording) // flagged for always DSP
		{
			return true;
		}
	}
	return false;
}
//common
int getLastX()
{
	return getLastXWindow(&gMain);
}

int getLastY()
{
	return getLastYWindow(&gMain);
}

void setLastX(int x)
{
	setLastXWindow(&gMain, x);
}

void setLastY(int y)
{
	setLastYWindow(&gMain, y);
}

void setLiveRecFlag(int live)
{
	gMain.gLiveRecordingFlag = live;
}

void setLimiterVals(int db, int pre, int gain)
{
	setLimiterValues(&gMain, db, pre, gain);
}

void setLimiter(int limiter)
{
	setLimiterFlag(&gMain, limiter);
}

void setStartMinimized(int mini)
{
	setStartMinimizedFlag(&gMain, mini);
}

void setAuto(int flag)
{
	setAutoConnect(&gMain, flag);
}

void writeMainConfig()
{
	writeConfigFile(&gMain);
}

void setMetadataFromMediaPlayer(char *metadata)
{
	if(gMain.metadataWindowClassInd) 
	{
		return;
	}

	if(!strcmp(gMain.externalMetadata, "DISABLED")) 
	{
		setMetadata(metadata);
	}
}

void setMetadata(char *metadata)
{
	char	modifiedSong[4096] = "";
	char	modifiedSongBuffer[4096] = "";
	char	*pData;

	strcpy(modifiedSong, metadata);
	if(strlen(gMain.metadataRemoveStringAfter) > 0) {
		char	*p1 = strstr(modifiedSong, gMain.metadataRemoveStringAfter);
		if(p1) {
			*p1 = '\000';
		}
	}

	if(strlen(gMain.metadataRemoveStringBefore) > 0) {
		char	*p1 = strstr(modifiedSong, gMain.metadataRemoveStringBefore);
		if(p1) {
			memset(modifiedSongBuffer, '\000', sizeof(modifiedSongBuffer));
			strcpy(modifiedSongBuffer, p1 + strlen(gMain.metadataRemoveStringBefore));
			strcpy(modifiedSong, modifiedSongBuffer);
		}
	}

	if(strlen(gMain.metadataAppendString) > 0) {
		strcat(modifiedSong, gMain.metadataAppendString);
	}

	pData = modifiedSong;

	pWindow->m_Metadata = modifiedSong;
	pWindow->inputMetadataCallback(0, (void *) pData, FILE_LINE);

	for(int i = 0; i < gMain.gNumEncoders; i++) {
		if(getLockedMetadataFlag(&gMain)) {
			if(setCurrentSongTitle(g[i], (char *) getLockedMetadata(&gMain))) {
				pWindow->inputMetadataCallback(i, (void *) getLockedMetadata(&gMain), FILE_LINE);
			}
		}
		else {
			if(setCurrentSongTitle(g[i], (char *) pData)) {
				pWindow->inputMetadataCallback(i, (void *) pData, FILE_LINE);
			}
		}
	}
}

bool getDirName(LPCSTR inDir, LPSTR dst, int lvl=1)
{
	// inDir = ...\winamp\Plugins
	char * dir = _strdup(inDir);
	bool retval = false;
	// remove trailing slash

	if(dir[strlen(dir)-1] == '\\')
	{
		dir[strlen(dir)-1] = '\0';
	}

	char *p1;

	for(p1 = dir + strlen(dir) - 1; p1 >= dir; p1--)
	{
		if(*p1 == '\\')
		{
			if(--lvl > 0)
			{
				*p1 = '\0';
			}
			else
			{
				strcpy(dst, p1);
				retval = true;
				break;
			}
		}
	}
	free(dir);
	return retval;
}

void LoadConfigs(char *currentDir, char *subdir, bool dsp) // different for DSP
{
	char	configFile[1024] = "";
	char	currentlogFile[1024] = "";

	char tmpfile[MAX_PATH] = "";
	char tmp2file[MAX_PATH] = "";

	char cfgfile[MAX_PATH];
	wsprintf(cfgfile, "%s_0.cfg", logPrefix);

	bool canUseCurrent = testLocal(currentDir, NULL);
	bool hasCurrentData = false;
	bool hasAppData = false;
	bool canUseAppData = false;
	bool hasProgramData = false;
	bool canUseProgramData = false;
	if(canUseCurrent)
	{
		hasCurrentData = testLocal(currentDir, cfgfile);
	}
	if(!hasCurrentData)
	{
		int iHasAppData = -1;
		if(dsp)
		{
			int iHasWinampDir = -1;
			int iHasPluginDir = -1;
			int iHasEdcastDir = -1;
			int iHasEdcastCfg = -1;
			char wasubdir[MAX_PATH] = "";
			char wa_instance[MAX_PATH] = "";
			
			getDirName(currentDir, wa_instance, 2); //...../{winamp name}/plugins - we want {winamp name}

			wsprintf(wasubdir, "%s", wa_instance);
			iHasWinampDir = getAppdata(false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile);
			if(iHasWinampDir < 2)
			{
				wsprintf(wasubdir, "%s\\Plugins", wa_instance);
				iHasPluginDir = getAppdata(false, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile);
				if(iHasPluginDir < 2)
				{
					wsprintf(wasubdir, "%s\\Plugins\\%s", wa_instance, subdir);
					iHasAppData = getAppdata(true, CSIDL_APPDATA, SHGFP_TYPE_CURRENT, wasubdir, cfgfile, tmpfile);
				}
			}
		}
		else
		{
			// todo:
			// look for edcast instance name - our path will be like C:\Program Files\{this instance name}
			//
			iHasAppData = getAppdata(true, CSIDL_LOCAL_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmpfile);
		}
//common
		hasAppData = (iHasAppData == 0);
		canUseAppData = (iHasAppData < 2);
		if(!hasAppData)
		{
			int iHasProgramData = getAppdata(true, CSIDL_COMMON_APPDATA, SHGFP_TYPE_CURRENT, subdir, cfgfile, tmp2file);
			hasProgramData = (iHasProgramData == 0);
			canUseProgramData = (iHasProgramData < 2);
		}
	}
	if(hasAppData && hasCurrentData)
	{
		// tmpfile already has the right value
	}
	else if (hasAppData)
	{
		// tmpfile already has the right value
	}
	else if (hasCurrentData)
	{
		strcpy(tmpfile, currentDir);
	}
	else if(canUseAppData)
	{
		// tmpfile alread has the right value
	}
	else if(canUseProgramData)
	{
		strcpy(tmpfile, tmp2file);
	}
	else if(canUseCurrent)
	{
		strcpy(tmpfile, currentDir);
	}
	else
	{
		// fail!!
	}
	strcpy(currentConfigDir, tmpfile);

	wsprintf(configFile, "%s\\%s", currentConfigDir, logPrefix);
	wsprintf(currentlogFile, "%s\\%s", currentConfigDir, logPrefix);

	setConfigDir(currentConfigDir);
    setDefaultLogFileName(currentlogFile);
    setgLogFile(&gMain, currentlogFile);
    setConfigFileName(&gMain, configFile);
	addUISettings(&gMain);
	
	AddSubMainSettings(); // sub::

	readConfigFile(&gMain);
	LogMessage(&gMain, LOG_INFO, "===================================================");
}

/* CMainWindow dialog */
const char	*kpcTrayNotificationMsg_ = "edcast";

CMainWindow::CMainWindow(CWnd *pParent /* NULL */ ) :
	CDialog(CMainWindow::IDD, pParent),
	bMinimized_(false),
	pTrayIcon_(0),
	nTrayNotificationMsg_(RegisterWindowMessage(kpcTrayNotificationMsg_))
{
	//{{AFX_DATA_INIT(CMainWindow)
	m_Bitrate = _T("");
	m_Destination = _T("");
	m_Bandwidth = _T("");
	m_Metadata = _T("");
	m_ServerDescription = _T("");
	m_LiveRec = FALSE;
	m_RecDevices = _T("");
	m_RecCards = _T("");
	m_RecVolume = 0;
	m_AutoConnect = FALSE;
	m_startMinimized = FALSE;
	m_Limiter = FALSE;
	m_StaticStatus = _T("");
	//}}AFX_DATA_INIT
	hIcon_ = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset(g, '\000', sizeof(g));
	m_AUDIOOpen = 0;
	pWindow = this;
	memset(m_currentDir, '\000', sizeof(m_currentDir));
	strcpy(m_currentDir, ".");
	LogMessage(&gMain, LOG_DEBUG, "CMainWindow complete");
}

CMainWindow::~CMainWindow() 
{
	for(int i = 0; i < MAX_ENCODERS; i++) 
	{
		if(g[i]) 
		{
			free(g[i]);
		}
	}
}

void CMainWindow::InitializeWindow()
{
	configDialog = new CConfig();
	configDialog->Create((UINT) IDD_CONFIG, this);
	configDialog->parentDialog = this;

	editMetadata = new CEditMetadata();
	editMetadata->Create((UINT) IDD_EDIT_METADATA, this);
	editMetadata->parentDialog = this;

	aboutBox = new CAbout();
	aboutBox->Create((UINT) IDD_ABOUT, this);
	LogMessage(&gMain, LOG_DEBUG, "CMainWindow::InitializeWindow complete");
}

LRESULT CMainWindow::startMinimized(WPARAM wParam, LPARAM lParam)
{
	if(gMain.gStartMinimized)
	{
		bMinimized_ = true;
		SetupTrayIcon();
		SetupTaskBarButton();
	}
	return 0L;
}

void CMainWindow::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainWindow)
	DDX_Control(pDX, IDC_RECCARDS, m_RecCardsCtrl);
	DDX_Control(pDX, IDC_ASIORATE, m_AsioRateCtrl);
	DDX_Control(pDX, IDC_VUONOFF, m_OnOff);
	DDX_Control(pDX, IDC_METER_PEAK, m_MeterPeak);
	DDX_Control(pDX, IDC_METER_RMS, m_MeterRMS);
	DDX_Control(pDX, IDC_AUTOCONNECT, m_AutoConnectCtrl);
	DDX_Control(pDX, IDC_MINIM, m_startMinimizedCtrl);
	DDX_Control(pDX, IDC_LIMITER, m_LimiterCtrl);
	DDX_Control(pDX, IDC_RECVOLUME, m_RecVolumeCtrl);
	DDX_Control(pDX, IDC_RECDEVICES, m_RecDevicesCtrl);
	DDX_Control(pDX, IDC_CONNECT, m_ConnectCtrl);
	DDX_Control(pDX, IDC_LIVEREC, m_LiveRecCtrl);
	DDX_Control(pDX, IDC_ENCODERS, m_Encoders);
	DDX_Text(pDX, IDC_METADATA, m_Metadata);
	DDX_Check(pDX, IDC_LIVEREC, m_LiveRec);
	DDX_CBString(pDX, IDC_RECDEVICES, m_RecDevices);
	DDX_CBString(pDX, IDC_RECCARDS, m_RecCards);
	DDX_CBString(pDX, IDC_ASIORATE, m_AsioRate);
	DDX_Slider(pDX, IDC_RECVOLUME, m_RecVolume);
	DDX_Check(pDX, IDC_AUTOCONNECT, m_AutoConnect);
	DDX_Check(pDX, IDC_LIMITER, m_Limiter);
	DDX_Check(pDX, IDC_MINIM, m_startMinimized);
	DDX_Text(pDX, IDC_STATIC_STATUS, m_StaticStatus);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_StaticStatusCtrl);

	DDX_Control(pDX, IDC_LIMITDB, m_limitdbCtrl);
	DDX_Slider(pDX, IDC_LIMITDB, m_limitdb);
	DDX_Text(pDX, IDC_LIMITDBTEXT, m_staticLimitdb);

	DDX_Control(pDX, IDC_GAINDB, m_gaindbCtrl);
	DDX_Slider(pDX, IDC_GAINDB, m_gaindb);
	DDX_Text(pDX, IDC_GAINDBTEXT, m_staticGaindb);

	DDX_Control(pDX, IDC_LIMITPREEMPH, m_limitpreCtrl);
	DDX_Slider(pDX, IDC_LIMITPREEMPH, m_limitpre);
	DDX_Text(pDX, IDC_LIMITPREEMPHTEXT, m_staticLimitpre);

	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMainWindow, CDialog)
//{{AFX_MSG_MAP(CMainWindow)
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_ADD_ENCODER, OnAddEncoder)
	ON_NOTIFY(NM_DBLCLK, IDC_ENCODERS, OnDblclkEncoders)
	ON_NOTIFY(NM_RCLICK, IDC_ENCODERS, OnRclickEncoders)
	ON_COMMAND(ID_POPUP_CONFIGURE, OnPopupConfigure)
	ON_COMMAND(ID_POPUP_CONNECT, OnPopupConnect)
	ON_BN_CLICKED(IDC_LIVEREC, OnLiverec)
	ON_BN_CLICKED(IDC_LIMITER, OnLimiter)
	ON_BN_CLICKED(IDC_MINIM, OnStartMinimized)
	ON_COMMAND(ID_POPUP_DELETE, OnPopupDelete)
	ON_CBN_SELCHANGE(IDC_RECDEVICES, OnSelchangeRecdevices)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_MANUALEDIT_METADATA, OnManualeditMetadata)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_ABOUT_ABOUT, OnAboutAbout)
	ON_COMMAND(ID_ABOUT_HELP, OnAboutHelp)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_METER, OnMeter)
	ON_NOTIFY(LVN_KEYDOWN, IDC_ENCODERS, OnKeydownEncoders)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_NOTIFY(NM_SETFOCUS, IDC_ENCODERS, OnSetfocusEncoders)
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_RECCARDS, OnSelchangeReccards)
	ON_CBN_SELCHANGE(IDC_ASIORATE, OnSelchangeAsioRate)
	//}}AFX_MSG_MAP
	ON_WM_SYSCOMMAND()
	ON_COMMAND(IDI_RESTORE, OnSTRestore)
	ON_MESSAGE(WM_MY_MESSAGE, startMinimized)
	ON_MESSAGE(WM_SHOWWINDOW, gotShowWindow)
END_MESSAGE_MAP()

void CMainWindow::generalStatusCallback(void *pValue, char *source, int line)
{
	LogMessage(&gMain, LM_INFO, source, line, "%s", (char *)pValue);
	SetDlgItemText(IDC_STATIC_STATUS, (char *) pValue);
	m_StaticStatus = (char*) pValue;
}

void CMainWindow::inputMetadataCallback(int enc, void *pValue, char *source, int line)
{
	SetDlgItemText(IDC_METADATA, (char *) pValue);
	if (enc == 0) {
		LogMessage(&gMain, LM_INFO, source, line, "%s", (char *)pValue);
	}
	else {
		LogMessage(g[enc-1],LM_INFO, source, line,  "%s", (char *)pValue);
	}
}

void CMainWindow::outputStatusCallback(int enc, void *pValue, char *source, int line, bool bSendToLog)
{
	if(enc != 0) 
	{
		if(bSendToLog)
		{
			LogMessage(g[enc-1], LM_INFO, source, line, "%s", (char *)pValue);
		}
		else
		{
			LogMessage(g[enc-1], LM_DEBUG, source, line, "%s", (char *)pValue);
		}

		int numItems = m_Encoders.GetItemCount();
		if(enc - 1 >= numItems) 
		{
			m_Encoders.InsertItem(enc - 1, (char *) "");
		}

		m_Encoders.SetItemText(enc - 1, 1, (char *) pValue);
	}
}

void CMainWindow::outputMountCallback(int enc, void *pValue)
{
	if(enc != 0)
	{
		int numItems = m_Encoders.GetItemCount();
		if(enc - 1 >= numItems) 
		{
			m_Encoders.InsertItem(enc - 1, (char *) "");
		}

		m_Encoders.SetItemText(enc - 1, 2, (char *) pValue);
	}
}

void CMainWindow::outputChannelCallback(int enc, void *pValue)
{
	if(enc != 0)
	{
		int numItems = m_Encoders.GetItemCount();
		if(enc - 1 >= numItems) 
		{
			m_Encoders.InsertItem(enc - 1, (char *) "");
		}

		m_Encoders.SetItemText(enc - 1, 3, (char *) pValue);
	}
}

void CMainWindow::writeBytesCallback(int enc, void *pValue)
{

	/* pValue is a long */
	static time_t startTime[MAX_ENCODERS];
	static time_t endTime[MAX_ENCODERS];
	static long bytesWrittenInterval[MAX_ENCODERS];
	static long totalBytesWritten[MAX_ENCODERS];
	static int	initted = 0;
	char		kBPSstr[255] = "";

	if(!initted) {
		initted = 1;
		memset(&startTime, '\000', sizeof(startTime));
		memset(&endTime, '\000', sizeof(endTime));
		memset(&bytesWrittenInterval, '\000', sizeof(bytesWrittenInterval));
		memset(&totalBytesWritten, '\000', sizeof(totalBytesWritten));
	}

	if(enc != 0) {
		int		enc_index = enc - 1;
		long	bytesWritten = (long) pValue;

		if(bytesWritten == -1) {
			strcpy(kBPSstr, "");
			outputStatusCallback(enc, kBPSstr, FILE_LINE);
			startTime[enc_index] = 0;
			return;
		}

		if(startTime[enc_index] == 0) {
			startTime[enc_index] = time(&(startTime[enc_index]));
			bytesWrittenInterval[enc_index] = 0;
		}

		bytesWrittenInterval[enc_index] += bytesWritten;
		totalBytesWritten[enc_index] += bytesWritten;
		endTime[enc_index] = time(&(endTime[enc_index]));
		if((endTime[enc_index] - startTime[enc_index]) > 4) {
			time_t		bytespersec = bytesWrittenInterval[enc_index] / (endTime[enc_index] - startTime[enc_index]);
			long	kBPS = (long)((bytespersec * 8) / 1000);
			wsprintf(kBPSstr, "%ld Kbps", kBPS);
			outputStatusCallback(enc, kBPSstr, FILE_LINE, false);
			startTime[enc_index] = 0;
		}
	}
}

void CMainWindow::outputServerNameCallback(int enc, void *pValue)
{

	/*
	 * SetDlgItemText(IDC_SERVER_DESC, (char *)pValue);
	 */
	;
}

void CMainWindow::outputBitrateCallback(int enc, void *pValue)
{
	if(enc != 0) {
		int numItems = m_Encoders.GetItemCount();
		if(enc - 1 >= numItems) {
			m_Encoders.InsertItem(enc - 1, (char *) pValue);
		}
		else {
			m_Encoders.SetItemText(enc - 1, 0, (char *) pValue);
		}
	}
}

void CMainWindow::outputStreamURLCallback(int enc, void *pValue)
{

	/*
	 * SetDlgItemText(IDC_DESTINATION_LOCATION, (char *)pValue);
	 */
}

void CMainWindow::stopedcast()
{
	for(int i = 0; i < gMain.gNumEncoders; i++) 
	{
		setForceStop(g[i], 1);
		disconnectFromServer(g[i]);
		g[i]->forcedDisconnect = false;
	}
}

int CMainWindow::startedcast(int which)
{
	if(which == -1)
	{
		for(int i = 0; i < gMain.gNumEncoders; i++) 
		{
			if(!g[i]->weareconnected) 
			{
				setForceStop(g[i], 0);
				if(!connectToServer(g[i])) 
				{
					g[i]->forcedDisconnect = true;
					continue;
				}
			}
		}
	}
	else 
	{
		if(!g[which]->weareconnected) 
		{
			setForceStop(g[which], 0);

			int ret = connectToServer(g[which]);
			if(ret == 0) 
			{
				g[which]->forcedDisconnect = true;
			}
		}
	}

	return 1;
}

void CMainWindow::DoConnect() 
{
	OnConnect();
	generalStatusCallback("AutoConnect", FILE_LINE);
	KillTimer(autoconnectTimerId);
}

void CMainWindow::OnConnect()
{
	static bool connected = false;

	if(!connected) 
	{
		_beginthreadex(NULL, 0, (unsigned(_stdcall *) (void *)) startedcastThread, (void *) this, 0, &edcastThreadTitsOnBull);
		connected = true;
		m_ConnectCtrl.SetWindowText("Disconnect");
		KillTimer(2);
		reconnectTimerId = SetTimer(2, 1000, (TIMERPROC) ReconnectTimer);

	}
	else 
	{
		stopedcast();
		connected = false;
		m_ConnectCtrl.SetWindowText("Connect");
		KillTimer(2);
	}
}

void CMainWindow::OnAddEncoder() // BASE - with overrides
{

	int orig_index = gMain.gNumEncoders;
	g[orig_index] = (edcastGlobals *) malloc(sizeof(edcastGlobals));

	memset(g[orig_index], '\000', sizeof(edcastGlobals));

	g[orig_index]->encoderNumber = orig_index + 1;


    char    currentlogFile[1024] = "";

	wsprintf(currentlogFile, "%s\\%s_%d", currentConfigDir, logPrefix, g[orig_index]->encoderNumber);



	setDefaultLogFileName(currentlogFile);

	setgLogFile(g[orig_index], currentlogFile);
	setConfigFileName(g[orig_index], gMain.gConfigFileName);
	gMain.gNumEncoders++;
	initializeGlobals(g[orig_index]);
    addBasicEncoderSettings(g[orig_index]);
	AddSpecificEncoderSettings(g[orig_index]); // sub::
	edcast_init(g[orig_index]);
}
void CMainWindow::SetupEncoderDisplay() // override in multi
{
	RECT	rect;

	rect.left = 340;
	rect.top = 190;
	m_Encoders.InsertColumn(0, "Encoder Settings");
	m_Encoders.InsertColumn(1, "Transfer Rate");
	m_Encoders.InsertColumn(2, "Mount");

	m_Encoders.SetColumnWidth(0, 190);
	m_Encoders.SetColumnWidth(1, 110);
	m_Encoders.SetColumnWidth(2, 90);

	m_Encoders.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_Encoders.SendMessage(LB_SETTABSTOPS, 0, NULL);
}

BOOL CMainWindow::OnInitDialog() // BASE - with sub function in the middle
{
	CDialog::OnInitDialog();

	UpdateData(TRUE);

	SetWindowText("edcast");
	
	SetupEncoderDisplay();
	
	liveRecOn.LoadBitmap(IDB_LIVE_ON);
	liveRecOff.LoadBitmap(IDB_LIVE_OFF);
	SetLiveRecordingFlag(); // sub::
	m_LiveRec = gMain.gLiveRecordingFlag;
	if(m_LiveRec) {
		m_LiveRecCtrl.SetBitmap(HBITMAP(liveRecOn));
	}
	else {
		m_LiveRecCtrl.SetBitmap(HBITMAP(liveRecOff));
	}

	for(int i = 0; i < gMain.gNumEncoders; i++) 
	{
		if(!g[i]) 
		{
			g[i] = (edcastGlobals *) malloc(sizeof(edcastGlobals));
			memset(g[i], '\000', sizeof(edcastGlobals));
		}

		g[i]->encoderNumber = i + 1;
		char    currentlogFile[1024] = "";

		wsprintf(currentlogFile, "%s\\%s_%d", currentConfigDir, logPrefix, g[i]->encoderNumber);
		setDefaultLogFileName(currentlogFile);
		setgLogFile(g[i], currentlogFile);
		setConfigFileName(g[i], gMain.gConfigFileName);
		initializeGlobals(g[i]);
	    addBasicEncoderSettings(g[i]);
		AddSpecificEncoderSettings(g[i]); // sub::
		edcast_init(g[i]);
	}
	
	InitAudioDeviceDropdown(); // sub::
	
	m_AUDIOOpen = 0;
	if(getLockedMetadataFlag(&gMain)) {
		m_Metadata = getLockedMetadata(&gMain);
	}

	m_AutoConnect = gMain.autoconnect;
	m_startMinimized = gMain.gStartMinimized;

	m_Limiter = gMain.gLimiter;

	m_limitdbCtrl.SetRange(-15, 0, TRUE);
	m_gaindbCtrl.SetRange(0, 20, TRUE);
	m_limitpreCtrl.SetRange(0, 75, TRUE);

	m_limitdb = gMain.gLimitdb;
	m_staticLimitdb.Format("%d", gMain.gLimitdb);

	m_gaindb = gMain.gGaindb;
	m_staticGaindb.Format("%d", gMain.gGaindb);

	m_limitpre = gMain.gLimitpre;
	m_staticLimitpre.Format("%d", gMain.gLimitpre);

	SetDialogControls(); // sub::

	reconnectTimerId = SetTimer(2, 1000, (TIMERPROC) ReconnectTimer);
	if(m_AutoConnect) {
		char	buf[255];
		wsprintf(buf, "AutoConnecting in 5 seconds");
		generalStatusCallback(buf, FILE_LINE);
		autoconnectTimerId = SetTimer(3, 5000, (TIMERPROC) AutoConnectTimer);
	}

	int metadataInterval = atoi(gMain.externalInterval);
	metadataInterval = metadataInterval * 1000;
	metadataTimerId = SetTimer(4, metadataInterval, (TIMERPROC) MetadataTimer);

	SetTimer(5, 1000, (TIMERPROC) MetadataCheckTimer);

	FlexMeters_InitStruct	*fmis = flexmeters.Initialize_Step1();	/* returns a pointer to a struct, */

	/* with default values filled in. */
	fmis->max_x = 512;	/* buffer size. must be at least as big as the meter window */
	fmis->max_y = 512;	/* buffer size. must be at least as big as the meter window */

	fmis->hWndFrame = GetDlgItem(IDC_METER)->m_hWnd;	/* the window to grab coordinates from */

	fmis->border.left = 3;					/* borders. */
	fmis->border.right = 3;
	fmis->border.top = 3;
	fmis->border.bottom = 3;

	fmis->meter_count = 2;					/* number of meters */

	fmis->horizontal_meters = 1;			/* 0 = vertical */

	flexmeters.Initialize_Step2(fmis);		/* news meter info objects. after this, you must set them up. */

	int a = 0;

	for(a = 0; a < fmis->meter_count; a++) {
		CFlexMeters_MeterInfo	*pMeterInfo = flexmeters.GetMeterInfoObject(a);

		pMeterInfo->extra_spacing = 3;

		/* nice gradient */
		pMeterInfo->colour[0].colour = 0x00FF00;
		pMeterInfo->colour[0].at_value = 0;

		pMeterInfo->colour[1].colour = 0xFFFF00;
		pMeterInfo->colour[1].at_value = 55;

		pMeterInfo->colour[2].colour = 0xFF0000;
		pMeterInfo->colour[2].at_value = 58;

		pMeterInfo->colours_used = 3;

		pMeterInfo->value = 0;
		pMeterInfo->peak = 0;
		pMeterInfo->meter_width = 60;
	}

	flexmeters.Initialize_Step3();			/* finalize init. */

	if(gMain.vuShow) {
		m_VUStatus = VU_ON;
		m_OnOff.ShowWindow(SW_HIDE);
		if(gMain.vuShow == 1)
		{
			m_MeterPeak.ShowWindow(SW_HIDE);
			m_MeterRMS.ShowWindow(SW_SHOW);
		}
		else
		{
			m_MeterRMS.ShowWindow(SW_HIDE);
			m_MeterPeak.ShowWindow(SW_SHOW);
		}

	}
	else {
		m_VUStatus = VU_OFF;
		m_MeterPeak.ShowWindow(SW_HIDE);
		m_MeterRMS.ShowWindow(SW_HIDE);
		m_OnOff.ShowWindow(SW_SHOW);
	}
	UpdateData(FALSE);
	SetTimer(73, 50, 0);					/* set up Timer. 20ms=50hz - probably good. */
	SetVisibleState(); // sub::
//if EDCASTSTANDALONE
//	visible = !gMain.gStartMinimized;
//else
//	visible = TRUE;
//endif
	return TRUE;
}

void CMainWindow::OnDblclkEncoders(NMHDR *pNMHDR, LRESULT *pResult)
{

	OnPopupConfigure();
	*pResult = 0;
}

void CMainWindow::OnRclickEncoders(NMHDR *pNMHDR, LRESULT *pResult)
{

	int iItem = m_Encoders.GetNextItem(-1, LVNI_SELECTED);

	if(iItem >= 0) 
	{

		CMenu	menu;
		VERIFY(menu.LoadMenu(IDR_CONTEXT));

		/* Pop up sub menu 0 */
		CMenu	*popup = menu.GetSubMenu(0);

		if(popup) 
		{
			if(g[iItem]->weareconnected) 
			{
				popup->ModifyMenu(ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Disconnect");
			}
			else 
			{
				if(g[iItem]->forcedDisconnect) 
				{
					popup->ModifyMenu(ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Stop AutoConnect");
				}
				else 
				{
					popup->ModifyMenu(ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Connect");
				}
			}

			POINT	pt;
			GetCursorPos(&pt);
			popup->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);
		}
	}

	*pResult = 0;
}

void CMainWindow::OnPopupConfigure()
{
	int iItem = m_Encoders.GetNextItem(-1, LVNI_SELECTED);

	if(iItem >= 0) 
	{
		configDialog->GlobalsToDialog(g[iItem]);
		configDialog->ShowWindow(SW_SHOW);
	}
}

void CMainWindow::OnPopupConnect()
{
	int iItem = m_Encoders.GetNextItem(-1, LVNI_SELECTED);

	if(iItem >= 0) 
	{
		if(!g[iItem]->weareconnected) 
		{
			if(g[iItem]->forcedDisconnect) 
			{
				g[iItem]->forcedDisconnect = 0;
				outputStatusCallback(iItem + 1, "AutoConnect stopped.", FILE_LINE);
			}
			else 
			{
				// xyzzy - use this same code for timed connect
				m_SpecificEncoder = iItem;
				_beginthreadex(NULL,
							   0,
							   (unsigned(_stdcall *) (void *)) startSpecificedcastThread,
							   (void *) iItem,
							   0,
							   &edcastThreadTitsOnBull);
			}
		}
		else 
		{
			// xyzzy - use this same code for timed disconnect
			disconnectFromServer(g[iItem]);
			setForceStop(g[iItem], 1);
			g[iItem]->forcedDisconnect = false;
		}
	}
}

void CMainWindow::OnLimiter()
{
	UpdateData(TRUE);
}

void CMainWindow::OnStartMinimized()
{
	UpdateData(TRUE);
}

void CMainWindow::ProcessConfigDone(int enc, CConfig *pConfig)
{
	if(enc > 0) 
	{
		pConfig->DialogToGlobals(g[enc - 1]);
		writeConfigFile(g[enc - 1]);
		edcast_init(g[enc - 1]);
	}

	SetFocus();
	m_Encoders.SetFocus();
}

void CMainWindow::ProcessEditMetadataDone(CEditMetadata *pConfig)
{
	pConfig->UpdateData(TRUE);

	bool	ok = true;

	if(pConfig->m_ExternalFlag == 0) 
	{
		strcpy(gMain.externalMetadata, "URL");
		ok = false;
	}

	if(pConfig->m_ExternalFlag == 1) 
	{
		strcpy(gMain.externalMetadata, "FILE");
		ok = false;
	}

	if(pConfig->m_ExternalFlag == 2) 
	{
		strcpy(gMain.externalMetadata, "DISABLED");
	}

	strcpy(gMain.externalInterval, LPCSTR(pConfig->m_MetaInterval));
	strcpy(gMain.externalFile, LPCSTR(pConfig->m_MetaFile));
	strcpy(gMain.externalURL, LPCSTR(pConfig->m_MetaURL));

	strcpy(gMain.metadataAppendString, LPCSTR(pConfig->m_AppendString));
	strcpy(gMain.metadataRemoveStringBefore, LPCSTR(pConfig->m_RemoveStringBefore));
	strcpy(gMain.metadataRemoveStringAfter, LPCSTR(pConfig->m_RemoveStringAfter));
	strcpy(gMain.metadataWindowClass, LPCSTR(pConfig->m_WindowClass));

	if(ok) 
	{
		setLockedMetadata(&gMain, (char *) LPCSTR(pConfig->m_Metadata));
		setLockedMetadataFlag(&gMain, editMetadata->m_LockMetadata);
		if(strlen((char *) LPCSTR(pConfig->m_Metadata)) > 0) 
		{
			setMetadata((char *) LPCSTR(pConfig->m_Metadata));
		}
	}

	gMain.metadataWindowClassInd = pConfig->m_WindowTitleGrab ? true : false;
	KillTimer(metadataTimerId);
	int metadataInterval = atoi(gMain.externalInterval);
	metadataInterval = metadataInterval * 1000;
	metadataTimerId = SetTimer(4, metadataInterval, (TIMERPROC) MetadataTimer);

	SetFocus();
}

void CMainWindow::OnPopupDelete()
{
	int i = 0;
	for(i = 0; i < gMain.gNumEncoders; i++) 
	{
		if(g[i]->weareconnected) 
		{
			MessageBox("You need to disconnect all the encoders before deleting one from the list", "Message", MB_OK);
			return;
		}
	}

	int iItem = m_Encoders.GetNextItem(-1, LVNI_SELECTED);

	if(iItem >= 0) 
	{
		int ret = MessageBox("Delete this encoder ?", "Message", MB_YESNO);
		if(ret == IDYES) 
		{
			if(g[iItem]) 
			{
				deleteConfigFile(g[iItem]);
				free(g[iItem]);
			}

			m_Encoders.DeleteAllItems();
			for(i = iItem; i < gMain.gNumEncoders; i++) 
			{
				if(g[i + 1]) 
				{
					g[i] = g[i + 1];
					g[i + 1] = 0;
					deleteConfigFile(g[i]);
					g[i]->encoderNumber--;
					writeConfigFile(g[i]);
				}
			}

			gMain.gNumEncoders--;
			for(i = 0; i < gMain.gNumEncoders; i++) 
			{
				edcast_init(g[i]);
			}
		}
	}
}

void CMainWindow::CleanUp()
{
	Sleep(100);
	if(specbmp) 
	{
		DeleteObject(specbmp);
	}

	if(specdc) 
	{
		DeleteDC(specdc);
	}
}

void CMainWindow::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar) // different - but could call sub
{
	if(pScrollBar->m_hWnd == m_limitdbCtrl.m_hWnd) 
	{
		UpdateData(TRUE);
		m_staticLimitdb.Format("%d", m_limitdb);
		UpdateData(FALSE);
	}
	else if(pScrollBar->m_hWnd == m_gaindbCtrl.m_hWnd) 
	{
		UpdateData(TRUE);
		m_staticGaindb.Format("%d", m_gaindb);
		UpdateData(FALSE);
	}
	else if(pScrollBar->m_hWnd == m_limitpreCtrl.m_hWnd) 
	{
		UpdateData(TRUE);
		m_staticLimitpre.Format("%d", m_limitpre);
		UpdateData(FALSE);
	}
	else if(pScrollBar->m_hWnd == m_RecVolumeCtrl.m_hWnd) 
	{
		SetVolume(); // sub::
	}
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainWindow::OnManualeditMetadata()
{
	editMetadata->m_LockMetadata = gMain.gLockSongTitle;
	editMetadata->m_Metadata = gMain.gManualSongTitle;

	if(!strcmp("DISABLED", gMain.externalMetadata)) 
	{
		editMetadata->m_ExternalFlag = 2;
	}
	else if(!strcmp("FILE", gMain.externalMetadata)) 
	{
		editMetadata->m_ExternalFlag = 1;
	}
	else if(!strcmp("URL", gMain.externalMetadata)) 
	{
		editMetadata->m_ExternalFlag = 0;
	}

	editMetadata->m_MetaFile = gMain.externalFile;
	editMetadata->m_MetaURL = gMain.externalURL;
	editMetadata->m_MetaInterval = gMain.externalInterval;

	editMetadata->m_AppendString = gMain.metadataAppendString;
	editMetadata->m_RemoveStringAfter = gMain.metadataRemoveStringAfter;
	editMetadata->m_RemoveStringBefore = gMain.metadataRemoveStringBefore;
	editMetadata->m_WindowClass = gMain.metadataWindowClass;

	editMetadata->m_WindowTitleGrab = gMain.metadataWindowClassInd;

	editMetadata->UpdateRadio();
	editMetadata->UpdateData(FALSE);
	editMetadata->ShowWindow(SW_SHOW);
}

void CMainWindow::OnClose() // BASE - though different for DSP
{
//ifndef EDCASTSTANDALONE // dsp overrides?
//	bMinimized_ = true;
//	SetupTrayIcon();
//	SetupTaskBarButton();
//else
	int ret = IDOK;
	if(!gMain.gSkipCloseWarning)
	{
		ret = MessageBox("WARNING: Exiting Edcast\r\nSelect OK to exit!", "Exit Selected", MB_OKCANCEL | MB_ICONWARNING);
	}
	if(ret == IDOK)
	{
		OnDestroy();
		EndModalLoop(1);
		exit(1);
	}
//endif
}

void CMainWindow::OnDestroy()
{
	RECT	pRect;
	bMinimized_ = false;
	SetupTrayIcon();

	GetWindowRect(&pRect);
	UpdateData(TRUE);
	setLastX(pRect.left);
	setLastY(pRect.top);
	setLiveRecFlag(m_LiveRec);
	setAuto(m_AutoConnect);
	setStartMinimized(m_startMinimized);
	setLimiter(m_Limiter);
	setLimiterVals(m_limitdb, m_limitpre, m_gaindb);
	if(gLiveRecording) 
	{
		stopRecording();
	}
	stopedcast();
	CleanUp();
	if(configDialog) 
	{
		configDialog->DestroyWindow();
		delete configDialog;
	}
	if(editMetadata) 
	{
		editMetadata->DestroyWindow();
		delete editMetadata;
	}
	if(aboutBox) 
	{
		aboutBox->DestroyWindow();
		delete aboutBox;
	}

	writeMainConfig();

	/*
	 * DestroyWindow();
	 */
	CDialog::OnDestroy();
}

void CMainWindow::OnAboutAbout()
{
	aboutBox->m_Version = _T("Built on : "__DATE__ " "__TIME__);
	aboutBox->m_Configpath.Format(_T("Config: %s"), currentConfigDir);
	aboutBox->UpdateData(FALSE);
	aboutBox->ShowWindow(SW_SHOW);
}

void CMainWindow::OnAboutHelp()
{
	char	loc[2046] = "";
	wsprintf(loc, "%s\\%s", m_currentDir, "edcast.chm");

	HINSTANCE	ret = ShellExecute(NULL, "open", loc, NULL, NULL, SW_SHOWNORMAL);
}

void CMainWindow::OnPaint()
{
	CPaintDC	dc(this);					/* device context for painting */
}

void CMainWindow::OnTimer(UINT nIDEvent)
{
	if(nIDEvent == 73) 
	{
		int			a = 0;
		static int	oldL = 0;
		static int	oldR = 0;
		static int	oldPeakL = 0;
		static int	oldPeakR = 0;
		static int	oldCounter = 0;

		//if(m_VUStatus == VU_OFF) {
		//	return;
		//}

		HWND	hWnd = GetDlgItem(IDC_METER)->m_hWnd;

		HDC		hDC = ::GetDC(hWnd);	/* get the DC for the window. */
		if((m_VUStatus == VU_ON) || (m_VUStatus == VU_SWITCHOFF)) 
		{
			if((flexmeters.GetMeterInfoObject(0)->value == -1) && (flexmeters.GetMeterInfoObject(1)->value == -1)) 
			{
				if(oldCounter > 10) 
				{
					flexmeters.GetMeterInfoObject(0)->value = 0;
					flexmeters.GetMeterInfoObject(1)->value = 0;
					flexmeters.GetMeterInfoObject(0)->peak = 0;
					flexmeters.GetMeterInfoObject(1)->peak = 0;
					oldCounter = 0;
				}
				else 
				{
					flexmeters.GetMeterInfoObject(0)->value = oldL;
					flexmeters.GetMeterInfoObject(1)->value = oldR;
					flexmeters.GetMeterInfoObject(0)->peak = oldPeakL;
					flexmeters.GetMeterInfoObject(1)->peak = oldPeakR;
					oldCounter++;
				}
			}
			else 
			{
				oldCounter = 0;
			}

			if(m_VUStatus == VU_SWITCHOFF) 
			{
				flexmeters.GetMeterInfoObject(0)->value = 0;
				flexmeters.GetMeterInfoObject(1)->value = 0;
				flexmeters.GetMeterInfoObject(0)->peak = 0;
				flexmeters.GetMeterInfoObject(1)->peak = 0;
				m_VUStatus = VU_OFF;
			}

			flexmeters.RenderMeters(hDC);	/* render */
			oldL = flexmeters.GetMeterInfoObject(0)->value;
			oldR = flexmeters.GetMeterInfoObject(1)->value;
			oldPeakL = flexmeters.GetMeterInfoObject(0)->peak;
			oldPeakR = flexmeters.GetMeterInfoObject(1)->peak;

			flexmeters.GetMeterInfoObject(0)->value = -1;
			flexmeters.GetMeterInfoObject(1)->value = -1;
			flexmeters.GetMeterInfoObject(0)->peak = -1;
			flexmeters.GetMeterInfoObject(1)->peak = -1;

		}
		else 
		{
			flexmeters.GetMeterInfoObject(0)->value = 0;
			flexmeters.GetMeterInfoObject(1)->value = 0;
			flexmeters.GetMeterInfoObject(0)->peak = 0;
			flexmeters.GetMeterInfoObject(1)->peak = 0;
			flexmeters.RenderMeters(hDC);	/* render */
		}
		::ReleaseDC(hWnd, hDC);			/* release the DC */
	}

	CDialog::OnTimer(nIDEvent);
}

void CMainWindow::OnMeter()
{
	if(m_VUStatus == VU_ON) 
	{
		if(gMain.vuShow == 1)
		{
			gMain.vuShow = 2;
			m_MeterPeak.ShowWindow(SW_SHOW);
			m_OnOff.ShowWindow(SW_HIDE);
			m_MeterRMS.ShowWindow(SW_HIDE);
		}
		else
		{
			m_VUStatus = VU_SWITCHOFF;
			gMain.vuShow = 0;
			m_OnOff.ShowWindow(SW_SHOW);
			m_MeterPeak.ShowWindow(SW_HIDE);
			m_MeterRMS.ShowWindow(SW_HIDE);
		}
	}
	else 
	{
		m_VUStatus = VU_ON;
		gMain.vuShow = 1;
		m_MeterRMS.ShowWindow(SW_SHOW);
		m_OnOff.ShowWindow(SW_HIDE);
		m_MeterPeak.ShowWindow(SW_HIDE);
	}
}

void CMainWindow::OnSTExit()
{
	OnCancel();
}

void CMainWindow::OnSTRestore()
{
	ShowWindow(SW_RESTORE);
	bMinimized_ = false;
	SetupTrayIcon();
	SetupTaskBarButton();
}

void CMainWindow::SetupTrayIcon()
{
	if(bMinimized_ && (pTrayIcon_ == 0)) 
	{
		pTrayIcon_ = new CSystemTray;
		pTrayIcon_->Create(0, nTrayNotificationMsg_, "edcast Restore", hIcon_, IDR_SYSTRAY);
		pTrayIcon_->SetMenuDefaultItem(IDI_RESTORE, false);
		pTrayIcon_->SetNotifier(this);
	}
	else 
	{
		delete pTrayIcon_;
		pTrayIcon_ = 0;
	}
}

/*
 =======================================================================================================================
    SetupTaskBarButton Show or hide the taskbar button for this app, depending on whether ;
    we're minimized right now or not.
 =======================================================================================================================
 */
void CMainWindow::SetupTaskBarButton()
{
	/* Show or hide this window appropriately */
	if(bMinimized_) 
	{
		ShowWindow(SW_HIDE);
	}
	else 
	{
		ShowWindow(SW_SHOW);
	}
}

void CMainWindow::OnSysCommand(UINT nID, LPARAM lParam)
{
	DoSysCommand(nID, lParam);
}

void CMainWindow::OnSelchangeRecdevices()
{
	DoSelchangeRecdevices();
}

void CMainWindow::OnSelchangeReccards()
{
	DoSelchangeReccards();
}

void CMainWindow::DoSysCommand(UINT nID, LPARAM lParam) // BASE - slight diff for DSP
{
	/* Decide if minimize state changed */
	bool	bOldMin = bMinimized_;
	if(nID == SC_MINIMIZE)
	{
//ifndef EDCASTSTANDALONE // dsp overrides?
//		bMinimized_ = false;
//else
		bMinimized_ = true;
		SetupTrayIcon();
		ShowWindow(SW_HIDE);
		return;
//endif
	}
	else if(nID == SC_RESTORE) 
	{
		bMinimized_ = false;
		if(bOldMin != bMinimized_) 
		{
			/*
			 * Minimize state changed. Create the systray icon and do ;
			 * custom taskbar button handling.
			 */
			SetupTrayIcon();
			SetupTaskBarButton();
		}
	}

	CDialog::OnSysCommand(nID, lParam);
}

void CMainWindow::OnKeydownEncoders(NMHDR *pNMHDR, LRESULT *pResult)
{
	LV_KEYDOWN	*pLVKeyDow = (LV_KEYDOWN *) pNMHDR;

	if (pLVKeyDow->wVKey == 32) 
	{
		OnPopupConfigure();
	}
	else if (pLVKeyDow->wVKey == 46) 
	{
		OnPopupDelete();
	}
	else if (pLVKeyDow->wVKey == 93) 
	{
		int iItem = m_Encoders.GetNextItem(-1, LVNI_SELECTED);

		CMenu	menu;
		VERIFY(menu.LoadMenu(IDR_CONTEXT));

		/* Pop up sub menu 0 */
		CMenu	*popup = menu.GetSubMenu(0);

		if(popup) 
		{
			if(g[iItem]->weareconnected) 
			{
				popup->ModifyMenu(ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Disconnect");
			}
			else 
			{
				if(g[iItem]->forcedDisconnect) 
				{
					popup->ModifyMenu(ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Stop AutoConnect");
				}
				else 
				{
					popup->ModifyMenu(ID_POPUP_CONNECT, MF_BYCOMMAND, ID_POPUP_CONNECT, "Connect");
				}
			}

			RECT	pt;
			WINDOWPLACEMENT wp;
			WINDOWPLACEMENT wp2;
			GetWindowPlacement(&wp);
			m_Encoders.GetWindowPlacement(&wp2);

			//GetCursorPos(&pt);
			pt.bottom = wp.rcNormalPosition.bottom + wp2.rcNormalPosition.bottom;
			pt.left = wp.rcNormalPosition.left + wp2.rcNormalPosition.left;
			pt.right = wp.rcNormalPosition.right + wp2.rcNormalPosition.right;
			pt.top = wp.rcNormalPosition.top + wp2.rcNormalPosition.top;

			popup->TrackPopupMenu(TPM_LEFTALIGN, pt.left, pt.top, m_Encoders.GetActiveWindow());
		}
	}

	*pResult = 0;
}

void CMainWindow::OnButton1()
{
	OnPopupConfigure();
}

void CMainWindow::OnCancel()
{
	;
}

void CMainWindow::OnSetfocusEncoders(NMHDR *pNMHDR, LRESULT *pResult)
{
	int mark = m_Encoders.GetSelectionMark();

	if(mark == -1) 
	{
		if(m_Encoders.GetItemCount() > 0) 
		{
			m_Encoders.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED);
			m_Encoders.EnsureVisible(0, FALSE);
		}
	}
	else 
	{
		m_Encoders.SetCheck(m_Encoders.GetSelectionMark(), true);
	}

	*pResult = 0;
}

LRESULT CMainWindow::gotShowWindow(WPARAM wParam, LPARAM lParam)
{
	if(wParam)
	{
		if(!visible)
		{
			visible = TRUE;
			PostMessage(WM_MY_MESSAGE);
		}
	}
	return DefWindowProc(WM_SHOWWINDOW, wParam, lParam);
}


