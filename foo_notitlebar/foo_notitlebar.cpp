#define VERSION ("1.0.1 [" __DATE__ " - " __TIME__ "]")

#define URL_HELP        "http://wiki.bowron.us/index.php/Foobar2000"

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"

#include <windows.h>
#include <WCHAR.h>

DECLARE_COMPONENT_VERSION( "cwbowron's hacks", VERSION, 
   "Options to hide or unhide title Bar and save and restore window positions\n"
   "By Christopher Bowron <chris@bowron.us>\n"
   "http://wiki.bowron.us/index.php/Foobar2000" )

// config variables
static const GUID var_guid000 = { 0x5a37997, 0x4e67, 0x4474, { 0xbf, 0xd8, 0xd8, 0x43, 0x6, 0x6b, 0xf1, 0xcb } };
static const GUID var_guid001 = { 0xf241484b, 0xe73a, 0x47ca, { 0x9c, 0x41, 0x46, 0x41, 0x5c, 0x50, 0x86, 0x4d } };
static const GUID var_guid002 = { 0x80e71528, 0xae6f, 0x4127, { 0xae, 0x14, 0x33, 0x19, 0xfb, 0x83, 0x6, 0x22 } };
static const GUID var_guid003 = { 0x10117192, 0xd7ee, 0x4559, { 0xbc, 0xe7, 0x46, 0x1d, 0x51, 0x85, 0x9b, 0x7a } };
static const GUID var_guid004 = { 0xfc238d86, 0x8d49, 0x4fc4, { 0x81, 0xe3, 0xcf, 0xcd, 0xe5, 0xf0, 0x7f, 0x5 } };
static const GUID var_guid005 = { 0xf814e89a, 0xbc34, 0x4676, { 0x89, 0xd4, 0x9f, 0xe9, 0xed, 0xc4, 0x7f, 0x39 } };
static const GUID var_guid006 = { 0x1bde91c7, 0x555e, 0x4740, { 0xbe, 0x7b, 0x7, 0xa2, 0x1c, 0x8d, 0xa2, 0xe2 } };
static const GUID var_guid007 = { 0x822e474d, 0x2cbb, 0x4583, { 0x92, 0xe9, 0xb1, 0xf7, 0x23, 0x1b, 0x28, 0xef } };
static const GUID var_guid008 = { 0x2c359849, 0x1b1c, 0x4753, { 0x9e, 0xda, 0xd9, 0xbc, 0x2a, 0x1f, 0x33, 0x22 } };
static const GUID var_guid009 = { 0x4712ef4d, 0xcfd, 0x46a8, { 0x8c, 0xfc, 0xf7, 0xbb, 0xf1, 0xdb, 0xe7, 0x5 } };
static const GUID var_guid010 = { 0xbd501b91, 0xa33c, 0x4ed5, { 0x89, 0x88, 0x7, 0x46, 0x71, 0xa7, 0x37, 0x22 } };
static const GUID var_guid011 = { 0xf021836b, 0x3792, 0x4680, { 0x92, 0x1b, 0xc9, 0x38, 0x0, 0x6, 0x3b, 0x37 } };
static const GUID var_guid012 = { 0x43b68bde, 0x106e, 0x4903, { 0xa9, 0xc2, 0x48, 0x6c, 0x77, 0x79, 0x79, 0x41 } };
static const GUID var_guid013 = { 0x645d4bc5, 0x54cd, 0x401c, { 0xaa, 0x2f, 0x15, 0xc, 0x6d, 0xc5, 0x22, 0xb0 } };

static cfg_int cfg_hide( var_guid000, 0 );
static cfg_struct_t<RECT> cfg_slot_1( var_guid001, 0 );
static cfg_struct_t<RECT> cfg_slot_2( var_guid002, 0 );
static cfg_struct_t<RECT> cfg_slot_3( var_guid003, 0 );
static cfg_struct_t<RECT> cfg_slot_4( var_guid004, 0 );
static cfg_struct_t<RECT> cfg_slot_5( var_guid005, 0 );

static cfg_int cfg_allow_store( var_guid006, 1 );

static cfg_string cfg_name_1( var_guid007, "slot 1" );
static cfg_string cfg_name_2( var_guid008, "slot 2" );
static cfg_string cfg_name_3( var_guid009, "slot 3" );
static cfg_string cfg_name_4( var_guid010, "slot 4" );
static cfg_string cfg_name_5( var_guid011, "slot 5" );
// end config variables

const char * g_script1[] = 
{ "Big View", "restore slot 1", "show toolbars" };

const char * g_script2[] =
{ "Small View", "restore slot 2", "show toolbars" };

cfg_struct_t<RECT> * slots[] = 
{
   &cfg_slot_1,
   &cfg_slot_2,
   &cfg_slot_3,
   &cfg_slot_4,
   &cfg_slot_5
};

cfg_string * names[] =
{
   &cfg_name_1,
   &cfg_name_2,
   &cfg_name_3,
   &cfg_name_4,
   &cfg_name_5,
};

enum { CMD_NOTITLE, 
   CMD_SAVE_1,
   CMD_SAVE_2,
   CMD_SAVE_3,
   CMD_SAVE_4,
   CMD_SAVE_5,
   CMD_REST_1,
   CMD_REST_2,
   CMD_REST_3,
   CMD_REST_4,
   CMD_REST_5,
   CMD_SCRIPT1,
   CMD_SCRIPT2,
   CMD_LAST };

bool run_main_menu( const char * str )
{
   GUID guid;
   
   if ( mainmenu_commands::g_find_by_name( str, guid ) )
   {
      mainmenu_commands::g_execute( guid );
      return true;
   }
   else
   {
      return false;
   }
}

bool run_script( const char ** script, int script_count )
{
   for (int n = 0; n < script_count; n++ )
   {
      const char * str = script[n];
      if ( !run_main_menu( str ) )
      {
         console::printf( "command not found: %s", str );
         console::printf( "aborting script" );
         return false;
      }
   }
   return true;
}

bool has_title_bar()
{
   DWORD dw = GetWindowLong( core_api::get_main_window(), GWL_STYLE );

   if ( dw & WS_CAPTION )
   {
      return true;
   }
   return false;
}

void hide_title_bar()
{
   DWORD dw = GetWindowLong( core_api::get_main_window(), GWL_STYLE );
   dw &= ~(WS_CAPTION);
   SetWindowLong( core_api::get_main_window(), GWL_STYLE, dw );
   SetWindowPos( core_api::get_main_window(), 0, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE );
}

void show_title_bar()
{
   DWORD dw = GetWindowLong( core_api::get_main_window(), GWL_STYLE );
   dw |= (WS_CAPTION);
   SetWindowLong( core_api::get_main_window(), GWL_STYLE, dw );
   SetWindowPos( core_api::get_main_window(), 0, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE );
}

static WNDPROC original_main_proc;

static UINT CALLBACK main_window_hook( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   switch ( msg )
   {
   case WM_NCHITTEST:
      {
         if ( !has_title_bar() )
         {
            UINT n = original_main_proc( wnd, msg, wp, lp );
            if ( n == HTTOP )
            {
               n = HTCAPTION;
            }
            return n;
         }
         break;
      }
   }

   return original_main_proc( wnd, msg, wp, lp );
}



static const GUID mainmenu_guid_001 = { 0x6e17e79a, 0xa8e8, 0x48c3, { 0x90, 0x76, 0xad, 0x75, 0xbc, 0xb, 0xb2, 0xa4 } };
static const GUID mainmenu_guid_002 = { 0x36fdc8e1, 0x263a, 0x42cc, { 0x90, 0x2e, 0x6e, 0xb7, 0xbd, 0x34, 0x56, 0xd } };
static const GUID mainmenu_guid_003 = { 0xd32f8d4a, 0x2a08, 0x49f1, { 0x93, 0xfd, 0xbf, 0x48, 0xfb, 0x4b, 0x5e, 0xec } };
static const GUID mainmenu_guid_004 = { 0x9aaa722b, 0x2b98, 0x46c1, { 0xab, 0x89, 0x16, 0x5e, 0x53, 0xe4, 0xf9, 0x78 } };
static const GUID mainmenu_guid_005 = { 0x9c6b4d2e, 0xfd56, 0x43c9, { 0xa7, 0xb6, 0x66, 0x53, 0x41, 0x17, 0xd8, 0xff } };
static const GUID mainmenu_guid_006 = { 0xdc6b84fd, 0x5085, 0x4031, { 0xb2, 0xea, 0xf2, 0xff, 0x25, 0xe8, 0xb6, 0x11 } };
static const GUID mainmenu_guid_007 = { 0xfff760c7, 0xa8db, 0x46d7, { 0xb8, 0x51, 0xa3, 0x6c, 0xfd, 0xd6, 0xc6, 0xd } };
static const GUID mainmenu_guid_008 = { 0x269cce0f, 0x9cb5, 0x42a7, { 0xa9, 0x64, 0xe, 0x74, 0x9a, 0xa6, 0xb3, 0xc0 } };
static const GUID mainmenu_guid_009 = { 0x27947358, 0x8489, 0x4d96, { 0xb3, 0x32, 0xeb, 0xbd, 0xfa, 0x6d, 0x93, 0xb0 } };
static const GUID mainmenu_guid_010 = { 0xf3ff75e1, 0xac73, 0x4418, { 0x88, 0xd4, 0x86, 0x90, 0x7c, 0x2a, 0x8e, 0x13 } };
static const GUID mainmenu_guid_011 = { 0xa4bff22e, 0x9dbc, 0x44fe, { 0xb9, 0xa1, 0x3c, 0xdb, 0xad, 0x5d, 0x97, 0x88 } };
static const GUID mainmenu_guid_012 = { 0x72cb3d0a, 0xcbf6, 0x4d80, { 0x92, 0x14, 0x81, 0x86, 0xc5, 0x1, 0xdc, 0xb1 } };
static const GUID mainmenu_guid_013 = { 0x66eda3c8, 0xd45d, 0x4ef0, { 0x9f, 0x4, 0x34, 0x43, 0x6a, 0xc9, 0x72, 0xa2 } };

static const GUID * guids[] = 
{
   &mainmenu_guid_001,
   &mainmenu_guid_002,
   &mainmenu_guid_003,
   &mainmenu_guid_004,
   &mainmenu_guid_005,
   &mainmenu_guid_006,
   &mainmenu_guid_007,
   &mainmenu_guid_008,
   &mainmenu_guid_009,
   &mainmenu_guid_010,
   &mainmenu_guid_011,
   &mainmenu_guid_012,
   &mainmenu_guid_013,
};

bool is_slot_valid( int n )
{
   if ( slots[n]->get_value().bottom != 0 )
      return true;
   if ( slots[n]->get_value().left != 0 )
      return true;
   if ( slots[n]->get_value().top != 0 )
      return true;
   if ( slots[n]->get_value().right != 0 )
      return true;
   return false;
}

class generic_group : public mainmenu_group_popup
{
   public:
      GUID           m_guid;
      GUID           m_parent;
      const char *   m_name;
      unsigned       m_priority;

      generic_group( const char * name, GUID guid, GUID parent, unsigned priority = mainmenu_commands::sort_priority_dontcare )
      {
         m_guid      = guid;
         m_parent    = parent;
         m_name      = name;
         m_priority  = priority;
      }

   virtual GUID get_guid() { return m_guid; }
   virtual GUID get_parent() { return m_parent; }
   virtual t_uint32 get_sort_priority() { return m_priority; }   
   virtual void get_display_string(pfc::string_base & p_out) { p_out.set_string( m_name ); }
};

static const GUID guid_menu_group_000 = { 0xc7543d7a, 0xd1df, 0x4e24, { 0xbd, 0xa4, 0xc, 0x41, 0x1e, 0x7e, 0x7a, 0x6c } };
static const GUID guid_menu_group_001 = { 0x5723dfb2, 0xc15c, 0x47c1, { 0xb8, 0xdb, 0xa, 0xfe, 0x77, 0x2d, 0x74, 0xf1 } };
static const GUID guid_menu_group_002 = { 0x951902c9, 0x1b2d, 0x42f5, { 0x8e, 0x1f, 0xd, 0xee, 0x82, 0x86, 0x63, 0xe3 } };

static service_factory_single_t<generic_group> g_group_tree( "cwbowron hacks", guid_menu_group_000, mainmenu_groups::view );
// static service_factory_single_t<generic_group> g_group_save( "save position", guid_menu_group_001, guid_menu_group_000 );
// static service_factory_single_t<generic_group> g_group_rest( "restore position", guid_menu_group_002, guid_menu_group_000 );

class music_browser_main : public mainmenu_commands
{
   t_uint get_command_count()
   {
      // return CMD_LAST;
      return CMD_NOTITLE + 1;
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      switch ( p_index )
      {
      case CMD_SAVE_1:
      case CMD_SAVE_2:
      case CMD_SAVE_3:
      case CMD_SAVE_4:
      case CMD_SAVE_5:
      case CMD_REST_1:
      case CMD_REST_2:
      case CMD_REST_3:
      case CMD_REST_4:
      case CMD_REST_5:
      case CMD_NOTITLE:
      case CMD_SCRIPT1:
      case CMD_SCRIPT2:
         return *guids[p_index];
      }
      return pfc::guid_null;
   }
   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case CMD_NOTITLE:
         p_out.set_string( "Hide Title Bar" );
         break;

      case CMD_SAVE_1:
      case CMD_SAVE_2:
      case CMD_SAVE_3:
      case CMD_SAVE_4:
      case CMD_SAVE_5:
         {
            int n = p_index - CMD_SAVE_1;
            p_out.set_string( pfc::string_printf( "save %s", 
               names[n]->get_ptr() ) );
         }
         break;
      case CMD_REST_1:
      case CMD_REST_2:
      case CMD_REST_3:
      case CMD_REST_4:
      case CMD_REST_5:
         {
            int n = p_index - CMD_REST_1;
            p_out.set_string( pfc::string_printf( "restore %s", 
               names[n]->get_ptr() ) );
         }
         break;
      case CMD_SCRIPT1:
         p_out.set_string( "->Big View" );
         break;
      case CMD_SCRIPT2:
         p_out.set_string( "->Small View" );
         break;
      }
   }
   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case CMD_NOTITLE:
         p_out.set_string( "Hide the main window title bar" );
         return true;

      case CMD_SAVE_1:
      case CMD_SAVE_2:
      case CMD_SAVE_3:
      case CMD_SAVE_4:
      case CMD_SAVE_5:
         {
            p_out.set_string( "save current screen location to a slot" );
            return true;
         }
         break;
      case CMD_REST_1:
      case CMD_REST_2:
      case CMD_REST_3:
      case CMD_REST_4:
      case CMD_REST_5:
         {
            p_out.set_string( "restore screen location from a slot" );
            return true;
         }
         break;
      case CMD_SCRIPT1:
      case CMD_SCRIPT2:
         p_out.set_string( "Change the layout and size" );
         return true;
      }
      return false;
   }

   virtual GUID get_parent()
   {
      return guid_menu_group_000;
   }

	bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags) 
   {
      get_name(p_index,p_text);

      switch ( p_index )
      {
      case CMD_SAVE_1:
      case CMD_SAVE_2:
      case CMD_SAVE_3:
      case CMD_SAVE_4:
      case CMD_SAVE_5:
         {
            int n = p_index - CMD_SAVE_1;

            if ( cfg_allow_store == 0 )
            {
               return false;
            }
            if ( is_slot_valid( n ) )
            {
               p_flags = flag_checked;
            }
         }
         break;

      case CMD_NOTITLE:
         if ( !has_title_bar() )
         {
            p_flags = flag_checked;
         }
         else
         {
            p_flags = 0;
         }
         break;

      case CMD_REST_1:
      case CMD_REST_2:
      case CMD_REST_3:
      case CMD_REST_4:
      case CMD_REST_5:
         {
            int n = p_index - CMD_REST_1;

            if ( is_slot_valid( n ) )
            {               
            }
            else
            {
               return false;
            }
         }
         break;
      }
      return true;
   };

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      switch ( p_index )
      {
      case CMD_SAVE_1:
      case CMD_SAVE_2:
      case CMD_SAVE_3:
      case CMD_SAVE_4:
      case CMD_SAVE_5:
         {
            if ( cfg_allow_store )
            {
               int n = p_index - CMD_SAVE_1;
               console::printf( "Storing to slot %d", n + 1 );

               RECT rc;
               GetWindowRect( core_api::get_main_window(), &rc );
               /*
               console::printf( "bottom = %d, left = %d, top = %d, right = %d",
                  rc.bottom, rc.left, rc.top, rc.right );
               */

               *slots[n] = rc;

               /*
               console::printf( "bottom = %d, left = %d, top = %d, right = %d",
                  slots[n]->get_value().bottom, 
                  slots[n]->get_value().left, 
                  slots[n]->get_value().top, 
                  slots[n]->get_value().right );
                  */
            }
         }
         break;

      case CMD_REST_1:
      case CMD_REST_2:
      case CMD_REST_3:
      case CMD_REST_4:
      case CMD_REST_5:
         {
            int n = p_index - CMD_REST_1;

            if ( is_slot_valid( n ) )
            {
               int cx = slots[n]->get_value().right - slots[n]->get_value().left;
               int cy = slots[n]->get_value().bottom - slots[n]->get_value().top;

               console::printf( "Restoring slot %d", n + 1 );

               SetWindowPos( core_api::get_main_window(), NULL,
                  slots[n]->get_value().left, 
                  slots[n]->get_value().top, 
                  cx,
                  cy,
                  SWP_NOZORDER );
            }
            else
            {
               console::printf( "No valid data in slot %d", n + 1 );
            }
         }
         break;

      case CMD_NOTITLE:
         if ( !has_title_bar() )
         {
            show_title_bar();
            cfg_hide = 0;
         }
         else
         {
            hide_title_bar();
            cfg_hide = 1;
         }
         break;
      case CMD_SCRIPT1:
         run_script( g_script1, tabsize( g_script1 ) );
         break;

      case CMD_SCRIPT2:
         run_script( g_script2, tabsize( g_script2 ) );
         break;
      }
   }
};

static mainmenu_commands_factory_t< music_browser_main > g_mainmenu;

bool find_menu_path( GUID & parent, pfc::string8 & out )
{
   if ( parent == pfc::guid_null )
   {
      out.set_string( "main menu" );
      return true;
   }
   else
   {
      service_enum_t<mainmenu_group> e;
	   service_ptr_t<mainmenu_group> ptr;

      while ( e.next( ptr ) )
      {
         if ( ptr->get_guid() == parent )
         {
            pfc::string8 tmp;

            service_ptr_t<mainmenu_group_popup> popup;

            if ( ptr->service_query_t( popup ) )
            {
               popup->get_display_string( tmp );  
               find_menu_path( ptr->get_parent(), out );
               out.add_string( "/" );
               out.add_string( tmp );
               return true;
            }
            else
            {
               find_menu_path( ptr->get_parent(), out );
               return true;
            }
         }
      }
   }

   out.set_string( "main menu" );
   return false;
}

#if 0

static const GUID guid_mm_001 = 
{ 0x24517ced, 0xaaa8, 0x4dc1, { 0x94, 0xf2, 0x7d, 0xd5, 0x32, 0xef, 0x97, 0xf6 } };

pfc::list_t<pfc::string8> g_mm_names;
pfc::list_t<GUID> g_mm_guids;
pfc::list_t<pfc::string8> g_mm_paths;

class mm_context_items : public contextmenu_item_simple
{
//public:
   unsigned get_num_items()
   {
      //return 1;
      // see if we already cached all this jazz
      if ( g_mm_names.get_count() > 0 )
      {
         return g_mm_names.get_count();
      }

      unsigned total = 0;
	   service_enum_t<mainmenu_commands> e;
	   service_ptr_t<mainmenu_commands> ptr;

      g_mm_names.remove_all();
      g_mm_guids.remove_all();
      while(e.next(ptr)) 
      {
		   unsigned count = ptr->get_command_count();

         for ( unsigned n = 0; n < count; n++ )
         {
            pfc::string8 path;
            pfc::string8 name;
            GUID guid;

            t_uint32 p_flags;
            pfc::string8 str_display;

            ptr->get_display( n, str_display, p_flags );

            ptr->get_name( n, name );
            guid = ptr->get_command( n );
            find_menu_path( ptr->get_parent(), path );
            
            g_mm_names.add_item( name );
            g_mm_guids.add_item( guid );
            g_mm_paths.add_item( path );
         }

         total += count;
      }
      return total;
   }

   GUID get_item_guid( unsigned int n )
   {
      return g_mm_guids[n];
   }

   void get_item_name( unsigned n, pfc::string_base & out )
   {
      out.set_string( g_mm_names[n] );
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {
      out.set_string( g_mm_paths[n] );           
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {      
      get_item_name( n, out );
      return true;
   }

   virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      mainmenu_commands::g_execute( g_mm_guids[p_index] );
   }
};

static service_factory_single_t<mm_context_items> g_context;
#endif


class woot : public initquit
{
   void on_init() 
   {
      if ( cfg_hide )
      {
         hide_title_bar();
      }

      original_main_proc = uHookWindowProc( core_api::get_main_window(), (WNDPROC)main_window_hook );
   }

   void on_quit() 
   {
   }
};

static service_factory_single_t<woot> g_woot;
