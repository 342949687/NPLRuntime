//-----------------------------------------------------------------------------
// Class:	CParaEngineApp
// Authors:	LiXizhi
// Emails:	LiXizhi@yeah.net
// Company: ParaEngine Co.
// Date:	2006.1.15, revised to multi-threaded 2010.2.22
// Desc: Main ParaEngine Application: manage device, windows, messages and global objects.
//-----------------------------------------------------------------------------

#include "ParaEngine.h"

#pragma region PE Includes
#include "resource.h"
#include "DirectXEngine.h"
#include <gdiplus.h>

//-- ParaEngine includes
#include "ParaWorldAsset.h"
#include "SceneObject.h"
#include "AutoCamera.h"
#include "EnvironmentSim.h"
#include "terrain/Terrain.h"
#include "BlockEngine/BlockWorldManager.h"
#include "OceanManager.h"
#include "2dengine/GUIRoot.h"
#include "2dengine/GUIIME.h"
#include "2dengine/GUIHighlight.h"
#include "2dengine/GUIDirectInput.h"
#include "util/StringHelper.h"
#include "FrameRateController.h"
#include "ic/ICDBManager.h"
#include "ic/ICConfigManager.h"
#include "ObjectManager.h"
#include "PredefinedEvents.h"
#include "DynamicAttributeField.h"
#include "EventClasses.h"

#include "MoviePlatform.h"
#include "DataProviderManager.h"
#include "AttributeProvider.h"
#include "AttributesManager.h"
#include "ViewportManager.h"
#include "PluginManager.h"
#include "PluginAPI.h"
#include "ParaEngineService.h"
#include "guicon.h"

/** define to load xact based audio engine */
// #define USE_XACT_AUDIO_ENGINE
/** define to load openal based audio engine */
#define USE_OPENAL_AUDIO_ENGINE

#ifdef USE_XACT_AUDIO_ENGINE
#include "AudioEngine.h"
#endif

#include "AudioEngine2.h"

#include "ParaScriptingGUI.h"
#include "ParaEngineSettings.h"
#include "ParaEngineInfo.h"

#include "FileLogger.h"
#include "BootStrapper.h"
#include "NPLHelper.h"
#include "EventsCenter.h"
#ifdef USE_FLASH_MANAGER
#include "FlashTextureManager.h"
#endif
#include "NPLRuntime.h"
#include "AISimulator.h"
#include "AsyncLoader.h"

#include <list>
#include <algorithm>

// header include
#include "WindowsApplication.h"
#include "OSWindows.h"
#include <time.h>
#include "ParaEngineAppBase.h"
#include "Render/WindowsRenderWindow.h"
#include "Render/D3D9RenderContext.h"
#include "RenderSystem/d3d9/RenderDeviceD3D9.h"


#include "resource.h"
#include "2dengine/GUIRoot.h"
#include "2dengine/GUIDirectInput.h"
#include "FrameRateController.h"
#include "MiscEntity.h"

#include <functional>


#ifndef GET_POINTERID_WPARAM
#define GET_POINTERID_WPARAM(wParam)                (wParam & 0xFFFF)
#endif

#ifndef WM_POINTERUPDATE
#define WM_POINTERUPDATE	0x245
#define WM_POINTERENTER	0x249
#define WM_POINTERDOWN	0x246
#define WM_POINTERUP 0x0247
#define WM_POINTERLEAVE	0x24a
#define WM_POINTERCAPTURECHANGED	0x24c
#endif

#ifndef SM_CONVERTIBLESLATEMODE
#define SM_CONVERTIBLESLATEMODE	0x2003
#endif


using namespace std;
using namespace ParaEngine;
using namespace ParaInfoCenter;
CFrameRateController g_doWorkFRC(CFrameRateController::FRC_CONSTANT_OR_BELOW);
HINSTANCE g_hAppInstance;

#ifndef MAX_LINE
#define MAX_LINE	500
#endif

#ifdef _DEBUG
/** if this is defined. CFileLogger will be used. it first checks for ./InstallFiles.txt,
* and load it if it exists; then it checks for log file at temp/filelog.txt and load it if it exists.
* Then it hooks the CParaFile interface and record each read file operation. Once deactivated,
* it wrote the file log to temp/filelog.txt.
*/
//#define LOG_FILES_ACTIVITY
/**
* if this is defined, All files will be copied to _InstallFiles/ directory when application exits.
* this function is also available from the scripting interface called ParaIO.UpdateMirrorFiles("_InstallFiles/", true)
*/
//#define EXTRACT_INSTALL_FILE
#endif


#ifndef _DEBUG
/**@def display water mark for pre-releases */
// #define DISPLAY_WATERMARK
#endif

using namespace ParaEngine;
namespace ParaEngine
{
	/** this value changes from 0 to 1, and back to 0 in one second.*/
	float g_flash = 0.f;

	/** the main rendering window. */
	HWND* g_pHwndHWND = NULL;

	INT_PTR CALLBACK DialogProcAbout(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

	Gdiplus::GdiplusStartupInput g_gdiplusStartupInput;
	ULONG_PTR           g_gdiplusToken;
}

#pragma endregion PE Includes

#define MSGFLT_ADD 1
extern "C" BOOL(STDAPICALLTYPE *pChangeWindowMessageFilter)(UINT, DWORD) = NULL;

#pragma region CtorDtor


CWindowsApplication::CWindowsApplication(const char* lpCmdLine)
	:CParaEngineAppBase(lpCmdLine)
	, m_bHasNewConfig(false)
	, m_pWinRawMsgQueue(NULL)
	, m_dwWinThreadID(0)
	, m_bIsKeyEvent(false)
	, m_bUpdateScreenDevice(false)
	, m_bServerMode(false)
	, m_dwCoreUsage(PE_USAGE_STANDALONE)
	, m_pAudioEngine(NULL)
	, m_bAutoLowerFrameRateWhenNotFocused(false)
	, m_nInitialGameEffectSet(0)
	, m_bDrawReflection(false)
	, m_bDisplayText(false)
	, m_bDisplayHelp(false)
	, m_bAllowWindowClosing(true)
	, m_pKeyboard(NULL)
	, m_bToggleSoundWhenNotFocused(true)
	, m_bAppHasFocus(true)
	, m_hwndTopLevelWnd(NULL)
{

	SetAppState(PEAppState_None);

	m_hWnd = NULL;
	m_hWndFocus = NULL;
	m_bWindowed = true;
	m_bActive = false;
	m_bDeviceLost = false;
	m_bMinimized = false;
	m_bMaximized = false;
	m_bIgnoreSizeChange = false;
	m_bDeviceObjectsInited = false;
	m_bDeviceObjectsRestored = false;
	m_dwCreateFlags = 0;
	m_bPassiveRendering = false;
	m_bEnable3DRendering = true;
	m_bFrameMoving = true;
	m_bSingleStep = false;
	m_fTime = 0.0f;
	m_fElapsedTime = 0.0f;
	m_fFPS = 0.0f;
	m_strDeviceStats[0] = _T('\0');
	m_strFrameStats[0] = _T('\0');
	m_fRefreshTimerInterval = -1.f;
	m_nFrameRateControl = 0;

	m_strWindowTitle = _T("D3D9 Application");
	m_dwCreationWidth = 400;
	m_dwCreationHeight = 300;
	m_bShowCursorWhenFullscreen = true;
	m_bStartFullscreen = false;
	m_bCreateMultithreadDevice = true;
	m_bAllowDialogBoxMode = false;

	m_pRenderDevice = NULL;
	m_pRenderContext = nullptr;

	Pause(true); // Pause until we're ready to render

				 // When m_bClipCursorWhenFullscreen is true, the cursor is limited to
				 // the device window when the app goes fullscreen.  This prevents users
				 // from accidentally clicking outside the app window on a multimon system.
				 // This flag is turned off by default for debug builds, since it makes 
				 // multimon debugging difficult.
#if defined(_DEBUG) || defined(DEBUG)
	m_bClipCursorWhenFullscreen = false;
#else
	m_bClipCursorWhenFullscreen = true;
#endif


	g_pHwndHWND = &m_hWnd;
	CFrameRateController::LoadFRCNormal();
	StartApp(lpCmdLine);
}



HRESULT CWindowsApplication::Create()
{
	HRESULT hr;

	if (m_hWnd == NULL)
	{
		OUTPUT_LOG("error: render window is not created when creating Create()\n");
		return E_FAIL;
	}

	SetAppState(PEAppState_Device_Created);


	m_pRenderContext = D3D9RenderContext::Create();


	if (m_pRenderContext == NULL)
		return E_FAIL;


	// The focus window can be a specified to be a different window than the
	// device window.  If not, use the device window as the focus window.
	if (m_hWndFocus == NULL)
		m_hWndFocus = m_hWnd;

	OUTPUT_LOG("DEBUG: main thread wnd handle : %d\n", m_hWndFocus);

	// Initialize the application timer
	DXUtil_Timer(TIMER_START);

	// Initialize the app's custom scene stuff
	if (FAILED(hr = OneTimeSceneInit()))
	{
		delete m_pRenderContext;
		m_pRenderContext = nullptr;
		return E_FAIL;
	}


	// Initialize the 3D environment for the app
	if (FAILED(hr = Initialize3DEnvironment()))
	{
		delete m_pRenderContext;
		m_pRenderContext = nullptr;
		return E_FAIL;
	}


	// The app is ready to go
	Pause(false);

	SetAppState(PEAppState_Ready);
	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: Render3DEnvironment()
// Desc: Here's what this function does:
//       - Checks to make sure app is still active (if fullscreen, etc)
//       - Checks to see if it is time to draw with DXUtil_Timer, if not, it just returns S_OK
//       - Calls FrameMove() to recalculate new positions
//       - Calls Render() to draw the new frame
//       - Updates some frame count statistics
//       - Calls m_pd3dDevice->Present() to display the rendered frame.
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::Render3DEnvironment(bool bForceRender)
{
	HRESULT hr = S_OK;

	if (m_bDeviceLost)
	{
		// Test the cooperative level to see if it's okay to render
		if (FAILED(hr = m_pRenderDevice->TestCooperativeLevel()))
		{
			// If the device was lost, do not render until we get it back
			if (D3DERR_DEVICELOST == hr)
				return S_OK;

			// Check if the device needs to be reset.
			if (D3DERR_DEVICENOTRESET == hr)
			{


				OUTPUT_LOG("TestCooperativeLevel needs to reset device with D3DERR_DEVICENOTRESET\n");

				if (FAILED(hr = Reset3DEnvironment()))
				{
					// This fixed a strange issue where d3d->reset() returns D3DERR_DEVICELOST. Perhaps this is due to strange. 
					// I will keep reset for 5 seconds, until reset() succeed, otherwise we will exit application.  
					for (int i = 0; i < 5 && hr == D3DERR_DEVICELOST; ++i)
					{
						::Sleep(1000);
						if (SUCCEEDED(hr = Reset3DEnvironment()))
						{
							OUTPUT_LOG("TestCooperativeLevel successfully reset devices.\n");
							return hr;
						}
					}
					return E_FAIL;
				}
			}
			return hr;
		}
		m_bDeviceLost = false;
	}

	// Get the app's time, in seconds. Skip rendering if no time elapsed
	double fAppTime = DXUtil_Timer(TIMER_GETAPPTIME);
	double fElapsedAppTime = DXUtil_Timer(TIMER_GETELAPSEDTIME);

	//////////////////////////////////////////////////////////////////////////
	//
	// frame rate control at 30 FPS
	//
	//////////////////////////////////////////////////////////////////////////

	static double fRenderTime = 0;
	static double fConstTime = 0;
	bool bUseIdealFrameRate = (m_nFrameRateControl == 1);

	fRenderTime += fElapsedAppTime;
	fConstTime += fElapsedAppTime;
	if (!bUseIdealFrameRate || (fConstTime >= IDEAL_FRAME_RATE) || bForceRender)
	{
		m_fElapsedTime = bUseIdealFrameRate ? fRenderTime : fElapsedAppTime;


		if (bForceRender)
		{
			fConstTime = 0;
		}
		else
		{
			// TRICKY: this helps to smooth the frame rate to IDEAL_FRAME_RATE, when force render is false
			fConstTime -= IDEAL_FRAME_RATE;
			if (fConstTime >= IDEAL_FRAME_RATE) // if 0.5 seconds is passed, we will catch up 
				fConstTime = 0;
		}
		fRenderTime = 0;

		if ((0.0f == fElapsedAppTime) && m_bFrameMoving)
			return S_OK;

		if (bUseIdealFrameRate)
		{
			// only use ideal frame rate if interval is smaller than it.
			if (m_fRefreshTimerInterval < IDEAL_FRAME_RATE)
			{
				// TRICKY: this fixed a bug of inaccurate timing, we will assume a perfect timing here, but correct it if it differentiate too much from   
				// FrameMove (animate) the scene, frame move is called before Render(), since the frame move may contain ForceRender() calls
				// Store the time for the app
				if (m_fTime < (fAppTime - IDEAL_FRAME_RATE)) // if 0.5 seconds is passed, we will catch up 
					m_fTime = fAppTime;
				else
					m_fTime += IDEAL_FRAME_RATE;
			}
			else
			{
				// if we special refresh timer to be 0.03334 or bigger, we shall use absolute time. 
				m_fTime = fAppTime;
			}
		}
		else
		{
			m_fTime = fAppTime;
		}

		// OUTPUT_LOG("%f:%f, %f, %f\n", m_fTime, fAppTime, m_fElapsedTime, fConstTime);

		__try
		{
			// UpdateViewPort();

			// Frame move the scene
			if (SUCCEEDED(hr = FrameMove()))
			{
			}

			// Render the scene as normal

			if (m_bActive && !IsPassiveRenderingEnabled())
			{
				if (Is3DRenderingEnabled() && !m_bMinimized && SUCCEEDED(hr) && SUCCEEDED(hr = Render()))
				{
					// only present if render returns true.
					hr = PresentScene();
				}
			}
			else
			{
				// passive mode: sleep until the next ideal frame move time and a little more. 0.1 seconds
				Sleep(100);
			}
		}
		__except (GenerateDump(GetExceptionInformation()))
		{
			exit(0);
		}
		// Show the frame on the primary surface.
	}
	return S_OK;
}

HRESULT CWindowsApplication::PresentScene()
{
	// OUTPUT_LOG("---------\n");
	HRESULT hr;
	// only present if render returns true.
	PERF1("present");
	hr = m_pRenderDevice->Present(NULL, NULL, NULL, NULL);
	if (D3DERR_DEVICELOST == hr)
		m_bDeviceLost = true;
	return hr;
}


bool CWindowsApplication::UpdateViewPort()
{
	if (CGlobals::GetRenderDevice())
	{
		D3DVIEWPORT9 CurrentViewport;
		CGlobals::GetRenderDevice()->GetViewport(&CurrentViewport);
		if (m_pRenderWindow->GetWidth() != CurrentViewport.Width && m_pRenderWindow->GetHeight() != CurrentViewport.Height)
		{
			CurrentViewport.Width = m_pRenderWindow->GetWidth();
			CurrentViewport.Height = m_pRenderWindow->GetHeight();
			CGlobals::GetRenderDevice()->SetViewport(&CurrentViewport);
		}
		return true;
	}
	return false;
}

HRESULT CWindowsApplication::Initialize3DEnvironment()
{
	HRESULT hr = S_OK;


	m_bWindowed = TRUE;



	RenderConfiguration cfg;
	cfg.isWindowed = !m_bStartFullscreen;
	cfg.renderWindow = m_pRenderWindow;

	m_pRenderDevice = m_pRenderContext->CreateDevice(cfg);
	if (!m_pRenderDevice)
	{
		OUTPUT_LOG("Error: Can not create render device. \n");
		return E_FAIL;
	}

	CGlobals::SetRenderDevice(m_pRenderDevice);


	// Initialize the app's device-dependent objects
	hr = InitDeviceObjects();



	if (FAILED(hr))
	{
		DeleteDeviceObjects();
	}
	else
	{
		m_bDeviceObjectsInited = true;
		hr = RestoreDeviceObjects();
		if (FAILED(hr))
		{
			InvalidateDeviceObjects();
		}
		else
		{
			m_bDeviceObjectsRestored = true;
			return S_OK;
		}
	}

	return hr;
}





HRESULT CWindowsApplication::Reset3DEnvironment()
{
	HRESULT hr = E_FAIL;

	// Release all vidmem objects
	if (m_bDeviceObjectsRestored)
	{
		m_bDeviceObjectsRestored = false;
		InvalidateDeviceObjects();
	}

	RenderConfiguration cfg;
	cfg.renderWindow = m_pRenderWindow;
	cfg.isWindowed = true;

	if (!m_pRenderContext->ResetDevice(m_pRenderDevice, cfg))
	{
		OUTPUT_LOG("reset d3d device failed because Reset function failed: %d\n", hr);
		InterpretError(hr, __FILE__, __LINE__);
		return hr;
	}


	// Initialize the app's device-dependent objects
	hr = RestoreDeviceObjects();
	if (FAILED(hr))
	{
		OUTPUT_LOG("reset d3d device failed because Restor func failed: %d\n", hr);
		InvalidateDeviceObjects();
		return hr;
	}
	m_bDeviceObjectsRestored = true;

	// If the app is paused, trigger the rendering of the current frame
	if (false == m_bFrameMoving)
	{
		m_bSingleStep = true;
		DXUtil_Timer(TIMER_START);
		DXUtil_Timer(TIMER_STOP);
	}

	return S_OK;
}


void CWindowsApplication::UpdateStats()
{
	// Keep track of the frame count
	static double fLastTime = 0.0f;
	static DWORD dwFrames = 0;
	double fTime = DXUtil_Timer(TIMER_GETABSOLUTETIME);
	++dwFrames;

	// Update the scene stats once per second
	if (fTime - fLastTime > 1.0f)
	{
		m_fFPS = (float)(dwFrames / (fTime - fLastTime));
		fLastTime = fTime;
		dwFrames = 0;

		const int cchMaxFrameStats = sizeof(m_strFrameStats) / sizeof(TCHAR);
		_sntprintf(m_strFrameStats, cchMaxFrameStats, _T("%.02f fps (%dx%d)"), m_fFPS,
			m_pRenderWindow->GetWidth(), m_pRenderWindow->GetHeight());
		m_strFrameStats[cchMaxFrameStats - 1] = TEXT('\0');
	}
}

void CWindowsApplication::SetRefreshTimer(float fTimeInterval, int nFrameRateControl)
{
	if (nFrameRateControl == 1)
	{
		CFrameRateController::LoadFRCNormal(fTimeInterval);
	}
	else
	{
		CFrameRateController::LoadFRCRealtime(fTimeInterval);
	}
	m_fRefreshTimerInterval = fTimeInterval;
	m_nFrameRateControl = nFrameRateControl;
	g_doWorkFRC.m_fConstDeltaTime = (m_fRefreshTimerInterval <= 0.f) ? IDEAL_FRAME_RATE : m_fRefreshTimerInterval;
}

float CWindowsApplication::GetRefreshTimer() const
{
	return m_fRefreshTimerInterval;
}

//-----------------------------------------------------------------------------
// Name: Pause()
// Desc: Called in to toggle the pause state of the app.
//-----------------------------------------------------------------------------
void CWindowsApplication::Pause(bool bPause)
{
	// OUTPUT_LOG("game is %s\n", bPause ? "paused" : "resumed");
	m_bActive = !bPause;
}

bool CWindowsApplication::IsPaused()
{
	return !m_bActive;
}


//-----------------------------------------------------------------------------
// Name: Cleanup3DEnvironment()
// Desc: Cleanup scene objects
//-----------------------------------------------------------------------------
void CWindowsApplication::Cleanup3DEnvironment()
{
	if (m_pRenderDevice != NULL)
	{
		if (m_bDeviceObjectsRestored)
		{
			m_bDeviceObjectsRestored = false;
			InvalidateDeviceObjects();
		}
		if (m_bDeviceObjectsInited)
		{
			m_bDeviceObjectsInited = false;
			DeleteDeviceObjects();
		}

		delete m_pRenderDevice;
		m_pRenderDevice = nullptr;
	}
}




int CWindowsApplication::Run(HINSTANCE hInstance)
{


	//add a console window for debug or when in server mode.
	if (!Is3DRenderingEnabled() || IsDebugBuild())
	{
		RedirectIOToConsole();
	}
	auto result = 0;
	if (Is3DRenderingEnabled())
	{
		int nWidth, nHeight;
		CGlobals::GetApp()->GetWindowCreationSize(&nWidth, &nHeight);

		WCHAR* WindowClassName = L"ParaWorld";
		WCHAR* WindowTitle = L"ParaEngine Window";
		m_pRenderWindow = new WindowsRenderWindow(hInstance, nWidth, nHeight, false);
		m_hWnd = m_pRenderWindow->GetHandle();
		auto msgCallback = std::bind(&CWindowsApplication::HandleWindowMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		m_pRenderWindow->SetMessageCallBack(msgCallback);
		SetMainWindow(m_hWnd, false);
		Create();

		while (!m_pRenderWindow->ShouldClose())
		{
			m_pRenderWindow->PollEvents();
			DoWork();
		}

	}
	else
	{
		// the console window is used.
		CParaEngineService service;
		result = service.Run(0, this);
	}
	return result;
}

HRESULT CWindowsApplication::DoWork()
{
	// continue with next activation. 
	double fCurTime = DXUtil_Timer(TIMER_GETAPPTIME);
	if (g_doWorkFRC.FrameMove(fCurTime) > 0)
	{
		return Render3DEnvironment();
	}
	return S_OK;
}


LRESULT CWindowsApplication::HandleWindowMessage(WindowsRenderWindow* sender, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = sender->GetHandle();
	if (uMsg == WM_DESTROY)
	{
		PostWinThreadMessage(PE_WM_QUIT, 0, 0);
	}
	return MsgProcWinThread(hWnd, uMsg, wParam, lParam, true);
}


CViewportManager* CWindowsApplication::GetViewportManager()
{
	return m_pViewportManager.get();
}

void CWindowsApplication::InitApp(const char* sCommandLine)
{
	if (m_pWinRawMsgQueue == 0)
		m_pWinRawMsgQueue = new CWinRawMsgQueue();

	SetAppCommandLine(sCommandLine);

	InitCommandLineParams();

	COSInfo::DumpSystemInfo();

	if (Is3DRenderingEnabled())
	{
		// Initialize GDI+.
		Gdiplus::GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);
	}

	// Initialize COM
	CoInitialize(NULL);
}

bool CWindowsApplication::IsServerMode()
{
	return m_bServerMode;
}

void CWindowsApplication::InitLogger()
{
#ifdef LOG_FILES_ACTIVITY
	CFileLogger::GetInstance()->BeginFileLog();
	CFileLogger::GetInstance()->LoadLogFromFile("InstallFiles.txt");
	CFileLogger::GetInstance()->LoadLogFromFile("temp/filelog.txt");
#endif

#ifdef _DEBUG
	//		CGlobals::GetAttributesManager()->PrintObject("test.txt", m_pRootScene);
	//		//CGlobals::GetAttributesManager()->PrintManual("test.txt");
#endif
}

void CWindowsApplication::BootStrapAndLoadConfig()
{
	FindBootStrapper();
	{
		// load settings from config/config.txt or config/config.new.txt
		string sConfigFile = CParaFile::GetCurDirectory(CParaFile::APP_CONFIG_DIR) + "config.new.txt";
		bool bHasNewConfig = CParaFile::DoesFileExist(sConfigFile.c_str());
		SetHasNewConfig(bHasNewConfig);
		if (!bHasNewConfig)
			sConfigFile = (CParaFile::GetCurDirectory(CParaFile::APP_CONFIG_DIR) + "config.txt");
		string sFileName = CBootStrapper::GetSingleton()->GetConfigFile().empty() ? sConfigFile : CBootStrapper::GetSingleton()->GetConfigFile();
		CParaFile file(sFileName.c_str());
		if (!file.isEof())
		{
			string content = file.getBuffer();
			ParaEngineSettings::GetSingleton().LoadDynamicFieldsFromString(content);
		}
		if (bHasNewConfig)
		{
			CParaFile::DeleteFile(sConfigFile, false);
		}
	}
}

void CWindowsApplication::InitWin3DSettings()
{
	m_nWindowedDesired = -1;

	/// Just turn off Full screen cursor, we will use mine.
	m_bShowCursorWhenFullscreen = false;
}

bool CWindowsApplication::CheckClientLicense()
{
	// check for license file
	CParaFile file((CParaFile::GetCurDirectory(CParaFile::APP_CONFIG_DIR) + "license.txt").c_str());
	if (!file.isEof())
	{
		char buf[MAX_LINE + 1];
		memset(buf, 0, sizeof(buf));
		// register the fist line of file
		if (file.GetNextLine(buf, MAX_LINE) > 0)
		{
			ParaEngineSettings::ActivateProduct(buf);
		}
	}
	return (ParaEngineSettings::IsProductActivated());
}


void CWindowsApplication::LoadAndApplySettings()
{
	// load from settings.
	ParaEngineSettings& settings = ParaEngineSettings::GetSingleton();

	CDynamicAttributeField* pField = NULL;
	if ((pField = settings.GetDynamicField("StartFullscreen")))
		m_bStartFullscreen = (bool)(*pField);
	else
		m_bStartFullscreen = false;


	if ((pField = settings.GetDynamicField("ScreenWidth")))
		m_dwCreationWidth = (int)(*pField);
	else
		m_dwCreationWidth = 1020;

	if ((pField = settings.GetDynamicField("ScreenHeight")))
		m_dwCreationHeight = (int)(*pField);
	else
		m_dwCreationHeight = 680;


	if ((pField = settings.GetDynamicField("ScriptEditor")))
		settings.SetScriptEditor((const string&)(*pField));

	if ((pField = settings.GetDynamicField("GameEffectSet")))
		m_nInitialGameEffectSet = (int)(*pField);

	if ((pField = settings.GetDynamicField("InverseMouse")))
		settings.SetMouseInverse((bool)(*pField));

	if ((pField = settings.GetDynamicField("language")))
		settings.SetLocale((const char*)(*pField));

	if ((pField = settings.GetDynamicField("TextureLOD")))
		settings.SetTextureLOD((int)(*pField));

	const char* sIsFullScreen = GetAppCommandLineByParam("fullscreen", NULL);
	if (sIsFullScreen)
		m_bStartFullscreen = (strcmp("true", sIsFullScreen) == 0);
}

void CWindowsApplication::InitSystemModules()
{
	m_pParaWorldAsset.reset(new CParaWorldAsset());
	CAISimulator::GetSingleton()->SetGameLoop(CBootStrapper::GetSingleton()->GetMainLoopFile());
	m_pRootScene.reset(new CSceneObject());
	m_pGUIRoot.reset(CGUIRoot::CreateInstance());
	m_pViewportManager.reset(new CViewportManager());
	m_pViewportManager->SetLayout(VIEW_LAYOUT_DEFAULT, m_pRootScene.get(), m_pGUIRoot.get());
#ifdef USE_XACT_AUDIO_ENGINE
	m_pAudioEngine = CAudioEngine::GetInstance();
#endif
}


bool ParaEngine::CWindowsApplication::StartApp(const char* sCommandLine /*= 0*/)
{
	SetCurrentInstance(this);
	std::string strCmd;
	VerifyCommandLine(sCommandLine, strCmd);
	InitApp(strCmd.c_str());
	// loading packages
	LoadPackages();
	BootStrapAndLoadConfig();
	InitSystemModules();
	InitLogger();
	InitWin3DSettings();
	CheckClientLicense();
	LoadAndApplySettings();
	return S_OK;
}


CWindowsApplication::~CWindowsApplication()
{
	StopApp();
}

void CWindowsApplication::StopApp()
{
	// if it is already stopped, we shall return
	if (!m_pParaWorldAsset)
		return;

	SAFE_DELETE(m_pWinRawMsgQueue);


	Cleanup3DEnvironment();
	delete m_pRenderContext;
	m_pRenderContext = nullptr;

	FinalCleanup();

	m_pParaWorldAsset.reset();
	m_pRootScene.reset();
	m_pGUIRoot.reset();
	m_pViewportManager.reset();
	m_pGUIRoot.reset();

	// delete m_pAudioEngine;
	CoUninitialize();

	//#ifdef LOG_FILES_ACTIVITY
	if (CFileLogger::GetInstance()->IsBegin())
	{
		CFileLogger::GetInstance()->EndFileLog();
		CFileLogger::GetInstance()->SaveLogToFile("temp/filelog.txt");
	}
	//#endif
#ifdef EXTRACT_INSTALL_FILE
	CFileLogger::GetInstance()->MirrorFiles("_InstallFiles/");
#endif

	Gdiplus::GdiplusShutdown(g_gdiplusToken);

	// delete all singletons
	DestroySingletons();
}
#pragma endregion CtorDtor

HINSTANCE CWindowsApplication::GetModuleHandle()
{
	return g_hAppInstance;
}

void CWindowsApplication::SetMainWindow(HWND hWnd, bool bIsExternalWindow)
{
	m_hWnd = hWnd;
	m_dwWinThreadID = ::GetWindowThreadProcessId(hWnd, NULL);
	m_bWindowed = true;

	/** let us find the top-level window which should be the foreground window. */
	m_hwndTopLevelWnd = hWnd;
	HWND wndParent = NULL;
	while ((wndParent = ::GetParent(m_hwndTopLevelWnd)) != NULL)
	{
		m_hwndTopLevelWnd = wndParent;
	}

	OUTPUT_LOG("Window is %s: 3d window: %x, top level: %x\n", bIsExternalWindow ? "external" : "native", m_hWnd, m_hwndTopLevelWnd);

#ifdef USE_FLASH_MANAGER
	CGlobals::GetAssetManager()->GetFlashManager().SetParentWindow(hWnd);
#endif
}

HWND CWindowsApplication::GetMainWindow()
{
	return m_hWnd;
}


HRESULT CWindowsApplication::Create(HINSTANCE hInstance)
{
	m_hInstance = hInstance;
	HRESULT hr = Create();

	return hr;
}

void CWindowsApplication::SetAllowWindowClosing(bool bAllowClosing)
{
	m_bAllowWindowClosing = bAllowClosing;
}

bool CWindowsApplication::IsWindowClosingAllowed()
{
	return m_bAllowWindowClosing;
};

HRESULT CWindowsApplication::OnCreateWindow()
{
	return S_OK;
}

HRESULT CWindowsApplication::Init(HWND* pHWND)
{
	//load config file
	CICConfigManager *cm = CGlobals::GetICConfigManager();
	cm->LoadFromFile();

	srand(GetTickCount());

	//Performance monitor
	PERF_BEGIN("Program");

	if (pHWND != 0)
	{
		SetMainWindow(*pHWND, false);

		bool g_bEnableDragAndDropFile = true;
		if (g_bEnableDragAndDropFile)
		{
			DragAcceptFiles(*pHWND, TRUE);

			if (COSInfo::GetOSMajorVersion() > 5)
			{
				/** fixing win vista or win 7 security filters. */
				HMODULE hMod = 0;

				if ((hMod = ::LoadLibrary(_T("user32.dll"))) != 0)
				{
					pChangeWindowMessageFilter = (BOOL(__stdcall *)(UINT, DWORD))::GetProcAddress(hMod, "ChangeWindowMessageFilter");
				}
				if (pChangeWindowMessageFilter)
				{
					pChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
					pChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
					pChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
				}
			}
		}



		HMENU hMenu = GetMenu(*pHWND);

		if (hMenu != 0)
		{
#ifdef _DEBUG
			UINT menustate = MF_ENABLED;
#else
			UINT menustate = MF_GRAYED;
#endif

			EnableMenuItem(hMenu, ID_GAME_DEBUGMODE, menustate);
			EnableMenuItem(hMenu, ID_GAME_PLAYMODE, menustate);
			EnableMenuItem(hMenu, ID_GAME_DEMOMODE, menustate);
			EnableMenuItem(hMenu, ID_GAME_DEBUGMODE, menustate);
			EnableMenuItem(hMenu, ID_GAME_SERVERMODE, MF_ENABLED);

			EnableMenuItem(hMenu, ID_GAME_FUNCTION1, menustate);
			EnableMenuItem(hMenu, ID_GAME_FUNCTION2, MF_ENABLED);
			EnableMenuItem(hMenu, ID_GAME_FUNCTION3, menustate);
			EnableMenuItem(hMenu, ID_GAME_FUNCTION4, menustate);
		}

		/// set up dSound
		// m_pDSound->Initialize( CGlobals::GetAppHWND(), DSSCL_PRIORITY );

		HRESULT hr;
#ifdef USE_XACT_AUDIO_ENGINE
		// Prepare the audio engine
		if (FAILED(hr = m_pAudioEngine->InitAudioEngine()))
		{
			OUTPUT_LOG("Audio engine init fail!\n");
			m_pAudioEngine->CleanupAudioEngine();
		}
#endif
#ifdef USE_OPENAL_AUDIO_ENGINE
		if (FAILED(hr = CAudioEngine2::GetInstance()->InitAudioEngine()))
		{
			OUTPUT_LOG("Audio engine init fail!\n");
			CAudioEngine2::GetInstance()->CleanupAudioEngine();
		}
#endif
	}

	//----------------------------------------------------------
	/// Create a blank root scene with certain dimensions
	/// units is Meter.
	//----------------------------------------------------------
	m_pRootScene->SetBoundRect(1000.f, 1000.f, 0); // a very large scene
	m_pRootScene->SetMyType(_Scene);
	m_pRootScene->GetSceneState()->pAssetManager = m_pParaWorldAsset.get();
	m_pRootScene->GetSceneState()->CleanupSceneState();
	m_pRootScene->GetSceneState()->pGUIState = &(m_pGUIRoot->GetGUIState());

	/// create the default system font, the game should also use this sys font to save resources


	{
		// Load font mapping
		string value0, value1;
		DWORD nSize = 0;
		HRESULT hr;
		hr = cm->GetSize("GUI_font_mapping", &nSize);
		if (hr == E_INVALIDARG || hr == E_ACCESSDENIED) {
			//error
		}
		else {
			for (int i = 0; i < (int)nSize; i += 2) {
				if (FAILED(cm->GetTextValue("GUI_font_mapping", value0, i))) {
					break;
				}
				if (FAILED(cm->GetTextValue("GUI_font_mapping", value1, i + 1))) {
					break;
				}
				SpriteFontEntity::AddFontName(value0, value1);
			}
		}
	}

	SpriteFontEntity* pFont = NULL;
	pFont = m_pParaWorldAsset->LoadGDIFont("sys", "System", 12);

	/// set up terrain engine parameters
	ParaTerrain::Settings::GetInstance()->SetVerbose(false);
	// enable editor mode to make device lost recoverable
	ParaTerrain::Settings::GetInstance()->SetEditor(true);
#ifdef _USE_NORMAL_
	ParaTerrain::Settings::GetInstance()->SetUseNormals(true);
#endif
	// perform GUI static initialization
	// perform CGUIRoot initialization
	m_pGUIRoot->OneTimeGUIInit();

	/************************************************************************/
	/* Create ocean manager                                                 */
	/************************************************************************/
	CGlobals::GetOceanManager()->create();
	// Load default mapping at the program start
	CGlobals::GetSettings().LoadGameEffectSet(m_nInitialGameEffectSet);

#ifdef _DEBUG
	//CBaseTable::test();

	//AttributeProvider* m_pProvider =  CGlobals::GetDataProviderManager()->GetAttributeProvider();
	//m_pProvider->TestDB();

	//CNpcDatabase* m_pProvider =  CGlobals::GetDataProviderManager()->GetNpcDB();
	//m_pProvider->TestDB();

	//CKidsDBProvider* m_pProvider =  CGlobals::GetDataProviderManager()->GetKidsDBProvider();
	//m_pProvider->TestDB();

	//// Old version:
	//CGlobals::GetDataProviderManager()->SetKidsDBProvider("database/Kids.db");
	//m_pProvider->TestDB();
#endif
	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
/// Called during initial app startup, this function performs all the
///       permanent initialization. ParaEngine modules are setup here.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::OneTimeSceneInit()
{
	return CWindowsApplication::Init(&m_hWnd);
}


//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
/// Initialize scene objects.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::InitDeviceObjects()
{
	HRESULT hr = S_OK;

	// stage b.1
	CGlobals::GetDirectXEngine().InitDeviceObjects(static_cast<D3D9RenderContext*>(m_pRenderContext)->GetD3D(), static_cast<RenderDeviceD3D9*>(m_pRenderDevice)->GetDirect3DDevice9(), NULL);

	// print stats when device is initialized.
	string stats;
	GetStats(stats, 0);
	OUTPUT_LOG("Graphics Stats:\n%s\n", stats.c_str());
	OUTPUT_LOG("VS:%d PS:%d\n", CGlobals::GetDirectXEngine().GetVertexShaderVersion(), CGlobals::GetDirectXEngine().GetPixelShaderVersion());

	/// Asset must be the first to be initialized. Otherwise, the global device object will not be valid
	m_pParaWorldAsset->InitDeviceObjects();
	m_pRootScene->InitDeviceObjects();
	m_pGUIRoot->InitDeviceObjects();		// GUI: 2D engine

	return hr;
}

//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
/// Initialize scene objects.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::RestoreDeviceObjects()
{
	/*------------------------------------------------------
	* start of ParaWorld code
	*-----------------------------------------------------------*/
	IRenderDevice* pRenderDevice = CGlobals::GetRenderDevice();
	CGlobals::GetDirectXEngine().RestoreDeviceObjects();

	UINT nBkbufWidth = CGlobals::GetDirectXEngine().m_d3dsdBackBuffer.Width;
	UINT nBkbufHeight = CGlobals::GetDirectXEngine().m_d3dsdBackBuffer.Height;

	/// Set up the camera's projection matrix
	m_fAspectRatio = nBkbufWidth / (FLOAT)nBkbufHeight;
	m_pRootScene->RestoreDeviceObjects();
	m_pParaWorldAsset->RestoreDeviceObjects();
	m_pGUIRoot->RestoreDeviceObjects(nBkbufWidth, nBkbufHeight);		// GUI: 2D engine

																		// for terrain
	ParaTerrain::Settings::GetInstance()->SetScreenWidth(nBkbufWidth);
	ParaTerrain::Settings::GetInstance()->SetScreenHeight(nBkbufHeight);

	/// these render state is preferred.
	pRenderDevice->SetRenderState(ERenderState::DITHERENABLE, TRUE);
	pRenderDevice->SetRenderState(ERenderState::LIGHTING, FALSE);		// disable lighting
															//pd3dDevice->SetRenderState( ERenderState::AMBIENT, 0x33333333 /*COLOR_ARGB( 255, 255, 255, 255 )*/ );
	pRenderDevice->SetRenderState(ERenderState::AMBIENT, COLOR_ARGB(255, 255, 255, 255));

	pRenderDevice->SetRenderState(ERenderState::ZENABLE, TRUE);
	pRenderDevice->SetRenderState(ERenderState::ZWRITEENABLE, TRUE);

	/// these render state is just for point occlusion testing.
	/// See also CBaseObject::DrawOcclusionObject()
	//#define POINT_OCCLUSION_OBJECT
#ifdef POINT_OCCLUSION_OBJECT
	float PointSize = 1.f;
	pRenderDevice->SetRenderState(ERenderState::POINTSIZE, *((DWORD*)&PointSize));
	pRenderDevice->SetRenderState(ERenderState::POINTSIZE_MIN, *((DWORD*)&PointSize));
	pRenderDevice->SetRenderState(ERenderState::POINTSCALEENABLE, false);
	pRenderDevice->SetRenderState(ERenderState::POINTSCALEENABLE, false);
#endif

	/* default state */
#ifdef FAST_RENDER
	/*pRenderDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	pRenderDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	pRenderDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	pRenderDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	pRenderDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	pRenderDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	pRenderDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP );
	pRenderDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP );*/
#else
	pRenderDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pRenderDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pRenderDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pRenderDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

#endif
	/* -------end of paraworld code ----------------------------*/
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
/// Called when the device-dependent objects are about to be lost.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::InvalidateDeviceObjects()
{
	m_pRootScene->InvalidateDeviceObjects();
	m_pParaWorldAsset->InvalidateDeviceObjects();
	m_pGUIRoot->InvalidateDeviceObjects();		// GUI: 2D engine
	CGlobals::GetDirectXEngine().InvalidateDeviceObjects();
	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
/// Called when the app is exiting, or the device is being changed,
///       this function deletes any device dependent objects.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::DeleteDeviceObjects()
{
	// --stage c.1
	m_pRootScene->DeleteDeviceObjects();
	m_pGUIRoot->DeleteDeviceObjects();		// GUI: 2D engine
	m_pParaWorldAsset->DeleteDeviceObjects();
	CGlobals::GetDirectXEngine().DeleteDeviceObjects();
	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: FinalCleanup()
/// Called before the app exits, this function gives the app the chance
///       to cleanup after itself.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::FinalCleanup()
{
	CBlockWorldManager::GetSingleton()->Cleanup();

	CParaEngineAppBase::FinalCleanup();

	CGlobals::GetMoviePlatform()->Cleanup();

	m_pGUIRoot->Release();		// GUI: 2D engine
	m_pRootScene->Cleanup();
	CSingleton<CObjectManager>::Instance().Finalize();
	CSingleton<CGUIHighlightManager>::Instance().Finalize();
	m_pParaWorldAsset->Cleanup();
	//Performance Monitor
	PERF_END("Program");
	PERF_REPORT();

#ifdef USE_XACT_AUDIO_ENGINE
	if (m_pAudioEngine)
	{
		m_pAudioEngine->CleanupAudioEngine();
	}
#endif
#ifdef USE_OPENAL_AUDIO_ENGINE
	CAudioEngine2::GetInstance()->CleanupAudioEngine();
#endif

	return S_OK;
}

/** set client rect. this will ensure that the left, top position is not changed. */
void SetClientRect(HWND hwnd, RECT& rect)
{
	RECT oldRect = rect;
	AdjustWindowRectEx(&rect, GetWindowLong(hwnd, GWL_STYLE),
		FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
	rect.left = oldRect.left;
	rect.top = oldRect.top;
	//SetWindowPos( hwnd, 0,0, 0,rect.right - rect.left,rect.bottom - rect.top,
	//	SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOZORDER);
	MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}
void CWindowsApplication::SetAppWndRect(const RECT& rect)
{
	RECT rect_ = rect;
	SetClientRect(CGlobals::GetAppHWND(), rect_);
}

bool tempRunOnceVar = false;

HRESULT CWindowsApplication::FrameMove()
{
	return FrameMove(m_fTime);
}
//-----------------------------------------------------------------------------
// Name: FrameMove()
/// Called once per frame, the call is the entry point for animating
///       the scene.
/// ParaEngine fixed code: must call these functions as given below
/** TODO: I have no idea whether we should simulate before calling IO functions or vice versa.
* since, some IO actions will not be validated by the simulator before it is rendered.
* However, the camera IO need to get the biped position that is being rendered for the current frame.
* hence the current order is changed to SIM->SCRIPT->IO(camera and biped control)->RENDER
* the old order is IO(camera and biped control)->SIM->SCRIPT->RENDER */
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::FrameMove(double fTime)
{
	bool bIOExecuted = false;
	/** process all messages in the main game thread.
	*/
	{
		HRESULT result = 0;
		CWinRawMsg msg;
		while (GetMessageFromApp(&msg))
		{
			if (MsgProcApp(msg.m_hWnd, msg.m_uMsg, msg.m_wParam, msg.m_lParam) == 2)
			{
				return S_OK;
			}
		}
	}
	if (GetAppState() == PEAppState_Stopped)
		return S_OK;

	double fElapsedGameTime = CGlobals::GetFrameRateController(FRC_GAME)->FrameMove(fTime);
	PERF_BEGIN("Main FrameMove");
	/**
	* <<fElapsedEnvSimTime>>
	* in worst case, it might be half of 60.f, that is only 1/30secs.
	* Animate the environment, remote script and network module
	* it must be called if IO is executed. Since IO may change the cognitive state of some characters
	* The environment simulator will provide feedback (constraint) on those subjective state.
	* e.g. An IO event might send a character to a moving state(mentally), however, if the character is blocked,
	* this cognitive state must be prohibited, before rendering function is called. Such constraints
	* are feedback by the physical environment
	*/
	double fElapsedEnvSimTime = CGlobals::GetFrameRateController(FRC_SIM)->FrameMove(fTime);

	if (bIOExecuted || (fElapsedEnvSimTime > 0))
	{
		/*static int i=0;
		OUTPUT_LOG("%d: %f, %f\n", ++i, fTime, fElapsedEnvSimTime);*/
		/// -- Call AI script and remote scripts as well as some simulation in the form of
		/// triggering scripts. It is important for this script to come before m_pEnvironmentSim->Animate()
		/// because the latter may add objects which may be deleted to the visiting biped list.
		/// Hence, when m_pEnvironmentSim->Animate() is called, the scene can no longer drop objects.
		PERF_BEGIN("Script&Net FrameMove");
		CAISimulator::GetSingleton()->FrameMove((float)fElapsedEnvSimTime);
		PERF_END("Script&Net FrameMove");

		/// -- Animate the scene --:
		/// -- Environment simulation is carried out by m_pEnvironmentSim
		/// always call Environment simulation  before you frame move any other object in the scene
		/// since Environment simulation may change the object's cached actions.
		PERF_BEGIN("EnvironmentSim");
		CGlobals::GetEnvSim()->Animate((float)fElapsedEnvSimTime);  // generate valid LLE from HLE
		PERF_END("EnvironmentSim");

#ifdef USE_OPENAL_AUDIO_ENGINE
		CAudioEngine2::GetInstance()->Update();
#endif

		{
			// animate g_flash value for some beeper effect, such as object selection.
			static float s_flash = 0.f;
			s_flash += (float)fElapsedEnvSimTime * 2;
			if (s_flash > 2)
				s_flash = 0;
			if (s_flash > 1)
				g_flash = 2 - s_flash;
			else
				g_flash = s_flash;
		}
	}

	/**
	* <<fElapsedIOTime>>
	* in worst case, it might be half of 60.f, that is only 1/30secs.
	* handle mouse and key board I/O events
	* Faster than the user input <= 1/30 sec
	*/
	double fElapsedIOTime = CGlobals::GetFrameRateController(FRC_IO)->FrameMove(fTime);
	if (fElapsedIOTime > 0.f)
	{
		bIOExecuted = true;

		//PERF_BEGIN("IC");
		////call the information center's frame move method
		//CICRoot *m_icroot=CICRoot::Instance();
		//m_icroot->FrameMove();
		//PERF_END("IC");

		if (m_bActive)
		{
			/**
			* process all user key and mouse messages
			*/
			HandleUserInput(); // user input.

							   // we update the mouse position after dispatch to ensure the correct begin position for next FrameMove;
							   /**
							   * Engine required: Camera control
							   * some object in the scene requires to update its parameters each frame
							   * currently only Camera control responses.
							   */
			m_pRootScene->Animate((float)fElapsedIOTime);
		}
	}
#ifdef USE_XACT_AUDIO_ENGINE
	/** for audio engine */
	if (m_pAudioEngine && m_pAudioEngine->IsAudioEngineEnabled())
	{
		if (m_pAudioEngine->IsValid())
		{
			PERF1("Audio Engine Framemove");
			m_pAudioEngine->DoWork();
		}
	}
#endif

	//// drop render frame rate to 2 FPS when in web browser mode and losing focus, this will save us lots of CPU cycles when user is not playing the game.
	//if((GetCoreUsage() & PE_USAGE_WEB_BROWSER)!=0)
	//{
	//	if(AppHasFocus())
	//	{
	//		Enable3DRendering(true);
	//		// OUTPUT_LOG("Time: %f, Focus is on\n", (float)fTime);
	//	}
	//	else
	//	{
	//		static CFrameRateController frcWebBrowser(CFrameRateController::FRC_BELOW);
	//		// 1 FPS per seconds when in web browser mode.Height
	//		frcWebBrowser.SetConstDeltaTime(1/1.f);
	//		bool bEnable = (frcWebBrowser.FrameMove(fTime) > 0);
	//		// OUTPUT_LOG("Time: %f, %s\n", (float)fTime, bEnable ? "true":"false");
	//		Enable3DRendering(bEnable);
	//	}
	//}

	/**
	* Render() will be called after this function
	* with m_fElapsedTime as its time advances
	*/
	PERF_END("Main FrameMove");

	UpdateScreenDevice();

	OnFrameEnded();
	return S_OK;
}

bool CWindowsApplication::AppHasFocus()
{
	return m_bAppHasFocus;
}

bool CWindowsApplication::UpdateScreenDevice()
{

	return true;
}





//-----------------------------------------------------------------------------
// Name: Render()
/// Called once per frame, the call is the entry point for 3d
///       rendering. This function sets up render states, clears the
///       viewport, and renders the scene.
/// ParaEngine fixed code: must call these functions as given below
//-----------------------------------------------------------------------------
HRESULT CWindowsApplication::Render()
{
	if (!m_bActive || m_bMinimized || GetAppState() != PEAppState_Ready)
		return E_FAIL;
	double fTime = m_fTime;
	UpdateFrameStats(m_fTime);

	if (CGlobals::WillGenReport())
	{
		UpdateStats();
	}
	if (m_bServerMode)
		return E_FAIL;

	CMoviePlatform* pMoviePlatform = CGlobals::GetMoviePlatform();
	pMoviePlatform->BeginCaptureFrame();

	float fElapsedTime = (float)(CGlobals::GetFrameRateController(FRC_RENDER)->FrameMove(fTime));

	IRenderDevice* pRenderDevice = CGlobals::GetRenderDevice();
	PERF1("Main Render");

	if (SUCCEEDED(pRenderDevice->BeginScene()))
	{
		CGlobals::GetAssetManager()->RenderFrameMove(fElapsedTime); // for asset manager
																	// since we use EnableAutoDepthStencil, The device will create a depth-stencil buffer when it is created. The depth-stencil buffer will be automatically set as the render target of the device.
																	// When the device is reset, the depth-stencil buffer will be automatically destroyed and recreated in the new size.
																	// However, we must SetRenderTarget to the back buffer in each frame in order for  EnableAutoDepthStencil work properly for the backbuffer as well.
		pRenderDevice->SetRenderTarget(0, CGlobals::GetDirectXEngine().GetRenderTarget(0)); // force setting render target to back buffer. and

																						 /// clear all render targets
		pRenderDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, m_pRootScene->GetClearColor(), 1.0f, 0L);

		/// force using less equal
		pRenderDevice->SetRenderState(ERenderState::ZFUNC, D3DCMP_LESSEQUAL);

		m_pViewportManager->UpdateViewport(m_pRenderWindow->GetWidth(), m_pRenderWindow->GetHeight());
		{
			PERF1("3D Scene Render");
			m_pViewportManager->Render(fElapsedTime, PIPELINE_3D_SCENE);
		}
		{
			PERF1("GUI Render");
			m_pViewportManager->Render(fElapsedTime, PIPELINE_UI);
		}
		{
			m_pViewportManager->Render(fElapsedTime, PIPELINE_POST_UI_3D_SCENE);
		}
#ifdef USE_FLASH_MANAGER
		//////////////////////////////////////////////////////////////////////////
		// render flash windows under full screen mode.
		if (!IsWindowedMode())
			CGlobals::GetAssetManager()->GetFlashManager().RenderFlashWindows(*(m_pRootScene->GetSceneState()));
#endif

		pRenderDevice->EndScene();
	}

	pMoviePlatform->EndCaptureFrame();
	return S_OK;
}

bool CWindowsApplication::IsDebugBuild()
{
#ifdef _DEBUG
	return true;
#else
	return false;
#endif
}


void CWindowsApplication::GetStats(string& output, DWORD dwFields)
{
	if (dwFields == 0)
	{
		UpdateStats();
		output = m_strDeviceStats;
		output.append("|");
		output.append(m_strFrameStats);
	}
	else
	{
		if (dwFields == 1)
		{
			TCHAR szOS[512];
			if (COSInfo::GetOSDisplayString(szOS))
			{
				output = szOS;
			}
		}
	}
}

void CWindowsApplication::HandleUserInput()
{
	/** handle 2D GUI input: dispatch mouse and key event for gui objects. */
	m_pGUIRoot->HandleUserInput();

	// escape input if app does not have focus
	if (!IsAppActive())
		return;

	/** handle the camera user input. One can also block camera input and handle everything from script. */
	CAutoCamera* pCamera = ((CAutoCamera*)(CGlobals::GetScene()->GetCurrentCamera()));
	if (pCamera)
		pCamera->HandleUserInput();

	/** handle 3D scene input */
	CGlobals::GetScene()->HandleUserInput();
}

void CWindowsApplication::GetScreenResolution(Vector2* pOut)
{
	if (pOut)
		*pOut = Vector2((float)(m_dwCreationWidth), (float)(m_dwCreationHeight));
}

void CWindowsApplication::SetScreenResolution(const Vector2& vSize)
{
	//m_d3dSettings.Fullscreen_DisplayMode.Width = (int)(vSize.x);
	//m_d3dSettings.Fullscreen_DisplayMode.Height = (int)(vSize.y);
	m_dwCreationWidth = (int)(vSize.x);
	m_dwCreationHeight = (int)(vSize.y);
	ParaEngineSettings& settings = ParaEngineSettings::GetSingleton();
	CVariable value;
	value = (int)(vSize.x);
	settings.SetDynamicField("ScreenWidth", value);
	value = (int)(vSize.y);
	settings.SetDynamicField("ScreenHeight", value);
}

void CWindowsApplication::GetResolution(float* pX, float* pY)
{
	Vector2 vSize;
	GetScreenResolution(&vSize);

	if (pX)
	{
		*pX = vSize.x;
	}
	if (pY)
	{
		*pY = vSize.y;
	}
}
void CWindowsApplication::SetResolution(float x, float y)
{
	SetScreenResolution(Vector2(x, y));
}

int CWindowsApplication::GetMultiSampleType()
{
	return 0;
}

void CWindowsApplication::SetMultiSampleType(int nType)
{

}

int CWindowsApplication::GetMultiSampleQuality()
{
	return 0;
}

void CWindowsApplication::SetMultiSampleQuality(int nType)
{

}

bool CWindowsApplication::UpdateScreenMode()
{
	m_bUpdateScreenDevice = true;
	return true;
}

bool CWindowsApplication::SetWindowedMode(bool bWindowed)
{
	if (IsWindowedMode() == bWindowed)
		return true;
	m_nWindowedDesired = bWindowed ? 1 : 0;

	return UpdateScreenMode();
}

bool CWindowsApplication::IsWindowedMode()
{
	return m_bWindowed;
}

void CWindowsApplication::SetFullScreenMode(bool bFullscreen)
{
	m_nWindowedDesired = bFullscreen ? 0 : 1;
}

bool CWindowsApplication::IsFullScreenMode()
{
	return !m_bWindowed;
}




void CWindowsApplication::SetIgnoreWindowSizeChange(bool bIgnoreSizeChange)
{
	m_bIgnoreSizeChange = bIgnoreSizeChange;
}

bool CWindowsApplication::GetIgnoreWindowSizeChange()
{
	return m_bIgnoreSizeChange;
}

void CWindowsApplication::SetWindowText(const char* pChar)
{
	static std::wstring g_sTitle;
	g_sTitle = ParaEngine::StringHelper::MultiByteToWideChar(pChar, CP_UTF8);
	::SetWindowTextW(CGlobals::GetAppHWND(), g_sTitle.c_str());
}

const char* CWindowsApplication::GetWindowText()
{
	static WCHAR g_wstr_title[256];
	static std::string g_title;
	::GetWindowTextW(CGlobals::GetAppHWND(), g_wstr_title, 255);
	g_title = ParaEngine::StringHelper::WideCharToMultiByte(g_wstr_title, CP_UTF8);
	return g_title.c_str();
}

void CWindowsApplication::WriteConfigFile(const char* FileName)
{
	string sFileName;
	if (FileName == NULL || FileName[0] == '\0')
		sFileName = "config/config.txt";
	else
		sFileName = FileName;

	{
		// remove the read-only file attribute
		DWORD dwAttrs = ::GetFileAttributes(sFileName.c_str());
		if (dwAttrs != INVALID_FILE_ATTRIBUTES)
		{
			if ((dwAttrs & FILE_ATTRIBUTE_READONLY))
			{
				::SetFileAttributes(sFileName.c_str(), dwAttrs & (~FILE_ATTRIBUTE_READONLY));
			}
		}
	}

	CParaFile file;
	if (!file.CreateNewFile(sFileName.c_str()))
	{
		OUTPUT_LOG("failed creating file %s\r\n", sFileName.c_str());
	}
	file.WriteFormated("-- ParaEngine startup settings.\n");
	file.WriteFormated("-- Auto generated by ParaEngine %s\n", ParaEngineInfo::CParaEngineInfo::GetVersion().c_str());

	file.WriteFormated("-- MultiSampleType: hardware fullscreen MultiSample AntiAliasing(AA): 0 to disable;2 to enable;4 is better\n");
	file.WriteFormated("-- MultiSampleQuality: 0 default quality, 1 for higher quality. the higher value, the more quality\n");
	file.WriteFormated("-- ScriptEditor: default is editor/notepad.exe, UltraEdit is another good choice if one has it.\n");
	file.WriteFormated("-- GameEffectSet: initial game effect set, 0 is recommended. possible value is [1024, 0,1,2,-1,-2]. 1024 means FF, otherwise the larger the better.\n");
	file.WriteFormated("-- language: possible values are enUS or zhCN\n");

	ParaEngineSettings& settings = ParaEngineSettings::GetSingleton();
	CVariable value;

	value = !(m_nWindowedDesired != 0); //  IsWindowedMode();
	settings.SetDynamicField("StartFullscreen", value);


	value = settings.GetScriptEditor();
	settings.SetDynamicField("ScriptEditor", value);

	value = settings.GetGameEffectSet();
	settings.SetDynamicField("GameEffectSet", value);

	value = settings.GetMouseInverse();
	settings.SetDynamicField("InverseMouse", value);

	value = settings.GetLocale();
	settings.SetDynamicField("language", value);

	value = settings.GetTextureLOD();
	settings.SetDynamicField("TextureLOD", value);

	string field_text;
	settings.SaveDynamicFieldsToString(field_text);
	file.WriteString(field_text);

	OUTPUT_LOG("ParaEngine config file is saved to %s\r\n", sFileName.c_str());
}

bool CWindowsApplication::HasNewConfig()
{
	return m_bHasNewConfig;
}

void CWindowsApplication::SetHasNewConfig(bool bHasNewConfig)
{
	m_bHasNewConfig = bHasNewConfig;
}

void CWindowsApplication::GetCursorPosition(int* pX, int * pY, bool bInBackbuffer /*= true*/)
{
	if (IsTouchInputting())
	{
		// tricky, since the touch input may change the virtual mouse position when it is near a click-able UI object.
		CGUIRoot::GetInstance()->GetMouse()->GetDeviceCursorPos(*pX, *pY);
	}
	else
	{
		POINT ptCursor;
		::GetCursorPos(&ptCursor);
		::ScreenToClient(CGlobals::GetAppHWND(), &ptCursor);

		int x = (int)(ptCursor.x);
		int y = (int)(ptCursor.y);
		ClientToGame(x, y, bInBackbuffer);

		*pX = x;
		*pY = y;
	}
}

void CWindowsApplication::GameToClient(int& inout_x, int & inout_y, bool bInBackbuffer)
{
	if (bInBackbuffer && IsWindowedMode())
	{
		// we need to scale cursor position according to backbuffer.
		RECT rcWindowClient;
		::GetClientRect(CGlobals::GetAppHWND(), &rcWindowClient);
		int width = (rcWindowClient.right - rcWindowClient.left);
		int height = (rcWindowClient.bottom - rcWindowClient.top);

		if ((width != m_pRenderWindow->GetWidth()) || (height != m_pRenderWindow->GetHeight()))
		{
			inout_x = (int)((float)width*(float)inout_x / m_pRenderWindow->GetWidth());
			inout_y = (int)((float)height*(float)inout_y / m_pRenderWindow->GetHeight());
		}
	}
}

void CWindowsApplication::ClientToGame(int& inout_x, int & inout_y, bool bInBackbuffer)
{
	if (bInBackbuffer && IsWindowedMode())
	{
		// we need to scale cursor position according to backbuffer.
		RECT rcWindowClient;
		::GetClientRect(CGlobals::GetAppHWND(), &rcWindowClient);
		int width = (rcWindowClient.right - rcWindowClient.left);
		int height = (rcWindowClient.bottom - rcWindowClient.top);

		if ((width != m_pRenderWindow->GetWidth()) || (height != m_pRenderWindow->GetHeight()))
		{
			inout_x = (int)(m_pRenderWindow->GetWidth() * (float)inout_x / (float)width);
			inout_y = (int)(m_pRenderWindow->GetHeight() * (float)inout_y / (float)height);
		}
	}
}

/* Using SetForegroundWindow on Windows Owned by Other Processes
In modern versions of Windows (XP, Vista, and beyond), the API call SetForegroundWindow() will bring
the specified window to the foreground only if it's owned by the calling thread.
The following code removes this limitation and provides a workaround:
*/
void NewSetForegroundWindow(HWND hWnd)
{
	if (GetForegroundWindow() != hWnd) {

		DWORD dwMyThreadID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
		DWORD dwOtherThreadID = GetWindowThreadProcessId(hWnd, NULL);
		if (dwMyThreadID != dwOtherThreadID)
		{
			AttachThreadInput(dwMyThreadID, dwOtherThreadID, TRUE);
			SetForegroundWindow(hWnd);
			AttachThreadInput(dwMyThreadID, dwOtherThreadID, FALSE);
		}
		else
			SetForegroundWindow(hWnd);

		/*if (IsIconic(hWnd))
		ShowWindow(hWnd, SW_RESTORE);
		else
		ShowWindow(hWnd, SW_SHOW);*/
	}
}
void CWindowsApplication::BringWindowToTop()
{
	if (!IsFullScreenMode())
	{
		if ((GetCoreUsage() & PE_USAGE_WEB_BROWSER) != 0)
		{
			NewSetForegroundWindow(m_hwndTopLevelWnd);
			// OUTPUT_LOG("BringWindowToTop: setting foreground HWND from %x, to %x\n", GetForegroundWindow(), m_hwndTopLevelWnd);
		}
		else
		{
			// only bring to front if it is not from a web browser
			::SetForegroundWindow(CGlobals::GetAppHWND());
			// ::SetWindowPos(CGlobals::GetAppHWND(),HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
			// this does not work
			// ::BringWindowToTop(CGlobals::GetAppHWND());
		}
	}
}

HKEY GetHKeyByName(const string& root_key)
{
	if (root_key == "HKCR" || root_key == "HKEY_CLASSES_ROOT")
		return HKEY_CLASSES_ROOT;
	else if (root_key == "HKLM" || root_key == "HKEY_LOCAL_MACHINE")
		return HKEY_LOCAL_MACHINE;
	else if (root_key == "HKCU" || root_key == "HKEY_CURRENT_USER")
		return HKEY_CURRENT_USER;
	else if (root_key == "HKU" || root_key == "HKEY_USERS")
		return HKEY_USERS;
	else
		return HKEY_CURRENT_USER;
}

/** get hkey by path
* one needs to close the key if it is not zero.
*/
HKEY GetHKeyByPath(const string& root_key, const string& sSubKey, DWORD dwOpenRights = KEY_QUERY_VALUE, bool bCreateGet = false)
{
	HKEY  hKey = NULL;
	HKEY  hParentKey = NULL;
	LPBYTE lpValue = NULL;
	LONG lRet = NULL;

	std::string path_;
	std::string::size_type nFrom = 0;
	for (int i = 0; i < 20 && nFrom != std::string::npos; ++i)
	{
		std::string::size_type nLastFrom = (nFrom == 0) ? 0 : (nFrom + 1);
		nFrom = sSubKey.find_first_of("/\\", nLastFrom);
		path_ = sSubKey.substr(nLastFrom, (nFrom == std::string::npos) ? nFrom : (nFrom - nLastFrom));

		if (hParentKey == NULL)
		{
			hParentKey = GetHKeyByName(root_key);
		}
		if (!bCreateGet)
		{
			lRet = ::RegOpenKeyEx(hParentKey,
				path_.c_str(),
				0,
				dwOpenRights,
				&hKey);
		}
		else
		{
			DWORD dwDisposition = 0;
			lRet = ::RegCreateKeyEx(hParentKey,
				path_.c_str(),
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				dwOpenRights,
				NULL,
				&hKey, &dwDisposition);
			if (dwDisposition == REG_CREATED_NEW_KEY)
			{
				OUTPUT_LOG("created the registry key %s \n", sSubKey.c_str());
			}
		}


		if (nFrom != std::string::npos && i > 0)
		{
			::RegCloseKey(hParentKey);
		}
		hParentKey = hKey;

		if (ERROR_SUCCESS != lRet)
		{
			// Error handling (see this FAQ)
			if (ERROR_ACCESS_DENIED == lRet)
			{
				OUTPUT_LOG("can not open the registry key %s because %s access denied.\n", sSubKey.c_str(), path_.c_str());
			}
			else
			{
				OUTPUT_LOG("can not open the registry key %s because %s does not exist.\n", sSubKey.c_str(), path_.c_str());
			}
			return NULL;
		}
	}
	return hKey;
}

bool CWindowsApplication::WriteRegStr(const string& root_key, const string& sSubKey, const string& name, const string& value)
{
	LPBYTE lpValue = NULL;
	LONG lRet = NULL;

	HKEY  hKey = GetHKeyByPath(root_key, sSubKey, KEY_WRITE, true);
	if (hKey == NULL)
		return NULL;

	lRet = ::RegSetValueEx(hKey,
		name.c_str(),
		0,
		REG_SZ,
		(const byte*)(value.c_str()),
		(int)(value.size()));
	::RegCloseKey(hKey);
	if (ERROR_SUCCESS != lRet)
	{
		// Error handling
		OUTPUT_LOG("can not set the registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
		return false;
	}
	return true;
}

const char* CWindowsApplication::ReadRegStr(const string& root_key, const string& sSubKey, const string& name)
{
	LPBYTE lpValue = NULL;
	LONG lRet = NULL;
	DWORD dwSize = 0;
	DWORD dwDataType = 0;

	static string g_tmp;
	g_tmp.clear();

	try
	{
		HKEY  hKey = GetHKeyByPath(root_key, sSubKey);
		if (hKey == NULL)
			return NULL;

		// Call once RegQueryValueEx to retrieve the necessary buffer size
		::RegQueryValueEx(hKey,
			name.c_str(),
			0,
			&dwDataType,
			lpValue,  // NULL
			&dwSize); // will contain the data size

		if (ERROR_SUCCESS == lRet && (dwSize > 0 || dwDataType == REG_DWORD))
		{
			// Alloc the buffer
			lpValue = (LPBYTE)malloc(dwSize);

			// Call twice RegQueryValueEx to get the value
			lRet = ::RegQueryValueEx(hKey,
				name.c_str(),
				0,
				&dwDataType,
				lpValue,
				&dwSize);
		}

		::RegCloseKey(hKey);

		if (ERROR_SUCCESS != lRet)
		{
			// Error handling
			OUTPUT_LOG("can not query the registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
			return NULL;
		}

		if (dwDataType == REG_SZ)
		{
			if (lpValue != NULL)
			{
				g_tmp = (const char*)lpValue;
			}
		}
		else if (dwDataType == REG_DWORD)
		{
			if (lpValue != NULL)
			{
				DWORD dwValue = *((const DWORD *)lpValue);
				char temp[30];
				memset(temp, 0, sizeof(temp));
				_itoa_s(dwValue, temp, 10);
				g_tmp = temp;
			}
		}

		// free the buffer when no more necessary
		if (lpValue != NULL)
			free(lpValue);

		return g_tmp.c_str();
	}
	catch (...)
	{
		OUTPUT_LOG("error: Exception: can not query the registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
	}
	return g_tmp.c_str();
}

bool CWindowsApplication::WriteRegDWORD(const string& root_key, const string& sSubKey, const string& name, DWORD value)
{
	LONG lRet = NULL;

	try
	{
		HKEY  hKey = GetHKeyByPath(root_key, sSubKey, KEY_SET_VALUE);
		if (hKey == NULL)
			return NULL;

		lRet = ::RegSetValueEx(hKey,
			name.c_str(),
			0,
			REG_DWORD,
			((const byte*)(&value)),
			sizeof(DWORD));
		::RegCloseKey(hKey);
		if (ERROR_SUCCESS != lRet)
		{
			// Error handling
			OUTPUT_LOG("can not set the registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
			return false;
		}
	}
	catch (...)
	{
		OUTPUT_LOG("error: Exception when WriteRegDWORD registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
		return false;
	}
	return true;
}

DWORD CWindowsApplication::ReadRegDWORD(const string& root_key, const string& sSubKey, const string& name)
{
	LPBYTE lpValue = NULL;
	LONG lRet = NULL;
	DWORD dwSize = 0;
	DWORD dwDataType = 0;

	try
	{
		HKEY  hKey = GetHKeyByPath(root_key, sSubKey);
		if (hKey == NULL)
			return NULL;

		// Call once RegQueryValueEx to retrieve the necessary buffer size
		::RegQueryValueEx(hKey,
			name.c_str(),
			0,
			&dwDataType,
			lpValue,  // NULL
			&dwSize); // will contain the data size

					  // Alloc the buffer
		lpValue = (LPBYTE)malloc(dwSize);

		// Call twice RegQueryValueEx to get the value
		lRet = ::RegQueryValueEx(hKey,
			name.c_str(),
			0,
			&dwDataType,
			lpValue,
			&dwSize);
		::RegCloseKey(hKey);
		if (ERROR_SUCCESS != lRet)
		{
			// Error handling
			OUTPUT_LOG("can not query the registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
			return NULL;
		}

		DWORD dwValue = 0;
		if (dwDataType == REG_DWORD)
		{
			dwValue = *((const DWORD *)lpValue);
		}
		// free the buffer when no more necessary
		free(lpValue);
		return dwValue;
	}
	catch (...)
	{
		OUTPUT_LOG("error: Exception: can not query the registry key %s with name %s\n", sSubKey.c_str(), name.c_str());
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Name: DialogProcHelper
// Desc:
//-----------------------------------------------------------------------------
INT_PTR CALLBACK ParaEngine::DialogProcAbout(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		HWND hWndText = ::GetDlgItem(hDlg, IDC_ABOUT_TEXT);
		CParaFile file("readme.txt");
		if (!file.isEof())
		{
			::SetWindowText(hWndText, file.getBuffer());
		}
		else
		{
			::SetWindowText(hWndText, "");
		}
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			break;
		default:
			break;
		}
		return TRUE;

	default:
		return FALSE;
	}
}


float CWindowsApplication::GetRefreshTimer()
{
	return m_fRefreshTimerInterval;
}

void CWindowsApplication::GetWindowCreationSize(int * pWidth, int * pHeight)
{
	if (pWidth)
		*pWidth = m_dwCreationWidth;
	if (pHeight)
		*pHeight = m_dwCreationHeight;
}

void CWindowsApplication::ActivateApp(bool bActivate)
{
	m_bAppHasFocus = bActivate;
	//OUTPUT_LOG("WM_ACTIVATEAPP:%s\n", m_bAppHasFocus? "true":"false");

	// we shall prevent activate to be called multiple times.
	if (CGlobals::GetGUI()->IsActive() == bActivate)
		return;

	if (m_bToggleSoundWhenNotFocused)
	{
		CAudioEngine2::GetInstance()->OnSwitch(bActivate);
	}
	// Pause(!bActivate);
	if (bActivate)
		CGlobals::GetGUI()->ActivateRoot();
	else
		CGlobals::GetGUI()->InactivateRoot();

	if (m_bAutoLowerFrameRateWhenNotFocused)
	{
		float fIdealInterval = (GetRefreshTimer() <= 0) ? IDEAL_FRAME_RATE : GetRefreshTimer();
		static float s_fLastRefreshRate = fIdealInterval;

		if (!bActivate)
		{
			// set to a lower frame rate when app is switched away.
			const float fLowTimer = 1 / 20.f;
			if (fIdealInterval < fLowTimer)
			{
				s_fLastRefreshRate = fIdealInterval;
				SetRefreshTimer(fLowTimer);
			}
		}
		else
		{
			// restore to original frame rate.
			if (s_fLastRefreshRate > 0.f && s_fLastRefreshRate < fIdealInterval)
			{
				SetRefreshTimer(s_fLastRefreshRate);
			}
		}
	}
}

bool CWindowsApplication::IsAppActive()
{
	/*CGUIRoot * pRoot = CGlobals::GetGUI();
	return  pRoot!=0 && pRoot->IsActive();*/
	return m_bAppHasFocus;
}

DWORD CWindowsApplication::GetCoreUsage()
{
	return m_dwCoreUsage;
}

void CWindowsApplication::SetCoreUsage(DWORD dwUsage)
{
	m_dwCoreUsage = (m_dwCoreUsage & 0xfffffff0) | dwUsage;
}

void CWindowsApplication::SetMinUIResolution(int nWidth, int nHeight, bool bAutoUIScaling /*= true*/)
{
	CGlobals::GetGUI()->SetMinimumScreenSize(nWidth, nHeight, bAutoUIScaling);
}

bool CWindowsApplication::IsSlateMode()
{
	// slate mode
	// laptop mode / non-tablet mode
	m_isSlateMode = (GetSystemMetrics(SM_CONVERTIBLESLATEMODE) == 0);
	return m_isSlateMode;
}

// obsoleted: since the parent window is not in the same thread,  GetFocus() will always return NULL even a child window is having the focus.
bool CWindowsApplication::HasFocus(HWND hWnd)
{
	if (hWnd == 0)
		hWnd = ::GetFocus();

	// try parent and parent's parent
	for (int i = 0; i < 2 && hWnd != NULL; i++)
	{
		if (hWnd != GetMainWindow())
		{
			hWnd = GetParent(hWnd);
		}
		else
			return true;
	}
	return false;
}

void CWindowsApplication::SetAutoLowerFrameRateWhenNotFocused(bool bEnabled)
{
	m_bAutoLowerFrameRateWhenNotFocused = bEnabled;
}

bool CWindowsApplication::GetAutoLowerFrameRateWhenNotFocused()
{
	return m_bAutoLowerFrameRateWhenNotFocused;
}

void CWindowsApplication::SetToggleSoundWhenNotFocused(bool bEnabled)
{
	m_bToggleSoundWhenNotFocused = bEnabled;
}

bool CWindowsApplication::GetToggleSoundWhenNotFocused()
{
	return m_bToggleSoundWhenNotFocused;
}

LRESULT CWindowsApplication::SendMessageToApp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_pWinRawMsgQueue)
	{
		CWinRawMsg_ptr msg(new CWinRawMsg(hWnd, uMsg, wParam, lParam));
		m_pWinRawMsgQueue->push(msg);
	}
	return 0;
}

bool CWindowsApplication::GetMessageFromApp(CWinRawMsg* pMsg)
{
	if (m_pWinRawMsgQueue)
	{
		CWinRawMsg_ptr msg;
		if (m_pWinRawMsgQueue->try_pop(msg))
		{
			if (pMsg)
			{
				(*pMsg) = *msg;
			}
			return true;
		}
	}
	return false;
}

bool CWindowsApplication::PostWinThreadMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	if (uMsg >= PE_WM_FIRST && uMsg <= PE_WM_LAST)
	{
		result = 1;
		switch (uMsg)
		{
		case PE_TIMERID_HEARTBEAT:
		{
			break;
		}
		case PE_WM_SHOWCURSOR:
		{
			// this will cause warning of DirectX, when setting debug level to middle, because it is calling a d3d function
			// in another thread. However, this warning can be ignored, since otherwise d3d ShowCursor will not be shown if not calling from win thread.
			if (CGlobals::GetRenderDevice())
			{
				// ::SetCursor( NULL );
				CGlobals::GetRenderDevice()->ShowCursor(wParam == 1);
				// OUTPUT_LOG1("PE_WM_SHOWCURSOR\n");
			}
			break;
		}
		case PE_WM_SETCAPTURE:
		{
			::SetCapture(CGlobals::GetAppHWND());
			break;
		}
		case PE_WM_RELEASECAPTURE:
		{
			::ReleaseCapture();
			break;
		}
		case PE_WM_SETFOCUS:
		{
			::SetFocus((HWND)wParam);
			break;
		}
		case PE_WM_QUIT:
		{
			HMENU hMenu;
			hMenu = GetMenu(m_hWnd);
			if (hMenu != NULL)
				DestroyMenu(hMenu);
			DestroyWindow(m_hWnd);
			PostQuitMessage((int)wParam);
			break;
		}
		case PE_IME_SETOPENSTATUS:
		{
			CGUIIME::SetIMEOpenStatus_imp(lParam != 0);
			break;
		}
		case PE_IME_SETFOCUS:
		{
			if (lParam == 1) {
				CGUIIME::OnFocusIn_imp();
			}
			else {
				CGUIIME::OnFocusOut_imp();
			}
			break;
		}
		case PE_APP_SWITCH:
		{
			// on app switch in/out
			if (lParam == 1) {
				// inject WM_ACTIVATEAPP to simulate activate APP
				MsgProcWinThread(GetMainWindow(), WM_ACTIVATEAPP, (WPARAM)TRUE, 0, false);
			}
			else {
				// inject WM_ACTIVATEAPP to simulate activate APP
				MsgProcWinThread(GetMainWindow(), WM_ACTIVATEAPP, (WPARAM)FALSE, 0, false);
			}

			break;
		}
		default:
			result = 0;
			break;
		}
	}
	return true;
}


// return 1 if we do not want default window procedure or other message handler to process the message.
LRESULT CWindowsApplication::MsgProcWinThread(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bCallDefProcedure)
{


	LRESULT result = 0;

	bool bContinue = true;

	if (uMsg <= WM_MOUSELAST && uMsg >= WM_MOUSEFIRST)
	{
		bContinue = false;
		SendMessageToApp(hWnd, uMsg, wParam, lParam);
		if (uMsg == WM_LBUTTONUP)
			bContinue = true;

		if (uMsg == WM_LBUTTONDOWN) {
			// 2014.5.14 andy: check input source using message info along with WM_LBUTTONDOWN instead of WM_POINTERCAPTURECHANGED
			//
			//GetMessageExtraInfo() returns the extra info associated with a message
			//	Mouse up and down messages are tagged with a special signature indicating they came from touch or pen :
			//Mask extra info against 0xFFFFFF80
			//	0xFF515780 for touch, 0xFF515700 for pen

#define IsTouchEvent(dw) (((dw) & 0xFFFFFF80) == 0xFF515780)
			SetTouchInputting(IsTouchEvent(GetMessageExtraInfo()));
			// OUTPUT_LOG("WM_LBUTTONDOWN: %d \n", GetMessageExtraInfo());
		}
	}
	if (bContinue)
	{
		switch (uMsg)
		{
			//--------------------------------------------------------------------------------------------
			//process touch message
			/*Touch input messages on windows 8 have following order:
			WM_POINTERENTER
			WM_POINTERDOWN
			WM_POINTERUPDATE
			.... (1-many WM_POINTERUPDATE which may further translated to other message e.g pan,rotate..)
			WM_POINTERUPDATE
			WM_POINTERUP
			WM_POINTERLEAVE

			DO NOT direct process WM_POINTXX message unless you need custom gestures event or multi-touch input.
			Window will translate most touch input to traditional windows messages and standard gestures message,
			however,there might be some delay, e.g you will receive mouse down/up message *after* WM_POINTERLEAVE:
			WM_POINTERENTER
			WM_POINTERDOWN
			WM_POINTERUPDATE
			....
			WM_POINTERUPDATE
			WM_POINTERUP
			WM_POINTERLEAVE
			WM_LBUTTONDOWN
			WM_LBUTTONUP

			Current input system(which based on DXInput) return wrong "mouse" position in touch mode,we add a quick fix here:
			use WM_POINTERENTER and WM_POINTERLEAVE to check if we're in touch mode and update "mouse position" when
			WM_POINTERDOWN and WM_POINTERUPDATE message come.Be ware touch input is very sensitive, sometimes you may get <10
			pixel offset even when you feel you don't move at all.       --clayman
			*/
		case WM_POINTERENTER:
		case WM_POINTERDOWN:
		case WM_POINTERUP:
		case WM_POINTERUPDATE:
		case WM_POINTERLEAVE:
		{
			m_touchPointX = GET_X_LPARAM(lParam);
			m_touchPointY = GET_Y_LPARAM(lParam);
			result = 0;
			SendMessageToApp(hWnd, uMsg, wParam, lParam);
			break;
		}
		case WM_SETTINGCHANGE:
		{
			// When either system metric SM_CONVERTIBLESLATEMODE or SM_SYSTEMDOCKED changes, a broadcast message is sent by the system by using WM_SETTINGCHANGE.
			// This method works when the system is running New Windows 8 UI mode.
			// https://software.intel.com/en-us/articles/detecting-slateclamshell-mode-screen-orientation-in-convertible-pc
			if (lParam)
			{
				if (wcscmp((const wchar_t*)lParam, L"ConvertibleSlateMode") == 0 || wcscmp((const wchar_t*)lParam, L"SystemDockMode") == 0)
				{
					//BOOL bSlateMode = (GetSystemMetrics(SM_CONVERTIBLESLATEMODE) == 0);
					//BOOL bDocked = (GetSystemMetrics(SM_SYSTEMDOCKED) != 0);
					SetTouchInputting(IsSlateMode());
					CEventsCenter::GetInstance()->PostEvent(PredefinedEvents::SettingChange, false);
					OUTPUT_LOG("slate mode settings changed\n");
				}
			}
			break;
		}
		case WM_DISPLAYCHANGE:
		{
			// In desktop environment, OS broadcasts WM_DISPLAYCHANGE message to the windows when it detects orientation changes.lParam��s low word is the width and high word is the height of the new orientation.

			break;
		}
		/*
		case 0x0119: //case WM_GESTURE:
		{
		if(!m_pTouchInput)
		LoadTouchInputPlug();

		if(m_pTouchInput)
		{
		PEGestureInfo gi;
		ZeroMemory(&gi, sizeof(PEGestureInfo));
		gi.cbSize = sizeof(PEGestureInfo);

		if(m_pTouchInput->DecodeGestureMessage(hWnd,uMsg,wParam,lParam,gi))
		{
		switch(gi.dwID)
		{
		case 4:  //GID_Pan
		{
		POINT p;
		p.x = gi.ptsLocation.x;
		p.y = gi.ptsLocation.y;

		ScreenToClient(hWnd,&p);
		SendMessageToApp(hWnd,WM_MOUSEMOVE,0,MAKELONG(p.x,p.y));
		result = 1;
		}
		break;
		default:
		result = 0;
		}
		}
		else
		result = 0;
		}
		}
		break;
		*/
		//--------------------------------------------------------------------------------------------

		case WM_CLOSE:
			if (!IsWindowClosingAllowed())
			{
				result = 1;
			}
			SetHasClosingRequest(true);
			SendMessageToApp(hWnd, uMsg, wParam, lParam);
			break;
		case WM_SETCURSOR:
			// do not display the windows cursors
			// ::SetCursor(NULL);
			// OUTPUT_LOG1("WM_SETCURSOR \n");
			SendMessageToApp(hWnd, uMsg, wParam, lParam);
			result = 1;
			break;

		case WM_IME_SETCONTEXT:

			if (CGUIIME::IsEnableImeSystem())
			{
				// We don't want anything to display, so we have to clear this
				lParam = 0;
				CGUIIME::HandleWinThreadMsg(uMsg, wParam, lParam);
			}
			else
			{
				lParam = ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUICANDIDATEWINDOW;
				// let windows draw the IME.
				// TODO: we need to set the candidate window position.
			}
			//OUTPUT_LOG("WM_IME_SETCONTEXT: fset:%d, %d\n", wParam, lParam);
			break;
		case WM_KEYUP:
		case WM_INPUTLANGCHANGE:
		case WM_IME_COMPOSITION:
		case WM_IME_STARTCOMPOSITION:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_NOTIFY:
		{
			if (CGUIIME::IsEnableImeSystem())
			{
				result = 1;
			}
			/*if(uMsg == WM_INPUTLANGCHANGE)
			OUTPUT_LOG("WM_INPUTLANGCHANGE");
			else if(uMsg == WM_IME_COMPOSITION)
			OUTPUT_LOG("WM_IME_COMPOSITION");
			else if(uMsg == WM_IME_STARTCOMPOSITION)
			OUTPUT_LOG("WM_IME_STARTCOMPOSITION");
			else if(uMsg == WM_IME_ENDCOMPOSITION)
			OUTPUT_LOG("WM_IME_ENDCOMPOSITION");
			else if(uMsg == WM_IME_NOTIFY)
			OUTPUT_LOG("WM_IME_NOTIFY");
			OUTPUT_LOG(": %d, %d\n", wParam, lParam);*/

			CGUIRoot* pRoot = CGlobals::GetGUI();
			if (pRoot != 0 && pRoot->HasIMEFocus())
			{
				CGUIIME::HandleWinThreadMsg(uMsg, wParam, lParam);
			}
			if (uMsg != WM_KEYUP)
				break;
		}
		// key strokes
		//case WM_KEYUP:
		case WM_KEYDOWN:
			// system
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_PAINT:
		case WM_ENTERSIZEMOVE:
		case WM_STYLECHANGED:
		case WM_SIZE:
		case WM_EXITSIZEMOVE:
		case WM_ENTERMENULOOP:
		case WM_EXITMENULOOP:
		case WM_ACTIVATEAPP:
		case WM_POWERBROADCAST:
			SendMessageToApp(hWnd, uMsg, wParam, lParam);
			break;
		case WM_NCHITTEST:
		{
			// Prevent the user from selecting the menu in full screen mode
			result = HTCLIENT;
			if (!m_bWindowed) {
				result = HTCLIENT;
			}
			else {
				result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}
			SendMessageToApp(hWnd, uMsg, result, 0);

			CGUIRoot* pRoot = CGlobals::GetGUI();
			if (pRoot != 0 && pRoot->IsNonClient())
				result = HTCAPTION; // this will allow dragging
			return result;
		}
		case WM_LBUTTONUP:
			// fall through to WM_MOUSEACTIVATE if this is the top level window, since WM_MOUSEACTIVATE is not called for top-level window, but always called on mouse click for child windows.
		case WM_MOUSEACTIVATE:
		{
			// This fixed a issue that in some buggy browser like(sougou), Foreground window can not be set back by clicking on the plugin window.
			BringWindowToTop();

			// this message is received whenever the user clicks on the window, even if there is child window.
			//
			// Note: since SetFocus is not called immediately during MouseActivate, the parent window in the browser process may
			// think that no child window is getting the focus, and therefore wrongly get focus, but at the same time,
			// the render process send PE_WM_SETFOCUS to set focus, so the two processes(threads) may call SetFocus in undetermined ordered.
			// if the render process calls first, then the final result of focus window is wrong.
			//
			// To fix above problem, we will handle mouse activate in the window process and SetFocus to the window.
			HWND hWndMain = GetMainWindow();
			{
				POINT ptCursor;
				::GetCursorPos(&ptCursor);
				::ScreenToClient(hWndMain, &ptCursor);

				HWND hMouseOverWnd = ChildWindowFromPointEx(hWndMain, ptCursor, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
				if (hMouseOverWnd == hWndMain)
				{
					::SetFocus(hWndMain);
					// result = MA_NOACTIVATE;
				}
			}
			// very tricky here: we will simulate activate app message when clicked.
			SendMessageToApp(hWnd, WM_ACTIVATEAPP, TRUE, 0);
			break;
		}
		case WM_COMMAND:

			switch (LOWORD(wParam))
			{
			case IDM_EXIT:
				// Received key/menu command to exit app
				SendMessage(hWnd, WM_CLOSE, 0, 0);
				break;
			default:
				SendMessageToApp(hWnd, WM_COMMAND, wParam, lParam);
				break;
			}

			break;
		case WM_DROPFILES:
		{
			HDROP query = (HDROP)wParam;
			int n = 0, count = DragQueryFile(query, 0xFFFFFFFF, 0, 0);
			std::string cmds = "";
			while (n < count) {
				char filename[MAX_FILENAME_LENGTH];
				if (DragQueryFile(query, n, filename, MAX_FILENAME_LENGTH) != 0)
				{
					OUTPUT_LOG("drop files: %s\n", filename);
					cmds += filename;
					cmds += ";";
				}
				n++;
			}
			// TODO: make m_cmd thread safe by using a lock.
			m_cmd = cmds;
			SendMessageToApp(hWnd, WM_DROPFILES, NULL, (LPARAM)(LPSTR)(m_cmd.c_str()));
			DragFinish(query);
			break;
		}
		case WM_COPYDATA:
		{
			PCOPYDATASTRUCT pMyCDS = (PCOPYDATASTRUCT)lParam;
			switch (pMyCDS->dwData)
			{
			case ID_GAME_COMMANDLINE:

				if (pMyCDS->lpData)
				{
					// TODO: make m_cmd thread safe by using a lock.
					m_cmd = (const char*)(pMyCDS->lpData);
				}
				SendMessageToApp(hWnd, WM_COMMAND, ID_GAME_COMMANDLINE, (LPARAM)(LPSTR)(m_cmd.c_str()));
				break;
			}
			break;
		}

		case WM_SYSCOMMAND:

			// Prevent moving/sizing and power loss in fullscreen mode
			switch (wParam)
			{
			case SC_MOVE:
			case SC_SIZE:
			case SC_MAXIMIZE:
			case SC_KEYMENU:
			case SC_MONITORPOWER:
				SendMessageToApp(hWnd, uMsg, wParam, lParam);
				break;
			}
			/*
			When you release the Alt key, the system generates a WM_SYSCOMMAND/SC_KEYMENU message.
			Futhermore, unless you press a key to open a specific popup menu, the lparam will be 0.
			DefWindowProc, upon receiving this, will enter the menu loop. So, all you have to do is
			detect this message and prevent it from getting to DefWindowProc:
			*/
			if (wParam == SC_KEYMENU)
			{
				return 0;
			}


			break;

			/*case WM_IME_CHAR:
			{
			OUTPUT_LOG("WM_IME_CHAR:%d\n",wParam);
			}
			break;
			case WM_UNICHAR:
			{
			OUTPUT_LOG("WM_UNICHAR:%d\n",wParam);
			}
			break;
			*/
		case WM_CHAR:
		{
			//BOOL bIsUnicode = IsWindowUnicode(hWnd);
			CGUIIME::SendWinMsgChar((WCHAR)wParam);
			// OUTPUT_LOG("WM_CHAR:%d\n",wParam);
		}
		break;

		}
	}
	if (result == 0)
	{
		if (bCallDefProcedure)
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	return result;
}


const char* CWindowsApplication::GetTouchEventSCodeFromMessage(const char * event_type, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	// msg = command line.
	int32_t x = GET_X_LPARAM(lParam);
	int32_t y = GET_Y_LPARAM(lParam);
	int32_t id = GET_POINTERID_WPARAM(wParam);

	//OUTPUT_LOG("before ScreenToClient %d, %d \n", x, y);
	POINT p;
	p.x = x;
	p.y = y;
	ScreenToClient(hWnd, &p);
	x = p.x;
	y = p.y;
	//OUTPUT_LOG("after ScreenToClient %d, %d \n", x, y);

	//OUTPUT_LOG("before ClientToGame %d, %d \n", x, y);
	ClientToGame(x, y, true);
	//OUTPUT_LOG("after ClientToGame %d, %d \n", x, y);

	char tmp[150];
	snprintf(tmp, sizeof(tmp), "msg={type=\"%s\",x=%d,y=%d,id=%d};", event_type, x, y, id);

	static string g_msg = "";
	g_msg = tmp;

	return g_msg.c_str();
}

// return 0 if not processed, 1 if processed, 2 if no further messages in the queue should ever be processed.
LRESULT CWindowsApplication::MsgProcApp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool bIsSceneEnabled = !(m_pRootScene == NULL || !m_pRootScene->IsInitialized());
	if (bIsSceneEnabled)
	{
		// let the GUI system to process the message first.
		bool bNoFurtherProcess = false;
		LRESULT result = m_pGUIRoot->MsgProc(hWnd, uMsg, wParam, lParam, bNoFurtherProcess);
		if (bNoFurtherProcess) {
			return 1;
		}
	}

	switch (uMsg)
	{
	case WM_PAINT:
		// Handle paint messages when the app is paused
		// this may lead to problems, so do nothing with WM_PAINT, especially during initialization.
		/*if( bIsSceneEnabled && (!m_bDisableD3D) && m_pd3dDevice && !m_bActive &&
		m_bDeviceObjectsInited && m_bDeviceObjectsRestored )
		{
		if(Is3DRenderingEnabled())
		{
		HRESULT hr;
		Render();
		hr = m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
		if( D3DERR_DEVICELOST == hr )
		m_bDeviceLost = true;
		}
		}*/
		break;
	case WM_ENTERSIZEMOVE:
		// Halt frame movement while the app is sizing or moving
		if (bIsSceneEnabled)
		{
			//Pause(true);
		}
		break;
	case WM_SIZE:
	{
		if (bIsSceneEnabled)
		{
#ifdef USE_FLASH_MANAGER
			// Flash Window Size changes.
			CGlobals::GetAssetManager()->GetFlashManager().OnSizeChange();
#endif
			if (SIZE_MINIMIZED == wParam)
			{
				if (m_bClipCursorWhenFullscreen && !m_bWindowed)
					ClipCursor(NULL);
				//Pause( true ); // Pause while we're minimized
				m_bMinimized = true;
				m_bMaximized = false;
			}
			if (SIZE_MAXIMIZED == wParam)
			{
				//if( m_bMinimized )
				//    Pause( false ); // Unpause since we're no longer minimized
				m_bMinimized = false;
				m_bMaximized = true;
			}
			else if (SIZE_RESTORED == wParam)
			{
				if (m_bMaximized)
				{
					m_bMaximized = false;
				}
				else if (m_bMinimized)
				{
					//Pause( false ); // Unpause since we're no longer minimized
					m_bMinimized = false;
				}
			}
		}

		break;
	}
	case WM_ENTERMENULOOP:
		if (bIsSceneEnabled)
		{
			// Pause the app when menus are displayed
			//Pause(true);
		}
		break;
	case WM_EXITMENULOOP:
		if (bIsSceneEnabled)
		{
			//Pause(false);
		}
		break;
	case WM_ACTIVATEAPP:
	{
		bool  bActive = !(FALSE == wParam);
		m_bAppHasFocus = bActive;
		if (bIsSceneEnabled)
		{
			ActivateApp(bActive);
		}
		break;
	}
	case WM_MOUSEACTIVATE:
	{
		// Set focus if mouse is over the main window but not over any of its visible child window
		//HWND hWndMain = GetMainWindow();
		//{
		//	POINT ptCursor;
		//	::GetCursorPos(&ptCursor);
		//	::ScreenToClient(hWndMain,&ptCursor);

		//	HWND hMouseOverWnd = ChildWindowFromPointEx(hWndMain, ptCursor, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED);
		//	if(hMouseOverWnd == hWndMain)
		//	{
		//		// Note: since SetFocus is not called immediately during MouseActivate, the parent window in the browser process may set
		//		// think that no child window is getting the focus, and therefore wrongly get focus, but at the same time,
		//		// the render process send PE_WM_SETFOCUS to set focus, so the two processes(threads) may call SetFocus in undertermined ordered.
		//		// if the render process calls first, then the final result of focus window is wrong.
		//		CGlobals::GetApp()->PostWinThreadMessage(PE_WM_SETFOCUS, (WPARAM)(hWndMain), 0);
		//		ActivateApp(true);
		//	}
		//}
		break;
	}
	case WM_NCHITTEST:
		// Prevent the user from selecting the menu in full screen mode
		if (!m_bWindowed) {
			using namespace ParaEngine;
			CGUIRoot::GetInstance()->SetMouseInClient(true);
		}
		else
		{
			LRESULT result = wParam;
			using namespace ParaEngine;
			MouseEvent event(0, 0, 0, ((LRESULT)wParam) == HTCLIENT ? 1 : 0);
			CGUIRoot::GetInstance()->handleNonClientTest(event);
		}
		break;
	case WM_DROPFILES:
	{
		const char* sCmd = (const char*)lParam;
		if (sCmd)
		{
			if (CGlobals::GetEventsCenter())
			{
				// msg = command line.
				string msg = "msg=";
				NPL::NPLHelper::EncodeStringInQuotation(msg, (int)msg.size(), sCmd);
				msg.append(";");
				SystemEvent event(SystemEvent::SYS_WM_DROPFILES, msg);
				CGlobals::GetEventsCenter()->FireEvent(event);
			}
		}
		break;
	}
	case WM_POWERBROADCAST:
		switch (wParam)
		{
#ifndef PBT_APMQUERYSUSPEND
#define PBT_APMQUERYSUSPEND 0x0000
#endif
		case PBT_APMQUERYSUSPEND:
			// At this point, the app should save any data for open
			// network connections, files, etc., and prepare to go into
			// a suspended mode.
			return true;

#ifndef PBT_APMRESUMESUSPEND
#define PBT_APMRESUMESUSPEND 0x0007
#endif
		case PBT_APMRESUMESUSPEND:
			// At this point, the app should recover any data, network
			// connections, files, etc., and resume running from when
			// the app was suspended.
			return true;
		}
		break;
	case WM_COMMAND:
	{
		/// menu command
		switch (LOWORD(wParam))
		{
		case IDM_TOGGLESTART:
			// Toggle frame movement
			m_bFrameMoving = !m_bFrameMoving;
			DXUtil_Timer(m_bFrameMoving ? TIMER_START : TIMER_STOP);
			break;

		case IDM_SINGLESTEP:
			// Single-step frame movement
			if (false == m_bFrameMoving)
				DXUtil_Timer(TIMER_ADVANCE);
			else
				DXUtil_Timer(TIMER_STOP);
			m_bFrameMoving = false;
			m_bSingleStep = true;
			break;
			/// application defined command
		case ID_GAME_COMMANDLINE:
		{
			const char* sCmd = (const char*)lParam;
			if (sCmd)
			{
				if (CGlobals::GetEventsCenter())
				{
					// msg = command line.
					string msg = "msg=";
					NPL::NPLHelper::EncodeStringInQuotation(msg, (int)msg.size(), sCmd);
					msg.append(";");
					SystemEvent event(SystemEvent::SYS_COMMANDLINE, msg);
					CGlobals::GetEventsCenter()->FireEvent(event);
				}
			}
			break;
		}
		case ID_SCREENSHOT_BEGINRECORDINGVIDEO:
		{
			CGlobals::GetMoviePlatform()->BeginCapture("");
			break;
		}
		case ID_SCREENSHOT_STOPRECORDINGVIDEO:
		{
			CGlobals::GetMoviePlatform()->EndCapture();
			break;
		}
		case ID_SCREENSHOT_PAUSE_RESUME:
		{
			if (CGlobals::GetMoviePlatform()->IsRecording())
			{
				CGlobals::GetMoviePlatform()->PauseCapture();
			}
			else
			{
				CGlobals::GetMoviePlatform()->ResumeCapture();
			}
			break;
		}
		case ID_HELP_ABOUT:
		{
			return DialogBox(NULL, MAKEINTRESOURCE(IDD_ABOUT),
				hWnd, DialogProcAbout);
			break;
		}
		}
	}
	break;
	case PE_APP_SHOW_ERROR_MSG:
	{
		// show error message.
		break;
	}
	case WM_POINTERDOWN:
	case WM_POINTERUP:
	case WM_POINTERUPDATE:
	{
		int32_t x = GET_X_LPARAM(lParam);
		int32_t y = GET_Y_LPARAM(lParam);
		int32_t id = GET_POINTERID_WPARAM(wParam);
		POINT p; p.x = x; p.y = y;
		ScreenToClient(hWnd, &p);
		x = p.x; y = p.y;
		ClientToGame(x, y, true);
		int nEventType = -1;
		if (uMsg == WM_POINTERUPDATE)
			nEventType = TouchEvent::TouchEvent_POINTER_UPDATE;
		//else if (uMsg == WM_POINTERENTER)
		//	nEventType = TouchEvent::TouchEvent_POINTER_ENTER;
		//else if (uMsg == WM_POINTERLEAVE)
		//	nEventType = TouchEvent::TouchEvent_POINTER_UP;
		else if (uMsg == WM_POINTERUP)
			nEventType = TouchEvent::TouchEvent_POINTER_UP;
		else if (uMsg == WM_POINTERDOWN)
			nEventType = TouchEvent::TouchEvent_POINTER_DOWN;

		if (nEventType >= 0)
		{
			TouchEvent event(EH_TOUCH, (TouchEvent::TouchEventMsgType)nEventType, id, (float)x, (float)y, GetTickCount());
			CGUIRoot::GetInstance()->handleTouchEvent(event);
		}
		break;
	}
	case WM_POINTERCAPTURECHANGED:
	{
		//delay actual state change until game thread start because mouse msg come after POINT msg.

	}
	break;

	case WM_CLOSE:
		// WM_CLOSE indicates a request to close a window, such as by clicking on the x. This is a good time to ask the user if they want to save their work, etc.
		// Calling DefWindowProc will destroy the window, while returning zero will leave the window intact.

		// if the user prohibit closing, then we will return 0.
		if (!IsWindowClosingAllowed())
		{
			if (CGlobals::GetEventsCenter())
			{
				// msg = command line.
				string msg = "msg={type=\"WM_CLOSE\"};";
				SystemEvent event(SystemEvent::SYS_WM_CLOSE, msg);
				CGlobals::GetEventsCenter()->FireEvent(event);
				SetHasClosingRequest(false);
			}
		}
		else
		{
			SetHasClosingRequest(false);
			Cleanup3DEnvironment();
			delete m_pRenderContext;
			m_pRenderContext = nullptr;
			FinalCleanup();
			// this will prevent render to be called.
			SetAppState(PEAppState_Stopped);

			// tell the window to destroy itself and Post WM_QUIT message.
			PostWinThreadMessage(PE_WM_QUIT, 0, 0);
			m_hWnd = NULL;
			return 2; // this will stop processing any other messages in the pool
		}
		break;
	}
	return 0;
}

void CWindowsApplication::Exit(int nReturnCode /*= 0*/)
{
	SetReturnCode(nReturnCode);
	OUTPUT_LOG("program exited with code %d\n", nReturnCode);
#ifdef PARAENGINE_CLIENT
	if (CGlobals::GetAppHWND())
	{
		::PostMessage(CGlobals::GetAppHWND(), WM_CLOSE, 0, 0);
	}
	else
	{
		SetAppState(PEAppState_Exiting);
	}
#else
	SetAppState(PEAppState_Exiting);
#endif
}


ITouchInputTranslator* CWindowsApplication::LoadTouchInputPlug()
{
	if (m_pTouchInput)
		return m_pTouchInput;


#ifdef _DEBUG
	const char* TouchInputPlugin_DLL_FILE_PATH = "TouchInputPlugin_d.dll";
#else
	const char* TouchInputPlugin_DLL_FILE_PATH = "TouchInputPlugin.dll";
#endif

	DLLPlugInEntity* pPluginEntity = CGlobals::GetPluginManager()->GetPluginEntity(TouchInputPlugin_DLL_FILE_PATH);
	if (pPluginEntity == 0)
	{
		pPluginEntity = ParaEngine::CGlobals::GetPluginManager()->LoadDLL("", TouchInputPlugin_DLL_FILE_PATH);
	}

	if (pPluginEntity != 0)
	{
		for (int i = 0; i < pPluginEntity->GetNumberOfClasses(); i++)
		{
			ClassDescriptor* pClassDesc = pPluginEntity->GetClassDescriptor(i);
			if (pClassDesc && (strcmp(pClassDesc->ClassName(), "ITouchInput") == 0))
			{
				m_pTouchInput = (ITouchInputTranslator*)pClassDesc->Create();
			}
		}
	}
	return m_pTouchInput;
}

float CWindowsApplication::GetFPS()
{
	return m_fFPS;
}

void CWindowsApplication::UpdateFrameStats(double fTime)
{
	// Keep track of the frame count
	static double fLastTime = 0.0f;
	static DWORD dwFrames = 0;
	++dwFrames;
	// Update the scene stats once per second
	if (fTime - fLastTime > 1.0f)
	{
		m_fFPS = (float)(dwFrames / (fTime - fLastTime));
		fLastTime = fTime;
		dwFrames = 0;
	}
}


bool CWindowsApplication::ForceRender()
{
#ifdef USE_DIRECTX_RENDERER
	auto pd3dDevice = CGlobals::GetRenderDevice();

	if (pd3dDevice == 0)
		return false;
	bool bSucceed = false;
#ifndef ONLY_FORCERENDER_GUI 
	if (SUCCEEDED(Render()))
	{
		bSucceed = SUCCEEDED(PresentScene());
	}
#else	
	if (SUCCEEDED(pd3dDevice->BeginScene()))
	{
		// since we use EnableAutoDepthStencil, The device will create a depth-stencil buffer when it is created. The depth-stencil buffer will be automatically set as the render target of the device. 
		// When the device is reset, the depth-stencil buffer will be automatically destroyed and recreated in the new size. 
		// However, we must SetRenderTarget to the back buffer in each frame in order for  EnableAutoDepthStencil work properly for the backbuffer as well. 
		pd3dDevice->SetRenderTarget(0, CGlobals::GetDirectXEngine().GetRenderTarget(0)); // force setting render target to back buffer. and 

																						 /// clear to scene
		pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			0x00000000, 1.0f, 0L);
		CGlobals::GetGUI()->AdvanceGUI(0);
		pd3dDevice->EndScene();
		bSucceed = SUCCEEDED(Present(NULL, NULL, NULL, NULL));
	}
#endif
	return bSucceed;
#else
	return true;
#endif
}