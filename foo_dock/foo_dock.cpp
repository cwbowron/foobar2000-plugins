#define VERSION ("1.0.7b [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4018) // don't warn about signed/unsigned
#pragma warning(disable:4065) // don't warn about default case 
#pragma warning(disable:4800) // don't warn about forcing int to bool
#pragma warning(disable:4244) // disable warning convert from t_size to int
#pragma warning(disable:4267) 
#pragma warning(disable:4995)
#pragma warning(disable:4312)
#pragma warning(disable:4311)

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "resource.h"

#include "../common/generic_menu.h"
#include "../common/titleformatting_variable_callback.h"

#define SNAP_PIXELS           5
#define COLLAPSED_SIZE        22
#define SUPER_COLLAPSED_SIZE  0
#define COLLAPSE_TIMEOUT      1000

#define PARENT_DOCK_PROP      _T("dock")
#define URL_HELP              "http://wiki.bowron.us/index.php/Foobar2000"

DECLARE_COMPONENT_VERSION( "Dockable Panels", VERSION, 
   "Dockable Panels\n"
   "By Christopher Bowron <chris@bowron.us>\n"
   "http://wiki.bowron.us/index.php/Foobar2000" )

static const GUID guid_host_001 = { 0xf20e17c7, 0x90e2, 0x40fd, { 0xbf, 0x55, 0x2a, 0x39, 0xda, 0xea, 0x1a, 0xe3 } };
static const GUID guid_config = { 0x57b7faeb, 0xab82, 0x48cc, { 0xba, 0x95, 0x2c, 0xab, 0xc3, 0x5d, 0xb6, 0x8a } };

HFONT    g_vert_font = NULL;
HBRUSH   g_activetitle_brush = NULL;
HBRUSH   g_inactivetitle_brush = NULL;

HWND     g_collapsing_hwnd = NULL;
bool     g_collapsing_horz = false;
bool     g_is_active = false;

///
static const GUID guid_var_000 = { 0x74ee6c5, 0x279a, 0x4ed1, { 0x87, 0x7e, 0xb8, 0x25, 0x35, 0x49, 0xd1, 0x39 } };
static const GUID guid_var_001 = { 0x2ac83f0e, 0x9e37, 0x4aa4, { 0xa7, 0x11, 0x4f, 0x81, 0x1d, 0xbb, 0x88, 0x18 } };
static const GUID guid_var_002 = { 0x857b5294, 0x18b, 0x4496, { 0x89, 0xbc, 0xb4, 0xb1, 0xba, 0x65, 0x36, 0xfb } };
static const GUID guid_var_003 = { 0x3d124682, 0x4ca2, 0x4e0c, { 0xbe, 0xb7, 0xb9, 0x6f, 0xba, 0x2, 0x3c, 0xa9 } };
static const GUID guid_var_004 = { 0xce8ef8de, 0xab5d, 0x4f16, { 0x8f, 0x56, 0x69, 0x4a, 0x55, 0x63, 0x2a, 0x36 } };
static const GUID guid_var_005 = { 0xb686cadd, 0x1b0a, 0x4e11, { 0xb5, 0xc3, 0xbc, 0x59, 0x96, 0xaa, 0xb7, 0x3 } };
static const GUID guid_var_006 = { 0x5831c0e1, 0x9656, 0x44af, { 0xad, 0xe7, 0x19, 0xda, 0x7c, 0xec, 0x54, 0x8f } };

cfg_int cfg_show_dock_arrows( guid_var_000, 1 );
cfg_int cfg_minimize_on_main_minimize( guid_var_001, 1 );
cfg_int cfg_use_expansion_tagz( guid_var_002, 1 );
cfg_int cfg_use_transparency( guid_var_003, 0 );
cfg_int cfg_opacity_active( guid_var_004, 255 );
cfg_int cfg_opacity_inactive( guid_var_005, 120 );
cfg_int cfg_snapping_distance( guid_var_006, 5 );
/// 


pfc::array_t<t_uint8> g_read_buf;
stream_reader_memblock_ref * g_buffer_reader = NULL;

bool is_control_pressed() 
{
   return GetAsyncKeyState( VK_CONTROL ) & 0xF000;
}

class cfg_dockable_panels : public cfg_var
{
public:
   cfg_dockable_panels( const GUID guid ) : cfg_var( guid ) {}
	virtual void get_data_raw(stream_writer * p_stream,abort_callback & p_abort);
	virtual void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort);
};

static cfg_dockable_panels cfg_panels( guid_config );

class dock_titleformat_hook : public titleformat_hook
{
public: 
   virtual bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) { return false; }
   virtual bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);
};
dock_titleformat_hook * g_dock_titleformat_hook = NULL;


bool ensure_rect_on_screen( RECT * rc )
{
   bool result = false;

   int x_min = GetSystemMetrics( SM_XVIRTUALSCREEN );
   int y_min = GetSystemMetrics( SM_YVIRTUALSCREEN );   

   int screen_x = GetSystemMetrics( SM_CXVIRTUALSCREEN );
   int screen_y = GetSystemMetrics( SM_CYVIRTUALSCREEN );

   int x_max = x_min + screen_x;
   int y_max = y_min + screen_y;

   int cx = rc->right - rc->left;
   int cy = rc->bottom - rc->top;

   if ( rc->left < x_min )
   {
      rc->left    = x_min;
      rc->right   = x_min + cx;
      result      = true;
   }

   if ( rc->right > x_max )
   {
      rc->right   = x_max;
      rc->left    = x_max - cx;
      result      = true;
   }

   if ( rc->top < y_min )
   {
      rc->top     = y_min;
      rc->bottom  = y_min + cy;
      result      = true;
   }

   if ( rc->bottom > y_max )
   {
      rc->bottom  = y_max;
      rc->top     = y_max - cy;
      result      = true;
   }

   return result;
}


class dockable_host_window : public ui_extension::window_host
{
public:
   HWND  m_dock_hwnd_top;        // what window is our top attached to
   HWND  m_dock_hwnd_bottom;     // ''  '' bottom
   HWND  m_dock_hwnd_left;       // ''  '' left
   HWND  m_dock_hwnd_right;      // ''  '' right
   int   m_dock_x;               // x offset from bottom / top docked window
   int   m_dock_y;               // y offset from left / right docked window
   HWND  m_docked_hwnd;          // the hwnd we are hosting
   HWND  m_hwnd;                 // our hwnd

   ui_extension::window_ptr m_panel_window;

   bool        m_collapsed;
   int         m_restore_cx;
   int         m_restore_cy;

   unsigned    m_dock_index_top;       // for holding the docking index when storing/retrieving
   unsigned    m_dock_index_bottom;    // 0 = side not docked
   unsigned    m_dock_index_left;      // 1 = docked to main
   unsigned    m_dock_index_right;     // n = docked to window - 2
   
   pfc::string8   m_custom_title;
   pfc::string8   m_expansion_tagz;

   bool        m_hide_titlebar;
   bool        m_hide_closebox;
   bool        m_horz_collapse;
   bool        m_auto_collapse;
   bool        m_locked;
   bool        m_no_frame;
   bool        m_super_collapsed;

   WNDPROC     m_window_proc;

   static  pfc::string8    g_string_input_title;

   dockable_host_window()
      : ui_extension::window_host()
      , m_dock_hwnd_top( NULL )
      , m_dock_hwnd_bottom( NULL )
      , m_dock_hwnd_left( NULL )
      , m_dock_hwnd_right( NULL )
      , m_dock_x( 0 )
      , m_dock_y( 0 )
      , m_panel_window( NULL )
      , m_docked_hwnd( NULL )
      , m_collapsed( false )
      , m_dock_index_top( 0 )
      , m_dock_index_bottom( 0 )
      , m_dock_index_left( 0 )
      , m_dock_index_right( 0 )
      , m_restore_cx( 0 )
      , m_restore_cy( 0 )
      , m_custom_title( )
      , m_hide_titlebar( false )
      , m_hide_closebox( false )
      , m_horz_collapse( false )
      , m_auto_collapse( false )
      , m_locked( false )
      , m_window_proc( NULL )
      , m_no_frame( false )
      , m_super_collapsed( false )
      , m_expansion_tagz( )
   {
   }

   ~dockable_host_window();

   static service_ptr_t<dockable_host_window> dockable_host_window::create();

   static dockable_host_window * find_by_child( HWND wnd );

   // we install this hook in all our child windows
   static LRESULT CALLBACK child_hook( HWND wnd, UINT msg, WPARAM wp, LPARAM lp );

   void install_child_hook()
   {        
      m_window_proc = uHookWindowProc( m_docked_hwnd, child_hook );
      SetProp( m_docked_hwnd, PARENT_DOCK_PROP, (HANDLE)this );
   }

   void uninstall_child_hook()
   {    
      uHookWindowProc( m_docked_hwnd, m_window_proc );
      RemoveProp( m_docked_hwnd, PARENT_DOCK_PROP );
   }

   void on_mousemove( const char * who )
   {
      if ( m_auto_collapse )
      {
         if ( m_collapsed )
         {
            expand();
         }

         SetTimer( m_hwnd, 0, COLLAPSE_TIMEOUT, NULL );
      }
   }

   void set_window_pointer( HWND wnd )
   {
      SetWindowLongPtr( m_hwnd, DWLP_USER, (LONG) this );
   }

   static dockable_host_window * get_pointer_from_hwnd( HWND hwnd )
   {
      return (dockable_host_window *)GetWindowLongPtr( hwnd, DWLP_USER );
   }

   void on_mouseleave( const char * who )
   {
   }


   bool creates_circle( dockable_host_window * find_me );
   
   void collapse();  
   void expand();

   void collapse_or_expand()
   {
      if ( m_collapsed ) 
         expand();
      else
         collapse();
   }

   void update_docked_windows();

   void refresh_child()
   {
      if ( m_docked_hwnd && !m_collapsed )
      {
         InvalidateRect( m_docked_hwnd, NULL, TRUE );
         UpdateWindow( m_docked_hwnd );
      }
   }

   void update_style()
   {
      DWORD dwStyle     = GetWindowLong( m_hwnd, GWL_STYLE );
      DWORD dwExStyle   = GetWindowLong( m_hwnd, GWL_EXSTYLE );

      if ( m_collapsed )
      {
         dwStyle &= ~WS_SIZEBOX;
         if ( m_docked_hwnd )
         {
            ShowWindow( m_docked_hwnd, SW_HIDE );
            // ShowWindow( m_docked_hwnd, SW_MINIMIZE );
         }
      }
      else
      {
         dwStyle |= WS_SIZEBOX;
         if ( m_docked_hwnd )
         {
            ShowWindow( m_docked_hwnd, SW_SHOW );
            // ShowWindow( m_docked_hwnd, SW_SHOWNORMAL );
         }
      }

      if ( ( m_hide_titlebar && !m_collapsed ) || ( m_collapsed && m_horz_collapse ) )
      {
         dwExStyle &= ~WS_EX_TOOLWINDOW;
         dwStyle   &= ~WS_CAPTION;
      }
      else
      {
         dwExStyle |= WS_EX_TOOLWINDOW;
         dwStyle   |= WS_CAPTION;
      }

      if ( m_collapsed && m_horz_collapse )
      {
         dwStyle |= WS_DLGFRAME;
      }

      if ( m_hide_closebox || m_collapsed )
      {
         dwStyle &= ~WS_SYSMENU;
      }
      else
      {
         dwStyle |= WS_SYSMENU;
      }

      if ( m_super_collapsed && m_collapsed )
      {
         dwStyle &= ~ WS_CAPTION;
      }

      if ( m_no_frame && !m_collapsed )
      {
         dwExStyle   &= ~WS_EX_TOOLWINDOW;
         dwStyle     &= ~WS_CAPTION;
         dwStyle     &= ~WS_DLGFRAME;
         dwStyle     &= ~WS_SIZEBOX;
      }

      if ( cfg_use_transparency )
      {
         dwExStyle |= WS_EX_LAYERED;
      }
      else
      {
         dwExStyle &= ~WS_EX_LAYERED;
      }

      SetWindowLong( m_hwnd, GWL_STYLE, dwStyle );
      SetWindowLong( m_hwnd, GWL_EXSTYLE, dwExStyle );

      if ( cfg_use_transparency )
      {
         if ( g_is_active )
         {
            SetLayeredWindowAttributes( m_hwnd, 0, cfg_opacity_active, LWA_ALPHA );
         }
         else
         {
            SetLayeredWindowAttributes( m_hwnd, 0, cfg_opacity_inactive, LWA_ALPHA );
         }
      }

      SetWindowPos( m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
   }

   void toggle_closebox()
   {
      m_hide_closebox = !m_hide_closebox;
      update_style();
   }

   void toggle_titlebar()
   {
      m_hide_titlebar = !m_hide_titlebar;
      update_style();
   }

   void toggle_no_frame()
   {
      m_no_frame = !m_no_frame;
      update_style();
   }

   void toggle_auto_collapse()
   {
      m_auto_collapse = !m_auto_collapse;
   }

   // Check to make sure that its still connected to whatever it was connected to...
   void update_docked_hwnd();

   void snap_to( RECT * pRc, bool moving = false );

   void do_contextmenu( int x, int y );

   void process_menu_command( int n );

   void get_title( pfc::string_base & title )
   {
      if ( !m_custom_title.is_empty() )
      {
         title.set_string( m_custom_title );
      }
      else if ( m_panel_window.is_valid() )
      {
         m_panel_window->get_name( title );
      }
      else
      {
         title.set_string( "Dockable Panel" );
      }
   }

   static BOOL CALLBACK rename_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      static pfc::string_base * str;

      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            str = (pfc::string_base *) lp;
            uSetDlgItemText( wnd, IDC_NAME, *str );
            if ( g_string_input_title.is_empty() == false )
            {
               uSetWindowText( wnd, g_string_input_title );
            }
            SetFocus( GetDlgItem( wnd, IDC_NAME ) );
         }
         return FALSE;

      case WM_COMMAND:
         switch ( LOWORD(wp) )
         {
         case IDOK:
            str->reset();
            uGetDlgItemText( wnd, IDC_NAME, *str );
            // FALL THROUGH

         case IDCANCEL:
            EndDialog( wnd, wp );
            return TRUE;
         }
      }

      return FALSE;
   }

   void get_string_input( pfc::string_base * in_out, const char * title = NULL )
   {
      g_string_input_title = title;
      uDialogBox( IDD_RENAME_DIALOG, m_hwnd, rename_dialog_proc, (LPARAM)in_out );
      g_string_input_title.reset();
   }

   void rename_custom()
   {
      get_string_input( &m_custom_title );
      update_title();
   }

   void get_expansion_tagz()
   {
      get_string_input( &m_expansion_tagz, "Expansion Tagz" );

      if ( m_expansion_tagz.is_empty() == false && cfg_use_expansion_tagz )
      {
         static_api_ptr_t<playback_control> p;
         metadb_handle_ptr handle;
         if ( p->get_now_playing( handle ) )
         {
            process_expansion_tagz( handle );
         }
      }
   }

   void process_expansion_tagz( metadb_handle_ptr handle )
   {
      if ( m_expansion_tagz.is_empty() == false )
      {
         pfc::string8_fastalloc tag_out;
         bool b = handle->format_title_legacy( g_dock_titleformat_hook, tag_out, m_expansion_tagz, NULL );

         if ( !b )
            return;

         bool expand_me = tag_out.is_empty() ? false : true;

         if ( m_collapsed )
         {
            if ( expand_me )
            {
               expand();
            }
         }
         else
         {
            if ( !expand_me )
            {
               collapse();
            }
         }
      }
   }

   static void process_all_expansion_tagz( metadb_handle_ptr handle, dockable_host_window * skip = NULL );

   static void process_all_expansion_tagz_now_playing( dockable_host_window * skip = NULL )
   {
      static_api_ptr_t<playback_control> p;
      metadb_handle_ptr handle;
      if ( p->get_now_playing( handle ) )
      {
         process_all_expansion_tagz( handle, skip );
      }
   }

   void activate()
   {
      if ( !IsIconic( m_hwnd )  )
      {
         SetWindowPos( m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
         if ( m_docked_hwnd )
         {
            InvalidateRect( m_docked_hwnd, NULL, TRUE );
            UpdateWindow( m_docked_hwnd );
         }
      }
   }

   void ensure_on_screen()
   {
      RECT rc;
      GetWindowRect( m_hwnd, &rc );

      if ( ensure_rect_on_screen( &rc ) )
      {
         MoveWindow( m_hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );     
      }
   }

   void update_title()
   {
      pfc::string8_fastalloc title;

      get_title( title );

      if ( cfg_show_dock_arrows )
      {
         if ( m_dock_hwnd_left || m_dock_hwnd_top || m_dock_hwnd_bottom || m_dock_hwnd_right )
         {
            title.add_string( " [" );

            if ( m_dock_hwnd_left )
            {
               title.add_char( 0x00AB );
            }      
            if ( m_dock_hwnd_top )
            {
               title.add_string( "^" );
            }
            if ( m_dock_hwnd_bottom )
            {
               title.add_string( "V" );
            }
            if ( m_dock_hwnd_right )
            {
               title.add_char( 0x00BB );
            }
            title.add_string( "]" );
         }
      }

      /*
      pfc::string_printf fmt( " ( %d, %d )", m_dock_x, m_dock_y );
      title.add_string( fmt );
      */

      uSetWindowText( m_hwnd, title );
   }

   static unsigned lookup_index( HWND hwnd );

   bool create_child( GUID guid, stream_reader * stream = NULL, t_size config_size = 0, abort_callback * abort = NULL )
   {
      bool was_created = ui_extension::window::create_by_guid( guid, m_panel_window );

      if ( !was_created )
      {
         console::printf( "create_by_guid failed: %s", (const char *)pfc::print_guid( guid ) );
         return false;
      }

      if ( stream && !m_panel_window->get_is_single_instance() )
      {
         if ( config_size > 0 )
         {
            m_panel_window->set_config( stream, config_size, *abort );
         }
      }

      m_docked_hwnd =  m_panel_window->create_or_transfer_window( m_hwnd, this );

      if ( m_docked_hwnd )
      {
         RECT rc;
         GetClientRect( m_hwnd, &rc );
         int x = rc.left;
         int y = rc.top;
         int cx = rc.right - rc.left;
         int cy = rc.bottom - rc.top;

         install_child_hook();

         SetWindowPos( m_docked_hwnd, NULL, 0, 0, cx, cy, SWP_SHOWWINDOW | SWP_NOZORDER  | SWP_NOOWNERZORDER );
         pfc::string8 window_name;                  
         m_panel_window->get_name( window_name );
         update_title();
      }
      else
      {
         console::printf( "failed to create panel" );
         return false;
      }
      return true;
   }

   bool load_config( unsigned fmt, stream_reader * f, abort_callback & abort )
   {
      f->read( &m_collapsed, sizeof(m_collapsed), abort );
      if ( m_collapsed )
      {
         DWORD dwStyle = GetWindowLong( m_hwnd, GWL_STYLE ) & ~(WS_SIZEBOX);
         SetWindowLong( m_hwnd, GWL_STYLE, dwStyle );
      }
      else
      {          
         DWORD dwStyle = GetWindowLong( m_hwnd, GWL_STYLE ) | (WS_SIZEBOX);         
         SetWindowLong( m_hwnd, GWL_STYLE, dwStyle );
      }

      f->read( &m_restore_cx, sizeof(m_restore_cx), abort );
      f->read( &m_restore_cy, sizeof(m_restore_cy), abort );

#ifdef DEBUG_LOAD
      console::printf( "m_collapsed: %s", m_collapsed ? "true" : "false" );
      console::printf( "m_restore_cx: %d", m_restore_cx );
      console::printf( "m_restore_cy: %d", m_restore_cy );
#endif      

      bool iconic;
      WINDOWPLACEMENT place;
      memset( &place, 0, sizeof(place) );
      place.length = sizeof(WINDOWPLACEMENT);     

      f->read( &iconic, sizeof(iconic), abort );
      f->read( &place.rcNormalPosition, sizeof(place.rcNormalPosition), abort );

#ifdef DEBUG_LOAD
      console::printf( "iconic: %s", iconic ? "true" : "false" );
      console::printf( "position = (%d, %d)-(%d, %d)", 
         place.rcNormalPosition.left, place.rcNormalPosition.top,
         place.rcNormalPosition.right, place.rcNormalPosition.bottom );
#endif

      // int screen_x = GetSystemMetrics( SM_CXVIRTUALSCREEN );
      // int screen_y = GetSystemMetrics( SM_CYVIRTUALSCREEN );

#ifdef DEBUG_LOAD      
      console::printf( "screen_x: %d", screen_x );
      console::printf( "screen_y: %d", screen_y );
#endif
      
      ensure_rect_on_screen( &place.rcNormalPosition );
      /*
      int cx = place.rcNormalPosition.right - place.rcNormalPosition.left;
      int cy = place.rcNormalPosition.bottom - place.rcNormalPosition.top ;

      if ( place.rcNormalPosition.left < 0 )
      {
         place.rcNormalPosition.left = 0;
         place.rcNormalPosition.right = cx;
      }
      if ( place.rcNormalPosition.right > screen_x )
      {
         place.rcNormalPosition.right = screen_x;
         place.rcNormalPosition.left = screen_x - cx;
      }
      if ( place.rcNormalPosition.top < 0 )
      {
         place.rcNormalPosition.top = 0;
         place.rcNormalPosition.bottom = cy;
      }
      if ( place.rcNormalPosition.bottom > screen_y )
      {
         place.rcNormalPosition.bottom = screen_y;
         place.rcNormalPosition.top = screen_y - cy;
      }
      */

      place.showCmd = iconic ? SW_MINIMIZE : SW_SHOWNORMAL;

      f->read( &m_dock_index_left, sizeof(m_dock_index_left), abort );
      f->read( &m_dock_index_top, sizeof(m_dock_index_top), abort );
      f->read( &m_dock_index_right, sizeof(m_dock_index_right), abort );
      f->read( &m_dock_index_bottom, sizeof(m_dock_index_bottom), abort );

      /*
      console::printf( "docked: %d, %d, %d, %d", m_dock_index_left, m_dock_index_top, 
         m_dock_index_right, m_dock_index_bottom );
      */

      f->read( &m_dock_x, sizeof(m_dock_x), abort );
      f->read( &m_dock_y, sizeof(m_dock_y), abort );

      /*
      console::printf( "docking: (%d, %d)", m_dock_x, m_dock_y );
      */

      bool has_guid;

      f->read_string( m_custom_title, abort );
      f->read( &has_guid, sizeof(has_guid), abort );
      if ( has_guid )
      {
         GUID guid;
         f->read( &guid, sizeof(guid), abort );

         t_size config_size = ~0;

         if ( fmt >= 4 )
         {
            f->read( &config_size, sizeof(config_size), abort );
         }         
         
         if ( !create_child( guid, f, config_size, &abort ) )
         {
            return false;
         }
      }

      if ( fmt > 0 )
      {
         f->read( &m_hide_closebox, sizeof(m_hide_closebox), abort );
         f->read( &m_hide_titlebar, sizeof(m_hide_titlebar), abort );
         f->read( &m_horz_collapse, sizeof(m_horz_collapse), abort );
         f->read( &m_auto_collapse, sizeof(m_auto_collapse), abort );
         f->read( &m_no_frame, sizeof(m_no_frame), abort );

         if ( fmt > 1 )
         {
            f->read( &m_super_collapsed, sizeof(m_super_collapsed), abort );         
            if ( fmt > 2 )
            {
               f->read_string( m_expansion_tagz, abort );
            }
         }
      }

      update_style();
      SetWindowPlacement( m_hwnd, &place );
      return true;
   }

   stream_writer_memblock m_buffer;

   void store_config_buffer()   
   {
	  if (m_buffer.m_data.get_size()>0) return;
      stream_writer* f = &m_buffer;
	  abort_callback_impl abort;

      f->write( &m_collapsed, sizeof(m_collapsed), abort );
      f->write( &m_restore_cx, sizeof(m_restore_cx), abort );
      f->write( &m_restore_cy, sizeof(m_restore_cy), abort );

      bool iconic = IsIconic( m_hwnd ) ? true : false;
      WINDOWPLACEMENT place;
      memset( &place, 0, sizeof(place) );
      place.length = sizeof(WINDOWPLACEMENT);
      GetWindowPlacement( m_hwnd, &place );

      f->write( &iconic, sizeof(iconic), abort );
      f->write( &place.rcNormalPosition, sizeof(place.rcNormalPosition), abort );

      m_dock_index_left = lookup_index( m_dock_hwnd_left );
      m_dock_index_top = lookup_index( m_dock_hwnd_top );
      m_dock_index_right = lookup_index( m_dock_hwnd_right );
      m_dock_index_bottom = lookup_index( m_dock_hwnd_bottom );

      f->write( &m_dock_index_left, sizeof(m_dock_index_left), abort );
      f->write( &m_dock_index_top, sizeof(m_dock_index_top), abort );
      f->write( &m_dock_index_right, sizeof(m_dock_index_right), abort );
      f->write( &m_dock_index_bottom, sizeof(m_dock_index_bottom), abort );

      f->write( &m_dock_x, sizeof(m_dock_x), abort );
      f->write( &m_dock_y, sizeof(m_dock_y), abort );

      bool has_guid = m_panel_window.is_valid();

      f->write_string( m_custom_title, abort );

      f->write( &has_guid, sizeof(has_guid), abort );

      if ( has_guid )
      {
         GUID guid = m_panel_window->get_extension_guid();
         f->write( &guid, sizeof(guid), abort );

         if ( !m_panel_window->get_is_single_instance() )
         {
            stream_writer_memblock fake_writer;
            m_panel_window->get_config( &fake_writer, abort );            		

            t_size mem_size = fake_writer.m_data.get_size();
            f->write( &mem_size, sizeof(mem_size), abort );

      		if ( mem_size > 0 )
            {
			      f->write( fake_writer.m_data.get_ptr(), mem_size, abort);
            }
         }
         else
         {
            t_size mem_size = 0;
            f->write( &mem_size, sizeof(mem_size), abort );
         }
      }

      f->write( &m_hide_closebox, sizeof(m_hide_closebox), abort );
      f->write( &m_hide_titlebar, sizeof(m_hide_titlebar), abort );
      f->write( &m_horz_collapse, sizeof(m_horz_collapse), abort );
      f->write( &m_auto_collapse, sizeof(m_auto_collapse), abort );
      f->write( &m_no_frame, sizeof(m_no_frame), abort );
      f->write( &m_super_collapsed, sizeof(m_super_collapsed), abort );

      f->write_string( m_expansion_tagz, abort );
   }

   void store_config(stream_writer * f, abort_callback & abort) {
	   if (m_buffer.m_data.get_size()==0) {
		   store_config_buffer();
	   }
	   f->write(m_buffer.m_data.get_ptr(), m_buffer.m_data.get_size(), abort);
   }
	   	
   static void g_on_activate_app( bool active );
   static void g_show_panels( bool show );

   // WINDOW_HOST FUNCTIONS
	virtual const GUID & get_host_guid() const 
   { 
      return guid_host_001; 
   }
	virtual void on_size_limit_change(HWND wnd, unsigned flags) {}
	virtual unsigned is_resize_supported(HWND wnd) const 
   {
      return false; 
   }
	virtual bool request_resize(HWND wnd, unsigned flags, unsigned width, unsigned height) { return false; }
   virtual bool get_keyboard_shortcuts_enabled() const { return true; }
   virtual bool is_visible(HWND wnd) const { return ( m_hwnd && IsWindowVisible( m_hwnd ) ); }
	virtual bool override_status_text_create(service_ptr_t<ui_status_text_override> & p_out) {return false;}
	virtual bool is_visibility_modifiable(HWND wnd, bool desired_visibility) const { return false; }
	virtual bool set_window_visibility(HWND wnd, bool visibility) { return true; }
	virtual void relinquish_ownership(HWND wnd) 
   { 
      m_docked_hwnd = NULL; 
      m_panel_window = NULL;
   }
   // END WINDOW_HOST FUNCTIONS
};

pfc::string8    dockable_host_window::g_string_input_title;

pfc::list_t< service_ptr_t<dockable_host_window> > g_host_windows;
dockable_host_window * g_last_active = NULL;


dockable_host_window::~dockable_host_window()
{
   if ( this == g_last_active )
   {
      g_last_active = NULL; 
   }
}

void dockable_host_window::g_on_activate_app( bool active )
{
   g_is_active = active;

   if ( active )
   {                 
      ShowOwnedPopups( core_api::get_main_window(), TRUE );
   }

   for ( int n = 0; n < g_host_windows.get_count(); n++ )
   {
      g_host_windows[n]->update_style();
   }
}

void dockable_host_window::g_show_panels( bool show )
{
   for ( int n = 0; n < g_host_windows.get_count(); n++ )
   {
      if ( g_host_windows[n]->m_hwnd )
      {
         ShowWindow( g_host_windows[n]->m_hwnd, show ? SW_SHOWNORMAL : SW_HIDE );
      }
   }
}

dockable_host_window * dockable_host_window::find_by_child( HWND wnd )
{
   for ( int n = 0; n < g_host_windows.get_count(); n++ )
   {
      if ( g_host_windows[n]->m_docked_hwnd == wnd )
         return g_host_windows[n].get_ptr();
   }
   return NULL;
}

   
void dockable_host_window::update_docked_windows()
{
   HWND * hwnds[] = { &m_dock_hwnd_top, &m_dock_hwnd_left, &m_dock_hwnd_right, &m_dock_hwnd_bottom };

   for ( int n = 0; n < tabsize(hwnds); n++ )
   {
      HWND h = *(hwnds[n]);
        
      if ( h )
      {
         dockable_host_window * w = dockable_host_window::get_pointer_from_hwnd( h );
         if ( w )
         {
            w->update_docked_hwnd();
         }
      }
   }
}

void dockable_host_window::collapse()
{
   m_collapsed = true;
   m_horz_collapse = false;

   update_style();

   RECT rc;
   GetWindowRect( m_hwnd, &rc );
   m_restore_cx = rc.right - rc.left;
   m_restore_cy = rc.bottom - rc.top;

   int cy = m_super_collapsed ? SUPER_COLLAPSED_SIZE : COLLAPSED_SIZE;

   bool docked_horz = ( m_dock_hwnd_left != NULL ) || ( m_dock_hwnd_right != NULL );
   bool docked_vert = ( m_dock_hwnd_top != NULL ) || ( m_dock_hwnd_bottom != NULL );

   g_collapsing_hwnd = m_hwnd;
   g_collapsing_horz = false;

   // collapse horizontally
   if ( docked_horz && !docked_vert )
   {
      int cx = m_super_collapsed ? SUPER_COLLAPSED_SIZE : COLLAPSED_SIZE;
      // int cx = 30;
      cy = rc.bottom - rc.top;
      m_horz_collapse = true;

      g_collapsing_horz = true;

      if ( m_dock_hwnd_left )
      {
         MoveWindow( m_hwnd, rc.left, rc.top, cx, cy, TRUE );
      }
      else
      {
         MoveWindow( m_hwnd, rc.right - cx, rc.top, cx, cy, TRUE );
      }
   }
   // collapse down if we are docked at the bottom
   else if ( m_dock_hwnd_bottom )
   {
      MoveWindow( m_hwnd, rc.left, rc.bottom - cy, rc.right - rc.left, cy, TRUE );
   }
   else
   {
      MoveWindow( m_hwnd, rc.left, rc.top, rc.right - rc.left, cy, TRUE );
   }

   g_collapsing_hwnd = NULL;
   update_docked_windows();
   update_docked_hwnd();  
   update_style();


   if ( m_super_collapsed && m_collapsed )
   {         
      ShowWindow( m_hwnd, SW_HIDE );    
   }

   process_all_expansion_tagz_now_playing( this );
}

void dockable_host_window::expand()
{
   m_collapsed = false;

   g_collapsing_hwnd = m_hwnd;
   g_collapsing_horz = false;

   RECT rc;
   GetWindowRect( m_hwnd, &rc );

   bool docked_horz = ( m_dock_hwnd_left != NULL ) || ( m_dock_hwnd_right != NULL );
   bool docked_vert = ( m_dock_hwnd_top != NULL ) || ( m_dock_hwnd_bottom != NULL );

   if ( docked_horz && !docked_vert )
   {
      g_collapsing_horz = true;
      if ( m_dock_hwnd_left )
      {
         MoveWindow( m_hwnd, rc.left, rc.top, m_restore_cx, m_restore_cy, TRUE );
      }
      else
      {
         MoveWindow( m_hwnd, rc.right - m_restore_cx, rc.top, m_restore_cx, m_restore_cy, TRUE );
      }
   }
   else if ( m_dock_hwnd_bottom )
   {
      MoveWindow( m_hwnd, rc.left, rc.bottom - m_restore_cy, m_restore_cx, m_restore_cy, TRUE );
   }
   else
   {
      MoveWindow( m_hwnd, rc.left, rc.top, m_restore_cx, m_restore_cy, TRUE );
   }

   ShowWindow( m_hwnd, SW_SHOWNORMAL );

   update_style();
   update_docked_windows();
   update_docked_hwnd();
   refresh_child();
   g_collapsing_hwnd = NULL;

   process_all_expansion_tagz_now_playing( this );
}

LRESULT CALLBACK dockable_host_window::child_hook( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   HWND parent = GetParent( wnd );

   dockable_host_window * host = (dockable_host_window*) GetProp( wnd, PARENT_DOCK_PROP );

   if ( host && ( parent != host->m_hwnd ) )
   {
      LRESULT lr = host->m_window_proc( wnd, msg, wp, lp );
      host->uninstall_child_hook();
      return lr;     
   }
   dockable_host_window * host_window = (dockable_host_window *) GetWindowLongPtr( parent, DWLP_USER );
     
   switch ( msg )
   {
   case WM_CONTEXTMENU:
      if ( is_control_pressed() )
      {
         host_window->do_contextmenu( GET_X_LPARAM(lp), GET_Y_LPARAM( lp ) );
         return TRUE;
      }
      break;
   }

   return host_window->m_window_proc( wnd, msg, wp, lp );
}

// am I created to find_me by a loop?
bool dockable_host_window::creates_circle( dockable_host_window * find_me )
{
   HWND * sides[] = { &m_dock_hwnd_top, &m_dock_hwnd_left, &m_dock_hwnd_right, &m_dock_hwnd_bottom };

   for ( int n = 0; n < tabsize(sides); n++ )
   {
      HWND attachee = *(sides[n]);
      if ( attachee && attachee != core_api::get_main_window() )
      {
         dockable_host_window * host = get_pointer_from_hwnd( attachee );

         if ( host )
         {
            if ( host == find_me )
            {
               return true;
            }
            else
            {
               if ( host->creates_circle( find_me ) )
               {
                  return true;
               }
            }
         }
      }
   }
   
   return false;
}

unsigned dockable_host_window::lookup_index( HWND hwnd )
{
   if ( hwnd == NULL )
      return 0;
   if ( hwnd == core_api::get_main_window() )
      return 1;

   for ( int n = 0; n < g_host_windows.get_count(); n++ )
   {
      if ( g_host_windows[n]->m_hwnd == hwnd )
         return n + 2;
   }

   return 0;
}

void dockable_host_window::update_docked_hwnd()
{
   RECT rcMe;
   RECT rcNeighbor;

   struct { 
      const char * str; 
      HWND * h; 
      HWND * opp;
      LONG * us; 
      LONG * them; 
      int * offset; 
      LONG * offset_me; 
      LONG * offset_them; 
      LONG * bound_test1;
      LONG * bound_limit1;
      LONG * bound_test2;
      LONG * bound_limit2;

   } ar[] = 
   {
      // host_window->m_dock_x = pRc->left - rcMain.left;
      { "left", &m_dock_hwnd_left, &m_dock_hwnd_right, &rcMe.left, &rcNeighbor.right, &m_dock_y, &rcMe.top, &rcNeighbor.top, 
         &rcMe.top, &rcNeighbor.bottom, &rcMe.bottom, &rcNeighbor.top },
      { "right", &m_dock_hwnd_right, &m_dock_hwnd_left, &rcMe.right, &rcNeighbor.left, &m_dock_y, &rcMe.top, &rcNeighbor.top,
         &rcMe.top, &rcNeighbor.bottom, &rcMe.bottom, &rcNeighbor.top },
      { "top", &m_dock_hwnd_top, &m_dock_hwnd_bottom, &rcMe.top, &rcNeighbor.bottom, &m_dock_x, &rcMe.left, &rcNeighbor.left,
         &rcMe.left, &rcNeighbor.right, &rcMe.right, &rcNeighbor.left },
      { "bottom", &m_dock_hwnd_bottom, &m_dock_hwnd_top, &rcMe.bottom, &rcNeighbor.top, &m_dock_x, &rcMe.left, &rcNeighbor.left,
         &rcMe.left, &rcNeighbor.right, &rcMe.right, &rcNeighbor.left },
   };

   GetWindowRect( m_hwnd, &rcMe );

   m_dock_hwnd_left     = NULL;
   m_dock_hwnd_right    = NULL;
   m_dock_hwnd_bottom   = NULL;
   m_dock_hwnd_top      = NULL;

   m_dock_x = 0;
   m_dock_y = 0;

   // FOR ALL WINDOWS
   for ( int n = 0; n <= g_host_windows.get_count(); n++ )
   {
      HWND hwnd_test;            
      dockable_host_window * test_window = NULL;
      
      if ( n == 0 )
      {
         hwnd_test = core_api::get_main_window();
      }
      else
      {
         hwnd_test = g_host_windows[n-1]->m_hwnd;
         test_window = (dockable_host_window *) GetWindowLongPtr( hwnd_test, DWLP_USER );
      }

      if ( test_window && test_window == this )
         continue;

      GetWindowRect( hwnd_test, &rcNeighbor );
      for ( int m = 0; m < tabsize( ar ); m++ )
      {
         if ( *(ar[m].h) )
            continue;

         if ( *(ar[m].opp) )
            continue;

         if ( *(ar[m].bound_test1) >= *(ar[m].bound_limit1) )
            continue;

         if ( *(ar[m].bound_test2) <= *(ar[m].bound_limit2) )
            continue;

         if ( n > 0 )
         {
            if ( test_window->creates_circle( this ) )
               continue;
         }

         if ( *(ar[m].us) == *( ar[m].them ) )
         {
            *( ar[m].h ) = hwnd_test;
            *(ar[m].offset) = *(ar[m].offset_me) - *(ar[m].offset_them);
         }
      }
   }

   update_title();
}
  
void dockable_host_window::process_all_expansion_tagz( metadb_handle_ptr handle, dockable_host_window * skip /*= NULL*/ )
{
   if ( cfg_use_expansion_tagz )
   {
      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         if ( g_host_windows[n].get_ptr() != skip )
         {
            g_host_windows[n]->process_expansion_tagz( handle );
         }
      }
   }
}

void dockable_host_window::snap_to( RECT * pRc, bool moving /* = true */ )
{
   RECT rcMain;

   HWND  wnd = m_hwnd;
   /*
   host_window->m_dock_hwnd_top     = NULL;
   host_window->m_dock_hwnd_bottom  = NULL;
   host_window->m_dock_hwnd_left    = NULL;
   host_window->m_dock_hwnd_right   = NULL;
   host_window->m_dock_x            = 0;
   host_window->m_dock_y            = 0;
   */
   bool attach_left     = false;
   bool attach_right    = false;
   bool attach_bottom   = false;
   bool attach_top      = false;

   for ( int n = 0; n <= g_host_windows.get_count(); n++ )
   {
      HWND hwnd_test;            
      dockable_host_window * test_window = NULL;
      
      if ( n == 0 )
      {
         hwnd_test = core_api::get_main_window();
      }
      else
      {
         hwnd_test = g_host_windows[n-1]->m_hwnd;
         test_window = (dockable_host_window *) GetWindowLongPtr( hwnd_test, DWLP_USER );
      }

      if ( hwnd_test == wnd )
      {
         continue;
      }

      GetWindowRect( hwnd_test, &rcMain );

      int cx = pRc->right - pRc->left;
      int cy = pRc->bottom - pRc->top;

      if ( pRc->top <= rcMain.bottom && pRc->bottom >= rcMain.top )
      {
         if ( test_window && test_window->m_dock_hwnd_left == wnd )
         {
         }
         else if ( test_window && test_window->m_dock_hwnd_right == wnd )
         {
         }
         else
         {
            if ( !attach_right &&  abs( pRc->right - rcMain.left ) < cfg_snapping_distance )
            {
               pRc->right = rcMain.left;
               if ( moving ) pRc->left = rcMain.left - cx;
               attach_right = true;
            }
            if ( !attach_right &&  abs( pRc->right - rcMain.right ) < cfg_snapping_distance )
            {
               pRc->right = rcMain.right;
               if ( moving ) pRc->left = rcMain.right - cx;
               attach_right = true;
            }
            if ( !attach_left && abs( pRc->left - rcMain.right ) < cfg_snapping_distance )
            {
               pRc->left = rcMain.right;
               if ( moving ) pRc->right = rcMain.right + cx;
               attach_left = true;
            }
            if ( !attach_left && abs( pRc->left - rcMain.left ) < cfg_snapping_distance )
            {
               pRc->left = rcMain.left;
               if ( moving ) pRc->right = rcMain.left + cx;
               attach_left = true;
            }
         }
      }
      
      if ( pRc->left <= rcMain.right && pRc->right >= rcMain.left )
      {
         if ( test_window && test_window->m_dock_hwnd_right == wnd )
         {
         }
         if ( test_window && test_window->m_dock_hwnd_left == wnd )
         {            
         }
         else
         {
            if ( !attach_bottom && abs( pRc->bottom - rcMain.top ) < cfg_snapping_distance )
            {
               pRc->bottom = rcMain.top;
               if ( moving ) pRc->top = rcMain.top - cy;
               attach_bottom = true;
            }
            if ( !attach_bottom && abs( pRc->bottom - rcMain.bottom ) < cfg_snapping_distance )
            {
               pRc->bottom = rcMain.bottom;
               if ( moving ) pRc->top = rcMain.bottom - cy;
               attach_bottom = true;
            }
            if ( !attach_top && abs( pRc->top - rcMain.bottom ) < cfg_snapping_distance )
            {
               pRc->top = rcMain.bottom;
               if ( moving ) pRc->bottom = rcMain.bottom + cy;
               attach_top = true;
            }
            if ( !attach_top && abs( pRc->top - rcMain.top ) < cfg_snapping_distance )
            {
               pRc->top = rcMain.top;
               if ( moving ) pRc->bottom = rcMain.top + cy;
               attach_top = true;
            }
         }
      }

      /*
      if ( pRc->top < rcMain.bottom && pRc->bottom > rcMain.top )
      {
         bool local_attach_horz = false;
         if ( !attach_right &&  abs( pRc->right - rcMain.left ) < SNAP_PIXELS )
         {
            if ( !test_window || test_window->m_dock_hwnd_left != wnd )
            {
               pRc->right = rcMain.left;
               if ( moving ) pRc->left = rcMain.left - cx;
               // host_window->m_dock_hwnd_right = hwnd_test;
               attach_right = true;
               local_attach_horz = true;
            }
         }
         else if ( !attach_left && abs( pRc->left - rcMain.right ) < SNAP_PIXELS )
         {
            if ( !test_window || test_window->m_dock_hwnd_right != wnd )
            {
               pRc->left = rcMain.right;
               if ( moving ) pRc->right = rcMain.right + cx;
               // host_window->m_dock_hwnd_left = hwnd_test;
               attach_left = true;
               local_attach_horz = true;
            }
         }

         if ( local_attach_horz )
         {
            if ( abs( rcMain.bottom - pRc->bottom ) < SNAP_PIXELS )
            {
               pRc->bottom = rcMain.bottom;
               if ( moving ) pRc->top = rcMain.bottom - cy;
            }
            if ( abs( rcMain.top - pRc->top ) < SNAP_PIXELS )
            {
               pRc->top = rcMain.top;
               if ( moving ) pRc->bottom = rcMain.top + cy;
            }
            // host_window->m_dock_y = pRc->top - rcMain.top;
         }
      }

      if ( pRc->left < rcMain.right && pRc->right > rcMain.left )
      {
         bool local_attach_vert = false;
         if ( !attach_bottom && abs( pRc->bottom - rcMain.top ) < SNAP_PIXELS )
         {
            if ( !test_window || test_window->m_dock_hwnd_top != wnd )
            {
               pRc->bottom = rcMain.top;
               if ( moving ) pRc->top = rcMain.top - cy;
               //host_window->m_dock_hwnd_bottom = hwnd_test;
               attach_bottom = true;
               local_attach_vert = true;
            }
         }
         else if ( !attach_top && abs( pRc->top - rcMain.bottom ) < SNAP_PIXELS )
         {
            if ( !test_window || test_window->m_dock_hwnd_bottom != wnd )
            {
               pRc->top = rcMain.bottom;
               if ( moving ) pRc->bottom = rcMain.bottom + cy;
               //host_window->m_dock_hwnd_top = hwnd_test;
               attach_top = true;
               local_attach_vert = true;
            }
         }

         if ( local_attach_vert )
         {
            if ( abs( rcMain.right - pRc->right ) < SNAP_PIXELS )
            {
               pRc->right = rcMain.right;
               if ( moving ) pRc->left = rcMain.right - cx;
            }
            if ( abs( rcMain.left - pRc->left ) < SNAP_PIXELS )
            {
               pRc->left = rcMain.left;
               if ( moving ) pRc->right = rcMain.left + cx;
            }
            // host_window->m_dock_x = pRc->left - rcMain.left;
         }
      }
      */
   }
}

static ui_extension::window_host_factory<dockable_host_window> g_dockable_host_factory;


// looks for windows that are docked to this window and moves them accordingly...
void on_window_moved( HWND wnd )
{
   // running list of all affected windows... - dont move one twice, bitches... 
   static pfc::list_t<HWND> moved_windows;
   
   // stack of windows moved so we can tell when we are done
   static pfc::list_t<HWND> window_stack;

   dockable_host_window * moved_host_window = 
      (dockable_host_window*) GetWindowLongPtr( wnd, DWLP_USER );

   if ( IsIconic( wnd ) )
   {
      return;
   }

   // keep a stack of moved windows so we dont loop
   if ( window_stack.have_item( wnd ) )
   {
      return;

   }

   if ( moved_windows.have_item( wnd ) )
   {
      return;
   }
   
   window_stack.add_item( wnd );   
   moved_windows.add_item( wnd );

   /*
   if ( moved_host_window )
   {
      pfc::string8_fastalloc str;
      moved_host_window->get_title( str );
      console::printf( "on_window_moved: %s", str.get_ptr() );
   }
   */

   bool is_collapsing = ( g_collapsing_hwnd && g_collapsing_hwnd == wnd );

   for ( int n = 0; n < g_host_windows.get_count(); n++ )
   {
      HWND hwnd_moved = wnd;

      service_ptr_t<dockable_host_window> host_window = g_host_windows[n];

      // make sure its not the same window
      if ( wnd == host_window->m_hwnd )
      {
         continue;
      }
        
      // make sure its docked to this window
      if ( ( host_window->m_dock_hwnd_right != hwnd_moved ) &&
         ( host_window->m_dock_hwnd_left != hwnd_moved ) &&
         ( host_window->m_dock_hwnd_bottom != hwnd_moved ) &&
         ( host_window->m_dock_hwnd_top != hwnd_moved ) )
      {
         continue;
      }      

      bool is_iconic = IsIconic( host_window->m_hwnd ) ? true : false;

      RECT rc;
      RECT rcMain;

      if ( is_iconic )
      {
         WINDOWPLACEMENT place;
         memset( &place, 0, sizeof(place) );
         place.length = sizeof(WINDOWPLACEMENT);
         GetWindowPlacement( host_window->m_hwnd, &place );
         rc.left     = place.rcNormalPosition.left;
         rc.right    = place.rcNormalPosition.right;
         rc.top      = place.rcNormalPosition.top;
         rc.bottom   = place.rcNormalPosition.bottom;
      }
      else
      {
         GetWindowRect( host_window->m_hwnd, &rc );
      }
      GetWindowRect( wnd, &rcMain );

      int x    = rc.left;
      int y    = rc.top;
      int cx   = rc.right - rc.left;
      int cy   = rc.bottom - rc.top;

      int main_x  = rcMain.left;
      int main_y  = rcMain.top;
      int main_cx = rcMain.right - rcMain.left;
      int main_cy = rcMain.bottom - rcMain.top;

      if ( host_window->m_dock_hwnd_right || host_window->m_dock_hwnd_left || host_window->m_dock_hwnd_top || host_window->m_dock_hwnd_bottom )
      {
         if ( host_window->m_dock_hwnd_right == hwnd_moved )
         {
            if ( is_collapsing && !g_collapsing_horz )
            {
               host_window->update_docked_hwnd();
               continue;
            }

            x = main_x - cx;
            y = main_y + host_window->m_dock_y;
         }
         
         if ( host_window->m_dock_hwnd_left == hwnd_moved )
         {
            if ( is_collapsing && !g_collapsing_horz )
            {
               host_window->update_docked_hwnd();
               continue;
            }

            x = main_x + main_cx;
            y = main_y + host_window->m_dock_y;
         }
         
         if ( host_window->m_dock_hwnd_bottom == hwnd_moved )
         {
            if ( is_collapsing && g_collapsing_horz )
            {
               host_window->update_docked_hwnd();
               continue;
            }

            x = main_x + host_window->m_dock_x;
            y = main_y - cy;
         } 

         if ( host_window->m_dock_hwnd_top == hwnd_moved )
         {
            if ( is_collapsing && g_collapsing_horz )
            {
               host_window->update_docked_hwnd();
               continue;
            }

            x = main_x + host_window->m_dock_x;
            y = main_y + main_cy;
         }

         if ( IsIconic( host_window->m_hwnd ) )
         {
            WINDOWPLACEMENT place;
            memset( &place, 0, sizeof(place) );
            place.length = sizeof(WINDOWPLACEMENT);
            place.showCmd = SW_MINIMIZE;
            place.rcNormalPosition.top    = y;
            place.rcNormalPosition.left   = x;
            place.rcNormalPosition.right  = cx + x;
            place.rcNormalPosition.bottom = cy + y;
            SetWindowPlacement( host_window->m_hwnd, &place );
         }
         else
         {
            SetWindowPos( host_window->m_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER );
         }
      }
   }

   window_stack.remove_item( wnd );

   if ( window_stack.get_count() == 0 )
   {
      /*
      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         dockable_host_window * h = g_host_windows[n].get_ptr();

         h->m_dock_hwnd_bottom   = NULL;
         h->m_dock_hwnd_left     = NULL;
         h->m_dock_hwnd_top      = NULL;
         h->m_dock_hwnd_right    = NULL;

      }

      for ( int n = moved_windows.get_count() - 1; n >= 0; n-- )
      {
         HWND hwnd = moved_windows[n];

         dockable_host_window * h = (dockable_host_window *) GetWindowLongPtr( hwnd, DWLP_USER );

         h->update_docked_hwnd();
      }

      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         dockable_host_window * h = g_host_windows[n].get_ptr();

         if ( moved_windows.have_item( h->m_hwnd ) )
            continue;

         h->update_docked_hwnd();
      }
      */
      
      for ( int n = moved_windows.get_count() - 1; n >= 0; n-- )
      {
         HWND hwnd = moved_windows[n];

         // dockable_host_window * h = (dockable_host_window *) GetWindowLongPtr( hwnd, DWLP_USER );
        
         // bool found = false;

         for ( int z = 0; z < g_host_windows.get_count(); z++ )
         {
            if ( g_host_windows[z]->m_hwnd == hwnd )
            {
               g_host_windows[z]->update_docked_hwnd();
            }
            break;
         }

         /*
         if ( h )
         {   
            h->update_docked_hwnd();
         }
         */
      }

      /*
      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         dockable_host_window * h = g_host_windows[n].get_ptr();

         if ( moved_windows.have_item( h->m_hwnd ) )
            continue;

         h->update_docked_hwnd();
      }
      */
      /*
      if ( moved_host_window )
      {
         moved_host_window->update_docked_hwnd();
      }
      */
      /*
      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         if ( wnd = g_host_windows[n]->m_hwnd )
            continue;

         g_host_windows[n]->update_docked_hwnd();
      }
      */

      moved_windows.remove_all();
   }
}

static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   dockable_host_window * host_window = (dockable_host_window *) GetWindowLongPtr( wnd, DWLP_USER );

   if ( !host_window )
   {
      return FALSE;
   }
      
   switch ( msg )
   {
   case WM_SHOWWINDOW:
      switch ( lp )
      {
      case SW_PARENTCLOSING:
         if ( cfg_minimize_on_main_minimize == 0 )
         {
            return TRUE;
         }
         break;
      /*
      case SW_PARENTOPENING:
         break;
         */
      }

      break;

   case WM_TIMER:
      switch ( wp )
      {
      case 0:
         {
            RECT rc;
            POINT pt;
            GetWindowRect( wnd, &rc );
            GetCursorPos( &pt );

            if ( pt.x > rc.left && pt.x < rc.right && pt.y > rc.top && pt.y < rc.bottom )
            {
            }
            else 
            {
               KillTimer( wnd, wp );
               host_window->collapse();
            }
         }
         break;
      }
      break;

   case WM_MOUSEMOVE:
   case WM_NCMOUSEMOVE:
      host_window->on_mousemove( "host" );
      break;

   case WM_MOUSELEAVE:
   case WM_NCMOUSELEAVE:
      host_window->on_mouseleave( "host" );
      break;

      /*
   case WM_LBUTTONDOWN:
      console::printf( "child_hook:LBUTTONDOWN" );
      break;
      */

   case WM_NCHITTEST:
      // console::printf( "dockable_host_window::NCHITTEST" );
      if ( host_window->m_collapsed && host_window->m_horz_collapse )
      {
         UINT n = DefWindowProc( wnd, msg, wp, lp );
         if ( n == HTCLIENT )
         {
            n = HTCAPTION;
         }                     
         SetWindowLong( wnd, DWL_MSGRESULT, n );
         return TRUE;
      }
      else if ( host_window->m_hide_titlebar || host_window->m_no_frame )
      {
         UINT n = DefWindowProc( wnd, msg, wp, lp );
         if ( n == HTTOP )
         {
            n = HTCAPTION;
         }
         SetWindowLong( wnd, DWL_MSGRESULT, n );
         return TRUE;
      }

      break;

   case WM_PAINT:
      if ( host_window->m_collapsed && host_window->m_horz_collapse && !host_window->m_super_collapsed )
      {
         // ANSI_FIXED_FONT
         HDC hdc = GetDC( wnd );
         RECT rc;

         //HFONT old_font = (HFONT)SelectObject(hdc, g_vert_font ); 
         HFONT old_font = (HFONT)SelectObject(hdc, g_vert_font ); 

         GetClientRect( wnd, &rc );
         if ( GetActiveWindow() == wnd )
         {
            FillRect( hdc, &rc, g_activetitle_brush );
            SetBkColor( hdc, GetSysColor( COLOR_ACTIVECAPTION ) );
            SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
         }
         else
         {
            FillRect( hdc, &rc, g_inactivetitle_brush );
            SetBkColor( hdc, GetSysColor( COLOR_INACTIVECAPTION ) );
            SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
         }
         // DrawCaption( wnd, hdc, &rc, DC_ACTIVE | DC_GRADIENT );

         pfc::string8_fastalloc str;
         host_window->get_title( str );
         pfc::stringcvt::string_wide_from_utf8 wide_str( str );
         GetClientRect( wnd, &rc );
         //RECT cRC;
         SetRect( &rc, 0, 0, rc.bottom, rc.bottom );
         // console::printf( "rc = (%d, %d)-(%d,%d)", rc.left, rc.top, rc.right, rc.bottom );
         UINT uiFmt = DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP;
         /*
         SetTextColor( hdc, RGB(255,0,0) );
         DrawText( hdc, wide_str, -1, &rc, DT_LEFT | DT_BOTTOM | uiFmt );
         SetTextColor( hdc, RGB(0,255,0) );
         DrawText( hdc, wide_str, -1, &rc, DT_LEFT | DT_TOP | uiFmt );
         SetTextColor( hdc, RGB(0,0,255) );
         DrawText( hdc, wide_str, -1, &rc, DT_RIGHT | DT_BOTTOM |uiFmt );
         SetTextColor( hdc, RGB(255,255,0) );
         DrawText( hdc, wide_str, -1, &rc, DT_RIGHT | DT_TOP | uiFmt );
         SetTextColor( hdc, RGB(0,255,255) );
         DrawText( hdc, wide_str, -1, &rc, DT_CENTER | DT_VCENTER | uiFmt );
         */
   
         //SetTextColor( hdc, RGB(255,0,0) );
         DrawText( hdc, wide_str, -1, &rc, DT_LEFT | DT_BOTTOM |uiFmt );
         /*
         SetTextColor( hdc, RGB(0,255,0) );
         DrawText( hdc, wide_str, -1, &rc, DT_LEFT | DT_TOP | uiFmt );
         SetTextColor( hdc, RGB(0,0,255) );
         DrawText( hdc, wide_str, -1, &rc, DT_RIGHT | DT_BOTTOM | uiFmt );
         SetTextColor( hdc, RGB(255,255,0) );
         DrawText( hdc, wide_str, -1, &rc, DT_RIGHT | DT_TOP | uiFmt );       
         SetTextColor( hdc, RGB(0,255,255) );
         DrawText( hdc, wide_str, -1, &rc, DT_CENTER | DT_VCENTER | uiFmt );
         */
         SelectObject( hdc, old_font );
         return 0;
      }
      else if ( host_window->m_docked_hwnd )
      {
         UpdateWindow( host_window->m_docked_hwnd );
      }
      break;

   case WM_GETMINMAXINFO:
      {
         MINMAXINFO * mmi = (MINMAXINFO*) lp;
         mmi->ptMinTrackSize.x = 0;
         mmi->ptMinTrackSize.y = 0;
         return 0;
      }
      break;

   case WM_LBUTTONDBLCLK:
   case WM_NCLBUTTONDBLCLK:
      host_window->collapse_or_expand();
      break;

   case WM_DESTROY:
	   host_window->store_config_buffer();
	   break;

   case WM_CLOSE:
      if ( host_window->m_panel_window.is_valid() )
      {
         host_window->m_panel_window->destroy_window();
      }
      host_window->m_docked_hwnd = NULL;

      if ( host_window->m_hwnd )
      {
         DestroyWindow( host_window->m_hwnd );
         host_window->m_hwnd = NULL;
      }
      g_host_windows.remove_item( host_window );
      break;

   case WM_ACTIVATEAPP:
      // console::printf( "WM_ACTIVATEAPP: %s", wp ? "true" : "false" );
      g_is_active = wp ? true : false;
      host_window->update_style();
      break;

   case WM_ACTIVATE:
      switch ( LOWORD(wp) ) 
      {
      case WA_ACTIVE:
      case WA_CLICKACTIVE:
         g_last_active = host_window;
      }
      break;

   case WM_CONTEXTMENU:      
      if ( is_control_pressed() )
      {
         host_window->do_contextmenu( GET_X_LPARAM(lp), GET_Y_LPARAM( lp ) );
         return TRUE;
      }
      else if ( host_window->m_docked_hwnd == NULL )
      {
         HMENU panel_menu = CreatePopupMenu();

         service_enum_t<ui_extension::window> e; 
         service_ptr_t<ui_extension::window> ptr;

         pfc::list_t<GUID> guids;

         int panel_count = 0;
         while( e.next( ptr ) ) 
         {
            if ( ptr->is_available( host_window ) )
            {                          
               unsigned int type = ptr->get_type();

               /*
               if ( type & ui_extension::type_splitter )
                  continue;
               */

               pfc::string8 str_name;
               ptr->get_name( str_name );              
               panel_count++;
               uAppendMenu( panel_menu, MF_STRING, panel_count, str_name );
               guids.add_item( ptr->get_extension_guid() );

               /*
               console::printf( "%s = %s", 
                  (const char *) pfc::print_guid( ptr->get_extension_guid() ), 
                  str_name.get_ptr() );
                  */
            }
         }

         if ( panel_count > 0 )
         {
            int x = LOWORD(lp);               
            int y = HIWORD(lp);
            int cmd = TrackPopupMenu( panel_menu, TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, host_window->m_hwnd, NULL );

            if ( cmd )
            {
               host_window->create_child( guids[cmd-1] );
            }

            DestroyMenu( panel_menu );
         }

         return TRUE;
      }
      else
      {    
         pfc::refcounted_object_ptr_t<uie::menu_hook_impl> menu_hook = new uie::menu_hook_impl;

         host_window->m_panel_window->get_menu_items( *(menu_hook.get_ptr()) );

         HMENU hmenu = CreatePopupMenu();
         menu_hook->win32_build_menu( hmenu, 1, 0xff );

         int x = LOWORD(lp);               
         int y = HIWORD(lp);
         int cmd = TrackPopupMenu( hmenu, TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, host_window->m_hwnd, NULL );

         if ( cmd > 0 )
         {
            menu_hook->execute_by_id( cmd );
         }

         DestroyMenu( hmenu );
      }
      break;

   case WM_SIZE:
   case WM_MOVE:
      {
         // move the hosted window
         if ( host_window && host_window->m_docked_hwnd )
         {
            RECT rc;
            GetClientRect( host_window->m_hwnd, &rc );
            MoveWindow( host_window->m_docked_hwnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE );
         }

         // move attached windows
         on_window_moved( wnd );
      }
      break;

   case WM_SIZING:
   case WM_MOVING:
      {
         bool sizing = ( msg == WM_SIZING );
         bool moving = ( msg == WM_MOVING );

         host_window->snap_to( (LPRECT) lp, moving );
         host_window->update_docked_hwnd();
         return 0;
      }
      break;
   }
   return FALSE;
}

static WNDPROC original_main_proc;

static LRESULT CALLBACK main_window_hook( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   if ( g_host_windows.get_count() > 0 )
   {
      switch ( msg )
      {
      case WM_SIZE:
      case WM_MOVE:
         on_window_moved( wnd );
         break;

         /*
      case WM_ACTIVATEAPP:
         dockable_host_window::g_on_activate_app( wp );
         break;
         */
      }
   }

   return original_main_proc( wnd, msg, wp, lp );
}

service_ptr_t<dockable_host_window> dockable_host_window::create()
{
   service_ptr_t<dockable_host_window> host_window = new service_impl_t<dockable_host_window>;

   g_host_windows.add_item( host_window );

   HWND hwnd_main = core_api::get_main_window();

   host_window->m_hwnd = uCreateDialog( IDD_BLANK, hwnd_main, dialog_proc, NULL );
   SetWindowLongPtr( host_window->m_hwnd, DWLP_USER, (LONG_PTR) host_window.get_ptr() );
   host_window->update_title();

   ShowWindow( host_window->m_hwnd, SW_SHOWNORMAL );
   return host_window;
}


void load_windows( stream_reader * f, foobar2000_io::abort_callback & abort )
{
   try {
      unsigned fmt = 0;
      f->read( &fmt, sizeof(fmt), abort );
      
      if ( fmt > 4 )
      {
         console::warning( "Incorrect format in foo_dockable_panels configuration" );
         return;
      }

      unsigned count = 0;
      f->read( &count, sizeof(count), abort );

      HWND hwnd_main = core_api::get_main_window();
      for ( int n = 0; n < count; n++ )
      {
         service_ptr_t<dockable_host_window> host_window = new service_impl_t<dockable_host_window>;

         host_window->m_hwnd = uCreateDialog( IDD_BLANK, hwnd_main, dialog_proc, NULL );
         uSetWindowText( host_window->m_hwnd, "Dockable Panel" );
         SetWindowLongPtr( host_window->m_hwnd, DWLP_USER, (LONG_PTR) host_window.get_ptr() );

         /*
         SetWindowLong( host_window->m_hwnd, GWL_EXSTYLE, 
            GetWindowLong( host_window->m_hwnd, GWL_EXSTYLE ) & ~WS_EX_APPWINDOW );
            */

         bool success = host_window->load_config( fmt, f, abort );

         if ( !success )
         {
            break;
         }
         g_host_windows.add_item( host_window );
      }

   } catch ( exception_io e )
   {
      console::printf( "exception_io::%s", e.g_what() );
   }
}

// stores the open windows information 
//
// FMT         unsigned - reserved in case I need to change the format ( must be 0 )
// count       unsigned - number of windows open
// window data 
void store_windows(  stream_writer * f, foobar2000_io::abort_callback & abort )
{
   unsigned fmt = 4;
   f->write( &fmt, sizeof(fmt), abort );
      
   unsigned count = g_host_windows.get_count();
   f->write( &count, sizeof(count), abort );

   if ( g_host_windows.get_count() > 0 )
   {    
      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         service_ptr_t<dockable_host_window> host_window = g_host_windows[n];
         host_window->store_config( f, abort );
      }
   }
}

void cfg_dockable_panels::set_data_raw(foobar2000_io::stream_reader *p_stream, t_size p_sizehint, foobar2000_io::abort_callback & abort)
{
   if ( p_sizehint > 0 )
   {
      g_read_buf.set_size( p_sizehint );
      p_stream->read( g_read_buf.get_ptr(), p_sizehint, abort);
      g_buffer_reader = new stream_reader_memblock_ref( g_read_buf.get_ptr(), p_sizehint );
   }
}

void cfg_dockable_panels::get_data_raw(foobar2000_io::stream_writer *p_stream, foobar2000_io::abort_callback &p_abort)
{
   store_windows( p_stream, p_abort );
}



class dock_initquit : public initquit
{
   void on_init() 
   {      		
      g_dock_titleformat_hook = new dock_titleformat_hook();
      g_is_active = true;
      g_vert_font = CreateFont(-10,0,900,0,FW_BOLD,FALSE,FALSE,
                        0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                        NULL,_T("Comic Sans MS") ); 

      g_activetitle_brush = CreateSolidBrush( GetSysColor(COLOR_ACTIVECAPTION) );
      g_inactivetitle_brush = CreateSolidBrush( GetSysColor(COLOR_INACTIVECAPTION) );

      HWND hwnd_main = core_api::get_main_window();

      original_main_proc = uHookWindowProc( hwnd_main, main_window_hook );

      // return;

      if ( g_buffer_reader != NULL )
      {
         foobar2000_io::abort_callback_impl abort;
         load_windows( g_buffer_reader, abort );

         delete g_buffer_reader;
         g_buffer_reader = NULL;

         for ( int n = 0; n < g_host_windows.get_count(); n++ )
         {
            service_ptr_t<dockable_host_window> host_window = g_host_windows[n];

            struct { int i; HWND * h; } map[] =
            {
               { host_window->m_dock_index_left, &host_window->m_dock_hwnd_left},
               { host_window->m_dock_index_top, &host_window->m_dock_hwnd_top},
               { host_window->m_dock_index_right, &host_window->m_dock_hwnd_right},
               { host_window->m_dock_index_bottom, &host_window->m_dock_hwnd_bottom},
            };

            for ( int z = 0; z < tabsize(map); z++ )
            {
               switch ( map[z].i )
               {
               case 0:
                  *(map[z].h) = NULL; break;           
               case 1:
                  *(map[z].h) = hwnd_main; break;
               default:
                  *(map[z].h) = g_host_windows[map[z].i-2]->m_hwnd; break;
               }
            }

            host_window->update_title();
            host_window->update_style();
            host_window->update_docked_hwnd();
            host_window->refresh_child();
         }
      }
   }

   void on_quit() 
   {
      if ( g_host_windows.get_count() > 0 )
      {
         for ( int n = 0; n < g_host_windows.get_count(); n++ )
         {
            service_ptr_t<dockable_host_window> host_window = g_host_windows[n];
            if ( host_window.is_valid() && host_window->m_panel_window.is_valid() )
            {
                host_window->m_panel_window->destroy_window();
            }
            if ( host_window->m_hwnd )
            {
               DestroyWindow( host_window->m_hwnd );
               host_window->m_hwnd = NULL;
            }       
         }

         g_host_windows.remove_all();
      }

      if ( g_dock_titleformat_hook  )
      {
         delete g_dock_titleformat_hook;
         g_dock_titleformat_hook = NULL;
      }

      if ( g_vert_font )
      {
         DeleteObject( g_vert_font );
      }

      DeleteBrush( g_activetitle_brush );
      DeleteBrush( g_inactivetitle_brush );
   }
};
static service_factory_single_t<dock_initquit> g_dock_initquit;


/// MAIN MENU STUFF
static const GUID guid_mainmenu_group_000 = { 0x8b33cceb, 0xe21b, 0x43ee, { 0x90, 0xba, 0x6b, 0x95, 0x23, 0x37, 0xe8, 0xef } };
static const GUID guid_mainmenu_group_001 = { 0xfc2dde13, 0xe57f, 0x450c, { 0x90, 0x73, 0xcb, 0x61, 0xca, 0x19, 0xcf, 0x50 } };
static const GUID guid_mainmenu_group_002 = { 0x8bca84a0, 0x1969, 0x4560, { 0xa5, 0xee, 0xcd, 0xd0, 0xa5, 0xc6, 0xa9, 0x99 } };
static const GUID guid_mainmenu_group_003 = { 0x21a7e4db, 0x33aa, 0x448f, { 0x87, 0x46, 0x6d, 0x80, 0xfd, 0x47, 0x31, 0x21 } };

static const GUID guid_mainmenu_000 = { 0xff319f61, 0x52ca, 0x4c4c, { 0x9a, 0x6, 0xd, 0x3f, 0x76, 0x64, 0xc4, 0x00 } };
static const GUID guid_mainmenu_001 = { 0x8a0b97b4, 0xcac5, 0x4d87, { 0x81, 0x4a, 0x4f, 0xb8, 0x85, 0xc, 0x1d, 0x0 } };
static const GUID guid_mainmenu_002 = { 0x7b30aa03, 0x7327, 0x461c, { 0xa7, 0xea, 0xe, 0xdb, 0x53, 0x6c, 0x4e, 0x2c } };

static const GUID guid_collapse_base = { 0x22923686, 0xd99c, 0x437c, { 0x8f, 0x18, 0x3c, 0xd7, 0x50, 0x6a, 0xec, 0x00 } };

static service_factory_single_t<generic_group> g_dock_main_group( "Dockable Panels", guid_mainmenu_group_000, mainmenu_groups::view, mainmenu_commands::sort_priority_base );
static service_factory_single_t<generic_group> g_dock_active_group( "Active Panel", guid_mainmenu_group_001, guid_mainmenu_group_000, mainmenu_commands::sort_priority_base );
static service_factory_single_t<generic_group> g_dock_collapse_group( "Collapse", guid_mainmenu_group_002, guid_mainmenu_group_000, mainmenu_commands::sort_priority_base );
static service_factory_single_t<generic_group> g_dock_activate_group( "Activate", guid_mainmenu_group_003, guid_mainmenu_group_000, mainmenu_commands::sort_priority_base );

class dock_main : public mainmenu_commands
{
   t_uint get_command_count()
   {
      return 3;
   }
   virtual GUID get_command(t_uint32 p_index)
   {
      switch ( p_index )
      {
      case 0:
         return guid_mainmenu_000;
      case 1:
         return guid_mainmenu_001;
      case 2:
         return guid_mainmenu_002;
      }
      return pfc::guid_null;
   }
   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case 0:
         p_out.set_string( "New..." );
         break;
      case 1:
         p_out.set_string( "Hide Panels" );
         break;
      case 2:
         p_out.set_string( "Show Panels" );
         break;
      }
   }
   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case 0:
         p_out.set_string( "Create a new dockable panel window" );
         return true;
      case 1:
         p_out.set_string( "Hide all panels" );
         return true;
      case 2:
         p_out.set_string( "Show All Panels" );
         return true;
      }
      return false;
   }

   virtual GUID get_parent()
   {
      return guid_mainmenu_group_000;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      switch ( p_index )
      {
      case 0:
         dockable_host_window::create();
         break;
      case 1:
         dockable_host_window::g_show_panels( false );
         break;
      case 2:
         dockable_host_window::g_show_panels( true );
         break;
      }
   }
};

static mainmenu_commands_factory_t< dock_main > g_mainmenu;


static const GUID guid_active_000 = { 0x32e03cb8, 0x7929, 0x44e3, { 0x87, 0xac, 0x86, 0x54, 0x89, 0xcb, 0x65, 0x1e } };
static const GUID guid_active_001 = { 0x459a7060, 0x9ecc, 0x4cdf, { 0xbf, 0x6a, 0x8e, 0x8c, 0xd2, 0x2d, 0x4b, 0x8a } };
static const GUID guid_active_002 = { 0x968d1e39, 0x8317, 0x4bbb, { 0x85, 0x50, 0x51, 0xf3, 0xad, 0x2d, 0xba, 0xcf } };
static const GUID guid_active_003 = { 0xc9b86e49, 0xa2ec, 0x4f2e, { 0xa5, 0x17, 0xbf, 0x4f, 0x67, 0x29, 0x86, 0xcc } };
static const GUID guid_active_004 = { 0xd97648ae, 0xac94, 0x4201, { 0x8d, 0xf1, 0x5, 0x52, 0x8b, 0xb, 0x26, 0xb2 } };
static const GUID guid_active_005 = { 0x69edfceb, 0xb143, 0x49e5, { 0x8f, 0xa2, 0x4a, 0x6f, 0xc6, 0x54, 0x76, 0x69 } };
static const GUID guid_active_006 = { 0x52ad4b5a, 0x9a2a, 0x427d, { 0xae, 0xca, 0x90, 0x19, 0x9c, 0x1d, 0xd8, 0xce } };
static const GUID guid_active_007 = { 0x1a07e441, 0x39ac, 0x4371, { 0xbb, 0xd3, 0x4d, 0x21, 0xfb, 0x57, 0x48, 0x16 } };
static const GUID guid_active_008 = { 0xf0fac385, 0xa1f7, 0x42a5, { 0x82, 0x48, 0x81, 0xc3, 0x8b, 0x9d, 0xfd, 0xa6 } };

static const char * am_strings[] = { "Hide Titlebar", "Auto Collapse", "Hide When Collapsed", "Hide Close Box", "No Frame", "Collapse", "Move Window", "Custom Title...", "Expansion Tagz..." };
enum { AM_TITLEBAR, AM_AUTO_COLLAPSE, AM_SUPERCOLLAPSE, AM_NOCLOSE, AM_NOFRAME, AM_COLLAPSE, AM_MOVE, AM_CUSTOMTITLE, AM_EXPANSIONTAGZ, AM_LAST };

class active_main : public mainmenu_commands
{
public:
   t_uint get_command_count()
   {
      return AM_LAST;
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      switch ( p_index )
      {
      case AM_TITLEBAR:       return guid_active_000;
      case AM_COLLAPSE:       return guid_active_001;
      case AM_NOCLOSE:        return guid_active_002;
      case AM_AUTO_COLLAPSE:  return guid_active_003;
      case AM_NOFRAME:        return guid_active_004;
      case AM_MOVE:           return guid_active_005;
      case AM_CUSTOMTITLE:    return guid_active_006;
      case AM_SUPERCOLLAPSE:  return guid_active_007;
      case AM_EXPANSIONTAGZ:  return guid_active_008;
      }
      return pfc::guid_null;     
   }

	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags) 
   {
      if ( !g_last_active )
      {
         if ( g_host_windows.get_count() > 0 )
         {
            g_last_active = g_host_windows[0].get_ptr();
         }
         else
         {
            return false;
         }
      }
      
      switch ( p_index )
      {
      case AM_NOCLOSE:
         p_flags = g_last_active->m_hide_closebox ? flag_checked : 0; get_name(p_index,p_text); return true;

      case AM_EXPANSIONTAGZ:
         p_flags = g_last_active->m_expansion_tagz.is_empty() ? 0 : flag_checked; get_name(p_index,p_text); return true;

      case AM_SUPERCOLLAPSE:
         p_flags = g_last_active->m_super_collapsed ? flag_checked : 0; get_name(p_index,p_text); return true;

      case AM_NOFRAME:
         p_flags = g_last_active->m_no_frame ? flag_checked : 0; get_name(p_index,p_text); return true;

      case AM_AUTO_COLLAPSE:
         p_flags = g_last_active->m_auto_collapse ? flag_checked : 0; get_name(p_index,p_text); return true;

      case AM_TITLEBAR:
         p_flags = g_last_active->m_hide_titlebar ? flag_checked : 0; get_name(p_index,p_text); return true;

      case AM_COLLAPSE:
         p_flags = g_last_active->m_collapsed ? flag_checked : 0; get_name(p_index,p_text); return true;

      case AM_CUSTOMTITLE:
         p_flags = g_last_active->m_custom_title.is_empty() ? 0 : flag_checked; get_name(p_index,p_text); return true;

      default:
         p_flags = 0; get_name(p_index,p_text); return true;
      }
   }


   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      if ( p_index < AM_LAST )
      {
         p_out.set_string( am_strings[p_index] );
         return true;
      }
      return false;
   }

   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      if ( p_index < AM_LAST )
      {
         p_out.set_string( am_strings[p_index] );
      }
   }

   virtual GUID get_parent()
   {
      return guid_mainmenu_group_001;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      if ( !g_last_active )
         return;

      g_last_active->process_menu_command( (int) p_index );
   }
};

static mainmenu_commands_factory_t< active_main > g_activemenu;

void dockable_host_window::process_menu_command( int n )
{
   switch ( n )
   {
   case AM_EXPANSIONTAGZ:
      get_expansion_tagz();
      break;

   case AM_SUPERCOLLAPSE:
      m_super_collapsed = !m_super_collapsed;
      break;

   case AM_MOVE:
      PostMessage( m_hwnd, WM_SYSCOMMAND, SC_MOVE, 0 ); 
      break;

   case AM_NOFRAME:
      toggle_no_frame();
      break;

   case AM_AUTO_COLLAPSE:
      toggle_auto_collapse();
      break;

   case AM_TITLEBAR:
      toggle_titlebar();
      break;

   case AM_COLLAPSE:
      collapse_or_expand();
      break;

   case AM_NOCLOSE:
      toggle_closebox();
      break;

   case AM_CUSTOMTITLE:
      rename_custom();
      break;
   }
}

void dockable_host_window::do_contextmenu( int x, int y )
{
   g_last_active = this;
   HMENU m = CreatePopupMenu();

   int max = g_activemenu.get_static_instance().get_command_count();
   for ( int n = 0; n < max; n++ )
   {
      pfc::string8_fastalloc text;
      t_uint32 flags;
      g_activemenu.get_static_instance().get_display( n, text, flags);

      if ( flags & mainmenu_commands::flag_checked )
      {
         uAppendMenu( m, MF_CHECKED | MF_STRING, n + 1, text );
      }
      else
      {
         uAppendMenu( m, MF_STRING, n + 1, text );
      }

      if ( n == AM_NOFRAME )
      {
         uAppendMenu( m, MF_SEPARATOR, -1, "" );
      }
   }

   uAppendMenu( m, MF_SEPARATOR, max + 2, "" );
   uAppendMenu( m, MF_STRING, max+3, "New Panel..." );

   int cmd = TrackPopupMenu( m, TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, m_hwnd, NULL );

   if ( cmd )
   {
      if ( cmd == max+3 )
      {
         // new_dockable_pane();
         dockable_host_window::create();
      }
      else
      {
         process_menu_command( cmd - 1 );
      }
   }

   DestroyMenu( m );
}

class collapse_main : public mainmenu_commands
{
public:
   t_uint get_command_count()
   {
      return (t_uint)g_host_windows.get_count();
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      GUID guid = guid_collapse_base;
      guid.Data4[7] += p_index;
      return guid;
   }

	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags) 
   {
      get_name( p_index, p_text );

      if ( g_host_windows[p_index]->m_collapsed )
      {
         p_flags |= flag_checked;
      }
      return true;
   }


   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      pfc::string8_fastalloc str;
      g_host_windows[p_index]->get_title( str );
      p_out.set_string( "collapse " );
      p_out.add_string( str );
      return true;
   }

   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      g_host_windows[p_index]->get_title( p_out );
   }

   virtual GUID get_parent()
   {
      return guid_mainmenu_group_002;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      g_host_windows[p_index]->collapse_or_expand();
   }
};

static mainmenu_commands_factory_t< collapse_main > g_collapse_main;

class activate_main : public mainmenu_commands
{
public:
   t_uint get_command_count()
   {
      return (t_uint)g_host_windows.get_count();
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      GUID guid = guid_mainmenu_000;
      guid.Data4[7] += p_index + 1;
      return guid;
   }

   /*
	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags) 
   {
      get_name( p_index, p_text );

      if ( g_host_windows[p_index]->m_collapsed )
      {
         p_flags |= flag_checked;
      }
      return true;
   }
   */

   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      pfc::string8_fastalloc str;
      g_host_windows[p_index]->get_title( str );
      p_out.set_string( "activate " );
      p_out.add_string( str );
      return true;
   }

   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      pfc::string8_fastalloc title;
      g_host_windows[p_index]->get_title( title );

      p_out.set_string( pfc::string_printf( "%d - %s", p_index + 1, title.get_ptr() ) );
   }

   virtual GUID get_parent()
   {
      return guid_mainmenu_group_003;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      HWND hwnd = g_host_windows[p_index]->m_hwnd;
      service_ptr_t<dockable_host_window> host = g_host_windows[p_index];
      host->ensure_on_screen();
      if ( host->m_collapsed )
      {
         host->expand();
      }
      ShowWindow( hwnd, SW_SHOWNORMAL );
      BringWindowToTop( hwnd );
      SetActiveWindow( hwnd );
   }
};

static mainmenu_commands_factory_t< activate_main > g_activate_main;

#include "dock_preferences.h"

//// USER INTERFACE STUFF
#ifdef USER_INTERFACE
static const GUID guid_ui = { 0x652ccd31, 0xdbd4, 0x48b5, { 0xac, 0xe4, 0xd4, 0x7b, 0x7f, 0xa8, 0x1e, 0x5d } };

//! Entrypoint service for user interface modules. Implement when registering an UI module. Do not call existing implementations; only core enumerates / dispatches calls. To control UI behaviors from other components, use ui_control API. \n
//! Use user_interface_factory_t<> to register, e.g static user_interface_factory_t<myclass> g_myclass_factory;
class dockable_window_ui : public user_interface
{
public:
	//!HookProc usage: \n
	//! in your windowproc, call HookProc first, and if it returns true, return LRESULT value it passed to you
	// typedef BOOL (WINAPI * HookProc_t)(HWND wnd,UINT msg,WPARAM wp,LPARAM lp,LRESULT * ret);

   HookProc_t m_hook;

   service_ptr_t<dockable_host_window> g_main;

	//! Retrieves name (UTF-8 null-terminated string) of the UI module.
	virtual const char * get_name()
   {
      return "Dockable Panels";
   }

	//! Initializes the UI module - creates the main app window, etc. Failure should be signaled by appropriate exception (std::exception or a derivative).
	virtual HWND init(HookProc_t hook)
   {
      m_hook = hook;

      if ( g_buffer_reader != NULL )
      {
         foobar2000_io::abort_callback_impl abort;
         load_windows( g_buffer_reader, abort );

         delete g_buffer_reader;
         g_buffer_reader = NULL;

         for ( int n = 0; n < g_host_windows.get_count(); n++ )
         {
            service_ptr_t<dockable_host_window> host_window = g_host_windows[n];

            struct { int i; HWND * h; } map[] =
            {
               { host_window->m_dock_index_left, &host_window->m_dock_hwnd_left},
               { host_window->m_dock_index_top, &host_window->m_dock_hwnd_top},
               { host_window->m_dock_index_right, &host_window->m_dock_hwnd_right},
               { host_window->m_dock_index_bottom, &host_window->m_dock_hwnd_bottom},
            };

            for ( int z = 0; z < tabsize(map); z++ )
            {
               switch ( map[z].i )
               {
               case 0:
                  *(map[z].h) = NULL; break;           
               case 1:
                  *(map[z].h) = NULL; break;
               default:
                  *(map[z].h) = g_host_windows[map[z].i-2]->m_hwnd; break;
               }
            }

            host_window->update_title();
            host_window->update_docked_hwnd();
            host_window->update_style();
            host_window->refresh_child();
         }
      }

      if ( g_host_windows.get_count() == 0 )
      {
         g_main = new_dockable_pane();
      }
      else
      {
         g_main = g_host_windows[0];
      }
      return g_main->m_hwnd;
   }

	//! Deinitializes the UI module - destroys the main app window, etc.
	virtual void shutdown()
   {
      CloseWindow( g_main->m_hwnd );
      g_main.release();
   }

	//! Activates main app window.
	virtual void activate()
   {
      ShowWindow( g_main->m_hwnd, SW_SHOWNORMAL );
      SetActiveWindow( g_main->m_hwnd );
   }

	//! Minimizes/hides main app window.
	virtual void hide()
   {
      ShowWindow( g_main->m_hwnd, SW_MINIMIZE );
   }

	//! Returns whether main window is visible / not minimized. Used for activate/hide command.
   virtual bool is_visible()
   {
      return IsWindowVisible( g_main->m_hwnd );
   }
	
   //! Retrieves GUID of your implementation, to be stored in configuration file etc.
	virtual GUID get_guid() 
   {
      return guid_ui;
   }

	//! Overrides statusbar text with specified string. The parameter is a null terminated UTF-8 string. The override is valid until another override_statusbar_text() call or revert_statusbar_text() call.
	virtual void override_statusbar_text(const char * p_text)
   {
   }

	//! Disables statusbar text override.
	virtual void revert_statusbar_text()
   {
   }

	//! Shows now-playing item somehow (e.g. system notification area popup).
	virtual void show_now_playing() 
   {
   }

	// static bool g_find(service_ptr_t<user_interface> & p_out,const GUID & p_guid);

	// FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(user_interface);
};

user_interface_factory<dockable_window_ui> g_ui;
#endif


/// PLAYBACK HOOK
class dock_play_callback : public play_callback_static
{
   virtual unsigned get_flags()
   {
      return flag_on_playback_new_track;
   }

  	//! Playback process is being initialized. on_playback_new_track() should be called soon after this when first file is successfully opened for decoding.
   virtual void FB2KAPI on_playback_starting(play_control::t_track_command p_command,bool p_paused){}
	//! Playback advanced to new track.
	virtual void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track)
   {
      dockable_host_window::process_all_expansion_tagz( p_track );
   }

	//! Playback stopped
   virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason){ }
	//! User has seeked to specific time.
	virtual void FB2KAPI on_playback_seek(double p_time){}
   //! Called on pause/unpause.
	virtual void FB2KAPI on_playback_pause(bool p_state) {}
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
   virtual void FB2KAPI on_volume_change(float p_new_val) {}
};

static service_factory_single_t<dock_play_callback> g_dock_play_callback;

class dock_variable_callback : public titleformatting_variable_callback
{
public:
   virtual void on_var_change( const char * var )
   {
      for ( int n = 0; n < g_host_windows.get_count(); n++ )
      {
         service_ptr_t<dockable_host_window> host = g_host_windows[n];

         if ( !host->m_expansion_tagz.is_empty() && ( host->m_expansion_tagz.find_first( var ) != infinite ) )
         {
            static_api_ptr_t<playback_control> p;
            metadb_handle_ptr handle;
            if ( p->get_now_playing( handle ) )
            {
               host->process_expansion_tagz( handle );
            }
         }
      }
   }
};

service_factory_single_t<dock_variable_callback> g_dock_variable_callback;


bool return_processed_true( bool & p_found_flag )
{
   p_found_flag = true;
   return true;
}

bool return_processed_false( bool & p_found_flag )
{
   p_found_flag = false;
   return true;
}

bool dock_titleformat_hook::process_function(titleformat_text_out * p_out,const char * p_fn_name,t_size p_fn_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
   static const char * prefix = "dock_";
   static int prefix_len = strlen(prefix);

   if ( p_fn_name_length < prefix_len )
      return false;

   if ( stricmp_utf8_ex( p_fn_name, prefix_len, "dock_", prefix_len ) == 0 )
   {
      if ( stricmp_utf8_ex( p_fn_name, p_fn_name_length, "dock_iscollapsed_index", ~0 ) == 0 )
      {
         if ( p_params->get_param_count() == 1 )
         {
            const char * p_name;
            t_size p_name_length;

            p_params->get_param( 0, p_name, p_name_length );            

            unsigned index = pfc::atoui_ex( p_name, p_name_length );

            if ( index < g_host_windows.get_count() )
            {
               if ( g_host_windows[index]->m_collapsed )
               {
                  p_out->write( titleformat_inputtypes::unknown, "1", ~0 );
                  return return_processed_true( p_found_flag );
               }
            }
         }

         return return_processed_false( p_found_flag );
      }
      /*
      else if ( stricmp_utf8_ex( p_fn_name, p_fn_name_length, "dock_iscollapsed", ~0 ) == 0 )
      {
      }
      */
   }
   return false;
}
