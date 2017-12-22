//-----------------------------------------------------------------------------
// Class:	D3d Window util
// Authors:	Li, Xizhi
// Emails:	LiXizhi@yeah.net
//-----------------------------------------------------------------------------
#include "ParaEngine.h"
#include "D3DWindowUtil.h"

using namespace ParaEngine;


//-----------------------------------------------------------------------------
// Name: D3DFormatToString
// Desc: Returns the string for the given D3DFORMAT.
//-----------------------------------------------------------------------------
LPCTSTR D3DUtil_D3DFormatToString( D3DFORMAT format, bool bWithPrefix )
{
	TCHAR* pstr = NULL;
	switch( format )
	{
	case D3DFMT_UNKNOWN:         pstr = TEXT("D3DFMT_UNKNOWN"); break;
	case D3DFMT_R8G8B8:          pstr = TEXT("D3DFMT_R8G8B8"); break;
	case D3DFMT_A8R8G8B8:        pstr = TEXT("D3DFMT_A8R8G8B8"); break;
	case D3DFMT_X8R8G8B8:        pstr = TEXT("D3DFMT_X8R8G8B8"); break;
	case D3DFMT_R5G6B5:          pstr = TEXT("D3DFMT_R5G6B5"); break;
	case D3DFMT_X1R5G5B5:        pstr = TEXT("D3DFMT_X1R5G5B5"); break;
	case D3DFMT_A1R5G5B5:        pstr = TEXT("D3DFMT_A1R5G5B5"); break;
	case D3DFMT_A4R4G4B4:        pstr = TEXT("D3DFMT_A4R4G4B4"); break;
	case D3DFMT_R3G3B2:          pstr = TEXT("D3DFMT_R3G3B2"); break;
	case D3DFMT_A8:              pstr = TEXT("D3DFMT_A8"); break;
	case D3DFMT_A8R3G3B2:        pstr = TEXT("D3DFMT_A8R3G3B2"); break;
	case D3DFMT_X4R4G4B4:        pstr = TEXT("D3DFMT_X4R4G4B4"); break;
	case D3DFMT_A2B10G10R10:     pstr = TEXT("D3DFMT_A2B10G10R10"); break;
	case D3DFMT_A8B8G8R8:        pstr = TEXT("D3DFMT_A8B8G8R8"); break;
	case D3DFMT_X8B8G8R8:        pstr = TEXT("D3DFMT_X8B8G8R8"); break;
	case D3DFMT_G16R16:          pstr = TEXT("D3DFMT_G16R16"); break;
	case D3DFMT_A2R10G10B10:     pstr = TEXT("D3DFMT_A2R10G10B10"); break;
	case D3DFMT_A16B16G16R16:    pstr = TEXT("D3DFMT_A16B16G16R16"); break;
	case D3DFMT_A8P8:            pstr = TEXT("D3DFMT_A8P8"); break;
	case D3DFMT_P8:              pstr = TEXT("D3DFMT_P8"); break;
	case D3DFMT_L8:              pstr = TEXT("D3DFMT_L8"); break;
	case D3DFMT_A8L8:            pstr = TEXT("D3DFMT_A8L8"); break;
	case D3DFMT_A4L4:            pstr = TEXT("D3DFMT_A4L4"); break;
	case D3DFMT_V8U8:            pstr = TEXT("D3DFMT_V8U8"); break;
	case D3DFMT_L6V5U5:          pstr = TEXT("D3DFMT_L6V5U5"); break;
	case D3DFMT_X8L8V8U8:        pstr = TEXT("D3DFMT_X8L8V8U8"); break;
	case D3DFMT_Q8W8V8U8:        pstr = TEXT("D3DFMT_Q8W8V8U8"); break;
	case D3DFMT_V16U16:          pstr = TEXT("D3DFMT_V16U16"); break;
	case D3DFMT_A2W10V10U10:     pstr = TEXT("D3DFMT_A2W10V10U10"); break;
	case D3DFMT_UYVY:            pstr = TEXT("D3DFMT_UYVY"); break;
	case D3DFMT_YUY2:            pstr = TEXT("D3DFMT_YUY2"); break;
	case D3DFMT_DXT1:            pstr = TEXT("D3DFMT_DXT1"); break;
	case D3DFMT_DXT2:            pstr = TEXT("D3DFMT_DXT2"); break;
	case D3DFMT_DXT3:            pstr = TEXT("D3DFMT_DXT3"); break;
	case D3DFMT_DXT4:            pstr = TEXT("D3DFMT_DXT4"); break;
	case D3DFMT_DXT5:            pstr = TEXT("D3DFMT_DXT5"); break;
	case D3DFMT_D16_LOCKABLE:    pstr = TEXT("D3DFMT_D16_LOCKABLE"); break;
	case D3DFMT_D32:             pstr = TEXT("D3DFMT_D32"); break;
	case D3DFMT_D15S1:           pstr = TEXT("D3DFMT_D15S1"); break;
	case D3DFMT_D24S8:           pstr = TEXT("D3DFMT_D24S8"); break;
	case D3DFMT_D24X8:           pstr = TEXT("D3DFMT_D24X8"); break;
	case D3DFMT_D24X4S4:         pstr = TEXT("D3DFMT_D24X4S4"); break;
	case D3DFMT_D16:             pstr = TEXT("D3DFMT_D16"); break;
	case D3DFMT_L16:             pstr = TEXT("D3DFMT_L16"); break;
	case D3DFMT_VERTEXDATA:      pstr = TEXT("D3DFMT_VERTEXDATA"); break;
	case D3DFMT_INDEX16:         pstr = TEXT("D3DFMT_INDEX16"); break;
	case D3DFMT_INDEX32:         pstr = TEXT("D3DFMT_INDEX32"); break;
	case D3DFMT_Q16W16V16U16:    pstr = TEXT("D3DFMT_Q16W16V16U16"); break;
	case D3DFMT_MULTI2_ARGB8:    pstr = TEXT("D3DFMT_MULTI2_ARGB8"); break;
	case D3DFMT_R16F:            pstr = TEXT("D3DFMT_R16F"); break;
	case D3DFMT_G16R16F:         pstr = TEXT("D3DFMT_G16R16F"); break;
	case D3DFMT_A16B16G16R16F:   pstr = TEXT("D3DFMT_A16B16G16R16F"); break;
	case D3DFMT_R32F:            pstr = TEXT("D3DFMT_R32F"); break;
	case D3DFMT_G32R32F:         pstr = TEXT("D3DFMT_G32R32F"); break;
	case D3DFMT_A32B32G32R32F:   pstr = TEXT("D3DFMT_A32B32G32R32F"); break;
	case D3DFMT_CxV8U8:          pstr = TEXT("D3DFMT_CxV8U8"); break;
	default:                     pstr = TEXT("Unknown format"); break;
	}
	if( bWithPrefix || _tcsstr( pstr, TEXT("D3DFMT_") )== NULL )
		return pstr;
	else
		return pstr + lstrlen( TEXT("D3DFMT_") );
}


//-----------------------------------------------------------------------------
// Name: DisplayErrorMsg()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
HRESULT CD3DWindowUtil::DisplayErrorMsg( HRESULT hr, DWORD dwType )
{
    static bool s_bFatalErrorReported = false;
    WCHAR strMsg[512];

    // If a fatal error message has already been reported, the app
    // is already shutting down, so don't show more error messages.
    if( s_bFatalErrorReported )
        return hr;

    switch( hr )
    {
        case D3DAPPERR_NODIRECT3D:
            wcscpy( strMsg, L"�ܱ�Ǹ���޷���ʼ��3D�Կ��豸\n����Ҫ���ز���װ���µ�DirectX����\n" );
            break;

        case D3DAPPERR_NOCOMPATIBLEDEVICES:
            /*_tcscpy( strMsg, _T("Could not find any compatible Direct3D\n")
                             _T("devices.") );*/
			/*_tcscpy( strMsg, _T("�Ҳ��������ǵĲ�Ʒ���ݵ�3D�Կ��豸\n") );*/
			wcscpy(strMsg, L"�ܱ�Ǹ������Կ������ǵĲ�Ʒ������\n");
            break;

        case D3DAPPERR_NOWINDOWABLEDEVICES:
            /*_tcscpy( strMsg, _T("This sample cannot run in a desktop\n")
                             _T("window with the current display settings.\n")
                             _T("Please change your desktop settings to a\n")
                             _T("16- or 32-bit display mode and re-run this\n")
                             _T("sample.") );*/
			/*_tcscpy( strMsg, _T("�����޷���Ŀǰ����ʾ�����ڴ���ģʽ������\n")
				_T("����������ʾ����Ϊ16λ��32λ���, Ȼ���������г���") );*/
			wcscpy( strMsg, L"�ܱ�Ǹ����ǰ��ʾ�������Ʒ����Ҫ�������\n ����������ʾ����Ϊ16λ��32λ���, Ȼ���������г���" );
            break;

        case D3DAPPERR_NOHARDWAREDEVICE:
            /*_tcscpy( strMsg, _T("No hardware-accelerated Direct3D devices\n")
                             _T("were found.") );*/
			wcscpy( strMsg, L"�ܱ�Ǹ��û���ҵ��κ�Ӳ�������豸\n" );
            break;

        case D3DAPPERR_HALNOTCOMPATIBLE:
            /*_tcscpy( strMsg, _T("This sample requires functionality that is\n")
                             _T("not available on your Direct3D hardware\n")
                             _T("accelerator.") );*/
			//_tcscpy( strMsg, _T("���ǵĳ�����Ҫʹ��һ������, �������3D�Կ�����֧��\n") );
			wcscpy( strMsg, L"�ܱ�Ǹ������Կ���֧�ֲ��ֹ���\n" );
            break;

        case D3DAPPERR_NOWINDOWEDHAL:
            /*_tcscpy( strMsg, _T("Your Direct3D hardware accelerator cannot\n")
                             _T("render into a window.\n")
                             _T("Press F2 while the app is running to see a\n")
                             _T("list of available devices and modes.") );*/
			/*_tcscpy( strMsg, _T("���3D�Կ��޷�ʹ�ô���ģʽ��Ⱦ\n")
				_T("�밴F2��, �鿴����Կ�֧�ֵ���ʾģʽ��") );*/
			wcscpy(strMsg, L"�ܱ�Ǹ����ǰ��ʾ�������Ʒ����Ҫ�������\n�밴F2��, �鿴����Կ�֧�ֵ���ʾģʽ��\n");
            break;

        case D3DAPPERR_NODESKTOPHAL:
            /*_tcscpy( strMsg, _T("Your Direct3D hardware accelerator cannot\n")
                             _T("render into a window with the current\n")
                             _T("desktop display settings.\n")
                             _T("Press F2 while the app is running to see a\n")
                             _T("list of available devices and modes.") );*/
			/*_tcscpy( strMsg, _T("���3D�Կ��޷��Գ���ǰ��������ʾ����\n")
							_T("��Ⱦ������ģʽ�¡��밴F2��, �鿴����Կ�֧�ֵ���ʾģʽ��\n"));*/
			wcscpy( strMsg, L"�ܱ�Ǹ����ǰ��ʾ�������Ʒ����Ҫ�������\n�밴F2��, �鿴����Կ�֧�ֵ���ʾģʽ��\n");
            break;

        case D3DAPPERR_NOHALTHISMODE:
            /*_tcscpy( strMsg, _T("This sample requires functionality that is\n")
                             _T("not available on your Direct3D hardware\n")
                             _T("accelerator with the current desktop display\n")
                             _T("settings.\n")
                             _T("Press F2 while the app is running to see a\n")
                             _T("list of available devices and modes.") );*/
			/*_tcscpy( strMsg, _T("���ǵĳ�����Ҫʹ��һ������, �������3D�Կ���\n")
							_T("�����ڵ�ǰ��������ʾģʽ��֧�֡�\n")
							_T("�밴F2��, �鿴����Կ�֧�ֵ���ʾģʽ��\n"));*/
			wcscpy( strMsg, L"�ܱ�Ǹ����ǰ��ʾ�������Ʒ����Ҫ�������\n�밴F2��, �鿴����Կ�֧�ֵ���ʾģʽ��\n");
            break;

        case D3DAPPERR_MEDIANOTFOUND:
        case 0x80070002: // HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ):
            /*_tcscpy( strMsg, _T("Could not load required media." ) );*/
			//_tcscpy( strMsg, _T("�޷��ҵ���Դ�ļ�\n") );
			wcscpy( strMsg, L"�ܱ�Ǹ���޷��ҵ���Դ�ļ�\n" );
            break;

        case D3DAPPERR_RESETFAILED:
            /*_tcscpy( strMsg, _T("Could not reset the Direct3D device." ) );*/
			//_tcscpy( strMsg, _T("�޷�����3D�Կ��豸\n") );
			wcscpy(strMsg, L"�ܱ�Ǹ���޷�����3D�Կ��豸\n");
            break;

        case D3DAPPERR_NONZEROREFCOUNT:
            /*_tcscpy( strMsg, _T("A D3D object has a non-zero reference\n")
                             _T("count (meaning things were not properly\n")
                             _T("cleaned up).") );*/
			wcscpy( strMsg, L"�ܱ�Ǹ��3D�Կ���������һ������û��ɾ��\n");
            break;

        case D3DAPPERR_NULLREFDEVICE:
            /*_tcscpy( strMsg, _T("Warning: Nothing will be rendered.\n")
                             _T("The reference rendering device was selected, but your\n")
                             _T("computer only has a reduced-functionality reference device\n")
                             _T("installed.  Install the DirectX SDK to get the full\n")
                             _T("reference device.\n") );*/
			wcscpy( strMsg, L"�ܱ�Ǹ��������û�а�װ��û������3D�Կ����޷�����3D��Ʒ���뻻̨�������԰ɡ�\n");
            break;

        case E_OUTOFMEMORY:
            /*_tcscpy( strMsg, _T("Not enough memory.") );*/
			wcscpy( strMsg, L"�ܱ�Ǹ��ϵͳ�ڴ治�㡣");
            break;

        case D3DERR_OUTOFVIDEOMEMORY:
            //_tcscpy( strMsg, _T("Not enough video memory.") );
			wcscpy( strMsg, L"�ܱ�Ǹ���Կ��ڴ治�㡣�볢���ð�ȫģʽ����");
            break;

        case D3DERR_DRIVERINTERNALERROR:
            //_tcscpy( strMsg, _T("A serious problem occurred inside the display driver.") );
			wcscpy( strMsg, L"�ܱ�Ǹ������Կ��������������һ���������⡣");
            dwType = MSGERR_APPMUSTEXIT;
            break;

        default:
            /*_tcscpy( strMsg, _T("Generic application error. Enable\n")
                             _T("debug output for detailed information.") );*/
			wcscpy( strMsg, L"�ܱ�Ǹ��������һ������, �뷢��Log�ļ�������");
    }

	if(CGlobals::GetApp())
	{
		CGlobals::GetApp()->SendMessageToApp(CGlobals::GetApp()->GetMainWindow(),  PE_APP_SHOW_ERROR_MSG, hr, dwType);
	}

    if( MSGERR_APPMUSTEXIT == dwType )
    {
        s_bFatalErrorReported = true;
        /*_tcscat( strMsg, _T("\n\nThis sample will now exit.") );*/
		wcscpy( strMsg, L"\n\n���򼴽��˳�");
        MessageBoxW( NULL, strMsg, L"ParaEngine Window", MB_ICONERROR|MB_OK );

        // Close the window, which shuts down the app
        if(CGlobals::GetApp())
		{
			CGlobals::GetApp()->SendMessageToApp(CGlobals::GetApp()->GetMainWindow(),  WM_CLOSE, 0, 0);
		}
    }
    else
    {
        if( MSGWARN_SWITCHEDTOREF == dwType )
		{
            /*_tcscat( strMsg, _T("\n\nSwitching to the reference rasterizer,\n")
                             _T("a software device that implements the entire\n")
                             _T("Direct3D feature set, but runs very slowly.") );*/
			/*_tcscat( strMsg, _T("\n\n�л������������ģʽ\n")
				_T("����������ģ��ȫ��3D�Կ��ļ��ٹ���,\n")
				_T("���ǻ����е÷ǳ�������") );*/
			wcscpy( strMsg, L"\n\n�л������������ģʽ\n����������ģ��ȫ��3D�Կ��ļ��ٹ���,\n�������л�Ƚϻ�����" );
		}
        MessageBoxW( NULL, strMsg, L"ParaEngine Window", MB_ICONWARNING|MB_OK );
    }
    return hr;
}