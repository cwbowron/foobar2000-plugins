#define VERSION         ("1.2.6 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4996)
#pragma warning(disable:4312)
#pragma warning(disable:4244)

#include <windows.h>
#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "resource.h"
#include "../common/string_functions.h"

#include "timestamp_action.h"

#define URL_HELP                 "http://wiki.bowron.us/index.php/Foobar2000"

#define TAGZ_ARTIST  "%artist%"
#define TAGZ_TITLE   "%title%"
#define TAG_RATING   "RATING"

DECLARE_COMPONENT_VERSION( "cwbowron's title format hooks", VERSION, 
   "useful title formatting functions\n"
   "By Christopher Bowron <chris@bowron.us>\n"
   "http://wiki.bowron.us/index.php/Foobar2000" )

// config strings
static const GUID guid_var_001 = { 0xde1a9ed6, 0x5670, 0x4505, { 0xbc, 0xc8, 0xa2, 0x87, 0xaa, 0x3a, 0x83, 0xc7 } };
static const GUID guid_var_002 = { 0xc6cfc324, 0x7f1c, 0x42a7, { 0xbe, 0x6, 0x52, 0x55, 0xed, 0x63, 0xe1, 0x2f } };
static const GUID guid_var_003 = { 0x36dc978c, 0xf2f0, 0x479e, { 0xb4, 0x77, 0xa8, 0x83, 0xae, 0x8a, 0x41, 0x5f } };
static const GUID guid_var_004 = { 0x48e6177b, 0x83c5, 0x4216, { 0xa1, 0x62, 0x65, 0x47, 0xb0, 0x7, 0xc3, 0x85 } };
static const GUID guid_var_005 = { 0xbfeed7c4, 0x238e, 0x47c9, { 0xa9, 0x3e, 0xac, 0x2c, 0xfb, 0x33, 0x9c, 0xd8 } };
static const GUID guid_var_006 = { 0x3d1bee52, 0x1d27, 0x441c, { 0x81, 0x2a, 0x3a, 0x53, 0x93, 0xf7, 0xa8, 0x81 } };
static const GUID guid_var_007 = { 0x5e7529e5, 0x68af, 0x4a2b, { 0xbf, 0x4b, 0x1c, 0x28, 0x99, 0xe6, 0xa2, 0x4f } };

static cfg_string cfg_userdef_1( guid_var_001, "%album%" );
static cfg_string cfg_userdef_2( guid_var_002, "%path%" );
static cfg_int cfg_notify( guid_var_003, 1 );

static cfg_int    cfg_skip_tag( guid_var_004, 0 );
static cfg_string cfg_skip_tag_action( guid_var_005, "" );
static cfg_guid   cfg_skip_tag_command( guid_var_006, pfc::guid_null );
static cfg_guid   cfg_skip_tag_subcommand( guid_var_007, pfc::guid_null );

// End config strings

// titleformtting related variables
pfc::list_t<metadb_handle_ptr> g_queue_handles;
pfc::string8 g_active_playlist;
pfc::string8 g_playing_playlist;
pfc::string8 g_queue_lastitem_pl;
pfc::string8 g_next_title;
pfc::string8 g_next_artist;
pfc::string8 g_playback_order;
pfc::string8 g_next_user1;
pfc::string8 g_next_user2;
pfc::string8 g_volume;
pfc::string8 g_selection_duration;
pfc::string8 g_activelist_duration;
pfc::string8 g_playinglist_duration;
pfc::string8 g_playback_state;

int g_active_playlist_count;
int g_playing_playlist_count;
int g_selection_count = 0;
int g_queue_count;
int g_playing_index = 0;

bool g_followcursor = false;
bool g_stopaftercurrent = false;
// end title formatting related variables

bool g_allow_mainthread_callbacks = false;

struct { 
   const char * str; 
   bool * data; 
} g_bools[] = 
{
   { "cwb_followcursor", &g_followcursor },
   { "cwb_stopaftercurrent", &g_stopaftercurrent },
};

struct {
   const char * str;
   pfc::string8 * data;
} g_strings[] =
{
   { "cwb_activelist", &g_active_playlist },
   { "cwb_playinglist", &g_playing_playlist },
   { "cwb_queue_end_playlist", &g_queue_lastitem_pl },
   { "cwb_next_title", &g_next_title },
   { "cwb_next_artist", &g_next_artist },
   { "cwb_playback_order", &g_playback_order },
   { "cwb_next_user1", &g_next_user1 },
   { "cwb_next_user2", &g_next_user2 },
   { "cwb_volume", &g_volume },
   { "cwb_selection_duration", &g_selection_duration },
   { "cwb_activelist_duration", &g_activelist_duration },
   { "cwb_playinglist_duration", &g_playinglist_duration },
   { "cwb_playback_state", &g_playback_state },
};

struct {
   const char * str;
   int * data;
} g_ints[] =
{
   { "cwb_activelist_count", &g_active_playlist_count },
   { "cwb_playinglist_count", &g_playing_playlist_count },
   { "cwb_queuelength", &g_queue_count },
   { "cwb_playing_index", &g_playing_index },
   { "cwb_selection_count", &g_selection_count }
};

class NOVTABLE titleformatting_variable_callback : public service_base 
{
public:
   virtual void on_var_change( const char * var ) = 0;

   FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(titleformatting_variable_callback);
};

const GUID titleformatting_variable_callback::class_guid = 
{ 0xfcb81645, 0xe7b2, 0x4bc1, { 0x92, 0xb0, 0xf6, 0xfb, 0x4b, 0x43, 0x70, 0xe3 } };

/*
class sample_callback : public titleformatting_variable_callback
{
public:
   void on_var_change( const char * var )
   {
      console::printf( "sample_callback: %s", var );
   }
};

service_factory_single_t<sample_callback> g_sample_callback;
*/

void set_float_string( pfc::string8 & str, double value )
{
   static char strVolume[20];
   sprintf( strVolume, "%0.2f", value );
   str.set_string( strVolume );
}

class var_notify_callback : public main_thread_callback
{
   pfc::string8_fastalloc m_var;

public:
   var_notify_callback( const char * var ) : main_thread_callback()
   {
      m_var = var;
   }

   void callback_run()
   {
      service_enum_t<titleformatting_variable_callback> e; 
      service_ptr_t<titleformatting_variable_callback> ptr; 
      while( e.next( ptr ) ) 
      {
         ptr->on_var_change( m_var );
      }
   }
};

/*
class cb : public main_thread_callback
{
public:
   void callback_run() 
   {
      uMessageBox( NULL, "poop", "poop", MB_OK );
   }
};
*/

void post_callback_notification( const char * str )
{
   if ( cfg_notify )
   {
      if ( core_api::are_services_available() && !core_api::is_initializing() && !core_api::is_shutting_down() )
      {
         service_ptr_t<main_thread_callback> p_callback = new service_impl_t<var_notify_callback>( str );
         static_api_ptr_t<main_thread_callback_manager> man;
         man->add_callback( p_callback );
      }
   }
}

class next_callback : public main_thread_callback
{
public:
   void callback_run() 
   {
      static_api_ptr_t<playlist_manager> pm;
      t_size playlist;
      t_size index;

      g_next_title.reset();
      g_next_artist.reset();

      metadb_handle_ptr h;
     
      if ( g_queue_handles.get_count() > 0 )
      {
         h = g_queue_handles[0];
      }
      else
      {
         t_size active = pm->get_active_playlist();

         if ( active == infinite ) 
         {
            return;
         }

         bool is_playing = pm->get_playing_item_location( &playlist, &index );

         if ( !is_playing )
         {
            g_playing_index = 0;
            return;
         }

         g_playing_index = index + 1;

         if ( g_followcursor )
         {
            if ( pm->playlist_get_focus_item( active ) != index )
            {
               pm->playlist_get_focus_item_handle( h, active );
               goto skiptomylou;
            }
         }

         index++;
         if ( g_playing_playlist_count > index )
         {
            pm->playlist_get_item_handle( h, playlist, index );
         }
      }

skiptomylou:;

      if ( h.is_valid() )
      {        
         h->format_title_legacy( NULL, g_next_title, TAGZ_TITLE, NULL );
         h->format_title_legacy( NULL, g_next_artist, TAGZ_ARTIST, NULL );                       
         //post_callback_notification( "cwb_next_artist" );         
         //post_callback_notification( "cwb_next_title" );

         if ( !cfg_userdef_1.is_empty() )
         {
            h->format_title_legacy( NULL, g_next_user1, cfg_userdef_1, NULL );           
            //post_callback_notification( "cwb_next_user1" );
         }

         if ( !cfg_userdef_2.is_empty() )
         {
            h->format_title_legacy( NULL, g_next_user2, cfg_userdef_2, NULL );
            //post_callback_notification( "cwb_next_user2" );
         }         
         post_callback_notification( "cwb_next_" );
      }
   }
};

void post_next_callback()
{
   if ( core_api::are_services_available() && !core_api::is_initializing() && !core_api::is_shutting_down() )
   {
      static_api_ptr_t<main_thread_callback_manager> man;
      service_ptr_t<next_callback> myc = new service_impl_t<next_callback>();
      man->add_callback( myc );
   }
}

int parse_timestamp( const char * date, SYSTEMTIME * time )
{
   ////////////////////////////////           1
   //////////////////////////////// 0123456789012345678
   if ( strlen( date ) >= strlen ( "yyyy-mm-dd hh:mm:ss" ) )
   {
      time->wYear    = atoi( date );
      time->wMonth   = atoi( date + 5 );
      time->wDay     = atoi( date + 8 );
      time->wHour    = atoi( date + 11 );
      time->wMinute  = atoi( date + 14 );
      time->wSecond  = atoi( date + 17 );
      return 2;
   }
   else if ( strlen( date ) >= strlen ( "yyyy-mm-dd") )
   {
      time->wYear    = atoi( date );
      time->wMonth   = atoi( date + 5 );
      time->wDay     = atoi( date + 8 );
      return 1;
   }
   else
   {
      return 0;
   }
}

int time_diff( const SYSTEMTIME * t1, const SYSTEMTIME * t2 )
{
   FILETIME ft1;
   FILETIME ft2;
   ULARGE_INTEGER ul1;
   ULARGE_INTEGER ul2;

   SystemTimeToFileTime( t1, &ft1 );
   SystemTimeToFileTime( t2, &ft2 );

   memcpy( &ul1, &ft1, sizeof( ULARGE_INTEGER ) );
   memcpy( &ul2, &ft2, sizeof( ULARGE_INTEGER ) );

   double elapsed;
   if ( ul1.QuadPart >= ul2.QuadPart )
   {
      elapsed = ul1.QuadPart - ul2.QuadPart;                  
   }
   else
   {
      elapsed = ul2.QuadPart - ul1.QuadPart;                  
   }

   double seconds = ( elapsed / 10000000 );
   double hours   = (seconds / 60) / 60;
   double days    = (hours / 24);

   return (int)days*24 + (int)hours;
}

void parse_http_string( pfc::string_base & out, const char * in, int len )
{
   for ( int i = 0; i < len; i++ )
   {
      if ( in[i] == '%' )
      {
         pfc::string8_fastalloc fast( in + i + 1, 2 );
         int n = strtol( fast, NULL, 16 );
         out.add_byte( n );
         i += 2;
      }
      else
      {
         out.add_byte( in[i] );
      }
   }
}


class http_clean_hook : public metadb_display_hook
{
   virtual bool process_field(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) 
   {  
      // All my stuff starts with cwb_ - check that first
      if ( p_name_length < 4 )
      {
         return false;
      }
      if ( stricmp_utf8_ex( p_name, 4, "cwb_", 4 ) != 0 )
      {
         return false;
      }

      for ( int n = 0; n < tabsize(g_bools); n++ )
      {
         if ( stricmp_utf8_ex( p_name, p_name_length, g_bools[n].str, ~0 ) == 0 )
         {
            if ( *(g_bools[n].data) )
            {
               p_found_flag = true;
               p_out->write( titleformat_inputtypes::unknown, "1", ~0 );
            }
            return true;
         }
      }

      for ( int n = 0; n < tabsize(g_ints); n++ )
      {
         if ( stricmp_utf8_ex( p_name, p_name_length, g_ints[n].str, ~0 ) == 0 ) 
         {
			   pfc::string_printf str( "%d", *g_ints[n].data );
			   p_out->write( titleformat_inputtypes::unknown , str, ~0 );		   
            p_found_flag = true;
            return true;
         }
      }

      for ( int n = 0; n < tabsize(g_strings); n++ )
      {
         if ( stricmp_utf8_ex( p_name, p_name_length, g_strings[n].str, ~0 ) == 0 )
         {
            p_out->write( titleformat_inputtypes::unknown , g_strings[n].data->get_ptr(), ~0 );		   
            p_found_flag = true;
            return true;
         }
      }

      if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_created", ~0 ) == 0 )
      {
         pfc::string8 filename;
         FILETIME creation;
         SYSTEMTIME stUTC, stLocal;

         if ( strstr( p_handle->get_path(), "file://") )
         {
            filesystem::g_get_display_path( p_handle->get_path(), filename );

            HANDLE file_handle = uCreateFile( filename, FILE_READ_ATTRIBUTES,
               FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

            if ( file_handle != INVALID_HANDLE_VALUE )
            {
               GetFileTime( file_handle, &creation, NULL, NULL );
               CloseHandle( file_handle );
             
               // Convert the last-write time to local time.         
               FileTimeToSystemTime( &creation, &stUTC );
               SystemTimeToTzSpecificLocalTime( NULL, &stUTC, &stLocal );

               static CHAR date_string[30];
         
               sprintf(date_string, "%d-%02d-%02d %02d:%02d:%02d", stLocal.wYear, stLocal.wMonth, stLocal.wDay,
	               stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
         
               p_out->write( titleformat_inputtypes::unknown , date_string, ~0 );		   
               p_found_flag = true;
               return true;
            }
         }

         p_found_flag = true;
         return false;
      }
	   else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_systemdate", ~0 ) == 0 )
	   {
		   static CHAR date_string[10];
		   SYSTEMTIME st;
		   GetLocalTime(&st);
		   sprintf(date_string, "%d-%02d-%02d", st.wYear, st.wMonth, st.wDay);

         p_out->write( titleformat_inputtypes::unknown, date_string, ~0 );

         p_found_flag = true;
		   return true;
	   }
	   else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_systemdatetime", ~0 ) == 0 )
	   {
		   static CHAR date_string[30];
		   SYSTEMTIME st;
		   GetLocalTime(&st);
		   sprintf(date_string, "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay,
			   st.wHour, st.wMinute, st.wSecond);

         p_out->write( titleformat_inputtypes::unknown , date_string, ~0 );
		   
         p_found_flag = true;
         return true;
	   }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_queueindex", ~0 ) == 0 )
      {
         int q_index = -1;
         for ( unsigned n = 0; n < g_queue_handles.get_count(); n++ )
         {
            if ( g_queue_handles[n] == p_handle )
            {
               q_index = n;
               break;
            }
         }

         if ( q_index >= 0 )
         {
            pfc::string_printf fmt( "%d", q_index + 1 );
            p_out->write( titleformat_inputtypes::unknown , fmt, ~0 );
            p_found_flag = true;
            return true;
         }
         p_found_flag = false;
         return true;
      }	 
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_queueindexes", ~0 ) == 0 )
      {
         bool found = false;
         pfc::string8 str;
         for ( unsigned n = 0; n < g_queue_handles.get_count(); n++ )
         {
            if ( g_queue_handles[n] == p_handle )
            {
               if (found)
               {
                  str += pfc::string_printf( ",%d", n + 1);
               }
               else
               {
                  str = pfc::string_printf( "%d", n + 1);
                  found = true;
               }
            }
         }

         if ( found )
         {
            p_out->write( titleformat_inputtypes::unknown , str, ~0 );
            p_found_flag = true;

            return true;
         }

         p_found_flag = false;
         return true;
      }
      return false;
   }

	virtual bool process_function(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
   {
      // All my stuff starts with cwb_ - check that first
      if ( p_name_length < 4 )
      {
         return false;
      }

#define HOTNESS_FN   "tdj_hotness"

      if ( stricmp_utf8_ex( p_name, p_name_length, HOTNESS_FN, ~0 ) == 0 )
      {
         p_found_flag = true;
         
         if ( p_params->get_param_count() < 4 ) 
            return false;

         const char * str_last;
         const char * str_first;
         const char * str_rating;
         const char * str_playcount;
         t_size str_last_len;
         t_size str_first_len;
         t_size str_rating_len;
         t_size str_playcount_len;

         p_params->get_param( 0, str_first, str_first_len );
         p_params->get_param( 1, str_last, str_last_len );
         p_params->get_param( 2, str_rating, str_rating_len );
         p_params->get_param( 3, str_playcount, str_playcount_len );
         // PARAMETERS: LAST PLAYED, FIRST PLAYED, RATING, PLAYCOUNT
         // OPTIONAL PARAMETERS: rating_baseline (4), decay_baseline (5), freq_baseline (6)...

         pfc::string8 s8_last( str_last, str_last_len );
         pfc::string8 s8_first( str_first, str_first_len );
         pfc::string8 s8_rating( str_rating, str_rating_len );
         pfc::string8 s8_playcount( str_playcount, str_playcount_len );

         int n_hotness  = -1;

         if ( !strstr( s8_last, "N/A" ) && !strstr( s8_first, "N/A" ) )
         {
            /*
            console::printf( "LAST_PLAYED:  %s", s8_last.get_ptr() );
            console::printf( "FIRST_PLAYED: %s", s8_first.get_ptr() );
            */

            SYSTEMTIME time_last, time_first, time_now;
            parse_timestamp( s8_last, &time_last );
            parse_timestamp( s8_first, &time_first );
            GetLocalTime( &time_now );

            int diff_first_last = time_diff( &time_last, &time_first );
            int diff_now_last = time_diff( &time_now, &time_last );

            // console::printf( "FIRST - LAST: %d", diff_first_last );
            // console::printf( "LAST - NOW:   %d", diff_now_last );

            int rating_baseline  = 3;
            int decay_baseline   = 672;
            int freq_baseline    = 672;

            int n_param = p_params->get_param_count();

            if ( n_param >= 5 )
            {
               rating_baseline = p_params->get_param_uint( 4 );

               if ( n_param >= 6 )
               {
                  decay_baseline = p_params->get_param_uint( 5 );

                  if ( n_param >= 7 )
                  {
                     freq_baseline = p_params->get_param_uint( 6 );
                  }
               }
            }


            int n_rating      = rating_baseline;
            int n_playcount   = 1;

            if ( !strstr( s8_rating, "?" ) )
            {
               n_rating = atoi( s8_rating );
            }

            if ( !strstr( s8_playcount, "?" ) )
            {
               n_playcount = atoi( s8_playcount );
            }

            double decay_num        = n_playcount * freq_baseline * decay_baseline * n_rating * 100.0;
            double decay_den        = max( diff_first_last, freq_baseline ) * rating_baseline;
            double decay            = decay_num / decay_den / 100.0;
            
            double raw_hotness      = max( decay - diff_now_last, 0 ) * 100 / decay;

            double t1               = max( decay_baseline - diff_now_last, 0 );
            double fc_h             = max( decay - ( t1 / 2 ) - diff_now_last, 0 );
            double forecast_hotness = fc_h * 100 / decay; 

            /*
            char tmp[80];
            sprintf( tmp, "raw_hotness: %f", raw_hotness );
            console::print( tmp );
            sprintf( tmp, "fcst_hotness: %f", forecast_hotness );
            console::print( tmp );
            */

            double hotness = (raw_hotness + forecast_hotness ) / 2;
            n_hotness  = (int) hotness;              
         }

         char str_hotness[20];
         sprintf( str_hotness, "%d", n_hotness );
         // pfc::string_printf s8_hotness( "%d", n_hotness );
         p_out->write( titleformat_inputtypes::unknown, str_hotness );
         // console::printf( "%d", n_hotness );
         return true;
      }

      if ( stricmp_utf8_ex( p_name, 4, "cwb_", 4 ) != 0 )
      {
         return false;
      }

      if ( ( stricmp_utf8_ex( p_name, p_name_length, "cwb_http_clean", ~0 ) ==  0 ) || 
         ( stricmp_utf8_ex( p_name, p_name_length, "cwb_urldecode", ~0 ) ==  0 ) )
      {
         const char * str;
         t_size str_length;

         p_params->get_param( 0, str, str_length );

         pfc::string8_fastalloc tmp;
         parse_http_string( tmp, str, str_length );
         p_out->write( titleformat_inputtypes::unknown , tmp, tmp.length() );
         p_found_flag = true;
         return true;
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_removethe", ~0 ) == 0 )
      {
         const char * articles[] = {"a ", "the "};
         const char * str;
         t_size str_length;

         p_params->get_param( 0, str, str_length );
         p_found_flag = true;

         for ( int i = 0; i < tabsize( articles ); i++ )
         {
            int len = strlen( articles[i] );
            if ( str_length >= len )
            {
               if ( stricmp_utf8_ex( str, len, articles[i], ~0 ) == 0 )
               {
                  p_out->write( titleformat_inputtypes::unknown , str + len, str_length - len );              
                  return true;
               }
            }
         }
         
         p_out->write( titleformat_inputtypes::unknown , str, str_length );
         return true;
      }	  
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_wdhms", ~0 ) == 0 )
      {
         const char * str;
         t_size str_length;

         if ( p_params->get_param_count() < 1 )
            return false;

         p_params->get_param( 0, str, str_length );
         p_found_flag = true;

         pfc::string8 s( str, str_length );
         char hms[20];

         double in_d = atof( s );
         int in = (int) in_d;

         int weeks = in / ( 60 * 60 * 24 * 7);
         int days = in / (60 * 60 * 24) % 7;
         int hours = in / ( 60 * 60 ) % 24;
         int minutes = ( in / 60 ) % 60;
         int seconds = in % 60;

         if ( weeks > 0 )
         {
            sprintf( hms, "%dwk %dd %d:%02d:%02d", weeks, days, hours, minutes, seconds );
         }
         else if ( days > 0 )
         {
            sprintf( hms, "%dd %d:%02d:%02d", days, hours, minutes, seconds );
         }
         else if ( hours > 0 )
         {
            sprintf( hms, "%d:%02d:%02d", hours, minutes, seconds );
         }
         else if ( minutes > 0 )
         {
            sprintf( hms, "%02d:%02d", minutes, seconds );         
         }
         else
         {         
            sprintf( hms, "%d", seconds );
         }

         p_out->write( titleformat_inputtypes::unknown , hms, ~0 );
         return true;
      }	  
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_hms", ~0 ) == 0 )
      {
         const char * str;
         t_size str_length;

         if ( p_params->get_param_count() < 1 )
            return false;

         p_params->get_param( 0, str, str_length );
         p_found_flag = true;

         pfc::string8 s( str, str_length );
         char hms[20];

         double in_d = atof( s );
         int in = (int) in_d;

         int hours = in / ( 60 * 60 );
         int minutes = ( in / 60 ) % 60;
         int seconds = in % 60;

         if ( hours > 0 )
         {
            sprintf( hms, "%d:%02d:%02d", hours, minutes, seconds );
         }
         else if ( minutes > 0 )
         {
            sprintf( hms, "%02d:%02d", minutes, seconds );         
         }
         else
         {         
            sprintf( hms, "%d", seconds );
         }

         p_out->write( titleformat_inputtypes::unknown , hms, ~0 );
         return true;
      }	  
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_splitnum", ~0 ) == 0 )
      {
         const char * str;
         t_size str_length;

         if ( p_params->get_param_count() < 0 )
            return false;

         pfc::string8 str_delim = " ";

         if ( p_params->get_param_count() == 2 )
         {
            p_params->get_param( 1, str, str_length );
            str_delim.set_string( str, str_length );
         }

         p_params->get_param( 0, str, str_length );
         p_found_flag = true;

         pfc::string8 str_num( str, str_length );
         pfc::string8 str_out;

         int m = str_num.length();
         for ( int n = 0; n < m; n++ )
         {
            if ( ( n > 0 ) && ( ( (m - n) % 3 ) == 0 ) )
            {
               str_out.add_string( str_delim );
            }
            str_out.add_byte( str_num[n] );
         }

         p_out->write( titleformat_inputtypes::unknown , str_out, ~0 );
         return true;
      }	  
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_ltrim", ~0 ) == 0 )	  
      {
         const char * param;
         t_size param_length;
         const char * str;
         t_size str_length;

         if ( p_params->get_param_count() > 0 )
         {
            p_params->get_param( 0, str, str_length );
            p_found_flag = true;

            for ( int i = 1; i < p_params->get_param_count(); i++ )
            {
               p_params->get_param(i,param,param_length);
               if ( str_length > param_length+1)
               {
                  if ( stricmp_utf8_ex( str, param_length, param, ~0 ) == 0 )
                  {
                     p_out->write( titleformat_inputtypes::unknown , str + param_length, str_length - param_length);
                     return true;
                  }
               }
            }
            p_out->write( titleformat_inputtypes::unknown , str, str_length );
            return true;
         }
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_fileexists", ~0 ) == 0 )
      {
         if ( p_params->get_param_count() > 0 )
         {
            const char * p0;
            t_size p0_length;
            p_params->get_param( 0, p0, p0_length );

            pfc::string8_fastalloc str( p0, p0_length );
            
            if ( uFileExists( str ) )
            {
               p_out->write( titleformat_inputtypes::unknown, "1", ~0 );
               p_found_flag = true;
            }
         }
         return true;
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_rand", ~0 ) == 0 )
      {
         long n = rand();
         pfc::string_printf str_out( "%d", n );
         p_out->write( titleformat_inputtypes::unknown , str_out, ~0 );
         p_found_flag = true;
         return true;
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "cwb_datediff", ~0 ) == 0 )
      {
         const char * date1;
         const char * date2;

         t_size date1_length;
         t_size date2_length;
         
         if ( p_params->get_param_count() == 2 )
         {
            p_params->get_param( 0, date1, date1_length );
            p_params->get_param( 1, date2, date2_length );

            pfc::string8 str_date1( date1, date1_length );
            pfc::string8 str_date2( date2, date2_length );

            SYSTEMTIME time1;
            SYSTEMTIME time2;

            memset( &time1, 0, sizeof(time1) );
            memset( &time2, 0, sizeof(time2) );

            int r1 = parse_timestamp( str_date1, &time1 );
            int r2 = parse_timestamp( str_date2, &time2 );

            if ( r1 && r2 )
            {
               FILETIME ft1;
               FILETIME ft2;
               ULARGE_INTEGER ul1;
               ULARGE_INTEGER ul2;

               // use time information
               if ( r1 == 2 && r2 == 2 )
               {
               }
               // use only date information
               else
               {
                  time1.wHour    = 0;
                  time1.wMinute  = 0;
                  time1.wSecond  = 0;

                  time2.wHour    = 0;
                  time2.wMinute  = 0;
                  time2.wSecond  = 0;
               }

               SystemTimeToFileTime( &time1, &ft1 );
               SystemTimeToFileTime( &time2, &ft2 );

               memcpy( &ul1, &ft1, sizeof( ULARGE_INTEGER ) );
               memcpy( &ul2, &ft2, sizeof( ULARGE_INTEGER ) );
               
               double elapsed;
               if ( ul1.QuadPart >= ul2.QuadPart )
               {
                  elapsed = ul1.QuadPart - ul2.QuadPart;                  
               }
               else
               {
                  elapsed = ul2.QuadPart - ul1.QuadPart;                  
               }

               double seconds = ( elapsed / 10000000 );
               double hours   = (seconds / 60) / 60;
               double days    = (hours / 24);

               pfc::string_printf str_out( "%d", (int) days );
               p_out->write( titleformat_inputtypes::unknown , str_out, ~0 );

               p_found_flag = true;        
               return true;
            }
         }
      }	  
      return false;
   }
};

static service_factory_single_t<http_clean_hook> g_http_clean_hook;

class cwb_hook_queue_callback : public playback_queue_callback 
{
   void on_changed(t_change_origin p_origin)
   {
      metadb_handle_list refresh_list;

      pfc::list_t<t_playback_queue_item> queue_contents;
      static_api_ptr_t<playlist_manager> pm;
      pm->queue_get_contents( queue_contents );
          
      if (queue_contents.get_count() > 0)
      {
         t_size pl_index;
         pl_index = queue_contents[queue_contents.get_count()-1].m_playlist;
         pm->playlist_get_name(pl_index,g_queue_lastitem_pl);
      }
      else
      {
         g_queue_lastitem_pl = "";
      }

      refresh_list.add_items( g_queue_handles );
      g_queue_handles.remove_all();
      for ( int n = 0; n < queue_contents.get_count(); n++ )
      {
         g_queue_handles.add_item( queue_contents[n].m_handle );
      }     
      g_queue_count = g_queue_handles.get_count();
      queue_contents.remove_all();

      refresh_list.add_items( g_queue_handles );     
      static_api_ptr_t<metadb_io> md_io;
      md_io->dispatch_refresh( refresh_list );

      post_next_callback();

      //post_callback_notification( "cwb_queuelength" );
      //post_callback_notification( "cwb_queue_end_playlist" );
      post_callback_notification( "cwb_queue" );
   }
};

static service_factory_single_t<cwb_hook_queue_callback> g_queue_callback;


class woot : public initquit
{
   void on_init() 
   {
      static_api_ptr_t<playlist_manager> pm;
      g_playback_order.set_string( pm->playback_order_get_name( pm->playback_order_get_active() ) );

      config_object::g_get_data_bool( standard_config_objects::bool_playback_follows_cursor, g_followcursor );
      config_object::g_get_data_bool( standard_config_objects::bool_playlist_stop_after_current, g_stopaftercurrent );

      static_api_ptr_t<playback_control> pc;
      static char strVolume[20];
      sprintf( strVolume, "%0.2f", pc->get_volume() );
      g_volume.set_string( strVolume );

      g_playback_state.set_string( "stop" );

      g_allow_mainthread_callbacks = true;     
   }

   void on_quit() 
   {
      g_queue_handles.remove_all();      
   }
};
static service_factory_single_t<woot> g_woot;

class config_callback : public config_object_notify
{
   virtual t_size get_watched_object_count()
   {
      return 2;
   }

	virtual GUID get_watched_object(t_size p_index) 
   {
      switch ( p_index )
      {
      case 0:
         return standard_config_objects::bool_playback_follows_cursor;
      case 1:
         return standard_config_objects::bool_playlist_stop_after_current;
      }      
      return pfc::guid_null;
   }
      
	virtual void on_watched_object_changed(const service_ptr_t<config_object> & p_object)
   {
      if ( p_object->get_guid() == standard_config_objects::bool_playback_follows_cursor )
      {
         p_object->get_data_bool( g_followcursor );
         post_callback_notification( "cwb_followcursor" );
      }
      else if ( p_object->get_guid() == standard_config_objects::bool_playlist_stop_after_current )
      {
         p_object->get_data_bool( g_stopaftercurrent );
         post_callback_notification( "cwb_stopaftercurrent" );
      }
   }
};
static service_factory_single_t<config_callback> g_config_callback;

class cwb_hook_play_callback : public play_callback_static
{
   virtual unsigned get_flags()
   {
      return flag_on_playback_new_track
         | flag_on_volume_change
         | flag_on_playback_stop
         | flag_on_playback_pause
         ;
   }

  	//! Playback process is being initialized. on_playback_new_track() should be called soon after this when first file is successfully opened for decoding.
   virtual void FB2KAPI on_playback_starting(play_control::t_track_command p_command,bool p_paused){}
	//! Playback advanced to new track.
	virtual void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track)
   {
      pfc::string8_fastalloc tmp_string;
      int tmp_int;

      bool b_list_change = false;
      bool b_count_change = false;

      static_api_ptr_t<playlist_manager> pm;

      t_size t_playing = pm->get_playing_playlist();
      pm->playlist_get_name( t_playing, tmp_string );
		tmp_int = pm->playlist_get_item_count( t_playing );

      if ( g_playing_playlist_count != tmp_int )
         b_count_change = true;
      if ( strcmp( g_playing_playlist, tmp_string ) != 0 )
         b_list_change = true;

      g_playing_playlist_count = tmp_int;
      g_playing_playlist.set_string( tmp_string );

     
      post_next_callback();

      //if ( b_list_change ) post_callback_notification( "cwb_playinglist" );
      //if ( b_count_change ) post_callback_notification( "cwb_playinglist_count" );


      metadb_handle_list handles;
      pm->playlist_get_all_items( t_playing, handles );
      set_float_string( g_playinglist_duration, metadb_handle_list_helper::calc_total_duration( handles ) );
      //post_callback_notification( "cwb_playinglist_duration" );

      post_callback_notification( "cwb_playinglist" );

      g_playback_state.set_string( "play" );
      post_callback_notification( "cwb_playback_state" );
   }

	//! Playback stopped
   virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason)
   {
      g_playing_index = 0;
      g_playback_state.set_string( "stop" );
      post_callback_notification( "cwb_playback_state" );
   }

	//! User has seeked to specific time.
	virtual void FB2KAPI on_playback_seek(double p_time){}
	
   //! Called on pause/unpause.
	virtual void FB2KAPI on_playback_pause(bool p_state)
   {
      if ( p_state )
      {
         g_playback_state.set_string( "pause" );
      }
      else
      {
         g_playback_state.set_string( "play" );
      }
      post_callback_notification( "cwb_playback_state" );
   }

	//! Called when currently played file gets edited.
	virtual void FB2KAPI on_playback_edited(metadb_handle_ptr p_track) {}
	//! Dynamic info (VBR bitrate etc) change.
	virtual void FB2KAPI on_playback_dynamic_info(const file_info & p_info) {}
	//! Per-track dynamic info (stream track titles etc) change. Happens less often than on_playback_dynamic_info().
	virtual void FB2KAPI on_playback_dynamic_info_track(const file_info & p_info) {}
	//! Called every second, for time display
	virtual void FB2KAPI on_playback_time(double p_time) {}
	//! User changed volume settings. Possibly called when not playing.
	//! @param p_new_val new volume level in dB; 0 for full volume.
	virtual void FB2KAPI on_volume_change(float p_new_val) 
   {
      static char strVolume[20];
      sprintf( strVolume, "%0.2f", p_new_val );
      g_volume.set_string( strVolume );
      post_callback_notification( "cwb_volume" );
   }

};

static service_factory_single_t<cwb_hook_play_callback> g_cwb_hook_play_callback;


class cwb_hooks_playlist_callback : public playlist_callback_static
{
public:
   cwb_hooks_playlist_callback() 
   {
   }

   ~cwb_hooks_playlist_callback()
   {
   }

   void on_playlist_operation( t_size p_playlist )
   {
      post_next_callback();

      static_api_ptr_t<playlist_manager> pm;

      if ( p_playlist == pm->get_active_playlist() )
      {
         pm->activeplaylist_get_name( g_active_playlist );
         g_active_playlist_count = pm->activeplaylist_get_item_count();

         metadb_handle_list handles;
         pm->activeplaylist_get_all_items( handles );
         set_float_string( g_activelist_duration, metadb_handle_list_helper::calc_total_duration( handles ) );

         post_callback_notification( "cwb_activelist" );
      }
   }


   virtual unsigned get_flags()
   {
      return flag_on_playlist_activate 
         | flag_on_playback_order_changed
         | flag_on_items_reordered
         | flag_on_items_added
         | flag_on_items_removed
         | flag_on_item_focus_change
         | flag_on_items_selection_change
         ;
   }

	virtual void FB2KAPI on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)      
   {
      on_playlist_operation( p_playlist );
   }
	
   virtual void FB2KAPI on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count)
   {
      on_playlist_operation( p_playlist );
   }

	virtual void FB2KAPI on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
   {
      on_playlist_operation( p_playlist );
   }

	virtual void FB2KAPI on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state)
   {
      metadb_handle_list handles;
      static_api_ptr_t<playlist_manager> pm;
      pm->playlist_get_selected_items( p_playlist, handles );
      set_float_string( g_selection_duration, metadb_handle_list_helper::calc_total_duration( handles ) );
      g_selection_count = handles.get_count();
      post_callback_notification( "cwb_selection_duration" );
   }

	virtual void FB2KAPI on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to)
   {
      if ( g_followcursor )
      {
         post_next_callback();
      }
   }
	
	virtual void FB2KAPI on_playlist_activate(t_size p_old,t_size p_new)
   {
      static_api_ptr_t<playlist_manager> pm;
      pm->activeplaylist_get_name( g_active_playlist );
      g_active_playlist_count = pm->activeplaylist_get_item_count();

      metadb_handle_list handles;
      pm->activeplaylist_get_all_items( handles );
      set_float_string( g_activelist_duration, metadb_handle_list_helper::calc_total_duration( handles ) );

      post_next_callback();
      post_callback_notification( "cwb_activelist" );
   }

   virtual void FB2KAPI on_playback_order_changed(t_size p_new_index)
   {
      static_api_ptr_t<playlist_manager> pm;
      g_playback_order.set_string( pm->playback_order_get_name( p_new_index ) );

      post_callback_notification( "cwb_playback_order" );
   }

	virtual void FB2KAPI on_items_removing(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
   virtual void FB2KAPI on_items_modified(t_size p_playlist,const bit_array & p_mask){}
	virtual void FB2KAPI on_items_modified_fromplayback(t_size p_playlist,const bit_array & p_mask,play_control::t_display_level p_level){}
	virtual void FB2KAPI on_items_replaced(t_size p_playlist,const bit_array & p_mask,const pfc::list_base_const_t<t_on_items_replaced_entry> & p_data){}
	virtual void FB2KAPI on_item_ensure_visible(t_size p_playlist,t_size p_idx){}
	virtual void FB2KAPI on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len){}
	virtual void FB2KAPI on_playlists_reorder(const t_size * p_order,t_size p_count){}
	virtual void FB2KAPI on_playlists_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	virtual void FB2KAPI on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	virtual void FB2KAPI on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len){}
   virtual void FB2KAPI on_default_format_changed() {}
	virtual void FB2KAPI on_playlist_locked(t_size p_playlist,bool p_locked){}
};

static service_factory_single_t<cwb_hooks_playlist_callback> g_cwb_hooks_playlist_callback;




// PREFERENCES /////////////////////////////////////
#include "../common/menu_stuff.h"

static const GUID guid_preferences = { 0x12a1ea80, 0xd89b, 0x42e4, { 0xad, 0xfa, 0x6d, 0x0, 0x60, 0x2a, 0x4b, 0x95 } };

bool_map bool_var_map[] = 
{
   { IDC_CHECK_NOTIFY, &cfg_notify },
   { IDC_SKIP_TAG, &cfg_skip_tag },
};

text_box_map var_map[] = 
{
   { IDC_EDIT_NEXT_USER1, &cfg_userdef_1 },
   { IDC_EDIT_NEXT_USER2, &cfg_userdef_2 },
};

script_map context_skip_tag_map[] =
{
   { IDC_SKIP_TAG_ACTION, &cfg_skip_tag_action, &cfg_skip_tag_command, &cfg_skip_tag_subcommand },
};

class hook_preferences : public preferences_page, public cwb_menu_helpers
{  
   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         setup_text_boxes( wnd, var_map, tabsize(var_map) );
         setup_bool_maps( wnd, bool_var_map, tabsize(bool_var_map) );           
         setup_script_maps( wnd, context_skip_tag_map, tabsize(context_skip_tag_map) );

         EnableWindow( GetDlgItem( wnd, IDC_SKIP_TAG_ACTION ), cfg_skip_tag ? TRUE : FALSE );
         break;

      case WM_COMMAND:
         process_text_boxes( wnd, wp, var_map, tabsize(var_map) );
         process_script_map( wnd, wp, context_skip_tag_map, tabsize(context_skip_tag_map) );

         if ( process_bool_map( wnd, wp, bool_var_map, tabsize(bool_var_map) ) )
         {
            EnableWindow( GetDlgItem( wnd, IDC_SKIP_TAG_ACTION ), cfg_skip_tag ? TRUE : FALSE );
         }
         break;
      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_PREFERENCES, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "CWB Hooks";
   }

   virtual GUID get_guid()
   {
      return guid_preferences;
   }

   virtual GUID get_parent_guid()
   {
      return preferences_page::guid_tools;
   }
	
   //! Queries whether this page supports "reset page" feature.
	virtual bool reset_query()
   {
      return false;
   }

   virtual void reset() 
   {
   }

   virtual bool get_help_url(pfc::string_base & p_out)
   {
      p_out.set_string( URL_HELP );
      return true;
   }
};




static service_factory_single_t<hook_preferences> g_pref;


//// SKIP TAGGER STUFF
class skip_tag_callback : public main_thread_callback
{   
protected:
   metadb_handle_ptr m_handle;

public:

   skip_tag_callback( metadb_handle_ptr handle )
   {
      m_handle = handle;
   }

   virtual void callback_run()
   {
      if ( m_handle.is_valid() )
      {
         if ( cfg_skip_tag )
         {
            metadb_handle_list list;
            list.add_item( m_handle );
            console::printf( "Processing skipped file with %s", cfg_skip_tag_action.get_ptr() );
            menu_helpers::run_command_context( cfg_skip_tag_command, cfg_skip_tag_subcommand, list );
         }
      }
   }
};


class cwb_skip_callback : public play_callback_static
{
public:
   metadb_handle_ptr m_last_handle;
   
   virtual unsigned get_flags()
   {
      return flag_on_playback_starting 
            | flag_on_playback_new_track
            | flag_on_playback_stop;
   }

  	//! Playback process is being initialized. on_playback_new_track() should be called soon after this when first file is successfully opened for decoding.
   virtual void FB2KAPI on_playback_starting(play_control::t_track_command p_command,bool p_paused)
   {
      switch ( p_command )
      {
      case play_control::track_command_rand:
      case play_control::track_command_next:
         if ( m_last_handle.is_valid() )
         {
            console::printf( "Skipping %s", m_last_handle->get_path() );
            service_ptr_t<skip_tag_callback> p_callback = 
               new service_impl_t<skip_tag_callback>( m_last_handle );
            static_api_ptr_t<main_thread_callback_manager> man;
            man->add_callback( p_callback );
         }
         break;                       
      }
   }

	//! Playback advanced to new track.
	virtual void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track)
   {
      m_last_handle = p_track;
   }

	//! Playback stopped
   virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason)
   {
      if ( p_reason != play_control::stop_reason_starting_another )
      {
         m_last_handle = NULL;
      }
   }

	//! User has seeked to specific time.
	virtual void FB2KAPI on_playback_seek(double p_time){}
	
   //! Called on pause/unpause.
	virtual void FB2KAPI on_playback_pause(bool p_state)
   {
   }

	//! Called when currently played file gets edited.
	virtual void FB2KAPI on_playback_edited(metadb_handle_ptr p_track) {}
	//! Dynamic info (VBR bitrate etc) change.
	virtual void FB2KAPI on_playback_dynamic_info(const file_info & p_info) {}
	//! Per-track dynamic info (stream track titles etc) change. Happens less often than on_playback_dynamic_info().
	virtual void FB2KAPI on_playback_dynamic_info_track(const file_info & p_info) {}
	//! Called every second, for time display
	virtual void FB2KAPI on_playback_time(double p_time) {}
	//! User changed volume settings. Possibly called when not playing.
	//! @param p_new_val new volume level in dB; 0 for full volume.
	virtual void FB2KAPI on_volume_change(float p_new_val) 
   {
   }
};

static service_factory_single_t<cwb_skip_callback> g_cwb_skip_callback;
