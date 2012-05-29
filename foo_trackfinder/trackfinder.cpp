#define VERSION ("2.1.1 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4018) // don't warn about signed/unsigned
#pragma warning(disable:4065) // don't warn about default case 
#pragma warning(disable:4800) // don't warn about forcing int to bool
#pragma warning(disable:4244) // disable warning convert from t_size to int
#pragma warning(disable:4267) 
#pragma warning(disable:4995)

#define URL_HELP        "http://wiki.bowron.us/index.php/Foobar2000"
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

#include <string.h>

#include "resource.h"

#include "../common/string_functions.h"
#include "../common/menu_stuff.h"

#define DEFAULT_TRACK_FINDER     "$upper($left(%artist%,1))|%artist%|%album%|[$num(%tracknumber%,2) - ]%title%\r\n$upper($left(%album%,1))|%album%|[$num(%tracknumber%,2) - ]%title%"

DECLARE_COMPONENT_VERSION( "Track Finder", VERSION, 
"Find Tracks Quickly By Artist or Album Names\n"
"By Christopher Bowron <chris@bowron.us>\n"
"http://wiki.bowron.us/index.php/Foobar2000" )

static const GUID var_guid000 = { 0x554f5366, 0x8e00, 0x4254, { 0x8f, 0xb8, 0x4, 0x0, 0xd5, 0x31, 0xdc, 0x60 } };
static const GUID var_guid001 = { 0x92142adc, 0xec, 0x41c7, { 0xbf, 0x8a, 0x74, 0x6f, 0xc1, 0xf7, 0x2b, 0x6 } };
static const GUID var_guid002 = { 0x5b2e9dc8, 0x673, 0x46d9, { 0x86, 0x6a, 0x9b, 0xe3, 0x9e, 0x15, 0x42, 0xe7 } };
static const GUID var_guid003 = { 0x9a37c815, 0x73fa, 0x4e19, { 0xa8, 0xdb, 0x15, 0xfe, 0x8b, 0xdc, 0xff, 0x66 } };
static const GUID var_guid004 = { 0xbf37922d, 0x9d12, 0x4acb, { 0xa9, 0x5c, 0xee, 0x74, 0xa5, 0x19, 0xf0, 0xe0 } };
static const GUID var_guid005 = { 0x859b2811, 0x2ce2, 0x4373, { 0xab, 0x8d, 0x47, 0xbb, 0x3, 0xda, 0x1f, 0xce } };
static const GUID var_guid006 = { 0xd91601f6, 0xb5b5, 0x44fe, { 0xba, 0xc6, 0xb3, 0x4b, 0x9a, 0x94, 0x60, 0xb8 } };

static cfg_string cfg_tf_format( var_guid000, DEFAULT_TRACK_FINDER );
static cfg_string cfg_tf_action1( var_guid001, "Playlist Tree/Add to active playlist" );
static cfg_string cfg_tf_action2( var_guid002, "Playlist Tree/Send to active playlist" );
static cfg_string cfg_tf_action3( var_guid003, "Add to playback queue" );
static cfg_string cfg_tf_action4( var_guid004, "" );
static cfg_string cfg_tf_action5( var_guid005, "" );
static cfg_int cfg_trackfinder_max( var_guid006, 4000 );

static const GUID guid_tf_context = { 0x10c60205, 0x8404, 0x4c63, { 0x89, 0xbd, 0x76, 0x1b, 0xba, 0x97, 0x11, 0xb8 } };

void add_actions_to_hmenu( HMENU menu, int & baseID, pfc::ptr_list_t<pfc::string8> actions )
{
   for ( int n = 0; n < actions.get_count(); n++ )
   {      
      uAppendMenu( menu, MF_STRING, baseID, actions[n]->get_ptr() );
      baseID++;
   }
}

bool run_context_command( const char * cmd, pfc::list_base_const_t<metadb_handle_ptr> * handles )
{
   bool result = false;
   if ( strcmp( cmd, "" ) != 0 ) 
   {
      GUID guid;

      if ( menu_helpers::find_command_by_name( cmd, guid ) )
      {
         if ( handles )
         {
            result = menu_helpers::run_command_context( guid, pfc::guid_null, *handles );
         }
      }
   }
   return result;
}


class simple_node;
int simple_node_compare( const simple_node * a, const simple_node * b );

class simple_node 
{
public:
   metadb_handle_ptr             m_handle;
   pfc::ptr_list_t<simple_node>  m_children;
   pfc::string8                  m_label;
   int                           m_id;

   simple_node( const char * label )
   {
      m_label.set_string( label );
   }

   simple_node ( const char * label, metadb_handle_ptr p )
   {
      m_label.set_string( label );
      m_handle = p;
   }

   ~simple_node()
   {      
      m_children.delete_all();
   }

   simple_node * find_by_id( int id )
   {
      if ( m_id == id ) return this;

      for ( int n = 0; n < m_children.get_count(); n++ )
      {
         simple_node * sn = m_children[n]->find_by_id( id );
         if ( sn ) return sn;
      }
      return false;
   }

   virtual bool get_entries( pfc::list_base_t<metadb_handle_ptr> & list ) 
   { 
      if ( m_handle.is_valid() )
      {
         list.add_item( m_handle );
      }

      for ( int n = 0; n < m_children.get_count(); n++ ) 
      {
         m_children[n]->get_entries( list );
      }   
      return true;
   }

   virtual void sort( bool recurse )
   {
      m_children.sort_t( &simple_node_compare );

      if ( recurse )
      {
         for ( int n = 0; n < m_children.get_count(); n++ )
         {
            m_children[n]->sort( true );
         }
      }
   }

   virtual simple_node * add_child_conditional( const char * str )
   {
      for ( unsigned i = 0; i < m_children.get_count(); i++)
      {
         if ( stricmp_utf8_ex( m_children[i]->m_label, ~0, str, ~0 ) == 0 )
         {
            return m_children[i];
         }
      }

      simple_node * fn = new simple_node( str );         
      m_children.add_item( fn );
      return fn;
   }

   simple_node * add_delimited_child( const char *strings, metadb_handle_ptr handle )
   {
      const char *car = strings;
      const char *cdr = strings + strlen( strings ) + 1;

      if ( !*cdr )
      {         
         simple_node * sn = new simple_node( car, handle );
         m_children.add_item( sn );
         return sn;
      }
      else 
      {
         simple_node *n = add_child_conditional( car );

         if ( !n ) 
         {
            return NULL;
         }

         return n->add_delimited_child( cdr, handle );
      }
   }

   virtual void add_to_hmenu( HMENU menu, int & baseID, pfc::ptr_list_t<pfc::string8> options )
   {      
      if ( m_children.get_count() == 0 )
      {
         add_actions_to_hmenu( menu, baseID, options );
      }
     
      if ( m_children.get_count() > 1 )
      {
         uAppendMenu( menu, MF_POPUP, (UINT_PTR) CreatePopupMenu(), "All" );
         uAppendMenu( menu, MF_SEPARATOR, 0, 0 );
      }

      for ( int n = 0; n < m_children.get_count(); n++ )
      {
         HMENU child = CreatePopupMenu();
         m_children[n]->m_id = (int)child;
         uAppendMenu( menu, MF_POPUP, (UINT_PTR)child, m_children[n]->m_label );
      }
   }

};

int simple_node_compare( const simple_node * a, const simple_node * b )
{  
   return stricmp_utf8_ex( a->m_label, ~0, b->m_label, ~0 );
}

//folder_node * generate_trackfinder_tree( const pfc::list_base_const_t<metadb_handle_ptr> & list )
simple_node * generate_trackfinder_tree( const pfc::list_base_const_t<metadb_handle_ptr> & list )
{
	int i, j, max_j;

   pfc::ptr_list_t<pfc::string8> track_formats;

   string_split( cfg_tf_format, "\r\n", track_formats );

   //folder_node *qt_tree = new folder_node( "Track Tree" );
   simple_node *qt_tree = new simple_node( "Track Tree" );

   pfc::string8 handle_info;
	
	max_j = track_formats.get_count();

   pfc::hires_timer timer;
   timer.start();

	for ( j = 0; j < max_j; j++)
   {
      service_ptr_t<titleformat_object> script;

      if ( static_api_ptr_t<titleformat_compiler>()->compile( script, (const char *)track_formats[j]->get_ptr() ) )
      {
         for (i = 0; i < list.get_count(); i++)
		   {
			   list[i]->format_title( NULL, handle_info, script, NULL );
   			 			  
            query_ready_string delimited( handle_info );

			   //node * temp = qt_tree->add_delimited_child( delimited, list[i] );
            simple_node * temp = qt_tree->add_delimited_child( delimited, list[i] );
		   }
      }
      else
      {
         console::printf( "error in formatting: %s", (const char *)track_formats[j]->get_ptr() );
      }
	}

   /*
   static char w00t[100];
   sprintf( w00t, "generating tree: %f", timer.query() );
   console::printf( w00t );
   */

	qt_tree->sort( true );
	track_formats.delete_all();

	return qt_tree;
}

pfc::ptr_list_t<pfc::string8> * g_actions;
int g_runningID = 1;
pfc::string8 g_last_tf_command;
//node * g_last_tf_node;
simple_node * g_last_tf_node;

static BOOL CALLBACK tfProc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   //static node * menuNode = NULL;
   static simple_node * menuNode = NULL;

   switch ( msg )
   {
   case WM_INITDIALOG:
      //menuNode = (node *) lp;
      menuNode = (simple_node *) lp;
      ShowWindow( wnd, SW_HIDE );
      break;

   case WM_CONTEXTMENU:
      console::printf( "WM_CONTEXTMENU" );
      break;

   case WM_MENUSELECT:
         {
            //console::printf( "WM_SELECT" );

            int item = LOWORD(wp);
            int flags = HIWORD(wp);
            if ( flags & MF_POPUP )
            {
               //pfc::string8 label;
               //uGetMenuString( (HMENU)lp, item, label, MF_BYPOSITION );
               //console::printf( "MF_POPUP: %s, item = %d, flags = 0x%x", 
               //   (const char *) label, item, flags );
               
               //node * nd = menuNode->find_by_id( lp );
               simple_node * nd = menuNode->find_by_id( lp );

               if ( nd )
               {
                  /*
                  if ( nd->is_leaf() )
                  {
                  }
                  else
                  */
                  {
                     HMENU sub = GetSubMenu( (HMENU)lp, item );
                     int z = GetMenuItemCount( sub );
                     //int zz;

                     /*
                     //console::printf( "SubItemCount = %d", z );
                     for ( zz = 0; zz < z; zz++ )
                     {
                        DeleteMenu( sub, 0, MF_BYPOSITION );
                        //console::printf( "Deleting Submenu" );
                     }
                     */

                     //folder_node * fn = (folder_node*) nd;
                     simple_node * fn = (simple_node*) nd;

                     if ( fn->m_children.get_count() > 1 )
                     {
                        item -= 2;
                     }

                     if ( item >= 0 )
                     {
                        if ( z == 0 )
                        {
                           fn->m_children[item]->add_to_hmenu( sub, g_runningID, *g_actions );
                        }
                     }
                     else if ( item == -2 )
                     {
                        g_last_tf_node = fn;

                        if ( z == 0 )
                        {
                           add_actions_to_hmenu( sub, g_runningID, *g_actions );
                        }
                        //console::printf( "you selected all" );
                     }
                  }
                  //console::printf( "node: %s", (const char *)nd->m_label );
               }
               else
               {
                  //console::printf( "node not found: %d", lp );
               }
               // uAppendMenu( GetSubMenu( (HMENU)lp, item ), MF_STRING, 1000, "w00t" );
            }
            else
            {
               //node * nd = menuNode->find_by_id( lp );
               simple_node * nd = menuNode->find_by_id( lp );
               if ( nd )
               {
                  g_last_tf_node = nd;
                  //console::printf( "node = %s", (const char *)nd->m_label );
               }
               pfc::string8 label;
               uGetMenuString( (HMENU)lp, item, label, MF_BYCOMMAND );
               //console::printf( "not popup: %s", (const char *)label );

               g_last_tf_command.set_string( label );
            }                       
         }
         return 0;
   }
   return 0;
}

void track_finder( const pfc::list_base_const_t<metadb_handle_ptr> & p_data )
{
   if ( (cfg_trackfinder_max > 0) && ( p_data.get_count() > cfg_trackfinder_max ) )
   {
      int zoop = MessageBox( core_api::get_main_window(),
         _T("The selection contains more files that the current maximum defined in the preferences. ")
         _T("Running Track Finder on such large sets may result in significant delay and possible ")
         _T("crash due to windows resource issues.  It is recommended that you select a smaller ")
         _T("set of files for track finder.  Do you REALLY want to continue?"), 
         _T("Track Finder maximum exceeded"), MB_OKCANCEL );

      if ( zoop != IDOK )
      {
         return;
      }
   }

   //folder_node * n = generate_trackfinder_tree( p_data );   
   simple_node * n = generate_trackfinder_tree( p_data );   
     
   pfc::ptr_list_t<pfc::string8> actions;

   if ( !cfg_tf_action1.is_empty() )
   {
      actions.add_item( new pfc::string8( cfg_tf_action1 ) );
   }
   if ( !cfg_tf_action2.is_empty() )
   {
      actions.add_item( new pfc::string8( cfg_tf_action2 ) );
   }
   if ( !cfg_tf_action3.is_empty() )
   {
      actions.add_item( new pfc::string8( cfg_tf_action3 ) );
   } 
   if ( !cfg_tf_action4.is_empty() )
   {
      actions.add_item( new pfc::string8( cfg_tf_action4 ) );
   }
   if ( !cfg_tf_action5.is_empty() )
   {
      actions.add_item( new pfc::string8( cfg_tf_action5 ) );
   }

   POINT pt;
   GetCursorPos( &pt );

   g_actions = &actions;
   g_runningID = 1;

   HMENU m2 = CreatePopupMenu();
   int ID2 = 1;   
   n->m_id = (int)m2;
   n->add_to_hmenu( m2, ID2, actions );
   // console::printf( "ID2 = %d", ID2 );
   HWND trackFinderWnd = uCreateDialog( IDD_BLANK, core_api::get_main_window(), tfProc, (LPARAM)n );
   HWND parent2 = trackFinderWnd;
   
   int cmd2 = TrackPopupMenuEx( m2, TPM_RIGHTBUTTON | TPM_RETURNCMD, 
         pt.x, pt.y, parent2, 0 );

   DestroyWindow( trackFinderWnd );

   // console::printf( "Selection = %d", cmd2 );
   // console::printf( "action: %s", (const char *) g_last_tf_command );

   if ( cmd2 && g_last_tf_node )
   {
      // console::printf( "node: %s", (const char *) g_last_tf_node->m_label );
      metadb_handle_list handles;
      g_last_tf_node->get_entries( handles );

      //run_context_command( g_last_tf_command, NULL, g_last_tf_node );
      run_context_command( g_last_tf_command, &handles );
      

   }

   DestroyMenu( m2 );

   delete n;
   actions.delete_all();
}

class tf_context : public contextmenu_item_simple
{
   unsigned get_num_items()
   {
      return 1;
   }

   GUID get_item_guid( unsigned int n )
   {
      return guid_tf_context;
   }

   void get_item_name( unsigned n, pfc::string_base & out )
   {
      out.set_string( "Track Finder..." );
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {
      out.set_string( "" );
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {
      out.set_string( "Generate a quick file selector from a set of files" );
      return true;
   }

   virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      track_finder( p_data );
   }	
};

static service_factory_single_t<tf_context> g_tf_context;

static const GUID guid_tf_preferences = { 0xcac0d3b7, 0x9b5, 0x4266, { 0x93, 0xfc, 0x63, 0x1c, 0x9e, 0xc2, 0xb2, 0xc6 } };

text_box_map tf_text[] =
{
   { IDC_TF_FORMAT, &cfg_tf_format },
};

context_item_map tf_contexts[] =
{
   { IDC_TF_ACTION1, &cfg_tf_action1 },
   { IDC_TF_ACTION2, &cfg_tf_action2 },
   { IDC_TF_ACTION3, &cfg_tf_action3 },
   { IDC_TF_ACTION4, &cfg_tf_action4 },
   { IDC_TF_ACTION5, &cfg_tf_action5 },
};

int_map tf_ints[] =
{
   { IDC_MAX, &cfg_trackfinder_max }
};
   
class tf_preferences : public preferences_page
{  
   static BOOL CALLBACK tf_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         setup_text_boxes( wnd, tf_text, tabsize(tf_text) );
         setup_context_item_maps( wnd, tf_contexts, tabsize(tf_contexts) );
         setup_int_maps( wnd, tf_ints, tabsize(tf_ints) );
         break;

      case WM_COMMAND:
         process_text_boxes( wnd, wp, tf_text, tabsize(tf_text) );
         process_context_item_map( wnd, wp, tf_contexts, tabsize(tf_contexts) );
         process_int_maps( wnd, wp, tf_ints, tabsize(tf_ints) );
         break;
      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_TF_CONFIG, parent, tf_dialog_proc );
   }

   virtual const char * get_name()
   {
      return "Track Finder";
   }

   virtual GUID get_guid()
   {
      return guid_tf_preferences;
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

static service_factory_single_t<tf_preferences> g_tf_pref;

static const GUID mm_000 = { 0x1a1cd6f8, 0x87dc, 0x445e, { 0xa7, 0x90, 0x95, 0xe9, 0xf9, 0x1, 0xf1, 0xe4 } };

class trackfinder_main : public mainmenu_commands
{
   t_uint get_command_count()
   {
      return 1;
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      switch ( p_index )
      {
      case 0:
         return mm_000;
      }
      return pfc::guid_null;
   }
   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case 0:
         p_out.set_string( "Track Finder (Active Playlist)" );
         break;
      }
   }
   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case 0:
         p_out.set_string( "Runs the track finder on the active playlist" );
         return true;
      }
      return false;
   }

   virtual GUID get_parent()
   {
      return mainmenu_groups::view;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      switch ( p_index )
      {
      case 0:
         static_api_ptr_t<playlist_manager> m;
         metadb_handle_list handles;
         m->activeplaylist_get_all_items( handles );
         track_finder( handles );
         break;
      }
   }
};

static mainmenu_commands_factory_t< trackfinder_main > g_mainmenu;
