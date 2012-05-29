#define VERSION ("1.1.2 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4018) // don't warn about signed/unsigned
#pragma warning(disable:4065) // don't warn about default case 
#pragma warning(disable:4800) // don't warn about forcing int to bool
#pragma warning(disable:4244) // disable warning convert from t_size to int
#pragma warning(disable:4267) 
#pragma warning(disable:4995)
#pragma warning(disable:4312)

#define _CRT_SECURE_NO_DEPRECATE 1

#define APP_NAME        "Send to Device"
#define TIMESTAMP_TAG   "SENT_TO_DEVICE"
#define URL_HELP        "http://wiki.bowron.us/index.php/Foobar2000"

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include <commctrl.h>
#include <windowsx.h>

#include <shlwapi.h>
#include <shlobj.h>
#include <atlbase.h>
#include <dbghelp.h>

#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <WCHAR.h>

#include "resource.h"
#include <set>
#include <string>

#include "../common/string_functions.h"
#include "../common/menu_stuff.h"

using namespace pfc;

/// VARIABLES
static const GUID cfg_guid001 = { 0xfc6ed904, 0xb302, 0x497d, { 0x92, 0xaf, 0x30, 0x58, 0x92, 0x84, 0xc8, 0xaa } };
static const GUID cfg_guid002 = { 0xf9b375d2, 0x8215, 0x4375, { 0xb2, 0xff, 0xb5, 0xd4, 0x30, 0xc1, 0x83, 0x25 } };
static const GUID cfg_guid003 = { 0x2852e9af, 0x6ce6, 0x4d3a, { 0xbf, 0x13, 0xcf, 0x43, 0xbe, 0xcb, 0xe4, 0x2b } };
static const GUID cfg_guid004 = { 0xcf808743, 0xf946, 0x4ca1, { 0xa2, 0x1c, 0x90, 0x59, 0x32, 0x5, 0x1f, 0x6e } };
static const GUID cfg_guid005 = { 0xa90eb7ff, 0xa359, 0x48de, { 0x9a, 0x82, 0x22, 0xc0, 0x92, 0x93, 0x7b, 0x39 } };
static const GUID cfg_guid006 = { 0xd78aa3d0, 0xdfb7, 0x410b, { 0xbd, 0x62, 0xc3, 0xff, 0x43, 0xb4, 0xf5, 0x29 } };
static const GUID cfg_guid007 = { 0xf4c78434, 0xff2, 0x4da5, { 0xa8, 0xa3, 0xdf, 0xe8, 0xb4, 0xb, 0xe8, 0xb6 } };
static const GUID cfg_guid008 = { 0xc11b3918, 0x9df9, 0x46c8, { 0x80, 0x5b, 0x6d, 0x56, 0x5b, 0xa6, 0xa9, 0xe } };
static const GUID cfg_guid009 = { 0xce853b12, 0x8f4b, 0x44e0, { 0xa6, 0x94, 0x35, 0x78, 0x5a, 0xbf, 0x76, 0x0 } };
static const GUID cfg_guid010 = { 0x4f30d1cf, 0x153b, 0x41ed, { 0x98, 0xb8, 0x87, 0xa5, 0xb, 0x2b, 0x3e, 0x27 } };
static const GUID cfg_guid011 = { 0xffa2a41f, 0x62e7, 0x4329, { 0xa3, 0xac, 0x37, 0x79, 0x7f, 0x99, 0x58, 0x90 } };


static cfg_string cfg_devices( cfg_guid001, "Z:\\Music\\%artist%\\%album%\\%title%.$ext(%_path%)" );
static cfg_int cfg_tagfiles( cfg_guid002, 0 );
static cfg_string cfg_last_device( cfg_guid003, "" );
static cfg_int cfg_run_script( cfg_guid004, 0 );
static cfg_string cfg_post_send_script( cfg_guid005, "" );
static cfg_guid cfg_send_script_guid_command( cfg_guid006, pfc::guid_null );
static cfg_guid cfg_send_script_guid_subcommand( cfg_guid007, pfc::guid_null );
static cfg_string cfg_lame_command_line( cfg_guid008, "\"c:\\program files\\foobar2000\\lame.exe\" --vbr-new -V 5" );
static cfg_int cfg_ask_to_convert( cfg_guid009, 1 );
static cfg_int cfg_copy_cover( cfg_guid010, 0);
static cfg_string cfg_cover_filename( cfg_guid011, "cover.jpg" );
/// END VARIABLES

DECLARE_COMPONENT_VERSION( "Send to Device", VERSION, 
   "Easily copy files to a portable device.  By Christopher Bowron <chris@bowron.us>\n( cover art copying by Hanno Stock <hanno.stock@gmx.net> )" )

static const GUID guid001 = { 0xafb58e16, 0x9163, 0x4608, { 0xad, 0x3c, 0xea, 0x5c, 0xc, 0xbf, 0x15, 0x72 } };
static const GUID guid002 = { 0x9aedbac1, 0xd218, 0x4004, { 0x9f, 0x92, 0xe2, 0x20, 0xc5, 0xb1, 0xb3, 0x75 } };


pfc::list_t<pfc::string8> g_temp_files;

void get_timestamp( pfc::string_base & p_out )
{
   SYSTEMTIME st;
   GetLocalTime( &st );

   p_out.set_string( pfc::string_printf( "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay,
	   st.wHour, st.wMinute, st.wSecond ) );
}


string8 handle_list_to_file_list( const pfc::list_base_const_t<metadb_handle_ptr> & p_data )
{
	string8 filelist;
   string8 filename;

	for ( int i = 0; i < p_data.get_count(); i++ )
	{
		filesystem::g_get_display_path( p_data[i]->get_path(), filename );
		filelist += filename;
		filelist += "|";
	}

   return filelist;
}

class list_index_hook : public titleformat_hook
{
public:
   int m_index;

   list_index_hook( int index )
   {
      m_index = index;
   }

	virtual bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag)
   {       
      if ( pfc::strcmp_ex( p_name, p_name_length, "list_index", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d", m_index ) );
         p_found_flag = true;
         return true;
      }
      return false;
   }


   virtual bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
   {
      return false;
   }
};

void get_output_filename( const char * format_string, metadb_handle_ptr handle, int list_index, pfc::string8 & p_out )
{
   list_index_hook format_hook( list_index + 1 );

   handle->format_title_legacy( &format_hook, p_out, format_string, NULL );

   p_out.replace_char( '\"', '_' );
   p_out.replace_char( '/', '_' );
   p_out.replace_char( '?', '_' );
   p_out.replace_char( '|', '_' );
   p_out.replace_char( '*', '_' );
   p_out.replace_char( ':', '_', 2 );

   while ( strstr( p_out, "..." ) )
   {
      pfc::string8_fastalloc tmp;
      string_replace( tmp, p_out, "...", "___" );
      p_out.set_string( tmp );
   }	
}

string8 handle_list_to_formatted_destinations( const char * format_string, const pfc::list_base_const_t<metadb_handle_ptr> & p_data )
{
	string8 the_list;
	string8 res;
	
   string8 s8_format_string = format_string;

   if ( s8_format_string.find_first( "." ) == ~0 )
   {
      s8_format_string.add_string( ".$ext(%_path%)" );
   }

	for ( int i = 0; i < p_data.get_count(); i++)
	{
      list_index_hook format_hook( i + 1 );

      p_data[i]->format_title_legacy( &format_hook, res, s8_format_string, NULL );

      res.replace_char( '\"', '_' );
      res.replace_char( '/', '_' );
      res.replace_char( '|', '_' );
      res.replace_char( '?', '_' );
      res.replace_char( '*', '_' );
      res.replace_char( ':', '_', 2 );

      while ( strstr( res, "..." ) )
      {
         pfc::string8_fastalloc tmp;
         string_replace( tmp, res, "...", "___" );
         res.set_string( tmp );
      }
		
		the_list += res;
		the_list += "|";
	}
	return the_list;
}

 void split( const char *in, const char *delim, ptr_list_t<string8> & out )
{
   string8 str = in;
	t_size start = 0;
	int count = 0;
	int pos;
				
	start = 0;
	count = 0;

	while ( ( pos = str.find_first( delim, start ) ) >=0 )
	{		
		string8 *blip = new string8( str.get_ptr() + start, pos - start );
		count ++;
		start = pos + strlen( delim );
      out.add_item( blip );
	}

	string8 *last = new string8( str.get_ptr() + start );
	out.add_item( last );
}

void replace_delim( TCHAR * s, const char d = '|' )
{
   int n = wcslen( s );

   for ( int i = 0; i < n; i ++ )
   {
      if ( s[i] == d )
      {
         s[i] = 0;
      }
   }
}

/*
bool get_playlist_filename( string8 & out )
{
   TCHAR fileName[MAX_PATH];
   TCHAR *filters = 	
		_T("m3u files (*.m3u)\0")
      _T("*.m3u\0")
      _T("All files (*.*)\0")
      _T("*.*\0")
      _T("\0");

	OPENFILENAME blip;
	memset((void*)&blip, 0, sizeof(blip));
	memset(fileName, 0, sizeof(fileName));
	blip.lpstrFilter = filters;
	blip.lStructSize = sizeof(OPENFILENAME); 
	blip.lpstrFile = fileName;
	blip.nMaxFile = sizeof(fileName)/ sizeof(*fileName); 
	blip.lpstrTitle = _T("Export Playlist...");
	blip.lpstrDefExt = _T("m3u");
	blip.Flags = OFN_OVERWRITEPROMPT;
   blip.hwndOwner = core_api::get_main_window();

   if ( GetSaveFileName( &blip ) ) 
   {
      out = stringcvt::string_utf8_from_wide( blip.lpstrFile );
      return true;
   }
   return false;
}
*/

bool copy_files( const string8 & src, const string8 & dst )
{
   int n_src = src.get_length() + 1;
   int n_dst = dst.get_length() + 1;

   TCHAR * t_src = new TCHAR[ n_src ];
   TCHAR * t_dst = new TCHAR[ n_dst ];

   stringcvt::convert_utf8_to_wide( t_src, n_src, src, -1 );
   stringcvt::convert_utf8_to_wide( t_dst, n_dst, dst, -1 );

   replace_delim( t_src );
   replace_delim( t_dst );

   SHFILEOPSTRUCT op;
	memset(&op, 0, sizeof(op));
	
	op.hwnd = NULL;
	op.wFunc = FO_COPY;
	op.pFrom = t_src;
   op.pTo = t_dst;
	op.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMMKDIR;
   op.hwnd = core_api::get_main_window();

   bool res = SHFileOperation( &op );

   if ( res ) 
   {
      console::warning( "copy to device failed" );
   }

   delete t_src;
   delete t_dst;

   return res;
}

bool copy_cover_art( const string8 & source_string, const string8 & destination_string )
{
	ptr_list_t<string8> sources;
	ptr_list_t<string8> destinations;
	split( source_string, "|", sources );
	split( destination_string, "|", destinations );

	string8 src;
	string8 dst;
	std::set<std::string> destinationsSet;

	bool coverArtToCopy = false;

	for(int i = 0; i < sources.get_count()-1; i++){
		string8 dstpath = *destinations[i];
		dstpath.truncate(destinations[i]->find_last('\\')+1);
		dstpath.add_string(cfg_cover_filename);
		if(destinationsSet.count(dstpath.get_ptr()) == 0){
			destinationsSet.insert(dstpath.get_ptr());

			string8 srcpath = *sources[i];
			srcpath.truncate(sources[i]->find_last('\\')+1);
			srcpath.add_string(cfg_cover_filename);
			
			if(PathFileExists(stringcvt::string_wide_from_utf8(srcpath.get_ptr()))){
				coverArtToCopy = true;
				dst.add_string(dstpath);
				dst.add_string("|");
				src.add_string(srcpath);
				src.add_string("|");
			}
		}
	}

	bool res = true;

	if(coverArtToCopy){
	   int n_src = src.get_length() + 1;
	   int n_dst = dst.get_length() + 1;

	   TCHAR * t_src = new TCHAR[ n_src ];
	   TCHAR * t_dst = new TCHAR[ n_dst ];

	   stringcvt::convert_utf8_to_wide( t_src, n_src, src, -1 );
	   stringcvt::convert_utf8_to_wide( t_dst, n_dst, dst, -1 );

	   replace_delim( t_src );
	   replace_delim( t_dst );

	   SHFILEOPSTRUCT op;
		memset(&op, 0, sizeof(op));
		
		op.hwnd = NULL;
		op.wFunc = FO_COPY;
		op.pFrom = t_src;
	   op.pTo = t_dst;
		op.fFlags = FOF_MULTIDESTFILES;
	   op.hwnd = core_api::get_main_window();

	   res = SHFileOperation( &op );

	   if ( res ) 
	   {
		  console::warning( "cover art copy failed" );
	   }

	   delete t_src;
	   delete t_dst;
	}
   return res;
}

static string8 g_selected_device_path;
static bool g_export_playlist;

static BOOL CALLBACK user_selection_callback( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   static ptr_list_t<string8> * s_paths = NULL;
    
   switch ( msg )
   {
   case WM_INITDIALOG:
      {
         s_paths = (ptr_list_t<string8> *) lp;
        
         HWND hwnd_selections = GetDlgItem( wnd, IDC_SELECTIONS );
			SendMessage( hwnd_selections, CB_RESETCONTENT, 0, 0 );

         for ( int i = 0; i < s_paths->get_count(); i++ )
         {
            uSendMessageText( hwnd_selections, CB_ADDSTRING, 0, 
                  s_paths->get_item( i )->get_ptr() );
         }
			
         if ( cfg_last_device.is_empty() == false )
         {
            if ( CB_ERR == uSendMessageText( hwnd_selections, CB_SELECTSTRING, -1, cfg_last_device ) )
            {
               uSendMessageText( hwnd_selections, CB_ADDSTRING, 0, cfg_last_device );
               uSendMessageText( hwnd_selections, CB_SELECTSTRING, 0, cfg_last_device );
            }
         }
         else
         {
            SendMessage( hwnd_selections, CB_SETCURSEL, 0, 0 );
         }
         
         SendMessage( hwnd_selections, CB_LIMITTEXT, 0, 0 );
         SetFocus( hwnd_selections );
      }
      break;

	case WM_COMMAND:
		switch ( wp )
		{
		case IDOK:
   		uGetDlgItemText( wnd, IDC_SELECTIONS, g_selected_device_path );
			g_export_playlist = IsDlgButtonChecked( wnd, IDC_EXPORT_PLAYLIST );
			// fallthrough
		case IDCANCEL:
			EndDialog( wnd, wp ); 
			break;
      }
   }
   return false;
}

bool get_user_device_path( string8 & out, bool & export_playlist, const ptr_list_t<string8> & paths )
{
   if ( IDOK == uDialogBox( IDD_USER_SELECTION, core_api::get_main_window(), user_selection_callback, (LPARAM)&paths ) )
   {
      out.set_string( g_selected_device_path );
      cfg_last_device = g_selected_device_path;
      export_playlist = g_export_playlist;
      return true;
   }
   return false;
}

bool get_exported_handles( const string8 & destinations, pfc::list_base_t<metadb_handle_ptr> & out_handles )
{
   ptr_list_t<string8> split_strings;
   list_t<metadb_handle_ptr> dst_handles;

   split( destinations, "|", split_strings );

   abort_callback_impl w00t;
   static_api_ptr_t<metadb> db; 

   for ( int i = 0; i < split_strings.get_count(); i++ )
   {
      if ( split_strings[i]->is_empty() == false )
      {
         metadb_handle_ptr p;
         string8 playable_loc_string;
         filesystem::g_get_canonical_path( split_strings[i]->get_ptr(), playable_loc_string );
         
         db->handle_create( p, make_playable_location( playable_loc_string, 0 ) );

         if ( p.is_valid() )
         {
            out_handles.add_item( p );
         }
      }
   }

   return true;
}

/*
bool export_playlist_worker( const string8 & filepath, const string8 & dst_list )
{
   ptr_list_t<string8> split_strings;
   list_t<metadb_handle_ptr> dst_handles;

   split( dst_list, "|", split_strings );

   abort_callback_impl w00t;
   static_api_ptr_t<metadb> db; 

   for ( int i = 0; i < split_strings.get_count(); i++ )
   {
      metadb_handle_ptr p;
      string8 playable_loc_string;
      filesystem::g_get_canonical_path( split_strings[i]->get_ptr(), playable_loc_string );
      db->handle_create( p, make_playable_location( playable_loc_string, 0 ) );
      dst_handles.add_item( p );
   }

   playlist_loader::g_save_playlist( filepath, dst_handles, w00t );
   return true;
}

bool export_playlist( const string8 & dst_list )
{
   string8 playlist_name;

   if ( get_playlist_filename( playlist_name ) )
   {
      export_playlist_worker( playlist_name, dst_list );
      return true;
   }
   return false;
}
*/

void stamp_files( const pfc::list_base_const_t<metadb_handle_ptr> & p_data )
{
   if ( cfg_tagfiles )
   {
      pfc::string8 str_timestamp;
      get_timestamp( str_timestamp );

		static_api_ptr_t<metadb_io> mdbio;

      metadb_handle_list h_list;
      pfc::list_t<file_info*> i_list;
      pfc::ptr_list_t<file_info_impl> delete_me;

      for ( int n = 0; n < p_data.get_count(); n++ )
      {
         metadb_handle_ptr handle = p_data[n];
         file_info_impl info;

         if ( handle->get_info( info ) )
         {
            file_info_impl * fi = new file_info_impl( info );

            fi->meta_set( TIMESTAMP_TAG, str_timestamp );
            h_list.add_item( handle );
            i_list.add_item( fi );
            delete_me.add_item( fi );
         }
      }

      if ( i_list.get_count() > 0 )
      {
         mdbio->update_info_multi( h_list, i_list, core_api::get_main_window(), true );
         delete_me.delete_all();
      }
   }
}

bool is_mp3( const metadb_handle_ptr p_handle )
{
   pfc::string_extension s_ext( p_handle->get_path() );
   if ( 0 == stricmp_utf8( s_ext, "mp3" ) )
   {
      return true;
   }
   else
   {
      return false;
   }
}

void decode_file( metadb_handle_ptr handle, const char * filepath, unsigned & fs_out )
{
   static_api_ptr_t<audio_postprocessor> post_process;
   abort_callback_impl abort;
   mem_block_container_impl pcm16;
   audio_chunk_impl chunk;
   input_helper in;

   in.open( NULL, handle, input_flag_no_seeking|input_flag_no_looping, abort );

   bool first = true;

   FILE * f = fopen( filepath, "wb" );
   while ( in.run( chunk, abort ) && ( abort.is_aborting() == false ) )
   {
      if ( first )
      {
         first = false;
         fs_out = chunk.get_sample_rate();
      }

      post_process->run( chunk, pcm16, 16, 16, false, 1.0 );
      fwrite( pcm16.get_ptr(), 1, 2*chunk.get_data_length(), f );
   }   
   
   fclose( f );   
}

void run_lame( const char * param )
{
   TCHAR params[MAX_PATH];
   // pfc::stringcvt::string_os_from_utf8 lame( "c:\\program files\\foobar2000\\lame.exe" );
   pfc::stringcvt::convert_utf8_to_wide( params, MAX_PATH, param, ~0 );
   
   //HINSTANCE res = uShellExecute( NULL, "open", lame, param, dir, SW_SHOW );
   STARTUPINFO startup;
   ::ZeroMemory( &startup, sizeof( startup ) );
   startup.cb           = sizeof( startup );
   startup.wShowWindow  = SW_SHOWNOACTIVATE;
   startup.dwFlags      = STARTF_USESHOWWINDOW;

   PROCESS_INFORMATION process;
   
   if ( CreateProcess( NULL, params, NULL, NULL, NULL, FALSE, NULL, NULL, &startup, &process ) )
   {
      WaitForSingleObject( process.hProcess, INFINITE );
      CloseHandle( process.hProcess );
      CloseHandle( process.hThread );
   }
   else
   {
      pfc::string8 err_msg;
      uFormatSystemErrorMessage( err_msg, GetLastError() );
      console::printf( "foo_sendtodevice: Creating lame encoder process failed: %s", err_msg.get_ptr() );
   }
}

void convert_file( metadb_handle_ptr handle, const char * dst_path, metadb_handle_ptr & p_out )
{
   pfc::string8_fastalloc path;
   filesystem::g_get_display_path( handle->get_path(), path );

   pfc::string8 tmp_path;
   pfc::string8 tmp_file;
   uGetTempPath( tmp_path );
   uGetTempFileName( tmp_path, "CWB_", 0, tmp_file );
   
   
   console::printf( "Decoding %s => %s", path.get_ptr(), tmp_file.get_ptr() );
   
   // pfc::string_printf dec_msg( "Decoding %s => %s", path.get_ptr(), tmp_file.get_ptr() );
   // popup_message::g_show( dec_msg, APP_NAME );
   unsigned fs;  // recieves sampling rate...

   decode_file( handle, tmp_file, fs ); 

   pfc::string8 cmd_line( cfg_lame_command_line );
   pfc::string8 dst_file( tmp_file );
   dst_file << ".mp3";

   console::printf( "Encoding %s => %s", tmp_file.get_ptr(), dst_file.get_ptr() );
   
   // pfc::string_printf msg( "Encoding %s => %s", tmp_file.get_ptr(), dst_file.get_ptr() );
   // popup_message::g_show( msg, APP_NAME );

   cmd_line << " -x -r -s " << fs << " \"" << tmp_file.get_ptr() << "\" \"" << dst_file.get_ptr() << "\"";
   run_lame( cmd_line );
   console::printf( "Running: %s", cmd_line.get_ptr() );

   static_api_ptr_t<metadb>()->handle_create( p_out, make_playable_location( dst_file, 0 ) );

   file_info_impl orig;
   file_info_impl copy;

   p_out->get_info( copy );
   handle->get_info( orig );
   copy.copy_meta( orig );
   static_api_ptr_t<metadb_io>()->update_info( p_out, copy, core_api::get_main_window(), true );

   g_temp_files.add_item( tmp_file );
   g_temp_files.add_item( dst_file );
}

/*
void convert_files( const pfc::list_base_const_t<metadb_handle_ptr> & non_mp3, const char * dst_path )
{
   for ( int n = 0; n < non_mp3.get_count(); n++ )
   {
      convert_file( non_mp3[n], dst_path );
   }
}
*/

bool check_convert_to_mp3( metadb_handle_list & p_data, const char * dst_path )
{
   metadb_handle_list non_mp3;

   for ( int n = 0; n < p_data.get_count(); n++ )
   {
      if ( !is_mp3( p_data[n] ) )
      {
         non_mp3.add_item( p_data[n] );
      }
   }

   if ( non_mp3.get_count() > 0 )
   {
      pfc::string8 msg;

      if ( non_mp3.get_count() < 10 )
      {
         msg.set_string( "The following files appear to not be mp3 filess.  Would you like to convert them to mp3?" );
         msg.add_string( "\n\n" );

         for ( int n = 0; n < non_mp3.get_count(); n++ )
         {
            msg.add_string( non_mp3[n]->get_path() );
            msg.add_string( "\n" );
         }
      }
      else
      {
         msg << "The list contains " << non_mp3.get_count() << " files that appear to not be mp3 filess.  Would you like to convert them to mp3?";
         msg.add_string( "\n\n" );
      }

      UINT uiResponse = uMessageBox( core_api::get_main_window(), msg, "foo_sendtodevice", MB_YESNO );

      if ( uiResponse == IDYES )
      {
         for ( int n = 0; n < p_data.get_count(); n++ )
         {
            if ( non_mp3.have_item( p_data[n] ) )
            {
               metadb_handle_ptr tmp_file;
               convert_file( p_data[n], dst_path, tmp_file );               
               p_data.remove_by_idx( n );
               p_data.insert_item( tmp_file, n );
            }
         }

         // popup_message::g_show( "Conversion Complete", "Send To Device" );
         console::printf( "Conversion Complete" );

         return true;
      }
   }
    
   return false;  
}

bool g_send( const pfc::list_base_const_t<metadb_handle_ptr> & p_data, bool use_default = false )
{
   //////////////////////
   /*
   static_api_ptr_t<audio_postprocessor> post_process;
   abort_callback_impl abort;
   audio_chunk_impl chunk;
   mem_block_container_impl pcm8;
   mem_block_container_impl pcm16;
   input_helper in;

   in.open( NULL, p_data[0], input_flag_no_seeking|input_flag_no_looping, abort );

   FILE * f8 = fopen( "C:\\tmp\\sendtodevice8.raw", "wb" );
   FILE * f16 = fopen( "C:\\tmp\\sendtodevice16.raw", "wb" );

   int display = true;

   unsigned bytes8 = 0;
   unsigned bytes16 = 0;

   while ( in.run( chunk, abort ) && ( abort.is_aborting() == false ) )
   {
      if ( display )
      {
         console::printf( "channels: %d", chunk.get_channels() );
         console::printf( "fs: %d", chunk.get_sample_rate() );
         console::printf( "sample count: %d", chunk.get_sample_count() );
         console::printf( "data length: %d", chunk.get_data_length() );
         console::printf( "endian: %s", chunk.flags_autoendian() == chunk.FLAG_BIG_ENDIAN ? "big" : "little" );
         display = false;
      }

      post_process->run( chunk, pcm8, 8, 8, false, 1.0 );
      bytes8 += fwrite( pcm8.get_ptr(), 1, 1*chunk.get_data_length(), f8 );

      post_process->run( chunk, pcm16, 16, 16, false, 1.0 );
      bytes16 += fwrite( pcm16.get_ptr(), 1, 2*chunk.get_data_length(), f16 );
   }   
   
   console::printf( "Bytes written (8 bit): %u", bytes8 );
   console::printf( "Bytes written (16 bit): %u", bytes16 );

   fclose( f8 );   
   fclose( f16 );   
   /////////////////////
   */
   
   ptr_list_t<string8> paths;

   split( cfg_devices, "\r\n", paths );

   string8 dst_path;
   bool write_m3u = false;

   if ( use_default )
   {
      dst_path.set_string( paths[0]->get_ptr() );
   }
   else
   {
      if ( get_user_device_path( dst_path, write_m3u, paths ) == false )
         return false;
   }

   console::printf( "Send to Device: %s", (const char *)dst_path );

   string8 dst;
   string8 src;

   metadb_handle_list handle_list( p_data );
   metadb_handle_list original_handle_list( p_data );

   if ( cfg_ask_to_convert )
   {
      check_convert_to_mp3( handle_list, dst_path );
   }

   dst = handle_list_to_formatted_destinations( dst_path, handle_list );
   src = handle_list_to_file_list( handle_list );
   copy_files( src, dst );

   if ( cfg_copy_cover ){
	   string8 origsrc = handle_list_to_file_list( original_handle_list );
	   copy_cover_art( origsrc, dst );
   }

   if ( write_m3u || cfg_run_script )
   {
      metadb_handle_list out_handles;
      get_exported_handles( dst, out_handles );

      if ( write_m3u )
      {
         standard_commands::context_save_playlist( out_handles );
      }

      if ( cfg_run_script )
      {
         metadb_handle_list out_handles;
         get_exported_handles( dst, out_handles );

         bool b1 = menu_helpers::run_command_context(
            cfg_send_script_guid_command, cfg_send_script_guid_subcommand, out_handles );
      }
   }

   // NOT handle_list -> WE WANT ORIGINAL FILES, not TEMP files in case of conversion
   stamp_files( p_data );

   if ( g_temp_files.get_count() > 0 )
   {
      console::printf( "Deleting temporary files: ");
      for ( int n = 0; n < g_temp_files.get_count(); n++ )
      {
         console::printf( "   %s", g_temp_files[n].get_ptr() );
         uDeleteFile( g_temp_files[n] );
      }
      g_temp_files.remove_all();
   }
   return true;
}

class sendtodevice_context : public contextmenu_item_simple
{
   unsigned get_num_items()
   {
      return 2;
   }

   GUID get_item_guid( unsigned int n )
   {
      switch ( n )
      {
      case 1:
         return guid001;
      case 0:
         return guid002;
      }
      return pfc::guid_null;
   }
   
   void get_item_name( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case 1:
         out.set_string( "Select..." );
         break;
      case 0:
         out.set_string( "Default" );
         break;
      }
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case 0:
      case 1:
         out.set_string( "Send to Device" );
         break;
      }
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case 1:
         out.set_string( "Easy copy to device.  Optionally allows export of m3u" );
         return true;
      case 0:
         out.set_string( "Easy copy to device" );
         return true;
      }

      return false;
   }

   virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      switch ( p_index )
      {
      case 1:
         g_send( p_data );
         break;
      case 0:
         g_send( p_data, true );
         break;
      }
   }
};

static service_factory_single_t<sendtodevice_context> g_sendtodevice;

bool_map bool_var_map[] = 
{
   { IDC_CHECK_TAG_FILES, &cfg_tagfiles },
   { IDC_CHECK_RUN_SCRIPT, &cfg_run_script },
   { IDC_CHECK_PROMPT_MP3, &cfg_ask_to_convert },
   { IDC_COPY_COVER, &cfg_copy_cover },
};

text_box_map text_var_map[] =
{
   { IDC_EDIT_DEVICES, &cfg_devices },
   { IDC_LAME, &cfg_lame_command_line },
   { IDC_COVER_FILENAME, &cfg_cover_filename },
};

script_map context_var_map[] =
{
   { IDC_COMBO_SCRIPT, &cfg_post_send_script, &cfg_send_script_guid_command, &cfg_send_script_guid_subcommand },
};

class sendtodevice_pref : public preferences_page, public cwb_menu_helpers
{
   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         setup_bool_maps( wnd, bool_var_map, tabsize(bool_var_map) );       
         setup_text_boxes( wnd, text_var_map, tabsize(text_var_map) );
         setup_script_maps( wnd, context_var_map, tabsize(context_var_map) );

         EnableWindow( GetDlgItem( wnd, IDC_COMBO_SCRIPT ), cfg_run_script ? TRUE : FALSE );
         break;

		case WM_COMMAND:
         if ( process_bool_map( wnd, wp, bool_var_map, tabsize(bool_var_map) ) )
         {
            EnableWindow( GetDlgItem( wnd, IDC_COMBO_SCRIPT ), cfg_run_script ? TRUE : FALSE );
         }

         process_script_map( wnd, wp, context_var_map, tabsize(context_var_map) );
         process_text_boxes( wnd, wp, text_var_map, tabsize(text_var_map) );
         break;

      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_PREFERENCES, parent, dialog_proc );
   }

   //! Retrieves name of the prefernces page to be displayed in preferences tree (static string).
	virtual const char * get_name()
   {
      return "Send To Device";
   }
	
   //! Retrieves GUID of the page.
	virtual GUID get_guid()
   {
      static const GUID s_pref_guid = 
      { 0x37323c13, 0xe4ae, 0x488e, { 0x89, 0xd6, 0x21, 0x19, 0xe6, 0xe7, 0x2c, 0x4a } };

      return s_pref_guid;
   }
	
   //! Retrieves GUID of parent page/branch of this page. See preferences_page::guid_* constants for list of standard parent GUIDs. Can also be a GUID of another page or a branch (see: preferences_branch).
	virtual GUID get_parent_guid()
   {
      return preferences_page::guid_tools;
   }
	
   //! Queries whether this page supports "reset page" feature.
	virtual bool reset_query()
   {
      return false;
   }
	
   //! Activates "reset page" feature. It is safe to assume that the preferences page dialog does not exist at the point this is called (caller destroys it before calling reset and creates it again afterwards).
	virtual void reset()
   {
   }
	
   //! Retrieves help URL. Without overriding it, it will redirect to foobar2000 wiki.
	virtual bool get_help_url(pfc::string_base & p_out)
   {
      p_out.set_string( URL_HELP );
      return true;
   }
};




static service_factory_single_t<sendtodevice_pref> g_pref;


/** 
Changelog

2006-03-23 
   changed "dialog" to Send to Device in Dialog Box
   changed "send to device" to "send to device..." in context menu
   add ".$ext(%_path%)" to format string if no period is found

2006-03-21 Initial Release
*/