/*************************************************************************************************
*
* File: ConfigProcs.cpp
*
* Description: Implements configuration GUI functions. Mostly window procs. Not C++ :/
*
* Copyright (c) 2010 Brian Pantano. All rights reserved.
*
*************************************************************************************************/
#include <TChar.h>
#include <shlobj.h>
#include <Uxtheme.h>
#include <Vsstyle.h>
#include <Dbt.h>

#include "ConfigProcs.h"
#include "MainProcs.h"
#include "Globals.h"
#include "resource.h"

#include "GameState.h"

VOID DoPreferences( HWND hWndOwner )
{
    int pDialogs[] = { IDD_PP1_VISUAL, IDD_PP2_AUDIO, IDD_PP4_CONTROLS, IDD_PP5_LIBRARY };
    DLGPROC pProcs[] = { VisualProc, AudioProc, ControlsProc, LibraryProc };
    LPCWSTR pTitles[] = { TEXT( "Visual" ), TEXT( "Audio" ), TEXT( "Controls" ), TEXT( "Library" ) };
    PROPSHEETPAGE psp[4];
    PROPSHEETHEADER psh;

    for ( int i = 0; i < sizeof( psp ) / sizeof( PROPSHEETPAGE ); i++ )
    {
        psp[i].dwSize = sizeof( PROPSHEETPAGE );
        psp[i].dwFlags = PSP_USETITLE;
        psp[i].hInstance = g_hInstance;
        psp[i].pszTemplate = MAKEINTRESOURCE( pDialogs[i] );
        psp[i].pszIcon = NULL;
        psp[i].pfnDlgProc = pProcs[i];
        psp[i].pszTitle = pTitles[i];
        psp[i].lParam = 0;
        psp[i].pfnCallback = NULL;
    }
    psh.dwSize = sizeof( PROPSHEETHEADER );
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP;
    psh.hwndParent = hWndOwner;
    psh.hInstance = g_hInstance;
    psh.pszIcon = NULL;
    psh.pszCaption = TEXT( "Preferences" );
    psh.nPages = sizeof( psp ) / sizeof( PROPSHEETPAGE );
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.pfnCallback = NULL;

    INT_PTR result = PropertySheet(&psh);
}

VOID Changed( HWND hWnd )
{
    SendMessage( GetParent( hWnd ), PSM_CHANGED, ( WPARAM )hWnd, 0 );
}

INT_PTR WINAPI VisualProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static bool bCanRead = false;

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hWndFirstKey = GetDlgItem( hWnd, IDC_FIRSTKEY );
            HWND hWndLastKey = GetDlgItem( hWnd, IDC_LASTKEY );

            // Enumerate the keys
            for ( int i = MIDI::A0; i <= MIDI::C8; i++ )
            {
                SendMessage( hWndFirstKey, CB_ADDSTRING, i, ( LPARAM )MIDI::NoteName(i).c_str() );
                SendMessage( hWndLastKey, CB_ADDSTRING, i, ( LPARAM )MIDI::NoteName(i).c_str() );
            }

            RECT rcBtn, rcOwner;
            HWND hWndBtn = GetDlgItem( hWnd, IDC_SETUSINGPIANO );
            GetWindowRect( hWndBtn, &rcBtn );
            GetWindowRect( hWnd, &rcOwner );
            OffsetRect( &rcBtn, -rcOwner.left, -rcOwner.top - 1 );
            SetWindowPos( hWndBtn, NULL, rcBtn.left, rcBtn.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE );

            // Config to fill out the form
            Config &config = Config::GetConfig();
            SetVisualProc( hWnd, config.GetVisualSettings() );
            return TRUE;
        }
        // Draws the colored buttons
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            if ( ( pdis->CtlID < IDC_COLOR1 || pdis->CtlID > IDC_COLOR6 ) && pdis->CtlID != IDC_BKGCOLOR )
                return FALSE;

            SetDCBrushColor( pdis->hDC, (COLORREF)GetWindowLongPtr( pdis->hwndItem, GWLP_USERDATA ) );
            FillRect( pdis->hDC, &pdis->rcItem, ( HBRUSH )GetStockObject( DC_BRUSH ) );
            return TRUE;
        }
        case WM_COMMAND:
        {
            int iId = LOWORD( wParam );
            Changed( hWnd );
            switch ( iId )
            {
                case IDC_SHOWCUSTOMKEYS:
                    EnableWindow( GetDlgItem( hWnd, IDC_FIRSTKEY ), TRUE );
                    EnableWindow( GetDlgItem( hWnd, IDC_THROUGH ), TRUE );
                    EnableWindow( GetDlgItem( hWnd, IDC_LASTKEY ), TRUE );
                    EnableWindow( GetDlgItem( hWnd, IDC_SETUSINGPIANO ), bCanRead );
                    return TRUE;
                case IDC_SHOWALLKEYS: case IDC_SHOWSONGKEYS:
                    EnableWindow( GetDlgItem( hWnd, IDC_FIRSTKEY ), FALSE );
                    EnableWindow( GetDlgItem( hWnd, IDC_THROUGH ), FALSE );
                    EnableWindow( GetDlgItem( hWnd, IDC_LASTKEY ), FALSE );
                    EnableWindow( GetDlgItem( hWnd, IDC_SETUSINGPIANO ), FALSE );
                    return TRUE;
                case IDC_SETUSINGPIANO:
                {
                    INT_PTR iResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE( IDD_NOTESPAN ), hWnd,
                                                      NoteSpanProc, ( LPARAM )hWnd );
                    if ( iResult == -1 ) return TRUE;
                    
                    HWND hWndFirstKey = GetDlgItem( hWnd, IDC_FIRSTKEY );
                    HWND hWndLastKey = GetDlgItem( hWnd, IDC_LASTKEY );
                    SendMessage( hWndFirstKey, CB_SETCURSEL, ( iResult >> 8 ) - MIDI::A0, 0 );
                    SendMessage( hWndLastKey, CB_SETCURSEL, ( iResult & 0xFF ) - MIDI::A0, 0 );
                    return TRUE;
                }
                // Color buttons. Pop up color choose dialog and set color.
                case IDC_COLOR1: case IDC_COLOR2: case IDC_COLOR3:
                case IDC_COLOR4: case IDC_COLOR5: case IDC_COLOR6: 
                case IDC_BKGCOLOR:
                {
                    static COLORREF acrCustClr[16] = { 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 
                                                       0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF }; 
                    HWND hWndBtn = ( HWND )lParam;

                    // Choose and save the color
                    CHOOSECOLOR cc = { 0 };
                    cc.lStructSize = sizeof( cc );
                    cc.hwndOwner = hWnd;
                    cc.lpCustColors = (LPDWORD) acrCustClr;
                    cc.rgbResult = (COLORREF)GetWindowLongPtr( hWndBtn, GWLP_USERDATA );
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if ( !ChooseColor( &cc ) ) return TRUE;

                    // Draw the button (indirect)
                    SetWindowLongPtr( hWndBtn, GWLP_USERDATA, cc.rgbResult );
                    InvalidateRect( hWndBtn, NULL, FALSE );
                    return TRUE;
                }
                case IDC_RESTOREDEFAULTS:
                {
                    VisualSettings cVisualSettings;
                    cVisualSettings.LoadDefaultValues();

                    SendMessage( hWnd, WM_SETREDRAW, FALSE, 0 );
                    SetVisualProc( hWnd, cVisualSettings  );
                    SendMessage( hWnd, WM_SETREDRAW, TRUE, 0 );
                    InvalidateRect( hWnd, NULL, FALSE );
                    return TRUE;
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = ( LPNMHDR )lParam;
            switch (lpnmhdr->code)
            {
                case PSN_SETACTIVE:
                {
                    const AudioSettings &cAudio = Config::GetConfig().GetAudioSettings();                    
                    MIDIInDevice midiInDevice;
                    bCanRead = ( cAudio.iInDevice >= 0 && midiInDevice.Open( cAudio.iInDevice ) );
                    midiInDevice.Close();
                    
                    if ( IsDlgButtonChecked( hWnd, IDC_SHOWCUSTOMKEYS ) ) 
                        EnableWindow( GetDlgItem( hWnd, IDC_SETUSINGPIANO ), bCanRead );

                    return TRUE;
                }
                // OK or Apply button pressed
                case PSN_APPLY:
                {
                    // Get a copy of the config to overwrite the settings
                    Config &config = Config::GetConfig();
                    VisualSettings cVisual = config.GetVisualSettings();
                    ViewSettings &cView = config.GetViewSettings();

                    // VisualSettings struct
                    bool bAlwaysShowControls = cVisual.bAlwaysShowControls;
                    cVisual.eKeysShown = ( IsDlgButtonChecked( hWnd, IDC_SHOWALLKEYS ) == BST_CHECKED ? cVisual.All : 
                                           IsDlgButtonChecked( hWnd, IDC_SHOWSONGKEYS ) == BST_CHECKED ? cVisual.Song :
                                           IsDlgButtonChecked( hWnd, IDC_SHOWCUSTOMKEYS ) == BST_CHECKED ? cVisual.Custom :
                                           cVisual.Song );
                    cVisual.bAlwaysShowControls = ( IsDlgButtonChecked( hWnd, IDC_SHOWCONTROLS ) == BST_CHECKED );
                    cVisual.bAssociateFiles = ( IsDlgButtonChecked( hWnd, IDC_ASSOCIATEFILES ) == BST_CHECKED );
                    cVisual.iFirstKey = (int)SendMessage( GetDlgItem( hWnd, IDC_FIRSTKEY ), CB_GETCURSEL, 0, 0 ) + MIDI::A0;
                    cVisual.iLastKey = (int)SendMessage( GetDlgItem( hWnd, IDC_LASTKEY ), CB_GETCURSEL, 0, 0 ) + MIDI::A0;
                    for ( int i = 0; i < IDC_COLOR6 - IDC_COLOR1 + 1; i++ )
                        cVisual.colors[i] = (int)GetWindowLongPtr( GetDlgItem( hWnd, IDC_COLOR1 + i ), GWLP_USERDATA );
                    cVisual.iBkgColor = (int)GetWindowLongPtr( GetDlgItem( hWnd, IDC_BKGCOLOR ), GWLP_USERDATA );

                    // Report success and return
                    config.SetVisualSettings( cVisual );
                    if ( cView.GetFullScreen() && bAlwaysShowControls != cVisual.bAlwaysShowControls )
                        cView.SetControls( cView.GetControls(), true );
                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

// Sets the values in the playback settings dialog. Used at init and restoring defaults
VOID SetVisualProc( HWND hWnd, const VisualSettings &cVisual )
{
    HWND hWndFirstKey = GetDlgItem( hWnd, IDC_FIRSTKEY );
    HWND hWndLastKey = GetDlgItem( hWnd, IDC_LASTKEY );

    // Set values
    CheckRadioButton( hWnd, IDC_SHOWALLKEYS, IDC_SHOWCUSTOMKEYS, IDC_SHOWALLKEYS + cVisual.eKeysShown );
    CheckDlgButton( hWnd, IDC_SHOWCONTROLS, cVisual.bAlwaysShowControls ? BST_CHECKED : BST_UNCHECKED );
    CheckDlgButton( hWnd, IDC_ASSOCIATEFILES, cVisual.bAssociateFiles ? BST_CHECKED : BST_UNCHECKED );
    SendMessage( hWnd, WM_COMMAND, IDC_SHOWALLKEYS + cVisual.eKeysShown, 0 );
    SendMessage( hWndFirstKey, CB_SETCURSEL, cVisual.iFirstKey - MIDI::A0, 0 );
    SendMessage( hWndLastKey, CB_SETCURSEL, cVisual.iLastKey - MIDI::A0, 0 );

    // Colors
    for ( int i = 0; i < IDC_COLOR6 - IDC_COLOR1 + 1; i++ )
        SetWindowLongPtr( GetDlgItem( hWnd, IDC_COLOR1 + i ), GWLP_USERDATA, cVisual.colors[i] );
    SetWindowLongPtr( GetDlgItem( hWnd, IDC_BKGCOLOR ), GWLP_USERDATA, cVisual.iBkgColor );
}

INT_PTR WINAPI NoteSpanProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static MIDIInDevice midiInDevice;
    static unsigned char cNote1;

	switch ( msg )
	{
	    case WM_INITDIALOG:
        {
            Config &config = Config::GetConfig();
            const AudioSettings &cAudio = config.GetAudioSettings();
            cNote1 = 0xFF;
            
            if ( cAudio.vMIDIInDevices.size() == 0 )
            {
                MessageBox( hWnd, TEXT( "No input MIDI device available." ), TEXT( "Error" ), MB_OK | MB_ICONEXCLAMATION );
			    EndDialog( hWnd, -1 );
                return FALSE;
            }

            if ( cAudio.iInDevice < 0 )
            {
                MessageBox( hWnd, TEXT( "Please select an input MIDI device in the Audio tab." ), TEXT( "Error" ), MB_OK | MB_ICONEXCLAMATION );
			    EndDialog( hWnd, -1 );
                return FALSE;
            }

            // Center align to the property page
            RECT rcPos, rcProp;
            HWND hWndProp = ( HWND )lParam;
            GetClientRect( hWnd, &rcPos );
            GetWindowRect( hWndProp, &rcProp );
            SetWindowPos( hWnd, NULL, rcProp.left + ( rcProp.right - rcProp.left - rcPos.right ) / 2,
                rcProp.top + ( rcProp.bottom - rcProp.top - rcPos.bottom ) / 2,
                0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE );

            SetWindowText( GetDlgItem( hWnd, IDC_INSTRUCTIONS ), TEXT( "On your piano, please play the lowest note to display" ) );
            midiInDevice.SetCallback( MIDIInProc, hWnd );
            if ( !midiInDevice.Open( cAudio.iInDevice ) )
            {
                MessageBox( hWnd, TEXT( "Unable to open the MIDI input device." ), TEXT( "Error" ), MB_OK | MB_ICONEXCLAMATION );
			    EndDialog( hWnd, -1 );
                return FALSE;
            }

            return TRUE;
        }
	    case WM_COMMAND:
        {
            int iId = LOWORD( wParam );
            switch ( iId )
            {
                case ID_MIDI_NOTEON:
                    if ( cNote1 == 0xFF )
                    {
                        cNote1 = static_cast< unsigned char >( lParam & 0xFF );
                        SetWindowText( GetDlgItem( hWnd, IDC_FIRSTNOTE ), MIDI::NoteName( cNote1 ).c_str() );
                        SetWindowText( GetDlgItem( hWnd, IDC_INSTRUCTIONS ), TEXT( "Good! Now play the highest note to display" ) );
                    }
                    else
                    {
                        SetWindowText( GetDlgItem( hWnd, IDC_LASTNOTE ), MIDI::NoteName( lParam & 0xFF ).c_str() );
                        SetWindowText( GetDlgItem( hWnd, IDC_INSTRUCTIONS ), TEXT( "All set" ) );
                        midiInDevice.Close();
                        Sleep( 500 );
			            EndDialog( hWnd, ( cNote1 << 8 ) | ( lParam & 0xFF ) );
                    }
			        return TRUE;
                case IDCANCEL:
                    midiInDevice.Close();
			        EndDialog( hWnd, -1 );
			        return TRUE;
            }
		    break;
        }
	}

	return FALSE;
}

void MIDIInProc( unsigned char cStatus, unsigned char cParam1, unsigned char cParam2, int iMilliSecs, void *pUserData )
{
    if ( static_cast< MIDIChannelEvent::ChannelEventType >( cStatus >> 4 ) != MIDIChannelEvent::NoteOn || cParam2 == 0 )
        return;

    // Send message will cause a deadlock when it shuts down the device
    PostMessage( ( HWND )pUserData, WM_COMMAND, ID_MIDI_NOTEON, cParam1 );
}

INT_PTR WINAPI AudioProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg)
    {
        case WM_INITDIALOG:
            SetAudioProc( hWnd, Config::GetConfig().GetAudioSettings() );
            return TRUE;
        case WM_COMMAND:
        {
            int iId = LOWORD( wParam );
            int iCode = HIWORD( wParam );
            if ( iCode == LBN_SELCHANGE ) 
                Changed( hWnd );
            break;
        }
        case WM_DEVICECHANGE:
            Sleep( 200 );
            SetAudioProc( hWnd, Config::GetConfig().GetAudioSettings() );
            break;
        case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = ( LPNMHDR )lParam;
            switch (lpnmhdr->code)
            {
                // OK or Apply button pressed
                case PSN_APPLY:
                {
                    // Get a copy of the config to overwrite the settings
                    Config &config = Config::GetConfig();
                    AudioSettings cAudio = config.GetAudioSettings();

                    // Get the values
                    cAudio.iInDevice = (int)SendDlgItemMessage( hWnd, IDC_MIDIIN, LB_GETCURSEL, 0, 0 );
                    if ( cAudio.iInDevice >= 0 ) cAudio.sDesiredIn = cAudio.vMIDIInDevices[cAudio.iInDevice];
                    cAudio.iOutDevice = (int)SendDlgItemMessage( hWnd, IDC_MIDIOUT, LB_GETCURSEL, 0, 0 );
                    if ( cAudio.iOutDevice >= 0 ) cAudio.sDesiredOut = cAudio.vMIDIOutDevices[cAudio.iOutDevice];

                    // Set the values
                    bool bChanged = ( cAudio.iOutDevice != config.GetAudioSettings().iOutDevice ||
                                      cAudio.iInDevice != config.GetAudioSettings().iInDevice );
                    config.SetAudioSettings( cAudio );

                    if ( bChanged )
                        HandOffMsg( WM_DEVICECHANGE, 0, 0 );

                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}

// Sets the values in the playback settings dialog. Used at init and restoring defaults
VOID SetAudioProc( HWND hWnd, const AudioSettings &cAudio )
{
    Config::GetConfig().LoadMIDIDevices();

    HWND hWndOutDevs = GetDlgItem( hWnd, IDC_MIDIOUT );
    while( SendMessage( hWndOutDevs, LB_DELETESTRING, 0, 0 ) > 0  );
    for ( vector< wstring >::const_iterator it = cAudio.vMIDIOutDevices.begin(); it != cAudio.vMIDIOutDevices.end(); ++it )
        SendMessage( hWndOutDevs, LB_ADDSTRING, 0, ( LPARAM )( it->c_str() ) );
    SendMessage( hWndOutDevs, LB_SETCURSEL, cAudio.iOutDevice, 0 );

    HWND hWndInDevs = GetDlgItem( hWnd, IDC_MIDIIN );
    while( SendMessage( hWndInDevs, LB_DELETESTRING, 0, 0 ) > 0 );
    for ( vector< wstring >::const_iterator it = cAudio.vMIDIInDevices.begin(); it != cAudio.vMIDIInDevices.end(); ++it )
        SendMessage( hWndInDevs, LB_ADDSTRING, 0, ( LPARAM )( it->c_str() ) );
    SendMessage( hWndInDevs, LB_SETCURSEL, cAudio.iInDevice, 0 );
}

INT_PTR WINAPI VideoProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            // Config to fill out the form
            Config &config = Config::GetConfig();
            const VideoSettings &cVideo = config.GetVideoSettings();

            CheckRadioButton( hWnd, IDC_DIRECT3D, IDC_GDI, IDC_DIRECT3D + cVideo.eRenderer );
            CheckDlgButton( hWnd, IDC_DISPLAYFPS, cVideo.bShowFPS ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hWnd, IDC_LIMITFPS, cVideo.bLimitFPS ? BST_CHECKED : BST_UNCHECKED );

            return TRUE;
        }
        case WM_COMMAND:
            Changed( hWnd );
            break;
        case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = ( LPNMHDR )lParam;
            switch (lpnmhdr->code)
            {
                // OK or Apply button pressed
                case PSN_APPLY:
                {
                    // Get a copy of the config to overwrite the settings
                    Config &config = Config::GetConfig();
                    VideoSettings cVideo = config.GetVideoSettings();

                    cVideo.eRenderer = ( IsDlgButtonChecked( hWnd, IDC_DIRECT3D ) == BST_CHECKED ? cVideo.Direct3D : 
                                         IsDlgButtonChecked( hWnd, IDC_OPENGL ) == BST_CHECKED ? cVideo.OpenGL :
                                         IsDlgButtonChecked( hWnd, IDC_GDI ) == BST_CHECKED ? cVideo.GDI :
                                         cVideo.Direct3D );
                    cVideo.bShowFPS = ( IsDlgButtonChecked( hWnd, IDC_DISPLAYFPS ) == BST_CHECKED );
                    cVideo.bLimitFPS = ( IsDlgButtonChecked( hWnd, IDC_LIMITFPS ) == BST_CHECKED );

                    config.SetVideoSettings( cVideo );
                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}

INT_PTR WINAPI ControlsProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static HWND hWndHotItem = NULL;
    static MIDIInDevice midiInDevice;
    static int pIds[] = { ID_PLAY_PLAYPAUSE, ID_PLAY_STOP, ID_PLAY_SKIPFWD, ID_PLAY_SKIPBACK,
                          ID_PLAY_LOOP, ID_PLAY_INCREASERATE, ID_PLAY_DECREASERATE, ID_PLAY_RESETRATE };

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            // Config to fill out the form
            Config &config = Config::GetConfig();
            const ControlsSettings &cControls = config.GetControlsSettings();
            hWndHotItem = NULL;

            // Turn them all off
            for ( int i = 0; i < sizeof( pIds ) / sizeof( int ); i++ )
                SetWindowLongPtr( GetDlgItem( hWnd, pIds[i] ), GWLP_USERDATA, -1 );

            // Turn on those specified in settings
            for ( int i = 0; i < 128; i++ )
                if ( cControls.aKeyboardMap[i] > 0 )
                    SetWindowLongPtr( GetDlgItem( hWnd, cControls.aKeyboardMap[i] + 33 ), GWLP_USERDATA, i );

            HWND hWndFwdBack = GetDlgItem( hWnd, IDC_LRARROWS );
            HWND hWndSpeedPct = GetDlgItem( hWnd, IDC_UDARROWS );

            // Edit boxes
            TCHAR buf[32];
            _stprintf_s( buf, TEXT( "%g" ), cControls.dFwdBackSecs );
            SetWindowText( hWndFwdBack, buf );
            _stprintf_s( buf, TEXT( "%g" ), cControls.dSpeedUpPct );
            SetWindowText( hWndSpeedPct, buf );
            
            return TRUE;
        }
        case WM_COMMAND:
        {
            int iId = LOWORD( wParam );
            int iCode = HIWORD( wParam );
            if ( ( iId != IDC_LRARROWS && iId != IDC_UDARROWS ) || iCode == EN_CHANGE )
                Changed( hWnd );

            switch ( iId )
            {
                case ID_PLAY_PLAYPAUSE: case ID_PLAY_STOP: case ID_PLAY_SKIPFWD: case ID_PLAY_SKIPBACK:
                case ID_PLAY_LOOP: case ID_PLAY_INCREASERATE: case ID_PLAY_DECREASERATE: case ID_PLAY_RESETRATE:
                {
                    HWND hWndBtn = ( HWND )lParam;
                    if ( hWndHotItem && hWndBtn != hWndHotItem )
                        InvalidateRect( hWndHotItem, NULL, TRUE );
                   
                    if ( hWndBtn == hWndHotItem )
                        hWndHotItem = NULL;
                    else
                        hWndHotItem = hWndBtn;
                    InvalidateRect( hWndBtn, NULL, TRUE );
                    return TRUE;
                }
                case IDC_RESET:
                {
                    HWND hWndTemp = hWndHotItem;
                    hWndHotItem = NULL;
                    for ( int i = 0; i < sizeof( pIds ) / sizeof( int ); i++ )
                    {
                        HWND hWndBtn = GetDlgItem( hWnd, pIds[i] );
                        if ( hWndBtn == hWndTemp || GetWindowLongPtr( hWndBtn, GWLP_USERDATA ) != -1 )
                        {
                            SetWindowLongPtr( hWndBtn, GWLP_USERDATA, -1 );
                            InvalidateRect( hWndBtn, NULL, TRUE );
                        }
                    }
                    return TRUE;
                }
                case ID_MIDI_NOTEON:
                    if ( hWndHotItem )
                    {
                        // Get rid of any other controls already set to the note
                        for ( int i = 0; i < sizeof( pIds ) / sizeof( int ); i++ )
                        {
                            HWND hWndBtn = GetDlgItem( hWnd, pIds[i] );
                            if ( hWndBtn != hWndHotItem && GetWindowLongPtr( hWndBtn, GWLP_USERDATA ) == lParam )
                            {
                                SetWindowLongPtr( hWndBtn, GWLP_USERDATA, -1 );
                                InvalidateRect( hWndBtn, NULL, TRUE );
                            }
                        }

                        // Set the note and refresh
                        SetWindowLongPtr( hWndHotItem, GWLP_USERDATA, lParam & 0xFF );
                        HWND hWndTemp = hWndHotItem;
                        hWndHotItem = NULL;
                        InvalidateRect( hWndTemp, NULL, TRUE );
                    }
                    return TRUE;
            }
            break;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;

            InflateRect( &pdis->rcItem, -1, -1 );
            if ( pdis->hwndItem == hWndHotItem || pdis->itemState & ODS_SELECTED )
                SetDCBrushColor( pdis->hDC, 0x00EAD999 );
            else
                SetDCBrushColor( pdis->hDC, 0x00FFFFFF );
            FillRect( pdis->hDC, &pdis->rcItem, ( HBRUSH )GetStockObject( DC_BRUSH ) );
            DrawEdge( pdis->hDC, &pdis->rcItem, EDGE_ETCHED, BF_RECT | BF_FLAT );

            int iNote = (int)GetWindowLongPtr( pdis->hwndItem, GWLP_USERDATA );
            const TCHAR *sBtnText;
            if ( pdis->hwndItem == hWndHotItem )
                sBtnText = TEXT( "Play a note..." );
            else if ( iNote >= 0 && iNote < 128 )
                sBtnText = MIDI::NoteName( iNote ).c_str();
            else
                sBtnText = TEXT( "Click to set..." );

            SetBkMode( pdis->hDC, TRANSPARENT );
            if ( pdis->itemState & ODS_DISABLED )
            {
                SetTextColor( pdis->hDC, GetSysColor( COLOR_BTNHIGHLIGHT ) );
                OffsetRect( &pdis->rcItem, 1, 1 );
                DrawText( pdis->hDC, sBtnText, -1, &pdis->rcItem, DT_SINGLELINE | DT_CENTER | DT_VCENTER );
                OffsetRect( &pdis->rcItem, -1, -1 );
                SetTextColor( pdis->hDC, GetSysColor( COLOR_GRAYTEXT ) );
            }
            else
                SetTextColor( pdis->hDC, GetSysColor( COLOR_BTNTEXT ) );

            DrawText( pdis->hDC, sBtnText, -1, &pdis->rcItem, DT_SINGLELINE | DT_CENTER | DT_VCENTER );
            return TRUE;
        }
        case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = ( LPNMHDR )lParam;
            switch (lpnmhdr->code)
            {
                // Spin boxes. Just increment or decrement
                case UDN_DELTAPOS:
                    switch ( lpnmhdr->idFrom )
                    {
                        case IDC_LRARROWSSPIN:
                        {
                            TCHAR buf[32];
                            LPNMUPDOWN lpnmud  = ( LPNMUPDOWN )lParam;
                            HWND hWndFwdBack = GetDlgItem( hWnd, IDC_LRARROWS );
                            double dOldVal = 0;
                            int len = GetWindowText( hWndFwdBack, buf, 32 );
                            if ( len > 0 && _stscanf_s( buf, TEXT( "%lf" ), &dOldVal ) == 1 )
                            {
                                double dNewVal = dOldVal - lpnmud->iDelta * .1;
                                _stprintf_s( buf, TEXT( "%g" ), dNewVal );
                                SetWindowText( hWndFwdBack, buf );
                            }
                            return TRUE;
                        }
                        case IDC_UDARROWSSPIN:
                        {
                            TCHAR buf[32];
                            LPNMUPDOWN lpnmud  = ( LPNMUPDOWN )lParam;
                            HWND hWndSpeedPct = GetDlgItem( hWnd, IDC_UDARROWS );
                            double dOldVal = 0;
                            int len = GetWindowText( hWndSpeedPct, buf, 32 );
                            if ( len > 0 && _stscanf_s( buf, TEXT( "%lf" ), &dOldVal ) == 1 )
                            {
                                double dNewVal = dOldVal - lpnmud->iDelta;
                                _stprintf_s( buf, TEXT( "%g" ), dNewVal );
                                SetWindowText( hWndSpeedPct, buf );
                            }
                            return TRUE;
                        }
                    }
                    break;
                case PSN_RESET:
                case PSN_KILLACTIVE:
                    midiInDevice.Close();
                    hWndHotItem = NULL;
                    return TRUE;
                case PSN_SETACTIVE:
                {
                    // Config to fill out the form
                    Config &config = Config::GetConfig();
                    const AudioSettings &cAudio = config.GetAudioSettings();
                    
                    bool bEnable = false;
                    midiInDevice.SetCallback( MIDIInProc, hWnd );
                    if ( cAudio.iInDevice >= 0 && midiInDevice.Open( cAudio.iInDevice ) )
                        bEnable = true;

                    for ( int i = 0; i < sizeof( pIds ) / sizeof( int ); i++ )
                        EnableWindow( GetDlgItem( hWnd, pIds[i] ), bEnable );
                    EnableWindow( GetDlgItem( hWnd, IDC_RESET ), bEnable );

                    if ( cAudio.iInDevice < 0 )
                        SetWindowText( GetDlgItem( hWnd, IDC_PIANOGROUP ), TEXT( "Keyboard controls (requires connected keyboard)" ) );
                    else
                        SetWindowText( GetDlgItem( hWnd, IDC_PIANOGROUP ), TEXT( "Keyboard controls" ) );

                    return TRUE;
                }
                // OK or Apply button pressed
                case PSN_APPLY:
                {
                    midiInDevice.Close();

                    // Get a copy of the config to overwrite the settings
                    Config &config = Config::GetConfig();
                    ControlsSettings cControls = config.GetControlsSettings();

                    memset( cControls.aKeyboardMap, 0, sizeof( cControls.aKeyboardMap ) );
                    for ( int i = 0; i < sizeof( pIds ) / sizeof( int ); i++ )
                    {
                        int iNote = (int)GetWindowLongPtr( GetDlgItem( hWnd, pIds[i] ), GWLP_USERDATA );
                        if ( iNote >= 0 && iNote < 128 )
                            cControls.aKeyboardMap[iNote] = pIds[i] - 33;
                    }

                    // Edit boxes
                    TCHAR buf[32];
                    double dEditVal = 0;
                    
                    HWND hWndFwdBack = GetDlgItem( hWnd, IDC_LRARROWS );
                    int len = GetWindowText( hWndFwdBack, buf, 32 );
                    if ( len > 0 && _stscanf_s( buf, TEXT( "%lf" ), &dEditVal ) == 1 )
                        cControls.dFwdBackSecs = dEditVal;
                    else
                    {
                        MessageBox( hWnd, TEXT( "Please specify a numeric value for the left and right arrows" ), TEXT( "Error" ), MB_OK | MB_ICONEXCLAMATION );
                        PostMessage( hWnd, WM_NEXTDLGCTL, ( WPARAM )hWndFwdBack, TRUE);
                        SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_INVALID );
                        return TRUE;
                    }
                    
                    HWND hWndSpeedPct = GetDlgItem( hWnd, IDC_UDARROWS );
                    len = GetWindowText( hWndSpeedPct, buf, 32 );
                    if ( len > 0 && _stscanf_s( buf, TEXT( "%lf" ), &dEditVal ) == 1 )
                        cControls.dSpeedUpPct = dEditVal;
                    else
                    {
                        MessageBox( hWnd, TEXT( "Please specify a numeric value for the up and down arrows" ), TEXT( "Error" ), MB_OK | MB_ICONEXCLAMATION );
                        PostMessage( hWnd, WM_NEXTDLGCTL, ( WPARAM )hWndSpeedPct, TRUE);
                        SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_INVALID );
                        return TRUE;
                    }
                    
                    // Report success and return
                    config.SetControlsSettings( cControls );
                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

INT_PTR WINAPI LibraryProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            // Config to fill out the form
            Config &config = Config::GetConfig();
            const SongLibrary &cLibrary = config.GetSongLibrary();
            const map< wstring, SongLibrary::Source > &mSources = cLibrary.GetSources();

            CheckDlgButton( hWnd, IDC_ALWAYSADD, cLibrary.GetAlwaysAdd() ? BST_CHECKED : BST_UNCHECKED );

            // Set up the list view
            RECT rc;
            HWND hWndLibrary = GetDlgItem( hWnd, IDC_LIBRARY );
            SendMessage( hWndLibrary, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER );
            GetClientRect( hWndLibrary, &rc );

            // Set up the columns of the list view
            LVCOLUMN lvc;
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = rc.right - 68;
            lvc.pszText = TEXT( "File or Folder" );
            SendMessage( hWndLibrary, LVM_INSERTCOLUMN, 0, ( LPARAM )&lvc );

            lvc.fmt = LVCFMT_CENTER;
            lvc.cx = 68;
            lvc.pszText = TEXT( "Subfolders?" );
            SendMessage( hWndLibrary, LVM_INSERTCOLUMN, 1, ( LPARAM )&lvc );

            // Add the library sources
            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = -1;
            for ( map< wstring, SongLibrary::Source >::const_iterator it = mSources.begin(); it != mSources.end(); ++it )
            {
                lvi.iItem++;
                lvi.iSubItem = 0;
                lvi.pszText = ( LPTSTR )( it->first.c_str() );
                lvi.iItem = (int)SendMessage( hWndLibrary, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
                if ( it->second != SongLibrary::File )
                {
                    lvi.iSubItem = 1;
                    lvi.pszText = it->second == SongLibrary::Folder ? TEXT ( "No" ) : TEXT ( "Yes" );
                    SendMessage( hWndLibrary, LVM_SETITEM, 0, ( LPARAM )&lvi );
                }
            }

            return TRUE;
        }
        case WM_COMMAND:
        {
            Changed( hWnd );
            int iId = LOWORD( wParam );
            switch ( iId )
            {
                case IDC_ADDFILE:
                {
                    // Get the file(s) to add
                    OPENFILENAME ofn = { 0 };
                    TCHAR sFilename[1024] = { 0 };
                    ofn.lStructSize = sizeof( OPENFILENAME );
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = TEXT( "MIDI Files\0*.mid;*.bmid\0" );
                    ofn.lpstrFile = sFilename;
                    ofn.nMaxFile = sizeof( sFilename ) / sizeof( TCHAR );
                    ofn.lpstrTitle = TEXT( "Please select a song to add to the library" );
                    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    if ( !GetOpenFileName( &ofn ) ) return TRUE;

                    // Prep the item structure
                    HWND hWndLibrary = GetDlgItem( hWnd, IDC_LIBRARY );
                    LVITEM lvi;
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = lvi.iSubItem = 0;
                    lvi.pszText = ofn.lpstrFile;

                    // To ensure no duplicates are added
                    LVFINDINFO lvfi;
                    lvfi.flags = LVFI_STRING;
                    lvfi.psz = ofn.lpstrFile;

                    // Add multiple files
                    if ( ofn.nFileOffset > 0 && ofn.lpstrFile[ofn.nFileOffset - 1] == '\0' )
                    {
                        ofn.lpstrFile[ofn.nFileOffset - 1] = '\\';
                        TCHAR *pFilename = ofn.lpstrFile + ofn.nFileOffset;
                        TCHAR *pNextFile = pFilename + _tcslen( pFilename ) + 1;
                        while ( *pFilename )
                        {
                            if ( SendMessage( hWndLibrary, LVM_FINDITEM, -1, ( LPARAM )&lvfi ) == -1 )
                                SendMessage( hWndLibrary, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
                            size_t len = _tcslen( pNextFile );
                            memmove( pFilename, pNextFile, ( len + 1 ) * sizeof( TCHAR ) ); // memmove because buffers overlap
                            pNextFile += len + 1;
                        }
                    }
                    // Add a single file
                    else if ( SendMessage( hWndLibrary, LVM_FINDITEM, -1, ( LPARAM )&lvfi ) == -1 )
                        SendMessage( hWndLibrary, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

                    return TRUE;
                }
                case IDC_ADDFOLDER:
                {
                    // Set up the data structure for the shell common dialog
                    TCHAR sFolder[MAX_PATH];
                    LPITEMIDLIST pidl = NULL;
                    BROWSEINFO bi = { 0 };
                    bi.hwndOwner = hWnd;
                    bi.pszDisplayName = sFolder;
                    bi.pidlRoot = NULL;
                    bi.lpszTitle = TEXT( "Please select a folder to add to the library" );
                    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

                    // Get the folder. COM bleh
                    if ( ( pidl = SHBrowseForFolder( &bi ) ) == NULL ) return TRUE;
                    BOOL bResult = SHGetPathFromIDList( pidl, sFolder );
                    CoTaskMemFree(pidl);

                    // Prep the item structure
                    HWND hWndLibrary = GetDlgItem( hWnd, IDC_LIBRARY );
                    LVITEM lvi;
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = lvi.iSubItem = 0;
                    lvi.pszText = sFolder;

                    // To ensure no duplicates are added
                    LVFINDINFO lvfi;
                    lvfi.flags = LVFI_STRING;
                    lvfi.psz = sFolder;

                    // Add it if it's not already there
                    if ( bResult && SendMessage( hWndLibrary, LVM_FINDITEM, -1, ( LPARAM )&lvfi ) == -1 )
                    {
                        lvi.iItem = (int)SendMessage( hWndLibrary, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
                        lvi.iSubItem = 1;
                        lvi.pszText = TEXT( "No" );
                        SendMessage( hWndLibrary, LVM_SETITEM, 0, ( LPARAM )&lvi );
                    }

                    return TRUE;
                }
                case IDC_REMOVE:
                {
                    HWND hWndLibrary = GetDlgItem( hWnd, IDC_LIBRARY );
                    int iItems = (int)SendMessage( hWndLibrary, LVM_GETITEMCOUNT, 0, 0 );
                    for ( int i = iItems - 1; i >= 0; i-- )
                        if ( SendMessage( hWndLibrary, LVM_GETITEMSTATE, i, LVIS_SELECTED ) )
                            SendMessage( hWndLibrary, LVM_DELETEITEM, i, 0 );
                    return TRUE;
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = ( LPNMHDR )lParam;
            switch ( lpnmhdr->code )
            {
                // Double click on the ListView. Toggle the 2nd column.
                case NM_DBLCLK:
                {
                    LPNMITEMACTIVATE lpnmia = ( LPNMITEMACTIVATE )lpnmhdr;
                    ToggleYN( lpnmhdr->hwndFrom, lpnmia->iItem );
                    Changed( hWnd );
                    return TRUE;
                }
                // OK or Apply button pressed
                case PSN_APPLY:
                {
                    // Get a copy of the config to overwrite the settings
                    Config &config = Config::GetConfig();
                    SongLibrary &cLibrary = config.GetSongLibrary();

                    cLibrary.SetAlwaysAdd( IsDlgButtonChecked( hWnd, IDC_ALWAYSADD ) == BST_CHECKED );

                    // Prep the item structures
                    LVITEM lviPath, lviYN;
                    TCHAR sPath[1024];
                    TCHAR YN[2];
                    lviPath.iSubItem = 0;
                    lviPath.pszText = sPath;
                    lviPath.cchTextMax = sizeof( sPath ) / sizeof( TCHAR );
                    lviYN.iSubItem = 1;
                    lviYN.pszText = YN;
                    lviYN.cchTextMax = 2;

                    // Get sources. Can't just add. Need to remove first
                    HWND hWndLibrary = GetDlgItem( hWnd, IDC_LIBRARY );
                    int iItems = (int)SendMessage( hWndLibrary, LVM_GETITEMCOUNT, 0, 0 );
                    map< wstring, SongLibrary::Source > mSrcSources;
                    for ( int i = 0; i < iItems; i++ )
                        if ( SendMessage( hWndLibrary, LVM_GETITEMTEXT, i, ( LPARAM )&lviPath )  > 0 )
                        {
                            SongLibrary::Source eSource = SongLibrary::File;
                            if ( SendMessage( hWndLibrary, LVM_GETITEMTEXT, i, ( LPARAM )&lviYN )  > 0 )
                                eSource = ( YN[0] == TEXT( 'Y' ) ? SongLibrary::FolderWSubdirs : SongLibrary::Folder );
                            mSrcSources[sPath] = eSource;
                        }

                    int iChanged = 0;

                    // Remove sources that have been removed
                    const map< wstring, SongLibrary::Source > &mSources = cLibrary.GetSources();
                    vector< wstring > vToRemove; // Can't remove while iterating, so save a list then remove
                    for ( map< wstring, SongLibrary::Source >::const_iterator it = mSources.begin(); it != mSources.end(); ++it )
                        if ( mSrcSources.find( it->first ) == mSrcSources.end() )
                            vToRemove.push_back( it->first );
                    for ( vector< wstring >::const_iterator it = vToRemove.begin(); it != vToRemove.end(); ++it )
                        iChanged += cLibrary.RemoveSource( *it );

                    // Add sources
                    for ( map< wstring, SongLibrary::Source >::iterator it = mSrcSources.begin(); it != mSrcSources.end(); ++it )
                        iChanged += cLibrary.AddSource( it->first, it->second );

                    if ( iChanged > 0 ) PopulateLibrary( GetDlgItem( g_hWndLibDlg, IDC_LIBRARYFILES ) );
                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, PSNRET_NOERROR );
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}

BOOL ToggleYN( HWND hWndListview, int iItem )
{
    if ( iItem < 0 ) return FALSE;

    LVITEM lvi;
    TCHAR YN[2];
    lvi.iSubItem = 1;
    lvi.pszText = YN;
    lvi.cchTextMax = 2;
    if ( SendMessage( hWndListview, LVM_GETITEMTEXT, iItem, ( LPARAM )&lvi )  <= 0 ) return FALSE;

    if ( YN[0] == TEXT( 'Y' ) ) lvi.pszText = TEXT( "No" );
    else if ( YN[0] == TEXT( 'N' ) ) lvi.pszText = TEXT( "Yes" );
    else return TRUE;

    SendMessage( hWndListview, LVM_SETITEMTEXT, iItem, ( LPARAM )&lvi );
    return TRUE;
}

BOOL GetCustomSettings( MainScreen *pGameState )
{
    INT_PTR iDlgResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE( IDD_TRACKSETTINGS ),
                                         g_hWnd, TracksProc, ( LPARAM )pGameState );
    return iDlgResult == IDOK;
}

INT_PTR WINAPI TracksProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static const VisualSettings &cVisual = Config::GetConfig().GetVisualSettings();
    static vector< bool > vScored, vMuted, vHidden; // Would rather be part of control, but no subitem lparam available
    static vector< unsigned > vColors;

	switch ( msg )
	{
	    case WM_INITDIALOG:
        {
            HWND hWndTracks = GetDlgItem( hWnd, IDC_TRACKS );
            TCHAR buf[MAX_PATH];
            SendMessage( hWndTracks, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER );

            // Get data
            MainScreen *pGameState = ( MainScreen* )lParam;
            const MIDI &midi = pGameState->GetMIDI();
            const MIDI::MIDIInfo &mInfo = midi.GetInfo();
            const vector< MIDITrack* > &vTracks = midi.GetTracks();
            SetWindowLongPtr( hWnd, GWLP_USERDATA, ( LONG_PTR )pGameState );

            // Fill out the static text vals
            SetWindowText( GetDlgItem( hWnd, IDC_FILE ), mInfo.sFilename.c_str() + mInfo.sFilename.find_last_of( L'\\' ) + 1 );
            SetWindowText( GetDlgItem( hWnd, IDC_FOLDER ), mInfo.sFilename.substr( 0, mInfo.sFilename.find_last_of( L'\\' ) ).c_str() );
            _stprintf_s( buf, TEXT( "%d" ), mInfo.iNoteCount );
            SetWindowText( GetDlgItem( hWnd, IDC_NOTES ), buf );
            _stprintf_s( buf, TEXT( "%lld:%02.0lf" ), mInfo.llTotalMicroSecs / 60000000,
                                                    ( mInfo.llTotalMicroSecs % 60000000 ) / 1000000.0 );
            SetWindowText( GetDlgItem( hWnd, IDC_LENGTH ), buf );

            // Initialize the state vars
            vScored.resize( mInfo.iNumChannels );
            vMuted.resize( mInfo.iNumChannels );
            vHidden.resize( mInfo.iNumChannels );
            vColors.resize( mInfo.iNumChannels );
            int iMax = sizeof( cVisual.colors ) / sizeof ( cVisual.colors[0] );
            for ( int i = 0; i < mInfo.iNumChannels; i++ )
            {
                vScored[i] = vMuted[i] = vHidden[i] = false;
                if ( i < iMax ) vColors[i] = cVisual.colors[i];
                else vColors[i] = Util::RandColor();
            }
            
            // Set up the columns of the list view
            RECT rcTracks;
            GetClientRect( hWndTracks, &rcTracks );
            int aFmt[7] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_RIGHT, LVCFMT_CENTER, LVCFMT_CENTER, LVCFMT_CENTER, LVCFMT_CENTER };
            int aCx[7] = { 27, rcTracks.right - 27 - 55 - 50 - 50 - 50 - 50, 55, 50, 50, 50, 50 };
            TCHAR *aText[7] = { TEXT( "Trk" ), TEXT( "Instrument" ), TEXT( "Notes" ), TEXT( "Scored" ), TEXT( "Muted" ), TEXT( "Hidden" ), TEXT( "Color" ) };

            LVCOLUMN lvc = { 0 };
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
            for ( int i = 0; i < sizeof( aFmt ) / sizeof( int ); i++ )
            {
                lvc.fmt = aFmt[i];
                lvc.cx = aCx[i];
                lvc.pszText = aText[i];
                SendMessage( hWndTracks, LVM_INSERTCOLUMN, i, ( LPARAM )&lvc );
            }

            // Set rows of the list view
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.pszText = buf;

            int iPos = 0;
            for ( int i = 0; i < mInfo.iNumTracks; i++ )
            {
                const MIDITrack::MIDITrackInfo &mTrackInfo = vTracks[i]->GetInfo();
                for ( int j = 0; j < 16; j++ )
                    if ( mTrackInfo.aNoteCount[j] > 0 )
                    {
                        lvi.iSubItem = 0;
                        _stprintf_s( buf, TEXT( "%d" ), iPos + 1 );
                        lvi.iItem = (int)SendMessage( hWndTracks, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

                        lvi.iSubItem++;
                        _stprintf_s( buf, TEXT( "%s" ), j == 9 ? TEXT( "Drums" ) : MIDI::Instruments[mTrackInfo.aProgram[j]].c_str() );
                        SendMessage( hWndTracks, LVM_SETITEM, 0, ( LPARAM )&lvi );

                        lvi.iSubItem++;
                        _stprintf_s( buf, TEXT( "%d" ), mTrackInfo.aNoteCount[j] );
                        SendMessage( hWndTracks, LVM_SETITEM, 0, ( LPARAM )&lvi );

                        lvi.iItem++;
                        iPos++;
                    }
            }

            if ( GetWindowLongPtr( hWndTracks, GWL_STYLE ) & WS_VSCROLL )
                SendMessage( hWndTracks, LVM_SETCOLUMNWIDTH, 1, aCx[1] - 17 );

            RECT rcItem = { LVIR_BOUNDS };
            SendMessage( hWndTracks, LVM_GETITEMRECT, 0, ( LPARAM )&rcItem );
            int iItemHeight = rcItem.bottom - rcItem.top;
            int iBmpSize = iItemHeight - 2;

            return TRUE;
        }
        case WM_NOTIFY:
        {
            LPNMHDR lpnmhdr = ( LPNMHDR )lParam;
            if ( lpnmhdr->idFrom == IDC_TRACKS)
            {
                switch ( lpnmhdr->code )
                {
                    // Prevent's item selection
                    case LVN_ITEMCHANGING:
                    {
                        LPNMLISTVIEW pnmv = ( LPNMLISTVIEW )lParam;
                        if ( ( pnmv->uChanged & LVIF_STATE ) &&
                             ( pnmv->uNewState & LVIS_SELECTED ) != ( pnmv->uOldState & LVIS_SELECTED ) )
                        {
                            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, TRUE );
                            return TRUE;
                        }
                        break;
                    }
                    // Turn all on or all off or reset colors
                    case LVN_COLUMNCLICK:
                    {
                        LPNMLISTVIEW lpnmlv = ( LPNMLISTVIEW )lParam;
                        bool bAllChecked = true;
                        size_t iMax = sizeof( cVisual.colors ) / sizeof ( cVisual.colors[0] );
                        switch ( lpnmlv->iSubItem )
                        {
                            case 3:
                                for ( size_t i = 0; i < vScored.size(); i++ ) bAllChecked &= vScored[i];
                                for ( size_t i = 0; i < vScored.size(); i++ ) vScored[i] = !bAllChecked;
                                if ( !bAllChecked ) for ( size_t i = 0; i < vScored.size(); i++ ) vMuted[i] = true;
                                InvalidateRect( lpnmlv->hdr.hwndFrom, NULL, FALSE );
                                return TRUE;
                            case 4:
                                for ( size_t i = 0; i < vMuted.size(); i++ ) bAllChecked &= vMuted[i];
                                for ( size_t i = 0; i < vMuted.size(); i++ ) vMuted[i] = !bAllChecked;
                                InvalidateRect( lpnmlv->hdr.hwndFrom, NULL, FALSE );
                                return TRUE;
                            case 5:
                                for ( size_t i = 0; i < vHidden.size(); i++ ) bAllChecked &= vHidden[i];
                                for ( size_t i = 0; i < vHidden.size(); i++ ) vHidden[i] = !bAllChecked;
                                InvalidateRect( lpnmlv->hdr.hwndFrom, NULL, FALSE );
                                return TRUE;
                            case 6:
                                for ( size_t i = 0; i < vColors.size(); i++ )
                                    if ( i < iMax ) vColors[i] = cVisual.colors[i];
                                    else vColors[i] = Util::RandColor();
                                InvalidateRect( lpnmlv->hdr.hwndFrom, NULL, FALSE );
                                return TRUE;
                        }
                        return TRUE;
                    }
                    // Toggle checkboxes or select color
                    case NM_CLICK:
                    case NM_DBLCLK:
                    {
                        // Have to manually figure out the corresponding item. Silly.
                        LPNMITEMACTIVATE lpnmia = ( LPNMITEMACTIVATE )lpnmhdr;
                        LVHITTESTINFO lvhti = { lpnmia->ptAction };
                        SendMessage( lpnmia->hdr.hwndFrom, LVM_SUBITEMHITTEST, 0, ( LPARAM )&lvhti );
                        if ( lvhti.iItem < 0 || lvhti.iItem >= ( int )vMuted.size() ) return FALSE;

                        RECT rcItem = { LVIR_BOUNDS };
                        SendMessage( lpnmia->hdr.hwndFrom, LVM_GETITEMRECT, lvhti.iItem, ( LPARAM )&rcItem );
                        switch ( lvhti.iSubItem )
                        {
                            case 3:
                                vScored[ lvhti.iItem ] = !vScored[ lvhti.iItem ];
                                if ( vScored[ lvhti.iItem ] ) vMuted[ lvhti.iItem ] = true;
                                InvalidateRect( lpnmia->hdr.hwndFrom, &rcItem, FALSE );
                                return TRUE;
                            case 4:
                                vMuted[ lvhti.iItem ] = !vMuted[ lvhti.iItem ];
                                InvalidateRect( lpnmia->hdr.hwndFrom, &rcItem, FALSE );
                                return TRUE;
                            case 5:
                                vHidden[ lvhti.iItem ] = !vHidden[ lvhti.iItem ];
                                InvalidateRect( lpnmia->hdr.hwndFrom, &rcItem, FALSE );
                                return TRUE;
                            case 6:
                            {
                                static COLORREF acrCustClr[16] = { 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 
                                                                   0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF, 0x00FFFFFF }; 
                                CHOOSECOLOR cc = { 0 };
                                cc.lStructSize = sizeof( cc );
                                cc.hwndOwner = hWnd;
                                cc.lpCustColors = (LPDWORD) acrCustClr;
                                cc.rgbResult = vColors[ lvhti.iItem ];
                                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                                if ( !ChooseColor( &cc ) ) return TRUE;
                                vColors[ lvhti.iItem ] = cc.rgbResult;
                                InvalidateRect( lpnmia->hdr.hwndFrom, &rcItem, FALSE );
                                return TRUE;
                            }
                        }
                        break;
                    }
                    // Draw the checkboxes and colors
                    case NM_CUSTOMDRAW:
                    {
                        LPNMCUSTOMDRAW lpnmcd = ( LPNMCUSTOMDRAW )lParam;
                        LPNMLVCUSTOMDRAW lpnmlvcd = ( LPNMLVCUSTOMDRAW )lParam;
                        switch ( lpnmcd->dwDrawStage )
                        {
                            case CDDS_PREPAINT: case CDDS_ITEMPREPAINT:
                                SetWindowLongPtr( hWnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW );
                                return TRUE;
                            case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                                if ( lpnmlvcd->iSubItem >= 3 && lpnmlvcd->iSubItem <= 6 )
                                {
                                    // Figure out size. Too big a rect is fine: will be clipped
                                    RECT rcOut;
                                    int iBmpSize = lpnmcd->rc.bottom - lpnmcd->rc.top - 2;
                                    rcOut.left = lpnmcd->rc.left + ( lpnmcd->rc.right - lpnmcd->rc.left - iBmpSize ) / 2;
                                    rcOut.top = lpnmcd->rc.top + ( lpnmcd->rc.bottom - lpnmcd->rc.top - iBmpSize ) / 2;
                                    rcOut.right = rcOut.left + iBmpSize;
                                    rcOut.bottom = rcOut.top + iBmpSize;

                                    // Drawing. Checkbox, checkbox, color
                                    if ( lpnmlvcd->iSubItem == 3 )
                                        DrawFrameControl( lpnmcd->hdc, &rcOut, DFC_BUTTON,
                                            DFCS_BUTTONCHECK | ( vScored[ lpnmcd->dwItemSpec ] ? DFCS_CHECKED : 0 ) );
                                    else if ( lpnmlvcd->iSubItem == 4 )
                                        DrawFrameControl( lpnmcd->hdc, &rcOut, DFC_BUTTON,
                                            DFCS_BUTTONCHECK | ( vMuted[ lpnmcd->dwItemSpec ] ? DFCS_CHECKED : 0 ) );
                                    else if ( lpnmlvcd->iSubItem == 5 )
                                        DrawFrameControl( lpnmcd->hdc, &rcOut, DFC_BUTTON,
                                            DFCS_BUTTONCHECK | ( vHidden[ lpnmcd->dwItemSpec ] ? DFCS_CHECKED : 0 ) );
                                    else
                                    {
                                        SetDCBrushColor( lpnmcd->hdc, lpnmlvcd->clrFace );
                                        FillRect( lpnmcd->hdc, &lpnmcd->rc, ( HBRUSH )GetStockObject( DC_BRUSH ) );
                                        InflateRect( &lpnmcd->rc, -1, -1 );
                                        SetDCBrushColor( lpnmcd->hdc, vColors[ lpnmcd->dwItemSpec ] );
                                        FillRect( lpnmcd->hdc, &lpnmcd->rc, ( HBRUSH )GetStockObject( DC_BRUSH ) );
                                        DrawEdge( lpnmcd->hdc, &lpnmcd->rc, BDR_SUNKENINNER, BF_RECT );
                                    }
                                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, CDRF_SKIPDEFAULT );
                                }
                                else
                                    SetWindowLongPtr( hWnd, DWLP_MSGRESULT, CDRF_DODEFAULT );
                                return TRUE;
                        }
                        break;
                    }
               }
            }
            break;
        }
	    case WM_COMMAND:
        {
            int iId = LOWORD( wParam );
            switch ( iId )
            {
                case IDOK:
                {
                    MainScreen *pGameState = ( MainScreen* )GetWindowLongPtr( hWnd, GWLP_USERDATA );
                    pGameState->SetChannelSettings( vScored, vMuted, vHidden, vColors );
                }
                case IDCANCEL:
			        EndDialog( hWnd, iId );
			        return TRUE;
            }
		    break;
        }
	}

	return FALSE;
}