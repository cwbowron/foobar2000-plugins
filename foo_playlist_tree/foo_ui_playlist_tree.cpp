#define VERSION ("3.0.5 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4018) // don't warn about signed/unsigned
#pragma warning(disable:4065) // don't warn about default case 
#pragma warning(disable:4800) // don't warn about forcing int to bool
#pragma warning(disable:4244) // disable warning convert from t_size to int
#pragma warning(disable:4267) 
#pragma warning(disable:4995)
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)

#define URL_HELP        "http://wiki.bowron.us/index.php/Foobar2000"
#define URL_USER_MAP	   "http://www.frappr.com/playlisttreeusers"
#define URL_CHANGELOG	"http://foobar.bowron.us/changelog.txt"
#define URL_BINARY		"http://foobar.bowron.us/foo_playlist_tree.zip"
#define URL_FORUM		   "http://bowron.us/smf/index.php"
#define URL_WIKI        "http://wiki.bowron.us/index.php/Playlist_Tree"

#define DEFAULT_SORT_CRITERIA    "$if(%_isleaf%,%artist%-%title%,%_name%)"
#define DEFAULT_QUERY_FORMAT     "%artist%|%album%|[$num(%tracknumber%,2) - ]%title%"
#define DEFAULT_TRACK_FORMAT     "%artist% - %title%"
// #define DEFAULT_TRACK_FINDER     "$upper($left(%artist%,1))|%artist%|%album%|[$num(%tracknumber%,2) - ]%title%\r\n$upper($left(%album%,1))|%album%|[$num(%tracknumber%,2) - ]%title%"

#define DEFAULT_TEXT_COLOR       RGB(0,0,0)
#define DEFAULT_BACKGROUND_COLOR RGB(255,255,255)
#define DEFAULT_LIBRARY_PLAYLIST "*Playlist Tree*"

#define PLAYLIST_TREE_EXTENSION  "pts"

#define MENU_SEPARATOR           8000

#define NEWLINE                  "\r\n"
#define CPP

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"

#include <commctrl.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <windows.h>
#include <winuser.h>

#include "resource.h"
#include "../columns_ui-sdk/ui_extension.h"

// #include "escheme.h"
#include <sys/stat.h>

#include "scheme.h"

#define MY_EDGE_SUNKEN     WS_EX_CLIENTEDGE
#define MY_EDGE_GREY       WS_EX_STATICEDGE
#define MY_EDGE_NONE       0

enum { EE_NONE, EE_SUNKEN, EE_GREY };
char * g_edge_styles[] = { "None", "Sunken", "Grey", NULL };

#define CF_NODE CF_PRIVATEFIRST

#include "guids.h"

bool g_is_shutting_down = false;

/// BEGIN scheme declarations
Scheme_Env * g_scheme_environment = NULL;
Scheme_Object * g_scheme_handle_type = NULL;
Scheme_Object * g_scheme_node_type = NULL;

bool g_scheme_installed = false;

void     scm_eval_startup();
void     install_scheme_globals();
static   Scheme_Object *eval_string_or_get_exn_message(char *s);
/// END scheme declarations

inline static LOGFONT get_def_font()
{
   LOGFONT foo;
   uGetIconFont( &foo );
   return foo;
}

enum {
   repop_new_track,
   repop_queue_change,
   repop_playlist_change,
   repop_playlists_change
};

// int g_repop_playlist = 0;

bool g_refresh_lock = false;

//////////////////////// CONFIGURATION VARIABLES /////////////////////////////
static cfg_string cfg_file_format( var_guid004, DEFAULT_TRACK_FORMAT );
static cfg_string cfg_sort_criteria( var_guid005, DEFAULT_SORT_CRITERIA );
static cfg_string cfg_folder_display( var_guid006, "%_name%" );
static cfg_string cfg_query_display( var_guid007, DEFAULT_QUERY_FORMAT );
static cfg_string cfg_default_query_sort( var_guid010, "" );

static cfg_int cfg_back_color( var_guid011, RGB(255,255,255) );
static cfg_int cfg_line_color( var_guid012, RGB(0,0,0) );
static cfg_int cfg_text_color( var_guid013, RGB(0,0,0) );
static cfg_struct_t<LOGFONT> cfg_font( var_guid014, get_def_font() );

static cfg_string cfg_selection_action_1( var_guid015, "Browse" );
static cfg_string cfg_selection_action_2( var_guid016, "" );
static cfg_string cfg_doubleclick( var_guid017, "" );
static cfg_string cfg_library_playlist( var_guid018, DEFAULT_LIBRARY_PLAYLIST );
static cfg_int cfg_edge_style( var_guid019, EE_SUNKEN );

static cfg_int cfg_bitmaps_enabled( var_guid020, 1 );
static cfg_int cfg_bitmap_leaf( var_guid021, 0 );
static cfg_int cfg_bitmap_folder( var_guid022, 1 );
static cfg_int cfg_bitmap_query( var_guid023, 2 );
static cfg_string cfg_bitmaps_more( var_guid024, "" );

static cfg_int cfg_nohscroll( var_guid025, 0 );
static cfg_int cfg_hideroot( var_guid026, 0 );
static cfg_int cfg_hidelines( var_guid027, 0 );
static cfg_int cfg_hideleaves( var_guid028, 0 );
static cfg_int cfg_hidebuttons( var_guid029, 0 );

// static cfg_string cfg_tf_format( var_guid030, DEFAULT_TRACK_FINDER );
static cfg_string cfg_tf_action1( var_guid031, "Playlist Tree/Add to active playlist" );
static cfg_string cfg_tf_action2( var_guid032, "Playlist Tree/Send to active playlist" );
static cfg_string cfg_tf_action3( var_guid033, "Add to playback queue" );
static cfg_string cfg_tf_action4( var_guid034, "" );
static cfg_string cfg_tf_action5( var_guid035, "" );

static cfg_int cfg_repopulate_doubleclick( var_guid036, 0 );
static cfg_int cfg_bitmaps_loadsystem( var_guid037, 0 );

static cfg_string cfg_rightclick( var_guid038, "[local] foobar2000 Context Menu" );
static cfg_string cfg_rightclick_shift( var_guid039, "[local] Local Context Menu" );
static cfg_string cfg_middleclick( var_guid040, "[local] Local Context Menu" );

static cfg_int cfg_activate_library_playlist( var_guid041, 0 );
static cfg_int cfg_remove_last_playlist( var_guid042, 0 );
static cfg_string cfg_last_library( var_guid043, "" );

static cfg_string cfg_icon_path( var_guid044, "c:\\program files\\foobar2000\\icons" );

static cfg_int cfg_trackfinder_max( var_guid045, 4000 );

static cfg_string cfg_enter( var_guid046, "" );
static cfg_string cfg_space( var_guid047, "" );
static cfg_int cfg_process_keydown( var_guid048, 1 );

static cfg_int cfg_bother_user( var_guid049, 1 );
static cfg_int cfg_custom_colors( var_guid050, 0 );
static cfg_int cfg_color_selection_focused( var_guid051, RGB( 0, 0, 127 ) );
static cfg_int cfg_color_selection_nonfocused( var_guid052, RGB( 63, 63, 63 ) );

static cfg_int cfg_selection_action_limit( var_guid053, 5000 );

static cfg_int cfg_use_default_queries( var_guid054, 1 );
static cfg_int cfg_selection_text( var_guid055, CLR_DEFAULT );
static cfg_int cfg_selection_text_nofocus( var_guid056, CLR_DEFAULT );

static cfg_string cfg_export_prefix( var_guid057, "+" );

static cfg_string cfg_last_saved_playlist( var_guid058, "" );

static cfg_int cfg_panel_count( var_guid059, 0 );

static cfg_guid cfg_selection_action1_guid_command(      var_guid060, pfc::guid_null );
static cfg_guid cfg_selection_action1_guid_subcommand(   var_guid061, pfc::guid_null );
static cfg_guid cfg_selection_action2_guid_command(      var_guid062, pfc::guid_null );
static cfg_guid cfg_selection_action2_guid_subcommand(   var_guid063, pfc::guid_null );

static cfg_string cfg_scheme_startup( var_guid064, ";;; Insert any scheme code you want run on initialization" NEWLINE );

/////////////////////////////////////////////////////////////////////////////


metadb_handle_ptr g_silence_handle;

static WNDPROC TreeViewProc;

// FORWARD DECLARATIONS ////////////////////////////////////////////////////

// classes
class playlist_tree_panel;
class node;
class folder_node;
class leaf_node;
class query_node;

// functions
static bool useable_handle(metadb_handle_ptr handle);
int inline shift_pressed() { return ::GetAsyncKeyState( VK_SHIFT ) & 0x8000; }
node * load( pfc::string8 * out_name = NULL );
node * do_load_helper( const char * );
bool get_save_name( pfc::string_base & out_name, bool prompt_overwrite = false, bool open = false, bool as_text = false );
void lazy_set_root( HWND, node * );
void lazy_add_user_query( folder_node * n );
bool run_context_command( const char * cmd, pfc::list_base_const_t<metadb_handle_ptr> * handles, node * n = NULL,
                         const GUID & guid_command = pfc::guid_null, const GUID & guid_subcommand = pfc::guid_null );
static int select_icon( HWND parent, HIMAGELIST image_list );
void mm_command_process( int p_index, node * n = NULL );
//void track_finder( const pfc::list_base_const_t<metadb_handle_ptr> & p_data );
void on_node_removing( node * ptr );
/////////////////////////////////////////////////////////////////////////////

pfc::ptr_list_t<node> g_tmp_node_storage;

/*
class NOVTABLE albumart_control_base : public service_base
{
public:
	virtual bool is_controllable()=0;
	virtual bool set_image(metadb_handle_ptr track)=0;
	virtual bool set_image()=0;
	virtual void redraw()=0;
	virtual void lock(bool l)=0;
	virtual bool is_locked()=0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(albumart_control_base);
};
*/
/*

class NOVTABLE albumart_control_bas e  : public service_base
{
public:
	static GUID get_class_guid()
	{
		// {5549FA34-673C-4c5b-B6CE-5CDC4813E7B4}
		static const GUID guid = 
		{ 0x5549fa34, 0x673c, 0x4c5b, { 0xb6, 0xce, 0x5c, 0xdc, 0x48, 0x13, 0xe7, 0xb4 } };
		return(guid);
	}

	static albumart_control_base* get() 
	{
      service_enum_create_t<albumart_control_base>
		return(service_enum_create_t<albumart_control_base>);
	}
	
	virtual bool is_controllable()=0;
	virtual bool set_image(metadb_handle * track)=0;
	virtual bool set_image()=0;
	virtual void redraw()=0;
	virtual void lock(bool l)=0;
	virtual bool is_locked()=0;
};
*/
/*
class NOVTABLE albumart_control_base : public service_base
{
public:
	static GUID get_class_guid()
	{
		// {5549FA34-673C-4c5b-B6CE-5CDC4813E7B4}
		static const GUID guid = 
		{ 0x5549fa34, 0x673c, 0x4c5b, { 0xb6, 0xce, 0x5c, 0xdc, 0x48, 0x13, 0xe7, 0xb4 } };
		return(guid);
	}

   static albumart_control_base* get() 
	{
		return(service_enum_create_t(albumart_control_base,0));
	}
	
	virtual bool is_controllable()=0;
	virtual bool set_image(metadb_handle * track)=0;
	virtual bool set_image()=0;
	virtual void redraw()=0;
	virtual void lock(bool l)=0;
	virtual bool is_locked()=0;
};

class albumart_control : public albumart_control_base
{
	virtual bool is_controllable();
	virtual bool set_image(metadb_handle * track);
	virtual bool set_image();
	virtual void redraw();
	virtual void lock(bool l);
	virtual bool is_locked();
};

*/

// query source designators
#define TAG_DROP				"@drop"
#define TAG_NODE				"@node"
#define TAG_PLAYLIST			"@playlist"
#define TAG_PLAYLISTS      "@playlists"
#define TAG_DATABASE       "@database"
#define TAG_QUEUE          "@queue" 

// directives 
#define TAG_REFRESH			"@refresh "
#define TAG_NOSAVE			"@nosave "
#define TAG_FORCE				"@force "
#define TAG_NOBROWSE			"@nobrowse "
#define TAG_ICON				"@icon"
#define TAG_FORMAT			"@format"
#define TAG_GLOBALS			"@globalformat"
#define TAG_QUOTE				"@quote"
#define TAG_ANY				"@any"
#define TAG_MULTIPLE			"@multiple"
#define TAG_DEFAULT			"@default"
#define TAG_LIMIT				"@limit"
#define TAG_BROWSE_AS		"@browse_as"
#define TAG_HIDDEN			"@hidden "
#define TAG_HIDDEN2        "@hidden2 "
#define TAG_HIDDEN3        "@hidden3 "
#define TAG_IGNORE			"@ignore "
#define TAG_FAKELEVEL		"@fakelevel "
#define TAG_RGB            "@rgb" 

// half assed functions
#define TAG_PLAYING			"$playing"
#define TAG_FIRST				"@first"
#define TAG_SUM				"@sum"
#define TAG_AVG				"@avg"
#define TAG_PARENT_QUERY	"@pquery"
#define TAG_PARENT			"@parent"
#define TAG_QUERY          "@query"
#define TAG_NOSHOW			"@noshow"

// half assed variables
#define TAG_DATETIME			"%_datetime%"
#define TAG_BROWSER_INDEX	"%_browserindex%"
#define TAG_NESTLEVEL		"%_nestlevel%"
#define TAG_SYSTEMDATE		"%_systemdate%"

#define META_QUERY_START		"%<"
#define META_QUERY_STOP			">%"

/*
struct {
   const char * name;
   const char * location;
} g_favorites[] =
{
   {"Jude - 430 N. Harper Ave.", "http://cdbaby.com/allmp3/jude1.m3u" },
   {"Brian Vander Ark - Live at Schubas", "http://www.emusic.com/samples/m3u/album/10892082/0.m3u" },
   {"Nathan Caswell - Missing You", "http://nathancaswell.com/meta/02missing_you.m3u"},
   {"Mary Abraham - You Came Along", "http://maryabraham.com/YouCameAlone.mp3" },
   {"The Verve Pipe - Colorful (Live from the Lounge)","http://www.thevervepipe.com/audio/livefromlounge/tvp_livefromlounge_colorful.mp3"},
};
*/

node * g_context_node = NULL;
bool g_has_been_active = false;

class tree_titleformat_hook : public titleformat_hook
{
public: 
   node * m_node;

   tree_titleformat_hook() {}

   tree_titleformat_hook( node * n );
   void set_node( node * n );

   virtual bool process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) ;

   virtual bool process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag) ;
};

tree_titleformat_hook * g_format_hook = NULL;

class search_titleformatting_hook : public metadb_display_hook
{
public:
   node *   m_node;

   search_titleformatting_hook()
   {
      m_node = NULL;
   }

   search_titleformatting_hook( node * n )
   {
      m_node = n;
   }

   void set_node( node * n)
   {
      m_node = n;
   }

   virtual bool process_field(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) 
   {
      if ( m_node )
      {
         /*
         pfc::string8 s( p_name, p_name_length );
         console::printf( "search_titleformatting_hook::process_field: %s", s.get_ptr() );
         */         
         tree_titleformat_hook h( m_node );
         return h.process_field( p_out, p_name, p_name_length, p_found_flag );
      }
      return false;
   }

	virtual bool process_function(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
   {
      if ( m_node )
      {
         tree_titleformat_hook h( m_node );
         return h.process_function( p_out, p_name, p_name_length, p_params, p_found_flag );
      }
      return false;
   }
};
service_factory_single_t<search_titleformatting_hook> g_search_hook;

HIMAGELIST              g_imagelist = NULL;

void add_actions_to_hmenu( HMENU menu, int & baseID, pfc::ptr_list_t<pfc::string8> actions )
{
   for ( int n = 0; n < actions.get_count(); n++ )
   {      
      uAppendMenu( menu, MF_STRING, baseID, actions[n]->get_ptr() );
      baseID++;
   }
}

void update_nodes_if_necessary();

#include "../common/string_functions.h"
#include "tree_drop_source.h"
#include "node.h"
#include "query_node.h"
#include "tree_drop_target.h"
#include "sexp_reader.h"
#include "../common/titleformatting_variable_callback.h"

#include "playlist_tree_scheme.h"

DECLARE_COMPONENT_VERSION( "Playlist Tree Panel", VERSION, 
"Media Library Manager using Trees\n"
"By Christopher Bowron <chris@bowron.us>\n"
"http://wiki.bowron.us/index.php/Foobar2000"
"\n"
"\n"
"Playlist Tree uses MzScheme which has the following license:\n"
"Copyright (C) 1995-2006 Matthew Flatt\n"
"Permission is granted to copy, distribute and/or modify this document\n"
"under the terms of the GNU Library General Public License, Version 2\n"
"published by the Free Software Foundation. A copy of the license is\n"
"included in the appendix entitled ``License.''\n"
"\n"
"libscheme: Copyright (C)1994 Brent Benson. All rights reserved.\n"
"\n"
"Conservative garbage collector: Copyright (C)1988, 1989 Hans-J. Boehm,\n"
"Alan J. Demers. Copyright (C)1991-1996 by Xerox Corporation. Copyright\n"
"(C)1996-1999 by Silicon Graphics. Copyright (C)1999-2001 by Hewlett Packard\n"
"Company. All rights reserved.\n"
"\n"
"Collector C++ extension by Jesse Hull and John Ellis: Copyright (C)1994\n"
"by Xerox Corporation. All rights reserved.\n"
)

pfc::ptr_list_t<node> g_nodes_to_update;


void update_nodes_if_necessary()
{
   for ( int i = 0; i<g_nodes_to_update.get_count(); i++)
   {
      g_nodes_to_update[i]->repopulate();
      g_nodes_to_update[i]->refresh_tree_label();
      g_nodes_to_update[i]->refresh_subtree( NULL );
      
      if ( g_nodes_to_update[i]->m_hwnd_tree )
      {
         SetFocus( g_nodes_to_update[i]->m_hwnd_tree );
      }
   }
   g_nodes_to_update.remove_all();
}

// update parent queries if necessary
void update_drop_watchers( node * target, const metadb_handle_list & list )
{
   g_refresh_lock = true;
   node * ptr = target;

   while ( ptr )
   {
      if ( ptr->is_query() )
      {
         query_node * qn = (query_node *) ptr;

         if ( qn->m_refresh_on_new_track )
         {
            if ( strstr( qn->m_source, TAG_QUEUE ) )
            {
               static_api_ptr_t<playlist_manager> pm;
               for ( int i = 0; i < list.get_count(); i++ )
               {
                  pm->queue_add_item( list[i] );
               }

               g_nodes_to_update.add_item( qn );
            }
           
            if ( strstr( qn->m_source, TAG_PLAYLIST ) )
            {
               pfc::string8 playlist;
               if ( string_get_function_param( playlist, qn->m_source, TAG_PLAYLIST  ) )
               {              
                  static_api_ptr_t<playlist_manager> pm;
                  t_size np = pm->find_playlist( playlist, ~0 );
                  if ( np != infinite )
                  {
                     bit_array_false f;
                     pm->playlist_add_items( np, list, f );
                  }

                  g_nodes_to_update.add_item( qn );
               }
            }
         }
      }              
      ptr = ptr->get_parent();
   }
   g_refresh_lock = false;
}

/*
static componentversion_impl_simple_factory g_componentversion_service2(
   "Track Finder", TF_VERSION, 
   "Find Tracks Quickly By Artist or Album Names\n"
   "By Christopher Bowron <chris@bowron.us>\n"
   "http://wiki.bowron.us/index.php/Foobar2000" );
*/

enum {DROP_BEFORE = -1, DROP_HERE = 0, DROP_AFTER = 1};

class tree_drop_target;

bool ask_about_user_map( HWND parent )
{
   bool result = false;
   if ( cfg_bother_user )
   {
	   if ( MessageBox( parent, 
			   _T("If you enjoy using playlist tree, please consider adding ")
			   _T("yourself to the playlist tree user map.  Consider it a very cheap ")
			   _T("form of registering. I would appreciate it. ")
			   _T("This message will not pop up again.  Thank you - cwbowron ")
			   _T("\n\nHit yes to go to launch user map url, no to skip"),
			   _T("foo_playlist_tree"),
			   MB_YESNO ) == IDYES )
	   {
		   ShellExecute(NULL, _T("open"), _T(URL_USER_MAP), NULL, NULL, SW_SHOWNORMAL ); 
         result = true;
	   }

	   cfg_bother_user = 0;
   }
   return result;
}

static bool useable_handle(metadb_handle_ptr handle)
{
   /*
   if ( handle.is_valid() )
      return true;
   */
   if ( handle->is_info_loaded() )
      return true;

   static_api_ptr_t<metadb_io> db_io;
   db_io->load_info( handle, metadb_io::load_info_default, NULL, false );

   if ( handle->get_length() > 0.00 )
      return true;
   if ( strstr(handle->get_path(),"http" ) )
      return true;

   console::printf( "Unusable Handle: %s", handle->get_path() );
   return false;
}


node * get_selected_node( HWND tree )
{ 
   HTREEITEM item = TreeView_GetSelection( tree );
   TVITEMEX tv;
   memset( &tv, 0, sizeof(tv) );
   tv.hItem = item;
   tv.mask = TVIF_PARAM;

   TreeView_GetItem( tree, &tv );

   return (node*)tv.lParam;
}


node * node::add_child_query_conditional( 
   const char *s1, 
   const char *s2, 
   const char *s3, 
   const char *s4, 
   const char *s5 )
{
   folder_node * as_folder = (folder_node *) this;

   if ( !s1 ) return NULL;
   if ( !s2 ) return NULL;

   for ( int n = 0; n < as_folder->get_num_children(); n++ )
   {
      if ( as_folder->m_children[n]->is_query() )
      {
         query_node * q = (query_node *) as_folder->m_children[n];

         // this query already exists
         if ( ( strcmp( q->m_label, s1 ) == 0 ) &&
            ( strcmp( q->m_query_criteria, s2 ) == 0 ) )
         {
            return NULL;
         }
      }
   }
   return add_child( new query_node( s1, false, s2, s3, s4, s5 ) );
}


node * node::add_delimited_child( const char *strings, metadb_handle_ptr handle )
{
   const char *car = strings;
   const char *cdr = strings + strlen( strings ) + 1;

   if ( !*cdr )
   {
      if ( strstr( car, TAG_QUERY ) )
      {
         pfc::string8  s = car;
         pfc::string8 p;
         pfc::ptr_list_t<pfc::string8> params;

         string_get_function_param( p, s, TAG_QUERY );

         string_split( p, ";", params );

         int n = params.get_count();

         node *q = add_child_query_conditional( 
            ( n >= 1 ) ? params[0]->get_ptr() : NULL,
            ( n >= 2 ) ? params[1]->get_ptr() : NULL,
            ( n >= 3 ) ? params[2]->get_ptr() : NULL,
            ( n >= 4 ) ? params[3]->get_ptr() : NULL,
            ( n >= 5 ) ? params[4]->get_ptr() : NULL  );

         if ( q ) 
         {
            q->repopulate();
         }
         return q;
      }
      else
      {
         if ( can_add_child() )
         {
            return add_child( new leaf_node( car, handle ) );
         }
         else
         {
            return NULL;
         }
      }
   }
   else 
   {
      node *n = add_child_conditional( car );

      if ( !n ) 
      {
         return NULL;
      }

      return n->add_delimited_child( cdr, handle );
   }
}

node * node::process_dropped_file( const char * file, bool do_not_nest, int index, bool dynamic, metadb_handle_list * handles )
{
	node *ret_val = NULL;

   if ( dynamic )
   {
      query_node * q = new query_node( pfc::string_filename(file), 
         false, pfc::string_printf( "@drop<%s>", file ), 
         NULL,
         "@default", 
         NULL );

      if ( q )
      {
         add_child( q, index );
         q->repopulate();
         q->refresh_subtree();
      }
      return q;
   }

   pfc::string_extension s_ext( file );
  
   if ( PathIsDirectory( pfc::stringcvt::string_wide_from_utf8( file )))
   {
      pfc::string8 stufferoo = file;
	   stufferoo += "\\*.*";

	   uFindFile *find_file = uFindFirstFile( stufferoo );

	   if ( do_not_nest )
	   {
		   ret_val = this;
	   }
	   else
	   {
         ret_val = add_child(
            new folder_node( pfc::string_filename_ext( file ) ), index );
	   }

      pfc::string8 as_utf8;

      pfc::ptr_list_t<const char> file_list;

	   do
	   {
		   const char *file_name = find_file->GetFileName();

		   as_utf8 = file;
		   as_utf8 += "\\";
		   as_utf8 += file_name;

		   if ( !is_dotted_directory( file_name ) ) 
		   {		
			   ret_val->process_dropped_file( as_utf8 );
		   } 

	   } while ( find_file->FindNext() );
   }
   else if ( strcmp( s_ext, PLAYLIST_TREE_EXTENSION ) == 0) 
   {
      ret_val = add_child( do_load_helper( file )  );
   }
   else
   {
      abort_callback_impl abort;
      playlist_loader_callback_impl loader( abort );
      //playlist_loader::g_process_path( file, loader );
      playlist_loader::g_process_path_ex( file, loader );      

      //console::printf( "Processing: %s [%d items]", (const char *) file, loader.get_count() );

      if ( loader.get_count() == 1 )
      {
         if ( useable_handle( loader[0] ) )
         {
            ret_val = add_child( new leaf_node( NULL, loader.get_item( 0 ) ), index );

            if ( handles )
            {
               handles->add_item( loader[0] );
            }
         }
      }
      else
      {
         if ( do_not_nest )
         {
            for ( int i = 0; i < loader.get_count(); i++ )
            {
               if ( useable_handle( loader.get_item( i ) ) )
               {
                  ret_val = add_child( new leaf_node( NULL, loader.get_item( i ) ), index );

                  if ( handles )
                  {
                     handles->add_item( loader[i] );
                  }
               }
            }
         }
         else
         {
            ret_val = add_child( new folder_node( pfc::string_filename_ext(file), false ), index );

            for ( int i = 0; i < loader.get_count(); i++ )
            {
               if ( useable_handle( loader.get_item( i ) ) )
               {
                  //ret_val = add_child( new leaf_node( NULL, loader.get_item( i ) ), index );
                  ret_val->add_child( new leaf_node( NULL, loader.get_item( i ) ) );

                  if ( handles )
                  {
                     handles->add_item( loader[i] );
                  }
               }
            }
         }
      }
      return ret_val;
   }

   return ret_val;
}



static const GUID guid_playlist_tree_panel = { 0x9c0b7ddc, 0xeb75, 0x4bd0, { 0xad, 0xfb, 0x99, 0xa8, 0x9b, 0xc7, 0x77, 0x9f } };

bool is_valid_htree( HTREEITEM item )
{
   if ( item == 0 )
   {
      return false;
   }
   if ( item == TVI_ROOT )
   {
      return false;
   }
   return true;
}

pfc::ptr_list_t<playlist_tree_panel> g_active_trees;

playlist_tree_panel * g_last_active_tree = NULL;



typedef BOOL (WINAPI * SH_GIL_PROC)(HIMAGELIST *phLarge, HIMAGELIST* fophSmall);
typedef BOOL (WINAPI * FII_PROC)   (BOOL fFullInit);

void get_default_file_names( int n, pfc::string_base & out )
{
   pfc::string_printf foo( "%s\\playlist-tree-%d.pts", 
      core_api::get_profile_path(), 
      n ) ;

   filesystem::g_get_display_path( foo, out );
}



class node_titleformatting_hook : public metadb_display_hook
{
public:
   virtual bool process_field(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag);

	virtual bool process_function(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag);   
};

static service_factory_single_t<node_titleformatting_hook> g_node_hook;

class playlist_tree_panel : public ui_extension::window
{
public:
   HWND           m_wnd;
   HIMAGELIST     m_imagelist;
   pfc::string8   m_file;
   bool           m_has_been_asked;
   HFONT          m_font;

   bool           m_was_active;

   bool LoadSystemBitmaps()
   {
      HMODULE      hShell32;
      SH_GIL_PROC  Shell_GetImageLists;
      FII_PROC     FileIconInit;	
      HIMAGELIST   hLarge;
      HIMAGELIST   hSmall;

      // Load SHELL32.DLL, if it isn't already
      hShell32 = LoadLibrary(_T("shell32.dll"));

      if ( hShell32 == 0 )
      {
          return false;
      }

      //
      // Get Undocumented APIs from Shell32.dll: 
      //
      Shell_GetImageLists  = (SH_GIL_PROC) GetProcAddress( hShell32, (LPCSTR)71 );
      FileIconInit         = (FII_PROC)    GetProcAddress( hShell32, (LPCSTR)660 );

      if ( FileIconInit != 0 )
      {
         FileIconInit( TRUE );
      }

      if ( Shell_GetImageLists == 0 )
	   {
		   FreeLibrary( hShell32 );
		   return false;
	   }

      Shell_GetImageLists( &hLarge, &hSmall );

      if ( hSmall )
      {
         for ( int n = 0; n < ImageList_GetImageCount( hSmall ); n++ )
         {
            HICON icon = ImageList_GetIcon( hSmall, n, ILD_NORMAL );
            ImageList_AddIcon( m_imagelist, icon );
            DestroyIcon( icon );
         }

         FreeLibrary( hShell32 );
         return true;
      }

      FreeLibrary( hShell32 );
      return false;
   }

   void load_foobar2000_icons( HWND tree )
   {
      const char * fb_path = cfg_icon_path;
      pfc::string8 icon_path = fb_path;

      icon_path.add_string ( "\\*.ico" );

      uFindFile *find_file = uFindFirstFile( icon_path );

      if ( find_file )
      {
	      do
	      {
            pfc::string8_fastalloc fullpath;
		      const char *file_name = find_file->GetFileName();

            fullpath.set_string( fb_path );
            fullpath.add_string( "\\" );
            fullpath.add_string( file_name );

            HICON hi = (HICON)uLoadImage( 
               NULL, 
               fullpath,
               IMAGE_ICON, 
               16, 
               16, 
               LR_LOADFROMFILE );

            if ( hi )
            {
               ImageList_AddIcon( m_imagelist, hi );
               DestroyIcon( hi );
            }
	      } while ( find_file->FindNext() );      
      }
   }
      
   void LoadBitmaps( HWND tree_hwnd )
   {
      HBITMAP bitmap = (HBITMAP)LoadImage( core_api::get_my_instance(), MAKEINTRESOURCE(IDB_BITMAPS),
         IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT );

      /*
      if ( cfg_bitmaps_loadsystem )
      {
         LoadSystemBitmaps();
      }
      */

      if ( m_imagelist == NULL )
      {
         m_imagelist = ImageList_Create(16,16, ILC_MASK|ILC_COLOR8, 10, 10);
      }

      ImageList_AddMasked( m_imagelist, bitmap, CLR_DEFAULT );
      DeleteObject( bitmap );

      if ( !cfg_icon_path.is_empty() )
      {
         load_foobar2000_icons( tree_hwnd );
      }

      if ( !cfg_bitmaps_more.is_empty() )
      {
         pfc::ptr_list_t<pfc::string8> bitmap_files;
         string_split( cfg_bitmaps_more, NEWLINE, bitmap_files );

         for ( int n = 0; n < bitmap_files.get_count(); n++ )
         {            
            pfc::stringcvt::string_wide_from_utf8 file_path( bitmap_files[n]->get_ptr() );
            HBITMAP bitmap = (HBITMAP)LoadImage( core_api::get_my_instance(), 
               (const wchar_t *) file_path,
               IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT );

            if ( bitmap )
            {
               ImageList_AddMasked( m_imagelist, bitmap, CLR_DEFAULT );
               DeleteObject(bitmap);
            }           
         }
      }

      LoadSystemBitmaps();

      HIMAGELIST rc = TreeView_SetImageList( tree_hwnd, m_imagelist, TVSIL_NORMAL );
   }

   static LRESULT CALLBACK TreeViewHook( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {	
      switch(msg)
      {
      case WM_SETFOCUS:
         if ( wnd )
         {
            g_last_active_tree = g_find_by_tree( wnd );
            post_variable_callback_notification( "treenode" );
         }
         break;
         
      case WM_KILLFOCUS:
         post_variable_callback_notification( "treenode" );
         break;

      case WM_KEYDOWN:
         {
            LPNMTVKEYDOWN pnkd = (LPNMTVKEYDOWN) lp; 

            HTREEITEM item = TreeView_GetSelection( wnd );

            TVITEMEX tv;
            memset( &tv, 0, sizeof(tv) );
            tv.hItem = item;  
            tv.mask = TVIF_PARAM;

            TreeView_GetItem( wnd, &tv );

            if ( tv.lParam )
            {
               metadb_handle_list selected_handles;
               node * n = (node *) tv.lParam;

               n->get_entries( selected_handles );

               if ( wp == VK_RETURN && !cfg_enter.is_empty() )
               {
                  //console::printf( "Return" );
                  run_context_command( cfg_enter, &selected_handles, n );
                  return 0;
               }
               else if ( wp == VK_SPACE && !cfg_space.is_empty() )
               {
                  //console::printf( "Space" );
                  run_context_command( cfg_space, &selected_handles, n );
                  return 0;
               }
               else if ( cfg_process_keydown )
               {
                  static_api_ptr_t<keyboard_shortcut_manager> man;
                  man->on_keydown_auto_context( selected_handles, wp, guid_playlist_tree_panel );
               }
            }
         }
         break;

      case WM_MBUTTONDOWN:
         {
            playlist_tree_panel * p = g_find_by_tree( wnd );

            if ( p )
            {
               POINT pt;
               pt.x = LOWORD(lp);
               pt.y = HIWORD(lp);
               ClientToScreen( wnd, &pt );

               node * n = p->find_node_by_pt( pt.x, pt.y, false );
               if ( n )
               {
                  if ( n->m_tree_item ) TreeView_SelectDropTarget( wnd, n->m_tree_item );                 
                  run_context_command( cfg_middleclick, NULL, n );
                  TreeView_SelectDropTarget( wnd, NULL );
               }
            }
         }         
         return 0;

      case WM_LBUTTONDBLCLK:
         {          
            node * n = g_find_node_by_client_pt( wnd, LOWORD(lp), HIWORD(lp) );

            /*
            service_ptr_t<albumart_control_base> aa = service_enum_create_t<albumart_control_base,0>;
            if ( aa )
            {
               if ( n )
               {
                  metadb_handle_ptr h;
                  n->get_first_entry( h );
               }
            }
            else
            {
               console::printf( "Albumart Service Not Found" );
            }
            */

            if ( n )
            {
               metadb_handle_list selected_handles;
               
               if ( cfg_repopulate_doubleclick )
               {
                  if ( n->is_query() )
                  {
                     ((query_node*)n)->repopulate();
                     n->refresh_subtree( n->m_hwnd_tree );
                  }
               }

               n->get_entries( selected_handles );                        
               run_context_command( cfg_doubleclick, NULL, n );
            }
         }

         return true;
      }	
      
      return uCallWindowProc( TreeViewProc, wnd, msg, wp, lp );
   }


   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
         /*
      case WM_MENUSELECT:
         {
            console::printf( "WM_SELECT" );

            int item = LOWORD(wp);
            int flags = HIWORD(wp);
            if ( flags & MF_POPUP )
            {
               pfc::string8 label;
               uGetMenuString( (HMENU)lp, item, label, MF_BYPOSITION );
               console::printf( "MF_POPUP: %s", (const char *) label );
               // uAppendMenu( GetSubMenu( (HMENU)lp, item ), MF_STRING, 1000, "w00t" );
            }
            else
            {
               pfc::string8 label;
               uGetMenuString( (HMENU)lp, item, label, MF_BYCOMMAND );
               console::printf( "not popup: %s", (const char *)label );
            }                       
         }
         break;
         */

      case WM_INITDIALOG:
         {           
            playlist_tree_panel *panel = (playlist_tree_panel *)lp;
            HWND tree = GetDlgItem( wnd, IDC_TREE);

            // console::printf( "TREE: %d", (int) tree );

            if ( cfg_bitmaps_enabled )
            {
               panel->LoadBitmaps( tree );
            }

            if ( g_last_active_tree == NULL )
            {
               g_last_active_tree = panel;
            }

		      TreeViewProc = uHookWindowProc( tree, TreeViewHook );
         }
         break;

      case WM_SIZE:
         {            
            RECT r;
            HWND tree = GetDlgItem( wnd, IDC_TREE );
            // console::printf( "TREE: %d", (int) tree );

            GetWindowRect( wnd, &r );	          

            SetWindowPos( tree, 0, 0, 0, r.right-r.left, r.bottom-r.top, SWP_NOZORDER );
         }
         break;

      case WM_NOTIFY:
         {
            LPNMHDR hdr = (LPNMHDR)lp;

            if ( hdr->idFrom == IDC_TREE )
            {
               switch ( hdr->code )
               {
               case NM_CUSTOMDRAW:
                  {
                     if ( cfg_custom_colors )
                     {
                        LPNMTVCUSTOMDRAW lptvcd = (NMTVCUSTOMDRAW *)lp; 
                        switch ( lptvcd->nmcd.dwDrawStage )
                        {
                        case CDDS_PREPAINT:
                           SetWindowLong( wnd, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW );
                           return TRUE;                           
                  
                        case CDDS_ITEMPREPAINT: 
                           {
                              if ( lptvcd->nmcd.lItemlParam )
                              {
                                 node * n = (node *) lptvcd->nmcd.lItemlParam;

                                 lptvcd->clrText = cfg_text_color;
         
                                 if ( TVIS_DROPHILITED & TreeView_GetItemState( hdr->hwndFrom, lptvcd->nmcd.dwItemSpec, TVIS_DROPHILITED ) )
                                 {
                                    lptvcd->clrTextBk = cfg_color_selection_focused;

                                    if ( cfg_selection_text != CLR_DEFAULT )
                                    {
                                       lptvcd->clrText = cfg_selection_text;
                                    }
                                 }
                                 else if ( lptvcd->nmcd.uItemState & CDIS_FOCUS )
                                 {
                                    lptvcd->clrTextBk = cfg_color_selection_focused;

                                    if ( cfg_selection_text != CLR_DEFAULT )
                                    {
                                       lptvcd->clrText = cfg_selection_text;
                                    }
                                 }
                                 else if ( lptvcd->nmcd.uItemState & CDIS_SELECTED )
                                 {
                                    lptvcd->clrTextBk = cfg_color_selection_nonfocused;

                                    if ( cfg_selection_text_nofocus != CLR_DEFAULT )
                                    {
                                       lptvcd->clrText = cfg_selection_text_nofocus;
                                    }
                                 }
                                 else
                                 {
                                    lptvcd->clrTextBk = cfg_back_color;
                                 }

                                 if ( n->m_custom_color != CLR_DEFAULT )
                                 {
                                    lptvcd->clrText = n->m_custom_color;
                                 }


                                 SetWindowLong( wnd, DWL_MSGRESULT, CDRF_DODEFAULT );
                                 return TRUE;                                                      
                              }
                           }
                           break;
                        }
                     }
                     return FALSE;
                  }
                  break;

               case TVN_ITEMEXPANDED:
                  SendMessage( hdr->hwndFrom, WM_SETREDRAW, TRUE, 0 );
                  break;

               case TVN_ITEMEXPANDING:
                  {
                     SendMessage( hdr->hwndFrom, WM_SETREDRAW, FALSE, 0 );

                     LPNMTREEVIEW param = (LPNMTREEVIEW)hdr;

                     folder_node *foop = (folder_node*)param->itemNew.lParam;

                     if ( foop )
                     {
                        if ( param->action & TVE_EXPAND )
                        {
                           foop->m_expanded = true;

                           if ( foop->get_num_children() > 0 )
                           {
                              foop->m_children[0]->ensure_in_tree();
                           }
                        }
                        else if ( param->action & TVE_COLLAPSE )
                        {
                           foop->m_expanded = false;
                        }
                     }
                     return 0;
                  }
                  break;

               case TVN_SELCHANGED:
                  {					
                     LPNMTREEVIEW param = (LPNMTREEVIEW)hdr;

                     if ( param->action != TVC_UNKNOWN )
                     {
                        node *n = (node *) param->itemNew.lParam;
                        if ( n )
                        {
                           n->select();
                        }
                     }
                  }                  
                  break;

               case TVN_BEGINDRAG:
                  {              
                     static long dragging = -1;

                     if ( InterlockedIncrement( &dragging ) == 0 )
                     {
                        NMTREEVIEWW *info = (NMTREEVIEW *) lp;

                        node * n = (node *)info->itemNew.lParam;

                        if ( n )
                        {
                           playlist_tree_panel * p = g_find_by_tree( hdr->hwndFrom );

                           if ( p )
                           {
                              n->do_drag_drop( p, shift_pressed() );
                              update_nodes_if_necessary();
                           }
                        }
                        InterlockedDecrement( &dragging );
                     }
                  }
                  break;

               }
            }
         }			

         break;

      case WM_CONTEXTMENU:
         {
            POINT pt = {(short)LOWORD(lp),(short)HIWORD(lp)};

            if ( pt.x == -1 || pt.y == -1 )
            {               
               node * n = g_last_active_tree->get_selection();
               TreeView_SelectDropTarget( n->m_hwnd_tree, n->m_tree_item );
               n->do_context_menu();
               TreeView_SelectDropTarget( n->m_hwnd_tree, NULL );
               return true;
            }

            TVHITTESTINFO ti;
            memset( &ti, 0, sizeof(ti) );
            ti.pt.x = (short)LOWORD(lp);
            ti.pt.y = (short)HIWORD(lp);

            ScreenToClient( GetDlgItem( wnd, IDC_TREE), &ti.pt );
            TreeView_HitTest( GetDlgItem( wnd, IDC_TREE ), &ti );

            if ( ti.hItem && ( ti.flags & TVHT_ONITEM ) )
            {
               TVITEMEX item;
               memset( &item, 0, sizeof( item ) );
               item.hItem = ti.hItem;
               item.mask = TVIF_PARAM;

               TreeView_GetItem( GetDlgItem( wnd, IDC_TREE ), &item );

               if ( item.lParam )
               {
                  node * n = (node *)item.lParam;

                  TreeView_SelectDropTarget( n->m_hwnd_tree, n->m_tree_item );
                  //n->do_context_menu( &pt );
                  
                  if ( shift_pressed() )
                  {
                     run_context_command( cfg_rightclick_shift, NULL, n );
                  }
                  else
                  {
                     run_context_command( cfg_rightclick, NULL, n );
                  }

                  TreeView_SelectDropTarget( n->m_hwnd_tree, NULL );
               }
            }
            else
            {
               playlist_tree_panel * p = g_find_by_hwnd( wnd );

               if ( p )
               {
                  p->do_context_menu( &pt );
               }
            }

            return true;           
         }
      }
      return false;
   }

public:
   ui_extension::window_host_ptr m_host;

   folder_node * m_root;

   tree_drop_target * m_drop_target;

   playlist_tree_panel()
   {
      m_wnd                = NULL;
      m_root               = NULL;
      m_drop_target        = new tree_drop_target( this );
      m_font               = NULL;
      m_has_been_asked     = false;
      m_imagelist          = NULL;
      m_was_active         = false;

   }

   virtual ~playlist_tree_panel()
   {
      delete m_drop_target;

      if ( m_font )
      {
         DeleteObject( m_font );
         m_font = NULL;
      }

      if ( m_imagelist )
      {
         ImageList_Destroy( m_imagelist );
         m_imagelist = NULL;
      }

      /*
      if ( m_was_active )
      {
         uMessageBox( NULL, "~playlist_tree_panel", "PT", MB_OK );
      }
      */
   }

   unsigned get_type() const 
   {
      return ui_extension::type_panel;
   }

   void destroy_window()
   {      
      //uMessageBox( NULL, "destroy_window", "PT", MB_OK );

      if ( g_last_active_tree == this )
      {
         g_last_active_tree = NULL;
      }
      // MessageBox( NULL, _T("destroy_window"), _T("Debug"), MB_OK );
      /*
      if ( !m_file.is_empty() )
      {
         pfc::stringcvt::string_wide_from_utf8 wide_file( m_file );

         FILE * f = _wfopen( wide_file, _T("w+") );

         if ( f ) 
         {
            m_root->write( f, 0 );
         }
         else
         {         
            if ( IDYES == 
               MessageBox( NULL, 
               _T("Error writing file.  Would you like to select another file?"), 
               _T("Playlist Tree"), MB_YESNO ) )
            {
               m_root->save( &m_file );
            }
         }
      }
      else
      {
         shutting_down();
      }
      */

      g_active_trees.remove_item( this );

      // dialog_proc( m_wnd, WM_DESTROY, 0, 0 );     
      DestroyWindow( m_wnd );
      
      if ( g_is_shutting_down )
      {
         delete m_root;
      }
      else
      {
         g_tmp_node_storage.add_item( m_root );
      }

      m_root = NULL;
      m_wnd = NULL;

      if ( g_active_trees.get_count() == 0 )
      {
         g_silence_handle = NULL;
      }
   }

   void get_category( pfc::string_base & out ) const
   {
      out.set_string( "Panels" );
   }

   virtual bool get_description( pfc::string_base & out ) const
   {
      out.set_string( "Playlist Tree Panel" );
      return true;
   }

   virtual bool get_short_name( pfc::string_base & out ) const 
   {
      out.set_string( "Playlist Tree" );
      return true;
   }

   virtual void get_name( pfc::string_base & out ) const
   {
      out.set_string( "Playlist Tree Panel" );
   }
  
   const GUID & get_extension_guid() const
   {
      return guid_playlist_tree_panel;
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

   virtual HWND get_tree( HWND wnd = NULL ) 
   {
      if ( wnd )
      {
         return GetDlgItem( wnd, IDC_TREE );
      }
      else
      {
         return GetDlgItem( m_wnd, IDC_TREE );
      }
   }

   virtual bool is_available( const ui_extension::window_host_ptr & p_host) const 
   {
      return ( m_wnd == NULL );
   }

   virtual bool search( const char * in )
   {
      static node *                 last_search_result = NULL;
      static pfc::string8_fastalloc last_search_string;
      static playlist_tree_panel *  last_tree = NULL;

      if ( in == NULL )
      {
         last_search_result = NULL;
         last_search_string.reset();
         last_tree = NULL;
         return true;
      }

      node * start;

      if ( last_tree == this && strcmp( last_search_string, in ) == 0 )
      {
         if ( last_search_result == NULL ) return false;
         start = last_search_result->iterate_next();
      }
      else
      {
         start                = m_root;
         last_search_string   = in;
         last_tree            = this;
      }

      search_tools::search_filter filt;
      filt.init( in );

      bool search_by_node = true;
#define SEARCH_BY_NODE

#ifdef SEARCH_BY_NODE
      // get a random handle to cheat and use as the formatting handle
      // if the handle matches the search, then dont search folders
      // because it cannot be a folder search anywho... 
      static_api_ptr_t<metadb> db;
      metadb_handle_ptr random_handle;

      if ( db->g_get_random_handle( random_handle ) )
      {
         if ( filt.test( random_handle ) )
         {
            console::printf( "random handle matches, ignoring folders in search" );
            search_by_node = false;
         }
      }
      else
      {
         search_by_node = false;
      }
   
#endif

      while ( start )
      {
         if ( start->is_leaf() )
         {
            leaf_node *lf = (leaf_node*) start;

            if ( filt.test( lf->m_handle ) )
            {
               console::printf( "Search Result: %s", (const char *) lf->m_label );
               last_search_result = lf;
               lf->ensure_in_tree();
               TreeView_SelectItem( lf->m_hwnd_tree, lf->m_tree_item );
               return true;
            }
         }
#ifdef SEARCH_BY_NODE
         else if ( search_by_node )
         {
            bool match = false;

            if ( strstr( start->m_label, in ) )
            {
               match = true;
            }

            if ( !match )
            {
               g_search_hook.get_static_instance().set_node( start );
               //service_ptr_t<search_titleformatting_hook> search_hook = 
               //   new service_impl_t<search_titleformatting_hook>(start);

               if ( filt.test( random_handle ) )
               {
                  match = true;
               }
               g_search_hook.get_static_instance().set_node( NULL );
            }

            if ( match )
            {
               console::printf( "Search Result: %s", (const char *) start->m_label );
               last_search_result = start;
               start->ensure_in_tree();
               TreeView_SelectItem( start->m_hwnd_tree, start->m_tree_item );
               return true;
            }

         }
#endif 
         start = start->iterate_next();
      }
      
      return false;
   }

   virtual void setup_tree()
   {
      TreeView_DeleteAllItems( get_tree() );
      if ( m_root )
      {
         if ( cfg_hideroot )
         {
            m_root->m_hwnd_tree = get_tree();
            m_root->m_tree_item = TVI_ROOT;            
            m_root->setup_children_trees( get_tree() );
         }
         else
         {
            m_root->setup_tree( get_tree(), TVI_ROOT );
         }
      }
   }

   virtual HWND create_or_transfer_window( HWND wnd_parent, const ui_extension::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position ) 
   {
      // console::printf( "create_or_transfer_window" );

      g_has_been_active = true;
      m_was_active = true;

      if ( g_silence_handle.is_empty() )
      {                   
         static_api_ptr_t<metadb>()->handle_create( g_silence_handle,
            make_playable_location( "silence://1", 0 ));
      }

      if ( !g_format_hook )
      {
         g_format_hook = new tree_titleformat_hook();
      }

      if ( m_wnd ) 
      {
         ShowWindow( m_wnd, SW_HIDE );
         SetParent( get_wnd(), wnd_parent );
         SetWindowPos( get_wnd(), NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER );
         m_host->relinquish_ownership( get_wnd() );
      }
      else
      {
         m_host = p_host;   
         m_wnd = uCreateDialog( IDD_BLANK, wnd_parent, dialog_proc, (LPARAM)this );
         RegisterDragDrop( get_wnd(), m_drop_target );

         // console::printf( "Creating playlist tree panel with this: %d", (int) this );
         g_active_trees.add_item( this );
         update_appearance();

         if ( g_tmp_node_storage.get_count() > 0 )
         {
            m_root = (folder_node*)g_tmp_node_storage[0];   
            g_tmp_node_storage.remove_by_idx( 0 );
            setup_tree();
         } 
         else
         {
            int n = g_active_trees.get_count() - 1;

            pfc::string8 filename;
            get_default_file_names( n, filename );

            if ( uFileExists( filename ) )
            {
               m_root = (folder_node*) do_load_helper( filename );
               setup_tree();
            }
         }
#if 0
         if ( m_file.is_empty() )
         {
            pfc::string_printf foo( "%s\\playlist-tree-%d.pts", 
               core_api::get_profile_path(), 
               /* (int) cfg_panel_count */
               g_find( this  ) ) ;
            
            filesystem::g_get_display_path( foo, m_file );
            cfg_panel_count = cfg_panel_count + 1;
            m_has_been_asked = true;

            console::printf( "Playlist Tree: No file information stored, using default: %s", 
               (const char *) m_file );

            /*
            if ( !cfg_last_saved_playlist.is_empty() )
            {
               pfc::string_printf msg("Would you like to restore your last saved playlist tree file? (%s)",
                  (const char *)cfg_last_saved_playlist );
               
               if ( IDYES == uMessageBox( m_wnd, msg, "Restore Playlist Tree?", MB_YESNO ) )
               {
                  m_file = cfg_last_saved_playlist;
               }                 
            }
            */
         }
         else // if ( !m_file.is_empty() )
         {
            node * n = do_load_helper( m_file );

            if ( n )
            {
               m_root = dynamic_cast<folder_node*>( n );

               setup_tree();
            } 
            else
            {
               console::warning( 
                  pfc::string_printf( "Error reading save / restore file %s", (const char *) m_file ) );
               m_file.reset();
            }
         }
#endif
      }

      if ( !m_root )
      {       
         m_root = new folder_node( "Playlist Tree", true );

         if ( cfg_use_default_queries )
         {
            m_root->add_child( 
               new query_node( "Playlists", false, TAG_PLAYLISTS, NULL,
               DEFAULT_QUERY_FORMAT, NULL, true ) );

            m_root->add_child( 
               new query_node( "by artist", false, 
               "@database", 
               "NOT artist MISSING",
               DEFAULT_QUERY_FORMAT,
               "@default", true ) );        
         }
         m_root->repopulate();

         setup_tree();
      }

      // ask_about_user_map( m_wnd );

      return m_wnd;
   }

   virtual bool get_prefer_multiple_instances() const
   {
      return false;
   }

   virtual void set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort)
   {    
#if 0
      try {
         p_reader->read_string( m_file, p_abort );
         p_reader->read( &m_has_been_asked, sizeof( m_has_been_asked ), p_abort );
      } 
      catch ( exception_io e )
      {        
         /*
         pfc::string_printf foo( "%s\\playlist-tree-%d.pts", 
            core_api::get_profile_path(), (int) cfg_panel_count ) ;
         
         filesystem::g_get_display_path( foo, m_file );
         cfg_panel_count = cfg_panel_count + 1;
         m_has_been_asked = true;

         console::printf( "No file information stored, using default: %s", 
            (const char *) m_file );
         */
      }
#endif
   }

   virtual void get_config(stream_writer * p_writer, abort_callback & p_abort) const 
   {
      //MessageBox( NULL, _T("get_config"), _T("Debug"), MB_OK );
      /*
      playlist_tree_panel * p = g_find_by_hwnd( m_wnd );
      if ( p )
      {
         p->shutting_down();               
         p->m_has_been_asked = true;
      }
      */
      /*
      bool has_been_asked = true;
      p_writer->write_string( m_file, p_abort );
      p_writer->write( &has_been_asked, sizeof( has_been_asked ), p_abort );
      */
   }   

   enum { CT_SELECT = 1, CT_SELECT_AND_LOAD, CT_CANCEL, 
      CT_REDRAW, CT_COLLAPSE, 
      // CT_FAVORITES,
      CT_END 
   };
   bool do_context_menu( POINT * pt )
   {
      HMENU menu = CreatePopupMenu();

      /*
      uAppendMenu( menu, MF_STRING, 1, "Select File..." );
      uAppendMenu( menu, MF_STRING, 2, "Select File and Load..." );
      uAppendMenu( menu, MF_STRING, 3, "Cancel Auto Save / Restore" );
      uAppendMenu( menu, MF_SEPARATOR, -1, "" );
      */
      uAppendMenu( menu, MF_STRING, CT_REDRAW, "Redraw Tree" );
      uAppendMenu( menu, MF_STRING, CT_COLLAPSE, "Collapse Tree" );
      // uAppendMenu( menu, MF_STRING, CT_FAVORITES, "cwbowron Favorites" );
      uAppendMenu( menu, MF_SEPARATOR, -1, "" );
      
      int x = CT_END;
      int i = 0;
      for ( i = 0; i < g_active_trees.get_count(); i++ )
      {
         pfc::string_printf str( "%s [panel %d]",
            (const char *)g_active_trees[i]->m_root->m_label, i );

         if ( g_active_trees[i] == this )
         {
            uAppendMenu( menu, MF_STRING | MF_CHECKED, x, str );
         }
         else
         {
            uAppendMenu( menu, MF_STRING, x, str );
         }
         x++;
      }

      for ( i = 0; i < g_tmp_node_storage.get_count(); i++ )
      {
         pfc::string_printf str( "%s [tmp %d]",
            (const char *)g_tmp_node_storage[i]->m_label, i );

         uAppendMenu( menu, MF_STRING, x, str );

         x++;
      }

      int n = TrackPopupMenu( menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, 
         pt->x, pt->y, 0, m_wnd, 0);

      if ( n )
      {
         if ( n >= CT_END )
         {
            int m = n - CT_END;
            if ( m < g_active_trees.get_count() )
            {
               // console::printf( "%s", (const char *)g_active_trees[m]->m_root->m_label );

               if ( g_active_trees[m] == this )
               {
                  console::printf( "SAME TREE" );
               }
               else
               {
                  // swap 'em
                  folder_node * t = m_root;
                  m_root = g_active_trees[m]->m_root;
                  g_active_trees[m]->m_root = t;
                  g_active_trees[m]->setup_tree();
                  setup_tree();
               }
            }
            else
            {
               int o = m - g_active_trees.get_count();
               //console::printf( "%s", (const char *)g_tmp_node_storage[o]->m_label );

               folder_node * t = m_root;
               m_root = (folder_node*)g_tmp_node_storage[o];
               g_tmp_node_storage.remove_by_idx( o );
               g_tmp_node_storage.add_item( t );
               setup_tree();
            }
         }
         else switch ( n )
         {
#if 0
         case CT_FAVORITES:
            {
               folder_node * fav = new folder_node( "cwbowron's suggestions", true );

               static_api_ptr_t<metadb> db;


               service_ptr_t<playlist_incoming_item_filter> p;            
               if ( playlist_incoming_item_filter::g_get( p ) )
               {                  
                  for ( int z = 0; z < tabsize( g_favorites ); z++ )
                  {
                     metadb_handle_list list;
                     const char * loc = g_favorites[z].location;
                     const char * name = g_favorites[z].name;

                     p->process_location( loc, list, false, NULL, NULL, get_wnd() );

                     if ( list.get_count() == 1 )
                     {
                        fav->add_child( new leaf_node ( name, list[0] ));
                     }
                     else
                     {
                        folder_node * fn = new folder_node( name, true );
                        fav->add_child( fn );

                        for ( unsigned i = 0; i < list.get_count(); i++ )
                        {
                           if (useable_handle( list[i] ) )
                           {
                              fn->add_child( new leaf_node ( NULL, list[i] ));
                           }
                        }
                     }

                     /*
                     metadb_handle_ptr h;
                     db->handle_create( h, make_playable_location( loc, 0 ) );
                     leaf_node * n = new leaf_node( name, h );
                     fav->add_child( n );
                     */
                  }
               }
               m_root->add_child( fav );
               setup_tree();
            }            
            break;
#endif
         case CT_COLLAPSE:
            m_root->collapse( true );
            setup_tree();
            break;

         case CT_REDRAW:
            setup_tree();
            break;

         case CT_CANCEL:
            m_file.reset();
            m_has_been_asked = false;
            break;

         case CT_SELECT:
            if ( get_save_name( m_file ) )
               return true;
            return false;

         case CT_SELECT_AND_LOAD:
            if ( get_save_name( m_file ) )
            {
               node * n = do_load_helper( m_file );

               if ( n )
               {
                  folder_node * fn = dynamic_cast<folder_node*>( n );

                  if ( fn )
                  {
                     set_root( fn );
                     setup_tree();
                     return true;
                  }                 
               }                             
            }
            return false;
            break;
         }

         return true;
      }

      return false;
   }

   node * find_node_by_pt( int screenx, int screeny, bool item_only = false )
   {
      return g_find_node_by_pt( get_tree(), screenx, screeny, item_only );
   }

   node *find_drop_target( int screenx, int screeny, int * out_location )
   {
      node *res = find_node_by_pt( screenx, screeny );

      if ( res ) 
      {
         RECT r;
         POINT pt;

         pt.x = screenx;
         pt.y = screeny;

         ScreenToClient( get_tree(), &pt );

         if ( !is_valid_htree( res->m_tree_item ) )
         {
            *out_location = DROP_HERE;
            return res;
         }

         TreeView_GetItemRect( get_tree(), res->m_tree_item, &r, false );			

         int height = r.bottom - r.top;
         int half_height = height / 2;
         int quarter_height = height / 4;
         int half = half_height + r.top;			

         int before_max = r.top + quarter_height;
         int after_min = before_max + half_height;

         if ( pt.y <= before_max )
         {
            *out_location = DROP_BEFORE;
            return res;
         }
         else if ( pt.y > after_min )
         {
            *out_location = DROP_AFTER;
            return res;
         }		
         else
         {
            *out_location = DROP_HERE;
            return res;
         }
      }

      *out_location = DROP_HERE;
      return m_root;
   }

   static node * g_find_node_by_client_pt( HWND tree, int clientx, int clienty, bool item_only = false )
   {
      TVHITTESTINFO ti;
      memset( &ti,0,sizeof(ti) );

      ti.pt.x = clientx;
      ti.pt.y = clienty;

      if ( TreeView_HitTest( tree, &ti ) )
      {
         if ( ti.hItem )
         {
            TVITEMEX item;
            memset( &item, 0, sizeof( item ) );
            item.hItem = ti.hItem;
            item.mask = TVIF_PARAM;

            TreeView_GetItem( tree, &item );

            if ( item.lParam )     
            {         
               if ( !item_only || ( ti.flags & TVHT_ONITEM )) 
               {
                  return (node *)item.lParam;
               }
            }
         }
      }
      return NULL;
   }


   static node * g_find_node_by_pt( HWND tree, int screenx, int screeny, bool item_only = false )
   {
      POINT pt;
      pt.x = screenx;
      pt.y = screeny;

      ScreenToClient( tree, &pt );

      return g_find_node_by_client_pt( tree, pt.x, pt.y, item_only );
   }


   void set_root( folder_node * n )
   {
      if ( m_root )
      {
         delete m_root;
      }
      m_root = n;
   }

   void update_appearance()
   {
      switch ( cfg_edge_style )
      {
      case EE_NONE:
         SetWindowLong( get_tree(), GWL_EXSTYLE, MY_EDGE_NONE );
         break;
      case EE_GREY:
         SetWindowLong( get_tree(), GWL_EXSTYLE, MY_EDGE_GREY );
         break;
      case EE_SUNKEN:
         SetWindowLong( get_tree(), GWL_EXSTYLE, MY_EDGE_SUNKEN );
         break;
      }

      DWORD style = WS_TABSTOP | 
	      WS_VISIBLE | 
         WS_CHILD | 
         TVS_SHOWSELALWAYS;
   
      if ( cfg_nohscroll )
      {
         style |= TVS_NOHSCROLL;
      }
      if ( !cfg_hidelines )
      {
         style |= TVS_HASLINES | TVS_LINESATROOT;
      }
      if ( !cfg_hidebuttons )
      {           
         style |= TVS_HASBUTTONS;
      }
      
      SetWindowLong( get_tree(), GWL_STYLE, style );

      SetBkColor( GetDC( m_wnd ),         cfg_back_color );
      TreeView_SetBkColor( get_tree(),    cfg_back_color);
      ImageList_SetBkColor( m_imagelist,  cfg_back_color);
      TreeView_SetLineColor( get_tree(),  cfg_line_color);
      TreeView_SetTextColor( get_tree(),  cfg_text_color);

      if ( m_font )
      {
         DeleteObject( m_font );
      }
         
      m_font = CreateFontIndirect( &(LOGFONT)cfg_font );			

      if (m_wnd)
      {
         uSendMessage( get_tree(), WM_SETFONT, (WPARAM)m_font, MAKELPARAM( 1, 0 ) );
      }

      SetWindowPos( get_tree(), 
         0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE  | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED );

      InvalidateRect( m_wnd, NULL, TRUE );      
   }

   void shutting_down()
   {
      /*
      if ( !m_has_been_asked )
      {
         if ( IDYES == 
            MessageBox( NULL, 
            _T( "You have not chosen a file to save and restore from for this panel.  Would you like to do so now?"), 
            _T("Playlist Tree"), 
            MB_YESNO ) )
         {
            m_root->save( &m_file );
            cfg_last_saved_playlist = m_file;
         }
         m_has_been_asked = true;
      }
      */
   }

   static void g_shutting_down()
   {
      for ( int n = 0; n < g_active_trees.get_count(); n++ )
      {
         g_active_trees[n]->shutting_down();
      }
   }

   static void g_update_appearance()
   {
      for ( int n = 0; n < g_active_trees.get_count(); n++ )
      {
         g_active_trees[n]->update_appearance();
      }
   }

   node * get_selection()
   {
      HTREEITEM item = TreeView_GetSelection( get_tree() );
      TVITEMEX tv;
      memset( &tv, 0, sizeof(tv) );
      tv.hItem = item;
      tv.mask = TVIF_PARAM;

      TreeView_GetItem( get_tree(), &tv );

      if ( tv.lParam )
      {
         node * n = (node *) tv.lParam;
         return n;
      }
      return NULL;
   }

   static int g_find( playlist_tree_panel * item )
   {
      for ( int n = 0; n < g_active_trees.get_count(); n++ )
      {
         if ( g_active_trees[n] == item )
         {
            return n;
         }
      }
      return -1;
   }

   static playlist_tree_panel * g_find_by_hwnd( HWND wnd )
   {
      for ( int n = 0; n < g_active_trees.get_count(); n++ )
      {
         if ( g_active_trees[n]->get_wnd() == wnd )
         {
            return g_active_trees[n];
         }
      }
      return NULL;
   }

   static playlist_tree_panel * g_find_by_tree( HWND tree )
   {
      for ( int n = 0; n < g_active_trees.get_count(); n++ )
      {
         if ( g_active_trees[n]->get_tree() == tree )            
         {
            return g_active_trees[n];
         }
      }
      return NULL;
   }
};

//static service_factory_single_t<playlist_tree_panel> g_playlist_tree_panel;
static ui_extension::window_factory<playlist_tree_panel> g_music_browser_factory;

HRESULT STDMETHODCALLTYPE tree_drop_target::DragEnter(
   IDataObject * pDataObject,
   DWORD grfKeyState,
   POINTL pt,        
   DWORD * pdwEffect) 
{
   BringWindowToTop( m_panel->get_tree() );
   //*pdwEffect = DROPEFFECT_COPY;
   return S_OK;
}

HRESULT STDMETHODCALLTYPE tree_drop_target::DragOver( DWORD grfKeyState, POINTL pt, DWORD * pdwEffect )
{
   BringWindowToTop( m_panel->get_tree() );

   int target_location;
   node *dest = m_panel->find_drop_target( pt.x, pt.y, &target_location );
   node *actual_target = NULL;

   if ( dest )
   {
      switch ( target_location )
      {
      case DROP_BEFORE:
         if ( dest->m_parent )
         {
            if ( is_valid_htree( dest->m_tree_item ) )
            {
               TreeView_SetInsertMark( m_panel->get_tree(), dest->m_tree_item, false );
            }

            if ( is_valid_htree ( dest->m_parent->m_tree_item ) )
            {
               TreeView_SelectDropTarget( m_panel->get_tree(), dest->m_parent->m_tree_item );
            }
            actual_target = dest->m_parent;
         }
         break;

      case DROP_AFTER:
         if ( dest->m_parent )
         {
            if ( is_valid_htree( dest->m_tree_item ) )
            {
               TreeView_SetInsertMark( m_panel->get_tree(), dest->m_tree_item, true );               
            }
            if ( is_valid_htree ( dest->m_parent->m_tree_item ) )
            {
               TreeView_SelectDropTarget( m_panel->get_tree(), dest->m_parent->m_tree_item );
            }
            actual_target = dest->m_parent;
         }
         break;

      case DROP_HERE:
         TreeView_SetInsertMark( m_panel->get_tree(), NULL, true );

         if ( is_valid_htree( dest->m_tree_item ) )
         {
            TreeView_SelectDropTarget( m_panel->get_tree(), dest->m_tree_item );			
         }
         actual_target = dest;
         break;
      }
   }

   if ( actual_target && actual_target->can_add_child() )
   {
      //*pdwEffect = DROPEFFECT_COPY;
   }
   else
   {
      *pdwEffect = DROPEFFECT_NONE;
   }
   return S_OK;
}

HRESULT STDMETHODCALLTYPE tree_drop_target::DragLeave( void )
{
   TreeView_SelectDropTarget( m_panel->get_tree(), 0 );
   TreeView_SetInsertMark( m_panel->get_tree(), NULL, false );
   return S_OK;
}

HRESULT STDMETHODCALLTYPE tree_drop_target::Drop( IDataObject * pDataObject,
                                                 DWORD grfKeyState, 
                                                 POINTL pt,
                                                 DWORD * pdwEffect )
{	
   node *destination = m_panel->m_root;		

   int insertion_location = -1;

   bool rv = false;

   int relative_location;
   node *drop_target = m_panel->find_drop_target( pt.x, pt.y, &relative_location );

   TreeView_SelectDropTarget( m_panel->get_tree(), 0 );
   TreeView_SetInsertMark( m_panel->get_tree(), NULL, false);

   if (drop_target)
   {
      switch ( relative_location )
      {
      case DROP_BEFORE:
         if ( drop_target->m_parent )
         {
            destination = drop_target->m_parent;
            insertion_location = destination->get_child_index( drop_target );
         }
         break;

      case DROP_AFTER:
         if ( drop_target->m_parent )
         {
            destination = drop_target->m_parent;
            insertion_location = destination->get_child_index( drop_target );
            insertion_location++;
         }
         break;

      case DROP_HERE:
         if (drop_target->is_leaf())
         {
            console::printf( "Illegal Drop on Leaf Node\n" );
            return S_OK;
         }
         destination = drop_target;
         break;
      }
   }

   if ( !destination->can_add_child() )
   {
      console::printf( "Illegal Drop (over limit)\n" );
      return S_OK;
   }

   FORMATETC fmt = { CF_NODE, 0, DVASPECT_CONTENT, -1, TYMED_NULL };
   STGMEDIUM medium = { TYMED_NULL, 0, 0 };

   if ( pDataObject->GetData( &fmt, &medium ) == S_OK )
   {
      if ( false )
      {
         MessageBox(NULL, _T("Local Drag And Drop Has Been Disabled"), 
            _T("Playlist Tree"), MB_OK);
         return E_INVALIDARG;
      }

      node *source = (node*)pDataObject;
      node *sourceParent = source->m_parent;

      if ( source->has_subchild( destination )) 
      {
         console::printf( "Illegal Drop" );     
         *pdwEffect = DROPEFFECT_NONE;
         return S_OK;
      }
      else 
      {
         node *select_me = source;

         if ( *pdwEffect & DROPEFFECT_MOVE ) 
         {
            if ( sourceParent )
            {
               sourceParent->remove_child( source );
            }

            destination->add_child( source, insertion_location );
            *pdwEffect = DROPEFFECT_MOVE;
         }
         else if ( *pdwEffect & DROPEFFECT_COPY )
         {
            select_me = source->copy();
            destination->add_child( select_me, insertion_location );
            *pdwEffect = DROPEFFECT_COPY;
         }

         destination->refresh_subtree( m_panel->get_tree() );

         //if ( sourceParent && sourceParent != destination )
         //{
         //sourceParent->refresh_subtree( m_panel->get_tree() );
         //}

         select_me->ensure_in_tree();
         select_me->force_tree_selection(); 

         metadb_handle_list list;
         select_me->get_entries( list );
         update_drop_watchers( destination, list );

         return S_OK;
      }
   }

   fmt.tymed    = TYMED_HGLOBAL;
   fmt.cfFormat = CF_HDROP;

   // This needs to be here to get directory structure on drops
#if 1
   // Test for file drops
   if ( pDataObject->GetData( &fmt, &medium ) == S_OK ) 
   {       
      metadb_handle_list list;

      node *newstuff = destination->process_hdrop( (HDROP)medium.hGlobal, 
         insertion_location, grfKeyState & MK_SHIFT, &list );

      ReleaseStgMedium( &medium );

      if ( newstuff->m_parent )
      {
         newstuff->m_parent->refresh_subtree( m_panel->get_tree() );
      }

      newstuff->ensure_in_tree();
      newstuff->force_tree_selection();

      update_drop_watchers( destination, list );
      update_nodes_if_necessary();
   } 
   else 
#endif
   {
      metadb_handle_list list;

      // service_ptr_t<playlist_incoming_item_filter> p;
      // if ( playlist_incoming_item_filter::g_get( p ) )

      static_api_ptr_t<playlist_incoming_item_filter> p;

      {                  
         if ( p->process_dropped_files( pDataObject, list, true, m_panel->get_wnd() ) )
         {
            for ( unsigned i = 0; i < list.get_count(); i++ )
            {
               if (useable_handle( list[i] ) )
               {
                  destination->add_child( new leaf_node ( NULL, list[i] ));
               }
               else
               {
                  console::printf( "Unuseable handle: %s\n", list[i]->get_path() );
               }
            }

            destination->refresh_subtree( m_panel->get_tree() );

            update_drop_watchers( destination, list );
            update_nodes_if_necessary();
         }
      }
   }


   *pdwEffect = DROPEFFECT_COPY;
   return S_OK;
}


HRESULT STDMETHODCALLTYPE tree_drop_source::QueryContinueDrag( BOOL fEscapePressed, DWORD grfKeyState )
{   
   if ( fEscapePressed || ( grfKeyState & MK_RBUTTON ) ) 
   {
      return DRAGDROP_S_CANCEL;
   }
   else if ( !( grfKeyState & MK_LBUTTON ) )
   {
      return DRAGDROP_S_DROP;
   }
   else 
   {
      return S_OK;
   }
}

HRESULT STDMETHODCALLTYPE tree_drop_source::GiveFeedback( DWORD dwEffect )
{
   if ( m_panel->get_wnd() )
   {
      DWORD pts = GetMessagePos();
      int x = LOWORD(pts);
      int y = HIWORD(pts);

      node *foop = m_panel->find_node_by_pt( x, y );

      if ( foop ) 		
      {
         TreeView_EnsureVisible( m_panel->get_wnd(), foop->m_tree_item );

         // cant drop into your own subchild...
         if ( m_node && m_node->has_subchild( foop ) )
         {
            //console::printf( "Cannot Drop on own subchild" );
            SetCursor( LoadCursor( 0, IDC_NO ) );
            return S_OK;
         }        
      }
      else 
      {
         RECT tree_rect;

         GetWindowRect( m_panel->get_wnd(), &tree_rect );

         if ( x > tree_rect.left && x < tree_rect.right )
         {
            if ( (y>tree_rect.bottom)  && (y<tree_rect.bottom+30) )
               SendMessage( m_panel->get_tree(), WM_VSCROLL, SB_LINEDOWN, NULL);					
            if ((y<tree_rect.top)  && (y>tree_rect.top-30))
               SendMessage( m_panel->get_tree(), WM_VSCROLL, SB_LINEUP, NULL);
         }
      }
   }

   return DRAGDROP_S_USEDEFAULTCURSORS;
}

void lazy_add_user_query( folder_node * n )
{
   query_node * f = new query_node( "New Query", false );

   n->add_child( f );
   f->ensure_in_tree();
   f->force_tree_selection();

   if ( !f->edit() )
   {
      delete f;
   }
}


void lazy_set_root( HWND tree, node * n )
{
   playlist_tree_panel * p = playlist_tree_panel::g_find_by_tree( tree );

   if ( p )
   {
      p->set_root( (folder_node *) n );
      p->setup_tree();
   }
}

tree_titleformat_hook::tree_titleformat_hook( node * n )
{
   set_node( n );
}

void tree_titleformat_hook::set_node( node * n )
{
   m_node = n;
}

bool tree_titleformat_hook::process_field(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag) 
{
   if ( stricmp_utf8_ex( p_name, p_name_length, "_name", ~0 ) == 0 )
   {
      p_out->write( titleformat_inputtypes::meta, m_node->m_label, m_node->m_label.get_length() );
      p_found_flag = true;         
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_displayname", ~0 ) == 0 )
   {
      if ( m_node->is_leaf() )
      {
         p_out->write( titleformat_inputtypes::meta, m_node->m_label, m_node->m_label.get_length() );
         p_found_flag = true;         
         return true;  
      }
      else
      {
         pfc::string8 dn;
         folder_node * fn = (folder_node *)m_node;
         fn->process_local_tags( dn, fn->m_label );
         p_out->write( titleformat_inputtypes::meta, dn, ~0 );
         p_found_flag = true;         
         return true;  
      }
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_size", ~0 ) == 0 )
   {
      p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d", m_node->get_file_size(), ~0 ) );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_size_abbr", ~0 ) == 0 )
   {
      __int64 size = m_node->get_file_size();
      __int64 megs = size / (1024*1024);

      if ( megs > 2 * 1024 )
      {
         __int64 gigs = megs / 1024;
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d GB", gigs, ~0 ) );           
      }
      else
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d MB", megs, ~0 ) );           
      }

      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_size_abb", ~0 ) == 0 )
   {
      __int64 size = m_node->get_file_size();
      __int64 megs = size / (1024*1024);

      if ( megs > 2 * 1024 )
      {
         __int64 gigs = megs / 1024;
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d G", gigs, ~0 ) );           
      }
      else
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf( "%d M", megs, ~0 ) );           
      }

      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_play_length", ~0 ) == 0 )
   {
      double length = m_node->get_play_length();
      int hours;
      int mins;
      int secs;
      int days;

      secs = (int)ceil(length) % 60;
      mins = (int)floor( length / 60.0 ) % 60;
      hours = (int)floor( length / (60.0 * 60.0) );
      days = (int)floor(length / (60.0 * 60.0 * 24.0));

      if (days>2) 
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf(">%d days",days), ~0 );
      }
      else if (hours>0) 
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d:%02d:%02d",hours, mins, secs), ~0 );
      }
      else
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d:%02d", mins, secs), ~0 );
      }
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_itemcount", ~0 ) == 0 )
   {
      p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_entry_count()), ~0 );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_foldercount", ~0 ) == 0 )
   {
      p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_folder_count()), ~0 );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_folderindex", ~0 ) == 0 )
   {
      if ( m_node->m_parent )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d",            
            m_node->m_parent->m_children.find_item( m_node ) + 1, ~0 )) ;
         p_found_flag = true;
      return true;         
      }           
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_overallindex", ~0 ) == 0 )
   {
      p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_folder_overall_index()), ~0 );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_nestlevel", ~0 ) == 0 )
   {
      p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_nest_level()), ~0 );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_isleaf", ~0 ) == 0 )
   {
      if ( m_node->is_leaf() )
      {
         p_out->write( titleformat_inputtypes::meta, "1", ~0 );
         p_found_flag = true;
         return true;
      }
      p_found_flag = true;
      return false;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_isfolder", ~0 ) == 0 )
   {
      if ( m_node->is_folder() )
      {
         p_out->write( titleformat_inputtypes::meta, "1", ~0 );
         p_found_flag = true;
         return true;
      }
      p_found_flag = true;
      return false;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "_isquery", ~0 ) == 0 )
   {
      if ( m_node->is_query() )
      {
         p_out->write( titleformat_inputtypes::meta, "1", ~0 );
         p_found_flag = true;
         return true;
      }
      p_found_flag = true;
      return false;
   }
   else
   {
      /*
      pfc::string8 str( p_name, p_name_length );
      
      console::printf( "Unknown tag (%s) in node (%s) formatting",
         (const char *)str, (const char * ) m_node->m_label );

      metadb_handle_ptr first;
      m_node->get_first_entry( first );

      if ( first.is_valid() )
      {
         file_info_impl info;
         first->get_info( info );
         int tag_index = info.meta_find( str );

         if ( tag_index != ~0 )
         {
            p_out->write( titleformat_inputtypes::meta, info.meta_get( str, tag_index), ~0 );
            p_found_flag = true;
            return true;
         }
      }
      */
   }

   return false;
}

bool tree_titleformat_hook::process_function(titleformat_text_out * p_out,const char * p_name,t_size p_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
   if ( stricmp_utf8_ex( p_name, p_name_length, "avg", ~0 ) == 0 )
   {
      const char *s;
      pfc::string8 ss;
      t_size len;

      p_params->get_param( 0, s, len );

      ss.set_string( s, len );
      metadb_handle_list lst;
      m_node->get_entries( lst );

      double sum = 0;
      int count = 0;         
      pfc::string8_fastalloc our_info;

      for ( unsigned i = 0; i < lst.get_count(); i++ )
      {
         service_ptr_t<titleformat_object> script;
         static_api_ptr_t<titleformat_compiler>()->compile_safe(script,ss);
         lst[i]->format_title( NULL, our_info, script, NULL );	
         sum += atoi(our_info);
         count++;
      }

      double avg = (count>0) ? sum / count : 0;

      char buff[40];
      _snprintf( buff, 40, "%.2f", avg );
      buff[ 40 - 1] = 0;

      p_out->write( titleformat_inputtypes::unknown, buff, ~0 );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "sum", ~0 ) == 0 )
   {
      const char *s;
      pfc::string8 ss;
      t_size len;

      p_params->get_param( 0, s, len );

      ss.set_string( s, len );
      metadb_handle_list lst;
      m_node->get_entries( lst );

      int sum = 0;
      pfc::string8_fastalloc our_info;

      for ( unsigned i = 0; i < lst.get_count(); i++ )
      {
         service_ptr_t<titleformat_object> script;
         static_api_ptr_t<titleformat_compiler>()->compile_safe(script,ss);
         lst[i]->format_title( NULL, our_info, script, NULL );	
         sum += atoi(our_info);
      }
      p_out->write( titleformat_inputtypes::unknown, pfc::string_printf( "%d", sum ), ~0 );
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "playing", ~0 ) == 0 )
   {
      const char *s;
      pfc::string8 ss;
      t_size len;

      p_params->get_param( 0, s, len );

      ss.set_string( s, len );

      static_api_ptr_t<playback_control> srv;
      
      metadb_handle_ptr temp;
		if ( srv->get_now_playing(temp) ) 
      {
         pfc::string8_fastalloc str;
         if ( temp->format_title_legacy( this, str, ss, NULL ) )
         {
            p_out->write( titleformat_inputtypes::unknown, str, ~0 );
            p_found_flag = true;                 
         }
      }
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "hidetext", ~0 ) == 0 )
   {
      p_found_flag = true;
      return true;
   }
   else if ( stricmp_utf8_ex( p_name, p_name_length, "parent", ~0 ) == 0 )
   {
      node * parent_node = NULL;

      if ( p_params->get_param_count() == 1 )
      {
         parent_node = m_node->get_parent();
      }
      else if ( p_params->get_param_count() == 2 )
      {
         int n = p_params->get_param_uint( 1 );           
         parent_node = m_node->get_parent( n );
      }

      if ( parent_node )
      {
         pfc::string8 ss;
         t_size len;
         const char *s;

         p_params->get_param( 0, s, len );
         ss.set_string( s, len );

         pfc::string8 parent_out;
         parent_node->format_title( parent_out, ss );
         p_out->write( titleformat_inputtypes::unknown, parent_out, ~0 );
         p_found_flag = true;
      }

      return true;
   }
   return false;
}

class woot : public initquit
{
   void on_init() 
   {
      if ( !g_format_hook )
      {
         g_format_hook = new tree_titleformat_hook();
      }

      // scheme_set_stack_base(NULL, 1);
      g_scheme_environment = scheme_basic_env();
   }

   void on_quit() 
   {
      g_is_shutting_down = true;

      int file_count = 0;

      for ( int p = 0; p < g_active_trees.get_count(); p++ )
      {
         pfc::string8 file;
         pfc::string_printf foo( "%s\\playlist-tree-%d.pts", 
            core_api::get_profile_path(), 
            file_count ) ;

         filesystem::g_get_display_path( foo, file );

         file_count++;

         g_active_trees[p]->m_root->write( file );
      }

      for ( int n = 0; n < g_tmp_node_storage.get_count(); n++ )
      {
         pfc::string8 file;
         pfc::string_printf foo( "%s\\playlist-tree-%d.pts", 
            core_api::get_profile_path(), 
            file_count ) ;
         file_count++;

         filesystem::g_get_display_path( foo, file );

         g_tmp_node_storage[n]->write( file );         
      }

      if ( g_tmp_node_storage.get_count() )
      {
         g_tmp_node_storage.delete_all();
      }

      if ( g_format_hook )
         delete g_format_hook;

      if ( g_scheme_environment )
      {
      }
   }
};

static service_factory_single_t<woot> g_woot;

bool get_save_name( pfc::string_base & out_name, bool prompt_overwrite, bool open, bool as_text )
{
   TCHAR fileName[MAX_PATH];

   TCHAR *filters;
   if ( as_text )
   {
      filters = _T("Text File (*.") _T("txt") _T(")\0")
         _T("*.") _T("txt") _T("\0")
         _T("All files (*.*)\0")
         _T("*.*\0")
         _T("\0");
   }
   else
   {
      filters = _T("Playlist Tree SEXP (*.") _T(PLAYLIST_TREE_EXTENSION) _T(")\0")
         _T("*.") _T(PLAYLIST_TREE_EXTENSION) _T("\0")
         _T("All files (*.*)\0")
         _T("*.*\0")
         _T("\0");
   }
   OPENFILENAME blip;
   memset((void*)&blip, 0, sizeof(blip));
   memset(fileName, 0, sizeof(fileName));
   blip.lpstrFilter = filters;
   blip.lStructSize = sizeof(OPENFILENAME); 
   blip.lpstrFile = fileName;
   blip.nMaxFile = sizeof(fileName)/ sizeof(*fileName);    
   blip.lpstrTitle = open ? _T("Select File To Open") : _T("Save Tree As...");
   blip.lpstrDefExt = _T(PLAYLIST_TREE_EXTENSION);
   blip.Flags = prompt_overwrite ? OFN_OVERWRITEPROMPT : 0 ;

   if ( open )
   {
      if ( GetOpenFileName( &blip ) )
      {
         out_name = pfc::stringcvt::string_utf8_from_wide( blip.lpstrFile );
         return true;         
      }
   }
   else
   {
      if ( GetSaveFileName( &blip ) ) 
      {
         out_name = pfc::stringcvt::string_utf8_from_wide( blip.lpstrFile );
         return true;
      }
   }
   return false;
}

static BOOL CALLBACK select_icon_callback( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   static HWND list_hwnd = 0;
   static int * selected_icon;

   switch( msg )
   {
   case WM_INITDIALOG:
      {
         selected_icon = (int*)lp;
         list_hwnd = GetDlgItem( wnd, IDC_ICONLIST );				
         ListView_SetImageList( list_hwnd, g_imagelist, LVSIL_NORMAL );
         ListView_SetImageList( list_hwnd, g_imagelist, LVSIL_SMALL );

         ListView_SetBkColor( list_hwnd, cfg_back_color );
         ListView_SetTextColor( list_hwnd, cfg_text_color );
         ListView_SetTextBkColor( list_hwnd, cfg_back_color );

         int max_icon = ImageList_GetImageCount( g_imagelist );
         LVITEM item;
         memset( &item, 0, sizeof(item) );

         for ( int i = 0 ; i < max_icon; i++ )
         {
            pfc::stringcvt::string_wide_from_utf8 text( pfc::string_printf( "%d", i ) );;

            item.iItem = i;
            item.pszText = (LPWSTR)(const wchar_t *)text;
            item.iImage = i;
            item.mask = LVIF_TEXT|LVIF_IMAGE;

            ListView_InsertItem( list_hwnd, &item );
         }
      }
      break;

   case WM_COMMAND:
      switch ( wp )
      {
      case IDOK:
         *selected_icon = ListView_GetSelectionMark( list_hwnd );
         /* fall through */
      case IDCANCEL:
         EndDialog( wnd, wp ); 
         break;
      }
   }
   return 0;
}

static int select_icon( HWND parent, HIMAGELIST image_list )
{
   int result = -1;

   g_imagelist = image_list;

   if ( g_imagelist == NULL )
   {
      console::printf( "No Image List Loaded\n" );
      return 0;
   }

   int res = DialogBoxParam( core_api::get_my_instance(), 
      MAKEINTRESOURCE( IDD_ICONSELECT ), 
      parent, 
      select_icon_callback, 
      (LPARAM)&result );

   if ( res == IDOK )
   {
      return result;
   }
   return -1;
}

enum { 
   CM_ADD_ACTIVE, 
   CM_SEND_ACTIVE,
   CM_NEW,
   CM_SEND_LIBRARY,
   CM_SEND_LIBRARY_SELECTED,
   CM_PLAY_LIBRARY,
   CM_LAST 
};

struct {
   char * label;
   char * desc;
   char * global_path;
   char * local_path;
   GUID guid;   
} cm_map[CM_LAST] = 
{
   { "Add to active playlist", 
   "Adds item to the active playlist", 
   "Playlist Tree",
   "Playlist",
   { 0x58060ff2, 0xaaf, 0x4597, { 0xab, 0xd2, 0x4b, 0xbe, 0x74, 0x45, 0xa8, 0x46 } } },

   { "Send to active playlist", 
   "Sends item to active playlist, clearing it",
   "Playlist Tree",
   "Playlist",        
   { 0x5c0ab9a7, 0x308d, 0x47bd, { 0x85, 0xd1, 0xc9, 0xca, 0xd1, 0x97, 0x86, 0xee } } },

   { "New playlist", 
   "Creates a new playlist from the items and activates it",
   "Playlist Tree", 
   "Playlist",            
   { 0x9a0fb175, 0x6507, 0x4b7d, { 0xae, 0x45, 0xe0, 0x63, 0xcb, 0x46, 0xfe, 0x97 } } },

   { "Send to Library Playlist", 
   "Sends items to designated library playlist",
   "Playlist Tree",
   "Playlist",
   { 0xa830ba23, 0xdef1, 0x4a7c, { 0xbd, 0xc1, 0xb9, 0x1f, 0xed, 0xef, 0xd, 0xf0 } } },

   { "Send to Library Playlist (Selected)", 
   "Sends items to designated library playlist",
   "Playlist Tree",
   "Playlist",
   { 0x7e1fea7, 0x3054, 0x4e17, { 0xb2, 0xb9, 0xe2, 0x6f, 0xe6, 0x0, 0x89, 0x6a } } },
   
   { "Play in Library Playlist",
   "Sends items to designated library playlists and plays it",
   "Playlist Tree",
   "Playlist",
   { 0xded9942c, 0x820b, 0x4225, { 0x88, 0xa6, 0x3b, 0x73, 0xba, 0xbc, 0x4a, 0x50 } } },
};

class tree_context_items : public contextmenu_item_simple
{
   unsigned get_num_items()
   {
      return CM_LAST;
   }

   GUID get_item_guid( unsigned int n )
   {
      if ( n < CM_LAST )
      {
         return cm_map[n].guid;
      }
      else
      {
         return pfc::guid_null;
      }
   }

   void get_item_name( unsigned n, pfc::string_base & out )
   {
      if ( n < CM_LAST )
      {
         out.set_string( cm_map[n].label );
      }      
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {
      if ( n < CM_LAST )
      {
         out.set_string( cm_map[n].global_path );
      }
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {
      if ( n < CM_LAST )
      {
         out.set_string( cm_map[n].desc );
         return true;
      }      
      return false;
   }

   virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      switch ( p_index )
      {        
      case CM_PLAY_LIBRARY:
      case CM_SEND_LIBRARY:
      case CM_SEND_LIBRARY_SELECTED:
         {
            static_api_ptr_t<playlist_manager> m;
            bit_array_false f;            

            t_size pl_index = infinite;

            static pfc::string8_fastalloc playlist_name;

            if ( has_tags( cfg_library_playlist ) )
            {
               if ( g_context_node )
               {
                  pfc::string8 tmp;
                  g_context_node->format_title( tmp, cfg_library_playlist );     

                  if ( g_context_node->is_leaf() )
                  {
                     playlist_name.set_string( tmp );
                  }
                  else
                  {
                     folder_node * fn = (folder_node *) g_context_node;
                     fn->process_local_tags( playlist_name, tmp );
                  }
               }
               else
               {
                  playlist_name.set_string( DEFAULT_LIBRARY_PLAYLIST );
               }
            }
            else
            {
               playlist_name.set_string( cfg_library_playlist );
            }

            int n = infinite;

            if ( cfg_remove_last_playlist )
            {
               n = m->find_playlist( cfg_last_library, ~0 );                              
               m->playlist_rename( n, playlist_name, ~0 );
            }

            if ( n == infinite )
            {
               n = m->find_or_create_playlist( playlist_name, ~0 );
            }

            m->playlist_undo_backup( n );
            m->playlist_clear( n );
            m->playlist_add_items( n, p_data, f );

            cfg_last_library = playlist_name;

            if ( cfg_activate_library_playlist )
            {
               m->set_active_playlist( n );
            }

            if ( p_index == CM_SEND_LIBRARY_SELECTED )
            {
               bit_array_true t;
               m->playlist_set_selection( n, t, t );
               console::printf( "Sending and Selection" );
            }

            if ( p_index == CM_PLAY_LIBRARY )
            {
               console::print( "Playing" );
               bit_array_true t;
               m->queue_remove_mask( t );
               m->set_playing_playlist( n );               
               static_api_ptr_t<playback_control> pc;              
               pc->start();
            }
         }
         break;

      case CM_NEW:
         {
            static_api_ptr_t<playlist_manager> m;
            bit_array_false f;

            int n;
            if ( g_context_node )
            {
               pfc::string8 title;
               g_context_node->get_display_name( title );
               n = m->create_playlist( title, ~0, ~0 );
            }
            else
            {
               n = m->create_playlist( "New", ~0, ~0 );
            }

            if ( n != ~0 )
            {               
               m->playlist_add_items( n, p_data, f );           
               m->set_active_playlist( n );
            }
         }
         break;

      case CM_SEND_ACTIVE:
         {
            static_api_ptr_t<playlist_manager> m;
            bit_array_false f;
            m->activeplaylist_undo_backup();
            m->activeplaylist_clear();
            m->activeplaylist_add_items( p_data, f );           
         }
         break;

      case CM_ADD_ACTIVE:
         {
            static_api_ptr_t<playlist_manager> m;
            bit_array_false f;
            m->activeplaylist_undo_backup();
            m->activeplaylist_add_items( p_data, f );           
         }
         break;
      }
   }
};

static service_factory_single_t<tree_context_items> g_context;

#include "mainmenu.h"

bool run_context_command( const char * cmd, pfc::list_base_const_t<metadb_handle_ptr> * handles, node * n,
                         const GUID & guid_command, const GUID & guid_subcommand )
{
   // console::printf( "run_context_command: %s", cmd );
   g_context_node = n;

   const char * local_tag = "[local]";
   bool result = false;
   if ( strcmp( cmd, "" ) != 0 ) 
   {
      if ( strstr( cmd, local_tag ) )
      {
         for ( int i = 0; i < tabsize(mm_map); i++ )
         {
            if ( ( strlen( mm_map[i].local_path ) > 0 ) &&               
               strstr( cmd, mm_map[i].local_path ) )
            {
               // console::printf( "calling mm_command_process on %s", mm_map[i].desc );
               mm_command_process( mm_map[i].id, n );
               result = true;
            }
         }
         if ( !result )
         {
            // console::printf ( "context_command not found" );
         }
      }
      else
      {
         if ( guid_command == pfc::guid_null && guid_subcommand == pfc::guid_null ) 
         {
            GUID guid;

            if ( menu_helpers::find_command_by_name( cmd, guid ) )
            {
               if ( handles )
               {
                  result = menu_helpers::run_command_context( guid, pfc::guid_null, *handles );
               }
               else if ( n )
               {
                  metadb_handle_list lst;
                  n->get_entries( lst );
                  result = menu_helpers::run_command_context( guid, pfc::guid_null, lst );
               }
            }
         }
         else
         {
            if ( handles )
            {
               result = menu_helpers::run_command_context( guid_command, guid_subcommand, *handles );
            }
            else if ( n )
            {
               metadb_handle_list lst;
               n->get_entries( lst );
               result = menu_helpers::run_command_context( guid_command, guid_subcommand, lst );
            }
         }
      }
   }

   g_context_node = false;
   return result;
}


HMENU generate_node_menu( node *in_ptr, int & ID )
{
	HMENU returnme = CreatePopupMenu();
   in_ptr->m_id = (int)returnme;

   //console::printf( "(%s).m_id = %d", (const char *) in_ptr->m_label, in_ptr->m_id );

   if ( !returnme ) 
   {
      console::warning( "CreatePopupMenu failed in genereate_node_menu" );
      return NULL;
   }

   pfc::string8 title;

   folder_node * ptr = NULL;

   if ( !in_ptr->is_leaf() )
   {
      ptr = (folder_node*) in_ptr;
   }

   if ( ptr )
   {
	   for ( int i = 0; i < ptr->get_num_children(); i++ )
	   {
		   node *child = ptr->m_children[i];
   		
         //child->get_display_name( title );
         title = child->m_label;
         
		   if ( child->is_leaf() )
		   {
            if ( child->m_id >= 0 )
            {
		         if ( child->m_id == MENU_SEPARATOR )
		         {
			         uAppendMenu( returnme, MF_SEPARATOR, 0, "");
		         }
               else
               {
                  uAppendMenu( returnme, MF_STRING, child->m_id, title );
               }
            }
            else
            {
               child->m_id = ID;
		         uAppendMenu( returnme, MF_STRING, ID, title );
               ID ++;
            }
		   }
		   else
		   {		
			   HMENU child_menu = generate_node_menu( child, ID );

            if ( child_menu )
            {
			      uAppendMenu( returnme, MF_POPUP, (long)child_menu, title );
            }
            else
            {
               return returnme;
            }
		   }
	   }
   }
	return returnme;
}

HMENU generate_node_menu_complex( node *ptr, int & ID, bool prependAll = false, pfc::ptr_list_t<pfc::string8> *actions = NULL )
{
	HMENU returnme = CreatePopupMenu();

   ptr->m_id = (int)returnme;

   //console::printf( "(%s).m_id = %d", (const char *) in_ptr->m_label, in_ptr->m_id );

   pfc::string8 title;

   if ( !ptr->is_leaf() )
   {
      folder_node * fn = (folder_node*)ptr;

      if ( prependAll && fn->get_num_children() > 1 )
      {
         ptr->m_id = ID;

         if ( actions->get_count() == 1 )
         {
            uAppendMenu( returnme, MF_STRING, ID, "All" );
            ID++;
         }      
         else
         {      
            HMENU tmp = CreatePopupMenu();

            for ( int n = 0; n < actions->get_count(); n ++ )
            {
               uAppendMenu( tmp, MF_STRING, ID, actions->get_item( n )->get_ptr() );
               ID++;
            }
            uAppendMenu( returnme, MF_POPUP, (long)tmp, "All" );
         }
        
         uAppendMenu( returnme, MF_SEPARATOR, 0, "" );
      }


	   for ( int i = 0; i < fn->get_num_children(); i++ )
	   {
		   node *child = fn->m_children[i];
   		
         title.set_string( child->m_label );
		   //child->get_display_name( title );
         
         
		   if ( child->is_leaf() )
		   {
			   if ( child->m_id == MENU_SEPARATOR )
			   {
				   uAppendMenu( returnme, MF_SEPARATOR, 0, "");
			   }
			   else
			   {
               if ( child->m_id >= 0 )
               {
                  uAppendMenu( returnme, MF_STRING, child->m_id, title );
               }
               else
               {
                  child->m_id = ID;

                  if ( actions->get_count() != 1 )
                  {
                     HMENU tmp = CreatePopupMenu();

                     for ( int n = 0; n < actions->get_count(); n ++ )
                     {
                        uAppendMenu( tmp, MF_STRING, ID, actions->get_item( n )->get_ptr() );
                        ID++;
                     }
                     uAppendMenu( returnme, MF_POPUP, (long)tmp, title );
                  }
                  else
                  {
				         uAppendMenu( returnme, MF_STRING, ID, title );
                     ID ++;
                  }
               }
			   }
		   }
		   else
		   {		
			   HMENU child_menu = generate_node_menu_complex( child, ID, prependAll, actions );
			   uAppendMenu( returnme, MF_POPUP, (long)child_menu, title );
		   }
	   }
   }

	return returnme;
}

HMENU node::generate_context_menu()
 {
   int i;

   node * n = new folder_node( "Context" );

   for ( i = 0; i < tabsize(mm_map); i++ )
   {
      if (( is_query() && mm_map[i].type & TYPE_QUERY ) ||          
         ( is_folder() && mm_map[i].type & TYPE_FOLDER ) ||
         ( is_leaf() && mm_map[i].type & TYPE_LEAF ) )
      {
         query_ready_string strings( mm_map[i].local_path );
         
         node * ch = n->add_delimited_child( strings, NULL );
         if ( ch )
         {
            ch->m_id = mm_map[i].id;
         }
      }
   }

   int tmp;
   HMENU m = generate_node_menu( n, tmp );
   delete n;
   return m;
}

void node::handle_context_menu_command( int n )
{
   mm_command_process( n, this );
}


#include "preferences.h"

static const GUID library_service_guid = { 0x19932591, 0x62ef, 0x4031, { 0x92, 0xf1, 0xc7, 0x79, 0xe3, 0xcc, 0x94, 0x87 } };

class tree_library_service : public library_viewer
{
   GUID get_preferences_page() { return guid_preferences; }
   GUID get_guid() { return library_service_guid; }
   bool have_activate() { return false; }
   void activate() { }
   const char *get_name() { return "Playlist Tree Panel"; }
};

static service_factory_single_t<tree_library_service> g_library_service;

class repopulate_callback : public main_thread_callback
{
   int      m_reason;
   WPARAM   m_wp;
   LPARAM   m_lp;

public:
   repopulate_callback( int reason, WPARAM wp, LPARAM lp ) : main_thread_callback()
   {
      m_reason = reason;
      m_wp     = wp;
      m_lp     = lp;
   }

   void callback_run()
   {
      for ( int n = 0; n < g_active_trees.get_count(); n++ )
      {
         g_active_trees[n]->m_root->repopulate_auto( m_reason, m_wp, m_lp );
      }
   }
};

void static post_repopulate( int reason, WPARAM wp = 0, LPARAM lp = 0 )
{
   if ( core_api::are_services_available() && !core_api::is_initializing() && !core_api::is_shutting_down() )
   {
      service_ptr_t<repopulate_callback> p_callback = 
         new service_impl_t<repopulate_callback>( reason, wp, lp );
      static_api_ptr_t<main_thread_callback_manager> man;
      man->add_callback( p_callback );
   }
}

#include "tree_search.h"

class tree_play_callback : public play_callback_static
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
      post_repopulate( repop_new_track );      
   }

	//! Playback stopped
   virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason){}
	//! User has seeked to specific time.
	virtual void FB2KAPI on_playback_seek(double p_time){}
	//! Called on pause/unpause.
	virtual void FB2KAPI on_playback_pause(bool p_state){}
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

static service_factory_single_t<tree_play_callback> g_tree_play_callback;

class queue_callback : public playback_queue_callback 
{
   virtual void on_changed(t_change_origin p_origin)
   {
      post_repopulate( repop_queue_change );
   }
};

static service_factory_single_t<queue_callback> g_queue_callback;

class playlist_tree_playlist_callback : public playlist_callback_static
{
public:
   playlist_tree_playlist_callback() 
   {
   }

   ~playlist_tree_playlist_callback()
   {
   }

   void update_all_playlists_queries()
   {
      post_repopulate( repop_playlists_change );
   }

   void update_all_playlist_watchers( t_size p_playlist )
   {
      post_repopulate( repop_playlist_change, p_playlist );
   }

   virtual unsigned get_flags()
   {
      return 
         flag_on_items_added |
		   flag_on_items_reordered	|
		   flag_on_items_removed |	
		   flag_on_items_replaced |
         flag_on_playlists_removed |
         flag_on_playlist_created |
         flag_on_playlist_renamed;
   }

	virtual void FB2KAPI on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection)      
   {
      update_all_playlist_watchers( p_playlist );
   }
	
   virtual void FB2KAPI on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count)
   {
      update_all_playlist_watchers( p_playlist );
   }

	virtual void FB2KAPI on_items_removing(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
   {
   }

	virtual void FB2KAPI on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
   {
      update_all_playlist_watchers( p_playlist );
   }

	virtual void FB2KAPI on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state)
   {
   }

	virtual void FB2KAPI on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to)
   {
   }
	
	virtual void FB2KAPI on_items_modified(t_size p_playlist,const bit_array & p_mask)
   {
   }

	virtual void FB2KAPI on_items_modified_fromplayback(t_size p_playlist,const bit_array & p_mask,play_control::t_display_level p_level)
   {
   }

	virtual void FB2KAPI on_items_replaced(t_size p_playlist,const bit_array & p_mask,const pfc::list_base_const_t<t_on_items_replaced_entry> & p_data)
   {
      update_all_playlist_watchers( p_playlist );
   }

	virtual void FB2KAPI on_item_ensure_visible(t_size p_playlist,t_size p_idx)
   {
   }

	virtual void FB2KAPI on_playlist_activate(t_size p_old,t_size p_new)
   {
   }

	virtual void FB2KAPI on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len)
   {
      update_all_playlists_queries();
   }

	virtual void FB2KAPI on_playlists_reorder(const t_size * p_order,t_size p_count)
   {
   }

	virtual void FB2KAPI on_playlists_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
   {
   }

	virtual void FB2KAPI on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count)
   {
      update_all_playlists_queries();
   }

	virtual void FB2KAPI on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len)
   {
      update_all_playlists_queries();
   }

	virtual void FB2KAPI on_default_format_changed() 
   {
   }

	virtual void FB2KAPI on_playback_order_changed(t_size p_new_index)
   {
   }

	virtual void FB2KAPI on_playlist_locked(t_size p_playlist,bool p_locked)
   {
   }
};

static service_factory_single_t<playlist_tree_playlist_callback> g_playlist_callback;

class tree_file_callback : public file_operation_callback
{
public:
   tree_file_callback()
   {
   }

   ~tree_file_callback()
   {
   }

	//! p_items is a metadb::path_compare sorted list of files that have been deleted.
	virtual void on_files_deleted_sorted(const pfc::list_base_const_t<const char *> & p_items) 
   {
      /*
      console::printf( "Files Deleted:" );
      for ( int n = 0; n < p_items.get_count(); n++ )
      {
         console::printf( "   %s", p_items[n] );
      }
      */
      for ( int i = 0; i < g_active_trees.get_count(); i++ )
      {
         g_active_trees[i]->m_root->files_removed( p_items );
      }
   }

	//! p_from is a metadb::path_compare sorted list of files that have been moved, p_to is a list of corresponding target locations
	virtual void on_files_moved_sorted(const pfc::list_base_const_t<const char *> & p_from,const pfc::list_base_const_t<const char *> & p_to)
   {
      /*
      console::printf( "Files Moved:" );
      for ( int n = 0; n < p_from.get_count(); n++ )
      {
         console::printf( "   %s", p_from[n] );
      }
      */
      for ( int i = 0; i < g_active_trees.get_count(); i++ )
      {
         g_active_trees[i]->m_root->files_moved( p_from, p_to );
      }
   }

	//! p_from is a metadb::path_compare sorted list of files that have been copied, p_to is a list of corresponding target locations.
	virtual void on_files_copied_sorted(const pfc::list_base_const_t<const char *> & p_from,const pfc::list_base_const_t<const char *> & p_to) 
   {
   }
};

service_factory_single_t<tree_file_callback> g_file_callback;

void remove_from_queue( const pfc::list_t<metadb_handle_ptr> & handles )
{
   pfc::list_t<t_playback_queue_item> queue;

   static_api_ptr_t<playlist_manager> pm;
   pm->queue_get_contents( queue );
   bit_array_bittable bits( queue.get_count() );

   for ( int i = 0; i < queue.get_count(); i++ )
   {
      bits.set( i, false );
   }

   for ( int n = 0; n < handles.get_count(); n++ )
   {
      for ( int i = 0; i < queue.get_count(); i++ )
      {
         if ( handles[n] == queue[i].m_handle )
         {
            // console::printf( "Setting bit on %s", handles[n]->get_path() );
            bits.set( i, true );
         }
      }
   }

   pm->queue_remove_mask( bits );
}

void remove_from_playlist( t_size playlist, pfc::list_t<metadb_handle_ptr> handles )
{
   metadb_handle_list queue;

   bit_array_true t;
   static_api_ptr_t<playlist_manager> pm;
   pm->playlist_get_items( playlist, queue, t );
   bit_array_bittable bits( queue.get_count() );

   for ( int i = 0; i < queue.get_count(); i++ )
   {
      bits.set( i, false );
   }

   for ( int n = 0; n < handles.get_count(); n++ )
   {
      for ( int i = 0; i < queue.get_count(); i++ )
      {
         if ( handles[n] == queue[i] )
         {
            // console::printf( "Setting bit on %s", handles[n]->get_path() );
            bits.set( i, true );
         }
      }
   }

   pm->playlist_remove_items( playlist, bits );

}

void on_node_removing( node * ptr )
{
   g_refresh_lock = true;

   node * p = ptr->m_parent;

   while ( p )
   {
      if ( p->is_query() )
      {
         query_node * qn = (query_node *) p;
         
         if ( strstr( qn->m_source, TAG_QUEUE ) )
         {
            metadb_handle_list handles;
            ptr->get_entries( handles );
            remove_from_queue( handles );               
            g_nodes_to_update.add_item( qn );
            break;
         }
         if ( strstr( qn->m_source, TAG_PLAYLIST ) )
         {
            pfc::string8 playlist;
            if ( string_get_function_param( playlist, qn->m_source, TAG_PLAYLIST  ) )
            {              
               static_api_ptr_t<playlist_manager> pm;
               t_size np = pm->find_playlist( playlist, ~0 );
               if ( np != infinite )
               {
                  metadb_handle_list handles;
                  ptr->get_entries( handles );
                  remove_from_playlist( np, handles );
               }
               g_nodes_to_update.add_item( qn );
            }
         }
      }

      p = p->m_parent;
   }

   g_refresh_lock = false;
}


/*
class sample_callback : public titleformatting_variable_callback
{
public:
   void on_var_change( const char * var )
   {
      console::printf( "playlist_tree::sample_callback: %s", var );
   }
};

service_factory_single_t<sample_callback> g_sample_callback;
*/

node::~node()
{  
   remove_tree();
   m_tree_item = NULL;
   m_hwnd_tree = NULL;

   if ( m_parent )
   {
      m_parent->m_children.remove_item( this );
   }
}

__int64 g_treenode_size;
double g_treenode_duration;
pfc::string8 g_treenode_displayname;

void cache_treenode_variables( node * n )
{
   g_treenode_size         = n->get_file_size();
   g_treenode_duration     = n->get_play_length();
   n->get_display_name( g_treenode_displayname );
}

void node::select()
{     
   // g_node_hook.get_static_instance().set_node( this );
   cache_treenode_variables( this );
   post_callback_notification( this );
   post_variable_callback_notification( "treenode" );
   static_api_ptr_t<metadb> db;
   db->database_lock();
   metadb_handle_list lst;
   get_entries( lst );
   db->database_unlock();

   if ( lst.get_count() > 0 )
   {
      bool overLimit = ( lst.get_count() > cfg_selection_action_limit );
      if ( cfg_selection_action_limit == 0 || !overLimit )
      {
         run_context_command( cfg_selection_action_1, &lst, this, cfg_selection_action1_guid_command, cfg_selection_action1_guid_subcommand );
         run_context_command( cfg_selection_action_2, &lst, this, cfg_selection_action2_guid_command, cfg_selection_action2_guid_subcommand );
      }
      else if ( overLimit )
      {
         console::printf( "%s exceeds selection action limit",
            (const char *) m_label );
      }
   }
}

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

bool node_titleformatting_hook::process_function(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_fn_name,t_size p_fn_name_length,titleformat_hook_function_params * p_params,bool & p_found_flag)
{
   if ( stricmp_utf8_ex( p_fn_name, p_fn_name_length, "treenode", 8 ) == 0 )
   {
      if ( !g_last_active_tree )
      {
         return return_processed_false( p_found_flag );
      }

      node * m_node = g_last_active_tree->get_selection();

      if ( !m_node )
      {
         return return_processed_false( p_found_flag );
      }

      if ( p_params->get_param_count() < 1 )
      {
         return return_processed_false( p_found_flag );
      }

      const char * p_name;
      t_size p_name_length;

      p_params->get_param( 0, p_name, p_name_length );

      if ( stricmp_utf8_ex( p_name, p_name_length, "name", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, m_node->m_label, m_node->m_label.get_length() );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "displayname", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, g_treenode_displayname, ~0 );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "size", ~0 ) == 0 )
      {
         pfc::format_uint str( g_treenode_size, 3 );
         p_out->write( titleformat_inputtypes::meta, str, ~0 ); 
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "duration", ~0 ) == 0 )
      {
         double length = g_treenode_duration;
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", (int) length), ~0 );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "itemcount", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_entry_count()), ~0 );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "foldercount", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_folder_count()), ~0 );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "folderindex", ~0 ) == 0 )
      {
         if ( m_node->m_parent )
         {
            p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d",            
               m_node->m_parent->m_children.find_item( m_node ) + 1, ~0 )) ;

            return return_processed_true( p_found_flag );
         }           
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "overallindex", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_folder_overall_index()), ~0 );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "nestlevel", ~0 ) == 0 )
      {
         p_out->write( titleformat_inputtypes::meta, pfc::string_printf("%d", m_node->get_nest_level()), ~0 );
         return return_processed_true( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "isleaf", ~0 ) == 0 )
      {
         if ( m_node->is_leaf() )
         {
            p_out->write( titleformat_inputtypes::meta, "1", ~0 );
            return return_processed_true( p_found_flag );
         }
         return return_processed_false( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "isfolder", ~0 ) == 0 )
      {
         if ( m_node->is_folder() )
         {
            p_out->write( titleformat_inputtypes::meta, "1", ~0 );
            return return_processed_true( p_found_flag );
         }
         return return_processed_false( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "isquery", ~0 ) == 0 )
      {
         if ( m_node->is_query() )
         {
            p_out->write( titleformat_inputtypes::meta, "1", ~0 );
            return return_processed_true( p_found_flag );
         }
         return return_processed_false( p_found_flag );
      }
      else if ( stricmp_utf8_ex( p_name, p_name_length, "isactive", ~0 ) == 0 )
      {
         HWND hwnd = GetFocus();
         bool active = ( hwnd == g_last_active_tree->get_wnd() ) || ( hwnd == g_last_active_tree->get_tree() );

         if ( !IsWindowVisible( g_last_active_tree->get_wnd() ) )
            active = false;

         if ( active )
         {
            p_out->write( titleformat_inputtypes::meta, "1", ~0 );
            return return_processed_true( p_found_flag );
         }
         return return_processed_false( p_found_flag );
      }

      return false;
   }

   return false;
}

bool node_titleformatting_hook::process_field(metadb_handle * p_handle,titleformat_text_out * p_out,const char * p_name,t_size p_name_length,bool & p_found_flag)
{
   return false;
}
