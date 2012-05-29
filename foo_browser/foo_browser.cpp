#define VERSION ("1.2.3b [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4018) // don't warn about signed/unsigned
#pragma warning(disable:4065) // don't warn about default case 
#pragma warning(disable:4800) // don't warn about forcing int to bool
#pragma warning(disable:4244) // disable warning convert from t_size to int
#pragma warning(disable:4267) 
#pragma warning(disable:4995)


#define MY_EDGE_SUNKEN     (WS_EX_CLIENTEDGE)
#define MY_EDGE_GREY       (WS_EX_STATICEDGE)
#define MY_EDGE_NONE       0

#define HEADER_NEW      "New"
//#define _PROFILE

#define URL_HELP        "http://wiki.bowron.us/index.php/Foobar2000"

#define LABEL_ALL       "[All]"
#define LABEL_MISSING   "<MISSING>"

#define TAG_PANEL       "_browser_panel"

#define SORT_BY_FORMAT  "*"

#define DEFAULT_HEADER_1 "Genre"
#define DEFAULT_HEADER_2 "Artist"
#define DEFAULT_HEADER_3 "Album"
#define DEFAULT_HEADER_4 "#Title"
#define DEFAULT_HEADER_5 "Title"

#define DEFAULT_FORMAT_1 "$if2(%genre%," LABEL_MISSING ")"
#define DEFAULT_FORMAT_2 "$if2(%artist%," LABEL_MISSING ")"
#define DEFAULT_FORMAT_3 "$if2(%album%," LABEL_MISSING ")"
#define DEFAULT_FORMAT_4 "[$num(%tracknumber%,2) - ]%title%"
#define DEFAULT_FORMAT_5 "%title%"

#define DEFAULT_SORT_1   SORT_BY_FORMAT
#define DEFAULT_SORT_2   SORT_BY_FORMAT
#define DEFAULT_SORT_3   SORT_BY_FORMAT
#define DEFAULT_SORT_4   SORT_BY_FORMAT
#define DEFAULT_SORT_5   SORT_BY_FORMAT

#define DEFAULT_PLAYLIST         "*Browser*"

#define DEFAULT_MULTI_HEADERS (DEFAULT_HEADER_1 NEWLINE DEFAULT_HEADER_2 NEWLINE DEFAULT_HEADER_3 NEWLINE DEFAULT_HEADER_4 NEWLINE DEFAULT_HEADER_5)
#define DEFAULT_MULTI_FORMATS (DEFAULT_FORMAT_1 NEWLINE DEFAULT_FORMAT_2 NEWLINE DEFAULT_FORMAT_3 NEWLINE DEFAULT_FORMAT_4 NEWLINE DEFAULT_FORMAT_5)
#define DEFAULT_MULTI_SORTS   (DEFAULT_SORT_1 NEWLINE DEFAULT_SORT_2 NEWLINE DEFAULT_SORT_3 NEWLINE DEFAULT_SORT_4 NEWLINE DEFAULT_SORT_5 )

#define DEFAULT_TEXT_COLOR       RGB(0,0,0)
#define DEFAULT_BACKGROUND_COLOR RGB(255,255,255)

#define NEWLINE   "\r\n"

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include <commctrl.h>
#include <windowsx.h>

#include <shlwapi.h>
#include <shlobj.h>
#include <atlbase.h>

#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <WCHAR.h>

#include "resource.h"

#include "../columns_ui-sdk/ui_extension.h"

#include "../common/string_functions.h"

#define WM_APP_BROWSE      (WM_APP + 1)
#define WM_APP_RESET       (WM_APP + 2)

#define TIMER_REFRESH      100

DECLARE_COMPONENT_VERSION( "Music Browser", VERSION, 
   "Browse Music in Column UI Panels\n"
   "By Christopher Bowron <chris@bowron.us>\n"
   "http://wiki.bowron.us/index.php/Foobar2000" )

inline static LOGFONT get_def_font()
{
	LOGFONT foo;
   uGetIconFont( &foo );
	return foo;
}
  
void safe_strcpy( char *destination, const char *source, int n ) {
	if ( strlen( source ) < n )	{
		strcpy( destination, source );
	}
	else	{
		strncpy( destination, source,n - 1 );
		destination[n-1] = 0;
	}
}

#define BC_STR_MAX 256

class browser_config
{
public:
   char   m_header[BC_STR_MAX];
   char   m_format[BC_STR_MAX];
   char   m_sort[BC_STR_MAX];
   int    m_precedence;
   int    m_alignment;

   browser_config() {}

   browser_config( const char * header, const char * format, const char * sort, int prec )
   {
		safe_strcpy( m_header, header, BC_STR_MAX );
      safe_strcpy( m_format, format, BC_STR_MAX );
      safe_strcpy( m_sort, sort, BC_STR_MAX );
      m_precedence = prec;
      m_alignment = 0;
   }

   const char * get_header() { return m_header; }
   const char * get_format() { return m_format; }
   const char * get_sort() { return m_sort; }
   int get_precedence() { return m_precedence; }  
};

//int browser_config_compare( const browser_config * a, const browser_config * b )
int browser_config_compare( browser_config & a, browser_config & b )
{
   //return a->m_precedence - b->m_precedence;
   if ( strcmp( a.m_header, HEADER_NEW ) == 0 )
      return 1;
   if ( strcmp( b.m_header, HEADER_NEW ) == 0 )
      return -1;

   return a.m_precedence - b.m_precedence;
}


enum { EE_NONE, EE_SUNKEN, EE_GREY };

char * g_edge_styles[] = { "None", "Sunken", "Grey", NULL };

static const GUID var_guid_012 = { 0x23159ae3, 0x6071, 0x418a, { 0x8d, 0x46, 0xd, 0xe5, 0x7, 0xad, 0x50, 0xf3 } };
static const GUID var_guid_013 = { 0x20e32db8, 0xd068, 0x4cd1, { 0xbf, 0x1c, 0xe1, 0x27, 0x8b, 0xef, 0x41, 0x65 } };
static const GUID var_guid_019 = { 0x37377136, 0x9937, 0x463a, { 0xaa, 0x49, 0xb2, 0x4e, 0xa6, 0xd4, 0x10, 0xb8 } };
static const GUID var_guid_020 = { 0x6efc5935, 0xf0d8, 0x47a8, { 0x87, 0xae, 0x2c, 0xd6, 0x2e, 0x92, 0x38, 0xb7 } };
static const GUID var_guid_021 = { 0xf6fb3990, 0xbbc0, 0x40ac, { 0xa1, 0x3, 0x96, 0xdc, 0x2c, 0x82, 0xb8, 0x76 } };
static const GUID var_guid_022 = { 0xa5147047, 0x25f5, 0x4b83, { 0xae, 0x3c, 0x9d, 0x2b, 0x57, 0xa, 0xc4, 0x4e } };
static const GUID var_guid_023 = { 0x102924e1, 0x4fb7, 0x4450, { 0x82, 0x9c, 0xf3, 0x82, 0xd9, 0x3e, 0x46, 0xd9 } };
static const GUID var_guid_024 = { 0x23c1f50d, 0x32d3, 0x45f5, { 0xbf, 0x28, 0x23, 0x92, 0x58, 0xd3, 0x7b, 0x88 } };
static const GUID var_guid_025 = { 0x22f6ca0a, 0x66f0, 0x4157, { 0xa4, 0xaf, 0xa, 0xba, 0xe9, 0x57, 0x5c, 0xc3 } };
static const GUID var_guid_026 = { 0x570312af, 0x4944, 0x4d7c, { 0x99, 0x42, 0x3f, 0xdc, 0xdb, 0x2e, 0x1e, 0x1b } };
static const GUID var_guid_027 = { 0xc6ce3a0d, 0x1fc5, 0x430a, { 0xb5, 0x57, 0xf, 0xf, 0x8a, 0xcd, 0xd4, 0x27 } };
static const GUID var_guid_028 = { 0xb13938c3, 0x9ff9, 0x41d6, { 0xa4, 0xe1, 0x27, 0xc7, 0x62, 0x72, 0x12, 0x65 } };
static const GUID var_guid_029 = { 0x917fbe48, 0xc99, 0x4a53, { 0xa5, 0xd8, 0xe4, 0x34, 0x17, 0x76, 0x87, 0x50 } };
static const GUID var_guid_030 = { 0xd70f1ae4, 0x4e79, 0x4e70, { 0x90, 0xb4, 0x76, 0xce, 0x2, 0x69, 0xf6, 0xa4 } };
static const GUID var_guid_031 = { 0xc9cb29a5, 0x67a5, 0x4e9f, { 0x9d, 0xf6, 0x93, 0x44, 0xb6, 0x97, 0x64, 0xa8 } };
static const GUID var_guid_032 = { 0xdaf9fffd, 0x8f1e, 0x49d7, { 0xaf, 0x2d, 0x27, 0xbb, 0xab, 0x19, 0x2b, 0xa3 } };
static const GUID var_guid_033 = { 0x58f393a9, 0xb837, 0x4928, { 0x99, 0x9d, 0xe, 0x49, 0xa0, 0x5c, 0x47, 0x12 } };
static const GUID var_guid_034 = { 0x193188bf, 0x2157, 0x4653, { 0x8b, 0xf2, 0xbe, 0x7f, 0x63, 0xaa, 0xf6, 0xf5 } };
static const GUID var_guid_035 = { 0xa9491802, 0x516c, 0x4441, { 0xb9, 0x3c, 0x42, 0x1f, 0x21, 0x66, 0xe2, 0xab } };
static const GUID var_guid_036 = { 0xd87aac31, 0x56d7, 0x4086, { 0x98, 0xc2, 0x54, 0x70, 0x6d, 0xa6, 0x4b, 0x3a } };
static const GUID var_guid_037 = { 0x465362a6, 0xdffa, 0x48eb, { 0xb3, 0x68, 0x11, 0xef, 0x48, 0xe9, 0x78, 0x7a } };
static const GUID var_guid_038 = { 0x7e74bee0, 0x5091, 0x4a1e, { 0x89, 0x5b, 0xb3, 0x27, 0xf0, 0x7, 0x36, 0x27 } };
static const GUID var_guid_039 = { 0x523177c2, 0x4bf4, 0x4b4c, { 0x80, 0xa, 0x69, 0x66, 0xbe, 0xf2, 0xc1, 0x90 } };
static const GUID var_guid_040 = { 0x561ab046, 0xf5a9, 0x4507, { 0xb4, 0x6b, 0x12, 0xb5, 0xfa, 0x89, 0x76, 0xa6 } };
static const GUID var_guid_041 = { 0xb93f524c, 0x177a, 0x448a, { 0x87, 0x2, 0xb6, 0x39, 0xde, 0xa1, 0xfe, 0xe5 } };

// cfg_vars
static cfg_int cfg_text_color( var_guid_012, DEFAULT_TEXT_COLOR );
static cfg_int cfg_background_color( var_guid_013, DEFAULT_BACKGROUND_COLOR );

static cfg_string cfg_playlist( var_guid_019, DEFAULT_PLAYLIST );

static cfg_int    cfg_auto_activate_playlist( var_guid_020, 0 );

static cfg_struct_t<LOGFONT> cfg_font( var_guid_021, get_def_font() );

static cfg_string cfg_doubleclick( var_guid_022, "" );


static cfg_string cfg_multi_headers( var_guid_023, DEFAULT_MULTI_HEADERS );
static cfg_string cfg_multi_formats( var_guid_024, DEFAULT_MULTI_FORMATS );
static cfg_string cfg_multi_sorts( var_guid_025, DEFAULT_MULTI_SORTS);

static cfg_int cfg_scrollbar_magic( var_guid_026, 0 );
static cfg_int cfg_edge_style( var_guid_027, EE_SUNKEN );
static cfg_int cfg_hide_headers( var_guid_028, 0 );
static cfg_int cfg_replace_last( var_guid_029, 0 );
static cfg_string cfg_last_playlist( var_guid_030, "" );
static cfg_int cfg_use_library_callback( var_guid_031, 1 );
static cfg_int cfg_custom_colors( var_guid_032, 0 );
static cfg_int cfg_color_selected( var_guid_033, RGB(190,190,255) );
static cfg_int cfg_color_selectednonfocus( var_guid_034, RGB(63,63,63) );
static cfg_int cfg_color_selected_text( var_guid_035, CLR_DEFAULT );
static cfg_int cfg_color_selected_text_nonfocus( var_guid_036, CLR_DEFAULT );

static cfg_int cfg_populate_on_load( var_guid_037, 1 );
static cfg_structlist_t<browser_config> cfg_browser_configs( var_guid_038 );
static cfg_int cfg_itemcount_in_all( var_guid_039, 1 );
static cfg_int cfg_populate_playlist_on_load( var_guid_040, 1 );
static cfg_int cfg_show_all( var_guid_041, 1 );
// end cfg_vars

// Has the user performed an action to modify the contents of the browser
// Main menu action or context item... 
bool g_user_action = false;

static const GUID music_browser_extension_guid = { 0xf87f5735, 0x3ee5, 0x4078, { 0xbb, 0x75, 0x83, 0x2a, 0xf3, 0x96, 0x10, 0x40 } };

static const GUID guid_preferences = { 0x19ac9a2c, 0x18eb, 0x48a5, { 0x98, 0xfd, 0xea, 0xe2, 0xe1, 0xb2, 0x31, 0x8d } };

bool g_ignore_item_changes = false;
bool g_populate_on_create = false;

int g_selected_count = 0;

class music_browser;
class row_info;

int inline shift_pressed()
{
	return ::GetAsyncKeyState( VK_SHIFT ) & 0x8000;
}

const music_browser * g_timer_browser = NULL;
row_info * g_timer_row_info = NULL;

VOID CALLBACK browser_timer_callback( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime );

int g_row_info_count = 0;

// each row has a label, a list of handles, and a link back to its parent
class row_info 
{
public:
   pfc::string8_fastalloc  label;
   bit_array_bittable    * handle_mask;
   const music_browser   * browser;
   UINT                    m_state;

   row_info( const char * str, t_size size, music_browser * mb )
   {
      label.set_string( str );
      handle_mask = new bit_array_bittable( size );
      browser     = mb;  
      g_row_info_count ++;
      m_state     = 0;
   }

   ~row_info()
   {
      delete handle_mask;
      label.reset();
      g_row_info_count --;
   }

   void set_bit( t_size idx, bool value )
   {
      handle_mask->set( idx, value );
   }
};

int row_info_compare( const row_info *a, const row_info *b )
{   
   if ( strcmp( a->label, LABEL_ALL ) == 0 ) return -1;
   if ( strcmp( b->label, LABEL_ALL ) == 0 ) return 1;
   if ( strcmp( a->label, LABEL_MISSING ) == 0 ) return -1;
   if ( strcmp( b->label, LABEL_MISSING ) == 0 ) return 1;
   return stricmp_utf8_ex( a->label, ~0, b->label, ~0 );
}

bool color_picker( HWND wnd, cfg_int & out, COLORREF custom = 0 )
{
	bool rv = false;
	COLORREF COLOR = out;
	COLORREF COLORS[16] = {custom,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	if (uChooseColor(&COLOR, wnd, &COLORS[0]))
	{
		out = COLOR;
		rv = true;
	}
	return rv;
}

bool run_context_command( const char * cmd, const pfc::list_base_const_t<metadb_handle_ptr> & handles )
{
   if ( strcmp( cmd, "" ) != 0 ) 
   {
      GUID guid;

      if ( menu_helpers::find_command_by_name( cmd, guid ) )
      {
         menu_helpers::run_command_context( guid, pfc::guid_null, handles );
         return true;
      }
   }
   return false;
}

class music_browser;

class browser_title_hook : public titleformat_hook
{
public:
   int m_base_panel;
   const music_browser * m_browser;
   const row_info * m_row;

   browser_title_hook( const row_info * r )
   {
      m_row = r;

      if ( r )
      {
         m_browser = m_row->browser;
      }
      else
      {
         m_browser = NULL;
      }
   }

	virtual bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag);

   virtual bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
   {
      return false;
   }
};


static pfc::ptr_list_t<music_browser> g_active_browsers;   

class music_browser : public ui_extension::window
{
public:

   virtual void do_context_menu( POINT * pt )
   {                      
      metadb_handle_list handles;

      get_selected_items( get_list(), handles );

      static_api_ptr_t<contextmenu_manager> man;
      man->win32_run_menu_context( m_wnd, handles, pt );          
   }

   static music_browser * g_find_browser( HWND wnd )
   {
      for ( int i = 0; i < g_active_browsers.get_count(); i++ )
      {
         if ( g_active_browsers[i]->m_wnd == wnd )
         {
            return g_active_browsers[i];
         }
      }
      return NULL;
   }

	static void get_selected_items( HWND listview, metadb_handle_list & list )
	{
		int result = -1;
			
      g_selected_count = 0;
      pfc::hires_timer timer;
      timer.start();

		LVITEM item;

		while ((result = ListView_GetNextItem( listview, result, LVNI_SELECTED )) >= 0)
		{
		   memset( &item, 0, sizeof(item) );
		   item.iItem = result;
		   item.mask = LVIF_PARAM;
		   ListView_GetItem(listview, &item);

         if ( !item.lParam ) break;

         row_info *ri = (row_info*) item.lParam;
         
         t_size v;
         t_size count = ri->browser->m_handles.get_count();

         // shortcut for all... 
         if ( result == 0 && cfg_show_all )
         {
            list.add_items( ri->browser->m_handles );
            break;
         }
         else
         {
            g_selected_count++;
            for_each_bit_array( v, (*ri->handle_mask), true, 0, count )
               list.add_item( ri->browser->m_handles[ v ] );
         }
		}

#ifdef _PROFILE
      static char w00t[100];
      sprintf( w00t, "populating selection handles time: %f", timer.query() );
      console::printf( w00t );
#endif
	}

   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            music_browser *p_mb = (music_browser*)lp;

            pfc::stringcvt::string_wide_from_utf8 wide_title( p_mb->m_header );

            LVCOLUMN col;
            memset( &col, 0, sizeof( col ) );

            RECT r;
            GetWindowRect( wnd, &r );

            col.cx = LVSCW_AUTOSIZE_USEHEADER;
            col.mask = LVCF_TEXT | LVCF_WIDTH; 
            col.pszText = (LPWSTR) (const wchar_t *) wide_title;

            //p_mb->m_list = GetDlgItem( wnd, IDC_LIST );
            p_mb->m_list = CreateWindowEx (
               0, 
			      WC_LISTVIEW, 
			      _T("Browser ListView"), 
               LVS_REPORT | 
               LVS_ALIGNLEFT | WS_TABSTOP | 
			      WS_VISIBLE | 
               WS_CHILD | 
               ( cfg_hide_headers ? LVS_NOCOLUMNHEADER : 0 ) |
			      LVS_SHOWSELALWAYS, 
			      0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
			      wnd, NULL, core_api::get_my_instance(), NULL); 

            ListView_InsertColumn( p_mb->m_list, 0, &col );

            DWORD style = LVS_EX_FULLROWSELECT;
            ListView_SetExtendedListViewStyleEx( p_mb->m_list, style, style );
         }
         break;

      case WM_SIZE:
         {
            music_browser * b = g_find_browser( wnd );

            if ( b )
            {
               RECT r;
				   GetWindowRect( wnd, &r );	
            
               SetWindowPos( b->get_list(), 
                  0, 0, 0, r.right-r.left, r.bottom-r.top, 
                  SWP_NOZORDER );

               b->resize();
            }
         }
         break;
         
      case WM_CONTEXTMENU:
         {
            POINT pt = {(short)LOWORD(lp),(short)HIWORD(lp)};
            
            music_browser * b = g_find_browser( wnd );
            
            if ( b )
            {
               b->do_context_menu( &pt );  
            }
            return true;
         }
         break;
         
      case WM_TIMER:
         {
            // KillTimer( wnd, wp );

            switch ( wp )
            {
               /*
            case TIMER_REFRESH:
               KillTimer( wnd, TIMER_REFRESH );
               console::printf( "Refresh Timer" );
               break;
                */

            case 1:
               if ( g_timer_browser && g_timer_row_info )
               {
                  TRACK_CALL_TEXT( "WM_TIMER handler" );
                  // console::printf( "Browsing" );
                  metadb_handle_list lst;
                  get_selected_items( g_timer_browser->get_list(), lst );

                  g_browse( lst, g_timer_browser, g_timer_row_info );

                  g_timer_browser = NULL;
                  g_timer_row_info = NULL;
               }
               break;
            }
         }
         break;
         

      case WM_NOTIFY:
         {
            LPNMHDR pnmh = (LPNMHDR) lp;

				switch ( pnmh->code )
            {
            case NM_CUSTOMDRAW:
               if ( cfg_custom_colors )
               {  
                  NMLVCUSTOMDRAW * lvCustomDraw = (NMLVCUSTOMDRAW*) lp;

                  switch ( lvCustomDraw->nmcd.dwDrawStage )
                  {
                  default:
                     console::printf( "unhandled draw stage: %x", lvCustomDraw->nmcd.dwDrawStage );
                     break;

                  case CDDS_PREPAINT:
                     SetWindowLong( wnd, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW );
                     return TRUE;        
                     
                  case CDDS_ITEMPREPAINT:                        
                     /*
                     SetWindowLong( wnd, DWL_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW );
                     return TRUE;                            
                     */
                  case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                     //console::printf( "CDDS_ITEMPREPAINT | CDDS_SUBITEM" );
                     //console::printf( "item = %d, subitem = %d", lvCustomDraw->nmcd.dwItemSpec, lvCustomDraw->iSubItem );
                     {
                        music_browser * pMB = g_find_browser( wnd );
                        
                        if ( pMB )
                        {
                           row_info * ri = (row_info *)lvCustomDraw->nmcd.lItemlParam;
                                                      
                           UINT state = ListView_GetItemState( pnmh->hwndFrom, lvCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED | LVIS_FOCUSED ); 

                           //console::printf( "ListView State: 0x%x", state );
                           //if ( state & LVIS_SELECTED )
                           {
                              row_info * ri = (row_info*)lvCustomDraw->nmcd.lItemlParam;

                              if ( ri )
                              {
                                 DWORD textColor = cfg_text_color;
                                 HBRUSH hBrush;
                                 //HBRUSH hOldBrush;

                                 bool selected = false;

                                 if ( state & LVIS_SELECTED )
                                 {
                                    selected = true;
                                    if ( GetFocus() == pnmh->hwndFrom ) 
                                    {
                                       hBrush = CreateSolidBrush( cfg_color_selected );
                                       if ( cfg_color_selected_text != CLR_DEFAULT )
                                       {
                                          textColor = cfg_color_selected_text;
                                       }
                                    }
                                    else
                                    {
                                       hBrush = CreateSolidBrush( cfg_color_selectednonfocus );
                                       if ( cfg_color_selected_text_nonfocus != CLR_DEFAULT )
                                       {                                       
                                          textColor = cfg_color_selected_text_nonfocus;
                                       }
                                    }
                                 } 
                                 else
                                 {
                                    hBrush = CreateSolidBrush( cfg_background_color );
                                 }

                                 //hOldBrush = (HBRUSH)SelectObject( lvCustomDraw->nmcd.hdc, hBrush );

                                 RECT rect;

                                 ListView_GetItemRect( pnmh->hwndFrom, lvCustomDraw->nmcd.dwItemSpec, &rect, LVIR_BOUNDS );

                                 // draw the bounding box
                                 FillRect( lvCustomDraw->nmcd.hdc, &rect, hBrush );

                                 // draw from bounding box to the edge of the window
                                 //if ( selected )
                                 {
                                    RECT wndRect;
                                    GetWindowRect( pnmh->hwndFrom, &wndRect );

                                    POINT pt;
                                    pt.x = wndRect.right;
                                    pt.y = wndRect.top;

                                    ScreenToClient( pnmh->hwndFrom, &pt );

                                    wndRect.left = rect.right;
                                    wndRect.bottom = rect.bottom;
                                    wndRect.top = rect.top;
                                    wndRect.right = pt.x;
                                    HDC dc = GetDC( pnmh->hwndFrom );
                                    FillRect(  dc, &wndRect, hBrush );
                                    ReleaseDC( pnmh->hwndFrom, dc );
                                 }

                                 //SelectObject( lvCustomDraw->nmcd.hdc, hOldBrush );
                                 DeleteObject( hBrush );

                                 // draw the label text
                                 ListView_GetItemRect( pnmh->hwndFrom, lvCustomDraw->nmcd.dwItemSpec, &rect, LVIR_LABEL );                                 

                                 // uTextOutColors( lvCustomDraw->nmcd.hdc, ri->label, ri->label.length(), rect.left + 2, rect.top + 1, &rect, false, textColor );

                                 uTextOutColorsTabbed( lvCustomDraw->nmcd.hdc, ri->label, ri->label.length(), &rect, 1, &rect, false, textColor, false );

                                 /*
                                 if ( lvCustomDraw->nmcd.uItemState & CDIS_FOCUS )
                                 {
                                    DrawFocusRect( lvCustomDraw->nmcd.hdc, &rect );
                                 }
                                 */

                                 SetWindowLong( wnd, DWL_MSGRESULT, CDRF_SKIPDEFAULT );
                                 return TRUE;
                              }
                           }                           

                           // Column header
                           SetWindowLong( wnd, DWL_MSGRESULT, CDRF_DODEFAULT );
                           return TRUE;                           
                        }

                     }
                  }
               }
               break;

               case NM_DBLCLK:
                  {
                     NMITEMACTIVATE *lpnmitem = (NMITEMACTIVATE *) lp;
                     
                     if ( lpnmitem->iItem != -1 )
                     {
                        static LVITEM item;
                        memset( &item, 0, sizeof( item ) );
                        item.iItem = lpnmitem->iItem;
                        item.mask = LVIF_PARAM;
                        ListView_GetItem( pnmh->hwndFrom, &item );

                        if ( item.mask & LVIF_PARAM )
                        {
                           row_info *ri = (row_info *)item.lParam;

                           if ( ri )
                           {
                              metadb_handle_list list;

                              int count = ri->browser->m_handles.get_count();
                              int v;

                              if ( lpnmitem->iItem == 0 && cfg_show_all )
                              {
                                 list.add_items( ri->browser->m_handles );
                              }
                              else
                              {
                                 for_each_bit_array( v, (*ri->handle_mask), true, 0, count )
                                    list.add_item( ri->browser->m_handles[ v ] );   
                              }
                              
                              run_context_command( cfg_doubleclick, list );
                           }
                        }
                     }
                  }
               break;

               case LVN_ITEMCHANGED:
                  if ( !g_ignore_item_changes )
                  {                                        
                     LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lp; 

                     if ( pnmv->iItem != -1 )
                     {
                        static LVITEM item;
                        memset( &item, 0, sizeof( item ) );
                        item.iItem = pnmv->iItem;
                        item.mask = LVIF_PARAM;

                        if ( ListView_GetItem( pnmh->hwndFrom, &item ) )
                        {
                           row_info *ri = (row_info *)item.lParam;
                           if ( ri )
                           {
                              g_timer_browser = ri->browser;
                              g_timer_row_info = ri;
                              //console::printf( "Setting Timer: %d", wnd );
                              //SetTimer( wnd, 1, 100, NULL );
                              //SetTimer( NULL, 1, 100, browser_timer_callback );
                              PostMessage( wnd, WM_TIMER, 1, 0 );
                           }
                        }
                     }
                  }                            
                  break;

		         case LVN_KEYDOWN:
                  {
                     LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) lp; 
                     metadb_handle_list selected_handles;
                     get_selected_items( pnmh->hwndFrom, selected_handles );

                     static_api_ptr_t<keyboard_shortcut_manager> man;
                     return man->on_keydown_auto_context( selected_handles, pnkd->wVKey, music_browser_extension_guid );
                  }
			         break;
            }
         }
      }
      return false;
   }
   
public:
   HWND     m_wnd;
   HWND     m_list;

   pfc::string8  m_format;
   pfc::string8  m_header;
   pfc::string8  m_sort;

   metadb_handle_list m_handles;

   int m_index;

   ui_extension::window_host_ptr m_host;

   pfc::ptr_list_t<row_info> m_row_info;

   HFONT m_font;

   music_browser()
   {
      m_wnd    = NULL;
      m_list   = NULL;
      m_font   = NULL;
      m_index  = 0;
   }

   virtual ~music_browser()
   {
      release_handles();

      if ( m_font )
      {
         DeleteObject( m_font );
         m_font = NULL;
      }
   }

   void delete_row_info()
   {
      m_row_info.delete_all();
   }

   void release_handles()
   {    
      m_handles.remove_all();     
      delete_row_info();       
   }

   unsigned get_type() const 
   {
      return ui_extension::type_panel;
   }

   void destroy_window()
   {
      // dialog_proc( m_wnd, WM_DESTROY, 0, 0 );
      DestroyWindow( m_wnd );
      
      if ( g_timer_browser == this )
      {
         g_timer_browser = NULL;
      }

      release_handles();
      m_wnd = NULL;

      g_active_browsers.remove_item( this );
   }

   void get_category( pfc::string_base & out ) const
   {
      out.set_string( "Panels" );
   }

   virtual bool get_description( pfc::string_base & out ) const
   {
      out.set_string( "Panel for browsing library" );
      return true;
   }

   virtual bool get_short_name( pfc::string_base & out ) const 
   {
      out.set_string( "foo_browser" );
      return true;
   }

   virtual void get_name( pfc::string_base & out ) const
   {
      out.set_string( "foo_browser" );
   }

   const GUID & get_extension_guid() const
   {
      return music_browser_extension_guid;
   }

   void get_size_limits( ui_extension::size_limit_t & p_out ) const
   {
      p_out.min_height = 30;
      p_out.min_width = 30;
   }
   
   virtual HWND get_wnd() const
   {
      return m_wnd;
   }

   virtual HWND get_list() const 
   {
      return m_list;
   }

   virtual bool is_available( const ui_extension::window_host_ptr & p_host) const 
   {
      return ( m_wnd == NULL );
   }

   virtual HWND create_or_transfer_window( HWND wnd_parent, const ui_extension::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position ) 
   {
      if ( m_wnd ) 
      {
         ShowWindow( m_wnd, SW_HIDE );
         SetParent( get_wnd(), wnd_parent );
         SetWindowPos( get_wnd(), NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER );
         m_host->relinquish_ownership( get_wnd() );
         return m_wnd;
      }
      else
      {
         if ( !g_active_browsers.have_item( this ) )
         {
            g_active_browsers.add_item( this );
         }

         m_host = p_host;   
         m_wnd = uCreateDialog( IDD_BLANK, wnd_parent, dialog_proc, (LPARAM)this );
         update_colors();
         
         if ( g_populate_on_create )
         {
            static_api_ptr_t<library_manager> lib;
            metadb_handle_list handles;
            lib->get_all_items( handles );
            browse( handles );
         }

         return m_wnd;
      }
   }

   virtual void resize()
   {    
      if ( !cfg_scrollbar_magic )
      {
         ListView_SetColumnWidth( get_list(), 0, LVSCW_AUTOSIZE_USEHEADER );
      }
      else
      {
         RECT rect;
         GetWindowRect( m_wnd, &rect );
      
         long wndStyle = GetWindowLong( get_list(), GWL_STYLE );

         if ( wndStyle & WS_VSCROLL )
         {
            int nWidth = rect.right - rect.left - GetSystemMetrics( SM_CXVSCROLL ) - 4;  
            ListView_SetColumnWidth( get_list(), 0, nWidth );
         }
         else
         {
            int nWidth = rect.right - rect.left - 4;
            ListView_SetColumnWidth( get_list(), 0, nWidth );
         }
      }
   }

   virtual bool add_row_info_item( const char * blip, int i, bool sort_by_format )
   {
      int j;

      if ( strstr(blip, "@skip" ) )
         return false;

      int max = m_row_info.get_count();

      int j_start = ( cfg_show_all ) ? 1 : 0;

      for ( j = j_start; j < max; j++ )
      {
         int cmp = stricmp( m_row_info[j]->label, blip );

         if ( cmp == 0 )
         {            
            m_row_info[j]->set_bit( i, true );
            return true;
         }

         if ( sort_by_format && cmp > 0 )
         {                     
            break;
         }
      }
      
      row_info * new_row = new row_info( blip, m_handles.get_count(), this );
      new_row->set_bit( i, true );

      m_row_info.insert_item( new_row, j );
      return false;
   }

   virtual void refresh_row_info()
   {
      TRACK_CALL_TEXT( "refresh_row_info" );

      pfc::string8_fastalloc blip;

      // setup the [All] entry
      if ( cfg_show_all )
      {
         row_info * ri = new row_info( LABEL_ALL, m_handles.get_count(), this );
         /*
         for ( int z = 0; z < m_handles.get_count(); z++ )
         {
            ri->set_bit( z, true );
         }
         */
         m_row_info.add_item( ri );    
      }

      bool sort_by_format = false;

      if ( strcmp( m_sort, SORT_BY_FORMAT ) == 0 )
         sort_by_format = true;

      pfc::hires_timer timer;
      timer.start();

      if ( !has_tags( m_format ) )
      {          
         pfc::string8_fastalloc tmp;
         tmp.set_string( META_QUERY_START );
         tmp.add_string( m_format );
         tmp.add_string( META_QUERY_STOP );
         m_format.set_string( tmp );
      }
      
      if ( strstr( m_format, META_QUERY_START ) )
      {         
         for ( int h_index = 0; h_index < m_handles.get_count(); h_index++ )
         {
            pfc::ptr_list_t<pfc::string8> strings;

            bool f1 = generate_meta_query_strings( m_format, m_handles[h_index], strings );

            if ( strings.get_count() == 0 )
            {                  
               m_handles[h_index]->format_title_legacy( NULL, blip, m_format, NULL );                 

               if ( strcmp( blip, "?" ) == 0 )
               {
                  add_row_info_item( LABEL_MISSING, h_index, sort_by_format );
               }
               else
               {
                  add_row_info_item( blip, h_index, sort_by_format );
               }
            }
            else
            {
               for ( int z = 0; z < strings.get_count(); z++ )           
               {
                  m_handles[h_index]->format_title_legacy( NULL, blip, strings[z]->get_ptr(), NULL );
                  add_row_info_item( blip, h_index, sort_by_format );
               }
            }

            strings.delete_all();
         }
      }
      else
      {
         service_ptr_t<titleformat_object> script;
         if ( static_api_ptr_t<titleformat_compiler>()->compile( script, m_format ) )
         {
            for ( int h_index = 0; h_index < m_handles.get_count(); h_index++ )
            {
               m_handles[h_index]->format_title( NULL, blip, script, NULL );
               add_row_info_item( blip, h_index, sort_by_format );
            }
         }
         else
         {
            console::warning( "Error in browser formatting" );
         }
      }

#ifdef _PROFILE
      char w00t[100];
      sprintf( w00t, "[%s]populate row_info time: %f", (const char*) m_header, timer.query() );
      console::printf( w00t );
      //console::printf( "number of strcmp: %d", nCompares );
#endif

      // We need this to get All/Missing on top
      if ( sort_by_format )   
      {
         timer.start();
         m_row_info.sort_t( &row_info_compare );

#ifdef _PROFILE
         char w00t[100];
         sprintf( w00t, "[%s]post populate sort: %f", (const char*) m_header, timer.query() );
         console::printf( w00t );
#endif
      }
   }

   virtual void refresh_listview()
   {
      TRACK_CALL_TEXT( "refresh_listview" );
      pfc::hires_timer timer;
      timer.start();

      SendMessage( m_wnd, WM_SETREDRAW, FALSE, 0 );
      ListView_DeleteAllItems( get_list() );

      LVITEM item;
      memset( &item, 0, sizeof(item) );

      if ( cfg_show_all && cfg_itemcount_in_all )
      {
         // singular or plural
         if ( m_row_info.get_count() > 2 )
         {
            pfc::string_printf foo( " (%d %ss)", m_row_info.get_count() - 1,                
               (const char *) m_header );
            m_row_info[0]->label.add_string( foo );

         }
         else
         {
            pfc::string_printf foo( " (%d %s)", m_row_info.get_count() - 1, 
               (const char *) m_header );
            m_row_info[0]->label.add_string( foo );
         }
      }

      for ( int j = 0; j < m_row_info.get_count(); j++ )
      {
         pfc::stringcvt::string_wide_from_utf8 wide_string( m_row_info[j]->label );
         item.pszText = (LPWSTR) (const wchar_t *)wide_string;
         item.lParam = (LPARAM)m_row_info[j];
         item.iItem = j;
         item.mask = LVIF_TEXT | LVIF_PARAM;
         int n = ListView_InsertItem( get_list(), &item );
      }

      resize();
      
      SendMessage( m_wnd, WM_SETREDRAW, TRUE, 0 );
      InvalidateRect( m_wnd, NULL, FALSE );

#ifdef _PROFILE
      char w00t[100];
      sprintf( w00t, "[%s]populate listview time: %f", (const char*) m_header, timer.query() );
      console::printf( w00t );
#endif
   }

   virtual void refresh_display()
   {
      if ( get_list() )
      {
         refresh_row_info();
         refresh_listview();
      }
   }

   virtual bool is_active()
   {
      return ( m_wnd != NULL );
   }

   void update_colors()
   {
      HWND listview = get_list();
      ListView_SetBkColor( listview, cfg_background_color );
		ListView_SetTextBkColor( listview, cfg_background_color );
		ListView_SetTextColor( listview, cfg_text_color );

      if ( m_font )
      {
         DeleteObject( m_font );
      }

      m_font = CreateFontIndirect( &(LOGFONT)cfg_font );			
      if (m_wnd)
      {
         uSendMessage( listview, WM_SETFONT, (WPARAM)m_font, MAKELPARAM( 1, 0 ) );
      }
      

      /*
      SetWindowLong( m_wnd, GWL_EXSTYLE,          
         ( cfg_edge_style == EE_SUNKEN ? MY_EDGE_SUNKEN : 0 ) |
         ( cfg_edge_style == EE_GREY ? MY_EDGE_GREY : 0 ) );   
      */

      SetWindowLong( get_list(), GWL_EXSTYLE,          
         ( cfg_edge_style == EE_SUNKEN ? MY_EDGE_SUNKEN : 0 ) |
         ( cfg_edge_style == EE_GREY ? MY_EDGE_GREY : 0 ) );

      SetWindowLong( get_list(), GWL_STYLE,
         LVS_REPORT | 
         //LVS_LIST |
         LVS_ALIGNLEFT | WS_TABSTOP | 
	      WS_VISIBLE | 
         //WS_BORDER |
         WS_CHILD | 
         ( cfg_hide_headers ? LVS_NOCOLUMNHEADER : 0 ) |
	      LVS_SHOWSELALWAYS );

      /*
      ListView_SetExtendedListViewStyle( get_list(), 
         ListView_GetExtendedListViewStyle( get_list() ) | LVS_EX_FULLROWSELECT | 
         LVS_EX_GRIDLINES );
         */      

      SetWindowPos( get_list(), 
         0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE  | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED );

      resize();

      InvalidateRect( m_wnd, NULL, TRUE );
      InvalidateRect( get_list(), NULL, TRUE );
   }

   void set_focus( int index )
   {
      HWND listview = get_list();
   	// unselect everyone
		ListView_SetItemState( listview, -1, 0, LVIS_SELECTED | LVIS_FOCUSED );
		// select this
		ListView_SetItemState( listview, index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
		ListView_EnsureVisible( listview, index, FALSE );
   }

   virtual void browse( const pfc::list_base_const_t<metadb_handle_ptr> & p_handles )
   {
      // console::printf( "Browse: %s", (const char *) m_header );

      release_handles();

      if ( m_header.is_empty() && m_format.is_empty() )
      {
         LVITEM item;
         memset( &item, 0, sizeof(item) );        
         item.pszText = _T("Press Shift+Right Click");
         item.lParam = NULL;
         item.iItem = 0;
         item.mask = LVIF_TEXT | LVIF_PARAM;
         ListView_InsertItem( get_list(), &item );
         
         item.iItem = 1;
         item.pszText = _T("to Select Format");
         ListView_InsertItem( get_list(), &item );
         return;         
      }

      m_handles = p_handles;

      if ( m_sort.is_empty() == false )
      {
         if ( strcmp( m_sort, SORT_BY_FORMAT ) != 0 )
         {
            m_handles.sort_by_format( m_sort, NULL );
         }
      }

      refresh_display();

      // select ALL
      set_focus( 0 );
   }

   static void g_update_colors()
   {     
      for ( int i = 0; i < g_active_browsers.get_count(); i++ )
      {
         if ( g_active_browsers[i]->is_active() )
         {               
            g_active_browsers[i]->update_colors();
         }
      }
   }


   static void g_send_to_playlist( 
      const pfc::list_base_const_t<metadb_handle_ptr> & p_handles, 
      const music_browser * mb = NULL, 
      const row_info * row = NULL
      )
   {
      pfc::string8 str_playlist;

      if ( has_tags( cfg_playlist ) )
      {
         browser_title_hook t_hook( row );

         if ( p_handles.get_count() > 0 )
         {
            p_handles[0]->format_title_legacy( &t_hook, str_playlist, cfg_playlist, NULL );
         }
         else
         {
            str_playlist.set_string( "*Browser*" );
         }
      }
      else
      {
         str_playlist = cfg_playlist;
      }

      static_api_ptr_t<playlist_manager> man;
      bit_array_false f;

      int n = infinite;

      if ( cfg_replace_last && !cfg_last_playlist.is_empty() )
      {
         n = man->find_playlist( cfg_last_playlist, ~0 );
         man->playlist_rename( n, str_playlist, ~0 );
      }
      
      if ( n = infinite )
      {          
         n = man->find_or_create_playlist( str_playlist, ~0 );
      }

      man->playlist_clear( n );
      man->playlist_insert_items_filter( n, infinite, p_handles, false );

      cfg_last_playlist = str_playlist;

      if ( cfg_auto_activate_playlist || ( man->get_playlist_count() == 1 ) )
      {
         g_activate_playlist( n );
         /*
         int n_last = man->get_playing_playlist();
         man->set_active_playlist( n );

         if ( n_last != infinite )
         {
            man->set_playing_playlist( n_last );
         }
         */
      }
   }

   static void g_activate_playlist( int n_guess = -1 )
   {
      static_api_ptr_t<playlist_manager> man;
      int n = ( n_guess != -1 ) ? n_guess : man->find_playlist( cfg_last_playlist, ~0 );
      int n_last = man->get_playing_playlist();

      if ( n != infinite )
      {
         man->set_active_playlist( n );

         if ( n_last != infinite )
         {
            man->set_playing_playlist( n_last );
         }
      }
   }

   static void g_browse( 
      const pfc::list_base_const_t<metadb_handle_ptr> & p_handles, 
      const music_browser * mb = NULL, 
      const row_info * row = NULL, 
      bool  populate_playlist = true
      )
   {
      TRACK_CALL_TEXT( "g_browse" );
      if ( populate_playlist && false == cfg_playlist.is_empty() )
      {
         g_send_to_playlist( p_handles, mb, row );
         /*
         pfc::string8 str_playlist;

         if ( has_tags( cfg_playlist ) )
         {
            browser_title_hook t_hook( row );

            if ( p_handles.get_count() > 0 )
            {
               p_handles[0]->format_title_legacy( &t_hook, str_playlist, cfg_playlist, NULL );
            }
            else
            {
               str_playlist.set_string( "*Browser*" );
            }
         }
         else
         {
            str_playlist = cfg_playlist;
         }

         static_api_ptr_t<playlist_manager> man;
         bit_array_false f;

         int n = infinite;

         if ( cfg_replace_last && !cfg_last_playlist.is_empty() )
         {
            n = man->find_playlist( cfg_last_playlist, ~0 );
            man->playlist_rename( n, str_playlist, ~0 );
         }
         
         if ( n = infinite )
         {          
            n = man->find_or_create_playlist( str_playlist, ~0 );
         }

         man->playlist_clear( n );
         man->playlist_insert_items_filter( n, infinite, p_handles, false );

         cfg_last_playlist = str_playlist;

         int n_last = man->get_playing_playlist();

         if ( cfg_auto_activate_playlist || ( man->get_playlist_count() == 1 ) )
         {
            man->set_active_playlist( n );

            if ( n_last != infinite )
            {
               man->set_playing_playlist( n_last );
            }
         }
         */
      }

      g_ignore_item_changes = true;

      for ( int i = 0; i < g_active_browsers.get_count(); i++ )
      {
         //if ( ( g_active_browsers[i]->m_index > start ) 
            //|| ( start == 0 ) 
            //|| ( g_active_browsers[i]->m_index == 9 )
            //)

         if ( mb == g_active_browsers[i] ) continue;

         bool bAffected = false;

         if ( mb == NULL ) bAffected = true;
         else if ( g_active_browsers[i]->m_index > mb->m_index ) bAffected = true;
         else if ( ( g_active_browsers[i]->m_index == mb->m_index ) &&
            ( (mb->m_index % 2) == 1 ) ) bAffected = true;

         if ( bAffected )
         {
            g_active_browsers[i]->browse( p_handles );
         }
      }

      g_ignore_item_changes = false;
   }

   static void g_reset()
   {
      metadb_handle_list                  handles;
      static_api_ptr_t<library_manager>   lib;

      lib->get_all_items( handles );
        
      g_browse( handles, NULL, NULL, cfg_populate_playlist_on_load );
   }
};

VOID CALLBACK library_callback_callback( HWND hwnd, UINT msg, UINT_PTR id, DWORD time )
{
   KillTimer( hwnd, id );
   music_browser::g_reset();
}

class browser_library_callback : public library_callback 
{
public:
   static void post_refresh_messages()
   {
      if ( cfg_populate_on_load || g_user_action )
      {
         if ( cfg_use_library_callback )
         {
            SetTimer( NULL, TIMER_REFRESH, 200, library_callback_callback );
         }
      }
      g_user_action = false;
   }

	//! Called when new items are added to the Media Library.
	virtual void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) 
   {
      post_refresh_messages();
   }
	
   //! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) 
   {
      post_refresh_messages();
   }

	//! Called when some items in the Media Library have been modified.
	virtual void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) 
   {
      // post_refresh_messages();
   }
};

static library_callback_factory_t<browser_library_callback> g_browser_library_callback;

// {79EF5137-91B7-4427-9E5E-2EF93AAA5FEE}
static const GUID library_service_guid = 
{ 0x79ef5137, 0x91b7, 0x4427, { 0x9e, 0x5e, 0x2e, 0xf9, 0x3a, 0xaa, 0x5f, 0xee } };

class music_browser_library_service : public library_viewer
{
   GUID get_preferences_page() { return guid_preferences; }
   GUID get_guid() { return library_service_guid; }
   bool have_activate() { return false; }
   void activate() { }
   const char *get_name() { return "Browser"; }
};

static service_factory_single_t<music_browser_library_service> g_library_service;

class music_browser_generic : public music_browser
{
public:
   GUID m_guid;

   music_browser_generic( const char * header, const char * format, const GUID & guid )
      : music_browser()
   {
      m_header = header;
      m_format = format;

      m_guid = guid;
      m_wnd = NULL;
   }

   virtual bool get_description( pfc::string_base & out ) const
   {
      out.set_string( "Panel for browsing library by " );
      out.add_string( m_header );
      return true;
   }

   virtual bool get_short_name( pfc::string_base & out ) const 
   {
      out.set_string( "Browser [" );
      out.add_string( "]" );
      return true;
   }

   virtual void get_name( pfc::string_base & out ) const
   {
      out.set_string( "Browser [" );
      out.add_string( "]" );
   }

   const GUID & get_extension_guid() const
   {
      return m_guid;
   }     
};

static const GUID context_guid_001 = { 0x4fe8e38a, 0x7efb, 0x4ef5, { 0x95, 0xd4, 0xb5, 0xdc, 0xa5, 0x0, 0x8f, 0xa } };
static const GUID context_guid_002 = { 0x5becd7c2, 0x94ac, 0x488f, { 0xa7, 0x7c, 0x9a, 0xe9, 0xab, 0x56, 0xb7, 0xc5 } };

class music_browser_context : public contextmenu_item_simple
{
   enum { CMD_BROWSE = 0, CMD_ACTIVATE_PLAYLIST, CMD_LAST };

   virtual t_enabled_state get_enabled_state( unsigned p_index ) 
   {
      switch ( p_index )
      {
      case CMD_BROWSE:
         return contextmenu_item::DEFAULT_ON;
      case CMD_ACTIVATE_PLAYLIST:
         return contextmenu_item::DEFAULT_OFF;
      }

      return contextmenu_item::DEFAULT_OFF;
   }

   unsigned get_num_items()
   {
      return CMD_LAST;
   }

   GUID get_item_guid( unsigned int n )
   {
      switch ( n )
      {
      case CMD_BROWSE:
         return context_guid_001;
      case CMD_ACTIVATE_PLAYLIST:
         return context_guid_002;
      }
      return pfc::guid_null;
   }
   
   void get_item_name( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      { 
      case CMD_ACTIVATE_PLAYLIST:
         out.set_string( "Activate browser playlist" );
         return;
      case CMD_BROWSE:
         out.set_string( "Browse" );
         return;
      }
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {  
      switch ( n )
      { 
      case CMD_ACTIVATE_PLAYLIST:
      case CMD_BROWSE:
         out.set_string( "" );
         break;
      }
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case CMD_ACTIVATE_PLAYLIST:
         out.set_string( "Activate the browser playlist" );
         return true;
      case CMD_BROWSE:
         out.set_string( "Browse items in music_browser services" );
         return true;
      }
      return false;
   }

   virtual void context_command(unsigned p_index, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      switch ( p_index )
      {
      case CMD_ACTIVATE_PLAYLIST:
         music_browser::g_activate_playlist();
         return;

      case CMD_BROWSE:
         g_user_action = true;
         music_browser::g_browse( p_data );
         return;
      }
   }
};


static service_factory_single_t<music_browser_context> g_context;

static const GUID mainmenu_guid_001 = { 0xdfbce280, 0x2431, 0x49c4, { 0xb3, 0xdc, 0xe3, 0xf1, 0x6c, 0x5, 0xbf, 0xcf } };

class music_browser_main : public mainmenu_commands
{
   t_uint get_command_count()
   {
      return 1;
   }
   virtual GUID get_command(t_uint32 p_index)
   {
      return mainmenu_guid_001;
   }
   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      p_out.set_string( "Refresh Browser Contents" );
   }
   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      p_out.set_string( "Resets browsers to full library" );
      return true;
   }

   virtual GUID get_parent()
   {
      return mainmenu_groups::library;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      //music_browser::g_reset();
      g_user_action = true;
      browser_library_callback::post_refresh_messages();
   }
};

static mainmenu_commands_factory_t< music_browser_main > g_mainmenu;

static const GUID guid_browser_factory = { 0x517ebbd1, 0x1365, 0x4a96, { 0xbe, 0x1c, 0x2f, 0x3a, 0x54, 0xc0, 0x29, 0x10 } };

class music_browser_factory : public music_browser_generic
{
public:
   music_browser_factory() : music_browser_generic( "", "", guid_browser_factory )
   {
   }

   ~music_browser_factory()
   {
   }

   virtual bool get_short_name( pfc::string_base & out ) const 
   {
      out.set_string( "Browser" );
      return true;
   }

   virtual void get_name( pfc::string_base & out ) const
   {
      out.set_string( "Browser Panel" );
   }

   virtual void set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort)
   {
      try {
         p_reader->read_string( m_header, p_abort );
         p_reader->read_string( m_format, p_abort );
         p_reader->read_string( m_sort, p_abort );
         p_reader->read_object( &m_index, sizeof(m_index), p_abort );
      } catch ( exception_io e )
      {
         console::printf( "exception while reading config:: %s", e.what() );
      }
   }

   virtual void get_config(stream_writer * p_writer, abort_callback & p_abort) const 
   {
      p_writer->write_string( m_header, p_abort );
      p_writer->write_string( m_format, p_abort );
      p_writer->write_string( m_sort, p_abort );
      p_writer->write_object( &m_index, sizeof(m_index), p_abort );
   }   

   virtual bool is_available( const ui_extension::window_host_ptr & p_host) const 
   {
      return ( m_host != p_host );
   }

   virtual bool get_prefer_multiple_instances() const
   {
      return false;
   }

   virtual bool do_context_menu_config( POINT *pt, HWND wnd )
   {
      /*
      pfc::ptr_list_t<pfc::string8> strings;
      string_split( cfg_multi_headers.get_ptr(), NEWLINE, strings );
      */

      HMENU menu = CreatePopupMenu();

      for ( int i = 0; i < cfg_browser_configs.get_count(); i++)
      {
         int nFlags = MF_STRING;

         if ( strcmp( m_header, cfg_browser_configs[i].m_header ) == 0 )
         {
            nFlags |= MF_CHECKED;
         }
         //uAppendMenu( menu, nFlags, i + 1, strings[i]->get_ptr() );
         uAppendMenu( menu, nFlags, i + 1, cfg_browser_configs[i].m_header );
      }

      int n = TrackPopupMenu( menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, 
         pt->x, pt->y, 0, wnd, 0);

      DestroyMenu( menu );

      if ( n )
      {
         n--;
         /*
         pfc::ptr_list_t<pfc::string8> formats;
         pfc::ptr_list_t<pfc::string8> sorts;
         string_split( cfg_multi_formats.get_ptr(), NEWLINE, formats );
         string_split( cfg_multi_sorts.get_ptr(), NEWLINE, sorts );
         

         if ( strings.get_count() > n ) { m_header.set_string( strings[n]->get_ptr() ); }
         if ( formats.get_count() > n ) { m_format.set_string( formats[n]->get_ptr() ); }
         if ( sorts.get_count() > n ) { m_sort.set_string( sorts[n]->get_ptr() ); }
         */
       
         m_format.set_string( cfg_browser_configs[n].m_format );
         m_header.set_string( cfg_browser_configs[n].m_header );
         m_sort.set_string( cfg_browser_configs[n].m_sort );
         m_index = cfg_browser_configs[n].m_precedence;

         console::printf( "Header: %s", (const char *) m_header );
         console::printf( "Format: %s", (const char *) m_format );
         console::printf( "Sort: %s", (const char *) m_sort );
         console::printf( "Precedence: %d", m_index );

         if ( get_list() )
         {
            LVCOLUMN column;
            pfc::stringcvt::string_wide_from_utf8 label( m_header );
            memset( &column, 0, sizeof(column) );
            column.pszText = (LPWSTR)(const wchar_t *) label;
            column.mask = LVCF_TEXT;

            ListView_SetColumn( get_list(), 0, &column );

            delete_row_info();
            refresh_display();            
         }
      }
      //strings.remove_all();
      return ( n > 0 );
   }

   virtual void do_context_menu( POINT * pt)
   {
      if ( shift_pressed() )
      {
         do_context_menu_config( pt, m_wnd );
      }
      else
      {
         music_browser::do_context_menu( pt );
      }
   }

   virtual bool show_config_popup(HWND wnd_parent)
   {
      POINT pt;
      GetCursorPos( &pt );
      return do_context_menu_config( &pt, wnd_parent );
   }

   virtual bool have_config_popup() const { return true; }
};

static ui_extension::window_factory<music_browser_factory> g_music_browser_factory;

/************************* preferences ***************************************/
struct { 
   int id; 
   cfg_string *var; 
} var_map[] =
{  
//   { IDC_MULTI_HEADER, &cfg_multi_headers },
//   { IDC_MULTI_FORMAT, &cfg_multi_formats },
//   { IDC_MULTI_SORTS, &cfg_multi_sorts },
   { IDC_PLAYLIST, &cfg_playlist },
};    

struct {
   int id;
   cfg_int *var;
} bool_var_map[] = 
{
   { IDC_AUTO_ACTIVATE, &cfg_auto_activate_playlist },
   { IDC_SCROLLBAR_MAGIC, &cfg_scrollbar_magic },
   { IDC_HIDE_HEADERS, &cfg_hide_headers },
   { IDC_REPLACE_LAST, &cfg_replace_last },
   { IDC_CUSTOM_COLORS, &cfg_custom_colors },
   { IDC_POPULATE_ON_STARTUP, &cfg_populate_on_load },
   { IDC_POPULATE_PLAYLIST_ON_STARTUP, &cfg_populate_playlist_on_load },
   { IDC_SHOW_ITEMCOUNTS, &cfg_itemcount_in_all },
   { IDC_SHOWALL, &cfg_show_all },
};

struct {
   int id;
   cfg_int *var;
   char **labels;
} list_var_map[] =
{
   { IDC_EDGE_STYLE, &cfg_edge_style, g_edge_styles }
};


static BOOL CALLBACK panel_edit_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   static int s_edit = -1;

   switch ( msg )
   {
   case WM_INITDIALOG:
      {
         s_edit = (int) lp;

         HWND cbPrec = GetDlgItem( wnd, IDC_COMBO_PREC);
         SendMessage( cbPrec, CB_RESETCONTENT, 0,0 );
         for ( int k = 0; k <= 9; k++ )
         {
            uSendMessageText( cbPrec, CB_ADDSTRING, 0, pfc::string_printf( "%d", k ) );
         }
         SendMessage( cbPrec, CB_SETCURSEL, cfg_browser_configs[s_edit].get_precedence(), 0 );

         uSetWindowText( GetDlgItem( wnd, IDC_EDIT_FORMAT ), cfg_browser_configs[s_edit].get_format() );
         uSetWindowText( GetDlgItem( wnd, IDC_EDIT_LABEL ), cfg_browser_configs[s_edit].get_header() );
         uSetWindowText( GetDlgItem( wnd, IDC_EDIT_SORT ), cfg_browser_configs[s_edit].get_sort() );

         SendMessage( GetDlgItem( wnd, IDC_EDIT_FORMAT ), EM_LIMITTEXT, 255, 0 );
         SendMessage( GetDlgItem( wnd, IDC_EDIT_LABEL ), EM_LIMITTEXT, 255, 0 );
         SendMessage( GetDlgItem( wnd, IDC_EDIT_SORT ), EM_LIMITTEXT, 255, 0 );
      }
      break;

   case WM_COMMAND:
      switch ( wp )
      {
      case IDOK:
         {
            int n = SendMessage( GetDlgItem( wnd, IDC_COMBO_PREC ), CB_GETCURSEL, 0, 0 );
            cfg_browser_configs[s_edit].m_precedence = n;

            pfc::string8 tmp;
            uGetWindowText( GetDlgItem( wnd, IDC_EDIT_FORMAT ), tmp );
            safe_strcpy( cfg_browser_configs[s_edit].m_format, tmp, BC_STR_MAX );

            uGetWindowText( GetDlgItem( wnd, IDC_EDIT_LABEL ), tmp );
            safe_strcpy( cfg_browser_configs[s_edit].m_header, tmp, BC_STR_MAX );

            uGetWindowText( GetDlgItem( wnd, IDC_EDIT_SORT ), tmp );
            safe_strcpy( cfg_browser_configs[s_edit].m_sort, tmp, BC_STR_MAX );

         }
         // fallthrough
      case IDCANCEL:
         EndDialog( wnd, wp ); 
         break;
      }
   }

   return 0;
}

class music_browser_pref : public preferences_page
{  
   static void update_panel_list( HWND panel_list )
   {
      int n;

      cfg_browser_configs.sort_t( &browser_config_compare );

      ListView_DeleteAllItems( panel_list );

      for ( n = 0; n < cfg_browser_configs.get_count(); n++ )
      {
         pfc::stringcvt::string_wide_from_utf8 title( cfg_browser_configs[n].get_header() );
         LVITEM item;
         memset( &item, 0, sizeof( item ) );
         item.pszText = (LPWSTR)title.get_ptr();
         item.mask = LVIF_TEXT;
         item.iItem = n;
         ListView_InsertItem( panel_list, &item );

         pfc::stringcvt::string_wide_from_utf8 prec( pfc::string_printf( "%d", cfg_browser_configs[n].get_precedence() ) );

         ListView_SetItemText( panel_list, n, 1, (LPWSTR)prec.get_ptr() );
      }
   }


   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
         /*
      case WM_NOTIFY:
         {
            LPNMHDR lpnmh = (LPNMHDR) lp;
            if ( lpnmh->idFrom == IDC_LIST_PANELS )
            {
               switch ( lpnmh->code )
               {
               case NM_KILLFOCUS:

                  if ( ( GetFocus() != GetDlgItem( wnd, IDC_BUTTON_EDIT ) ) &&
                     ( GetFocus() != GetDlgItem( wnd, IDC_BUTTON_REMOVE ) ) )
                  {
                     EnableWindow( GetDlgItem( wnd, IDC_BUTTON_EDIT ), FALSE );
                     EnableWindow( GetDlgItem( wnd, IDC_BUTTON_REMOVE ), FALSE );
                  }
                  break;
               case NM_SETFOCUS:
                  EnableWindow( GetDlgItem( wnd, IDC_BUTTON_EDIT ), TRUE );
                  EnableWindow( GetDlgItem( wnd, IDC_BUTTON_REMOVE ), TRUE );
                  break;
               }
            }
         }
         break;
         */
      case WM_INITDIALOG:
         {
            int n, m;
            m = tabsize( var_map );

            // Setup ALL Text Boxes
            for ( n = 0; n < m; n++ )
            {
               uSetDlgItemText( wnd, var_map[n].id, var_map[n].var->get_ptr() );
            }

            m = tabsize( bool_var_map );
            // Setup ALL check boxes
            for ( n = 0; n < m; n++ )
            {
               CheckDlgButton( wnd, bool_var_map[n].id, (bool_var_map[n].var->get_value()) ? BST_CHECKED : BST_UNCHECKED ) ;            
            }


            HWND panel_list = GetDlgItem( wnd, IDC_LIST_PANELS );

            LVCOLUMN col;
            memset( &col, 0, sizeof(col) );
            col.mask = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
            col.fmt = LVCFMT_LEFT;
            col.pszText = _T("Header");
            col.cx = 220;
            ListView_InsertColumn( panel_list, 0, &col );

            col.cx = 100;
            col.fmt = LVCFMT_CENTER;
            col.pszText = _T("Precedence");
            ListView_InsertColumn( panel_list, 1, &col );

            update_panel_list( panel_list );         

            m = tabsize( list_var_map );
            for ( n = 0; n < m; n++ )
            {
               SendMessage( GetDlgItem( wnd, list_var_map[n].id), CB_RESETCONTENT, 0,0 );
               for ( int k = 0; ; k++ )
               {
                  char *label = list_var_map[n].labels[k];
                  if ( label == NULL ) 
                  {
                     break;
                  }               
                  uSendDlgItemMessageText( wnd, list_var_map[n].id, CB_ADDSTRING, 0, label );
               }
               SendMessage( GetDlgItem( wnd, list_var_map[n].id ), CB_SETCURSEL, list_var_map[n].var->get_value(), 0 );
            }

            // 
            SendMessage( GetDlgItem( wnd, IDC_DOUBLE_CLICK ), CB_RESETCONTENT, 0,0 );
            
            // add empty string option...
            uSendDlgItemMessageText( wnd, IDC_DOUBLE_CLICK, CB_ADDSTRING, 0, "" );
            
            service_enum_t<contextmenu_item> menus;
            service_ptr_t<contextmenu_item> item;

            while ( menus.next( item ) )
            {
               pfc::string8_fastalloc menuPath;
               pfc::string8_fastalloc menuName;
               for ( int i = 0; i < item->get_num_items(); i++ )
               {
                  item->get_item_default_path( i, menuPath );
                  item->get_item_name( i, menuName );

                  if ( !menuPath.is_empty() )
                  {
                     menuPath.add_string( "/" );
                  }
                  menuPath.add_string( menuName );

                  int n = uSendDlgItemMessageText( wnd, IDC_DOUBLE_CLICK, CB_ADDSTRING, 0, menuPath );

               }

               SendMessage( GetDlgItem( wnd, IDC_DOUBLE_CLICK ), 
                  CB_SELECTSTRING, -1, 
                  (LPARAM)(const wchar_t *) pfc::stringcvt::string_wide_from_utf8( cfg_doubleclick ));
            }
         }
         break;

		case WM_COMMAND:
         if ( ( wp >> 16 ) == EN_CHANGE )
         {
            int edit_control = wp & 0xffff;
            int n;
            int m = tabsize( var_map );

            for ( n = 0; n < m; n++ )
            {
               if ( var_map[n].id == edit_control )
               {
                  pfc::string8 woop;
                  uGetDlgItemText( wnd, var_map[n].id, woop );
                  (*var_map[n].var) = woop;
                  break;
               }
            }
         }

         // Handle all the booleans in bool_var_map
         if ( wp >> 16 == BN_CLICKED )
         {
            int button = wp & 0xffff;
            int m = tabsize( bool_var_map );
            int n;

            for ( n = 0; n < m; n++ )
            {
               if ( bool_var_map[n].id == button )
               {
                  bool val = ( BST_CHECKED == IsDlgButtonChecked( wnd, bool_var_map[n].id ) );
                  (*bool_var_map[n].var) = val;
                  music_browser::g_update_colors();
                  return false;
               }
            }
         }

         if ( wp >> 16 == CBN_SELCHANGE )
         {
            int the_list = wp & 0xffff;
            int m = tabsize( list_var_map );
            int n;

            for ( n = 0; n < m; n++ )
            {
               if ( list_var_map[n].id == the_list )
               {
                  int s = SendDlgItemMessage( wnd, the_list, CB_GETCURSEL, 0, 0 );
                  if ( s != CB_ERR )
                  {
                     ( *list_var_map[n].var ) = s;
                     music_browser::g_update_colors();
                  }
                  break;
               }
            }           
         }

         switch ( wp )
         {
         case BN_CLICKED<<16|IDC_BUTTON_REMOVE:
            {
               int n = ListView_GetSelectionMark( GetDlgItem( wnd, IDC_LIST_PANELS ) );

               cfg_browser_configs.remove_by_idx( n );
               update_panel_list( GetDlgItem( wnd, IDC_LIST_PANELS ) );
            }
            break;

         case BN_CLICKED<<16|IDC_BUTTON_ADD:
            {
               browser_config bc( HEADER_NEW, "", "*", 9 );
               cfg_browser_configs.add_item( bc );
               update_panel_list( GetDlgItem( wnd, IDC_LIST_PANELS ) );
               int n = cfg_browser_configs.get_count() - 1;

               if ( DialogBoxParam( core_api::get_my_instance(), 
                     MAKEINTRESOURCE( IDD_PANEL_SETUP ),
                     wnd, panel_edit_proc, n ) == IDCANCEL )
               {
                  cfg_browser_configs.remove_by_idx( n );
               }

               update_panel_list( GetDlgItem( wnd, IDC_LIST_PANELS ) );
            }          
            break;
         case BN_CLICKED<<16|IDC_BUTTON_EDIT:
            {
               int n = ListView_GetSelectionMark( GetDlgItem( wnd, IDC_LIST_PANELS ) );

               if ( n >= 0 && n < ListView_GetItemCount( GetDlgItem( wnd, IDC_LIST_PANELS ) ) )
               {
                  DialogBoxParam( core_api::get_my_instance(), 
                     MAKEINTRESOURCE( IDD_PANEL_SETUP ),
                     wnd, panel_edit_proc, n );

                  update_panel_list( GetDlgItem( wnd, IDC_LIST_PANELS ) );
               }
            }
            break;

			case CBN_SELCHANGE<<16|IDC_DOUBLE_CLICK:
            {
               int n = SendMessage( GetDlgItem( wnd, IDC_DOUBLE_CLICK ), CB_GETCURSEL, 0, 0 );

               if ( n != CB_ERR )
               {
                  cfg_doubleclick.reset();

                  wchar_t tmp_buffer[1024];
                  
                  SendMessage( GetDlgItem( wnd, IDC_DOUBLE_CLICK ),
                     CB_GETLBTEXT, n, (LPARAM) tmp_buffer );

                  cfg_doubleclick.set_string( pfc::stringcvt::string_utf8_from_wide( tmp_buffer ) );
               }
            }
				break;

         case BN_CLICKED<<16|IDC_COLOR_SEL_TEXT:		
            if ( color_picker( wnd, cfg_color_selected_text ))		
            {
               music_browser::g_update_colors();
            }
            break;

         case BN_CLICKED<<16|IDC_COLOR_SEL_TEXT_NON:		
            if ( color_picker( wnd, cfg_color_selected_text_nonfocus ))		
            {
               music_browser::g_update_colors();
            }
            break;

         case BN_CLICKED<<16|IDC_COLOR_SELECTION:		
            if ( color_picker( wnd, cfg_color_selected ))		
            {
               music_browser::g_update_colors();
            }
            break;

         case BN_CLICKED<<16|IDC_COLOR_SELECTIONNONFOCUS:						
            if ( color_picker( wnd, cfg_color_selectednonfocus ))		
            {
               music_browser::g_update_colors();
            }
            break;

         case BN_CLICKED<<16|IDC_BUTTON_TEXT_COLOR:						
            if ( color_picker( wnd, cfg_text_color ))		
            {
               music_browser::g_update_colors();
            }
            break;

         case BN_CLICKED<<16|IDC_BUTTON_BACKGROUND_COLOR:
            if ( color_picker( wnd, cfg_background_color ))		
            {
               music_browser::g_update_colors();
            }
            break;

         case BN_CLICKED<<16|IDC_BUTTON_FONT:
            {
					LOGFONT temp = cfg_font;
               CHOOSEFONT choose;
               memset( &choose, 0, sizeof(choose) );
               choose.lStructSize = sizeof(choose);
	            choose.Flags=CF_SCREENFONTS|CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT;
	            choose.nFontType=SCREEN_FONTTYPE;				
               choose.hwndOwner = wnd;
               choose.lpLogFont = &temp;

					if ( ChooseFont( &choose ) )
					{
                  cfg_font = *choose.lpLogFont;
                  music_browser::g_update_colors();
					}
            }
            break;

         }
         break;

      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_CONFIG, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "Browser";
   }

   virtual GUID get_guid()
   {
      return guid_preferences;
   }

   virtual GUID get_parent_guid()
   {
      return preferences_page::guid_media_library;
   }
	
   //! Queries whether this page supports "reset page" feature.
	virtual bool reset_query()
   {
      //return true;
      return false;
   }

   virtual void reset() 
   {
      cfg_multi_headers    = DEFAULT_MULTI_HEADERS;
      cfg_multi_formats    = DEFAULT_MULTI_FORMATS;
      cfg_multi_sorts      = DEFAULT_MULTI_SORTS;

      cfg_text_color       = DEFAULT_TEXT_COLOR;
      cfg_background_color = DEFAULT_BACKGROUND_COLOR;

      cfg_font             = get_def_font();
      cfg_playlist         = DEFAULT_PLAYLIST;
      cfg_doubleclick      = "";

      cfg_auto_activate_playlist = 0;

      music_browser::g_update_colors();
   }

   virtual bool get_help_url(pfc::string_base & p_out)
   {
      p_out.set_string( URL_HELP );
      return true;
   }
};

static service_factory_single_t<music_browser_pref> g_pref;

/*********************end preferences ***************************************/


// Initialize browser panels in initquit function so that the UI does not take
// forever to load.  Every panel added after initquit will get populated when
// created. - g_populate_on_create
class woot : public initquit
{
   void on_init() 
   {
      if ( cfg_populate_on_load )
      {
         music_browser::g_reset();
      }
      //g_populate_on_create = true;
      g_populate_on_create = cfg_populate_on_load;

      // cfg_browser_configs.remove_all();
#if 1
      // for people upgrading from older versions
      //if ( 1 )
      if ( cfg_browser_configs.get_count() == 0 )
      {
         pfc::ptr_list_t<pfc::string8> strings;   
         pfc::ptr_list_t<pfc::string8> formats;
         pfc::ptr_list_t<pfc::string8> sorts;
         string_split( cfg_multi_formats.get_ptr(), NEWLINE, formats );
         string_split( cfg_multi_sorts.get_ptr(), NEWLINE, sorts );
         string_split( cfg_multi_headers.get_ptr(), NEWLINE, strings );

         int mn = min( strings.get_count(), formats.get_count() );
         mn = min( mn, sorts.get_count() );

         for ( int xx = 0; xx < mn; xx++ )
         {
            browser_config cb( *strings[xx], *formats[xx], *sorts[xx], xx + 1 );
            cfg_browser_configs.add_item( cb );
         }
         /*
         cfg_browser_configs.remove_all();

         browser_config cb1( DEFAULT_HEADER_1, DEFAULT_FORMAT_1, DEFAULT_SORT_1, 1 );
         browser_config cb2( DEFAULT_HEADER_2, DEFAULT_FORMAT_2, DEFAULT_SORT_2, 2 );
         browser_config cb3( DEFAULT_HEADER_3, DEFAULT_FORMAT_3, DEFAULT_SORT_3, 3 );
         browser_config cb4( DEFAULT_HEADER_4, DEFAULT_FORMAT_4, DEFAULT_SORT_4, 4 );
         browser_config cb5( DEFAULT_HEADER_5, DEFAULT_FORMAT_5, DEFAULT_SORT_5, 5 );
         cfg_browser_configs.add_item( cb1 );
         cfg_browser_configs.add_item( cb2 );
         cfg_browser_configs.add_item( cb3 );
         cfg_browser_configs.add_item( cb4 );
         cfg_browser_configs.add_item( cb5 );
         */

         
      }
      else
      {
         /*
         console::printf( "%d items in config", cfg_browser_configs.get_count() );
         for ( int n = 0; n < cfg_browser_configs.get_count(); n++ )
         {
            console::printf( "format = %s, header = %s, sort = %s, precedence = %d",
               cfg_browser_configs[n].get_format(),
               cfg_browser_configs[n].get_header(),
               cfg_browser_configs[n].get_sort(),
               cfg_browser_configs[n].get_precedence() );
         }
         */
      }
#endif

   }

   void on_quit() 
   {
   }
};
static service_factory_single_t<woot> g_woot;
   
/*
VOID CALLBACK browser_timer_callback( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
   MessageBox( NULL, _T("woot"), _T("woot"), MB_OK );
}
*/

bool browser_title_hook::process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag)
{
   if ( m_row )
   {
      if ( pfc::strcmp_ex( p_name, p_name_length, TAG_PANEL, ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d", m_browser->m_index ) );
         p_found_flag = true;
         return true;
      }
      else if ( pfc::strcmp_ex( p_name, p_name_length, "_browser", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, m_browser->m_header );
         p_found_flag = true;
         return true;
      }
      else if ( pfc::strcmp_ex( p_name, p_name_length, "_browser_row", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, m_row->label );
         p_found_flag = true;
         return true;
      }
      else if ( pfc::strcmp_ex( p_name, p_name_length, "_browser_selcount", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d", g_selected_count ) );
         p_found_flag = true;
         return true;
      }

   }
   return false;
}
