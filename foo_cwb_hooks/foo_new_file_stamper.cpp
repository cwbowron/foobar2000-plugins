#pragma warning(disable:4996)
#pragma warning(disable:4312)
#pragma warning(disable:4244)

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "resource.h"
#include "../common/string_functions.h"
#include "../common/menu_stuff.h"

#define URL_HELP                 "http://wiki.bowron.us/index.php/Foobar2000"

static const GUID cfg_stamper_000 = { 0xee6605c1, 0x3838, 0x48d4, { 0x9d, 0x9b, 0xcb, 0x29, 0x35, 0xd, 0x2a, 0x74 } };
static const GUID cfg_stamper_001 = { 0xde6a9099, 0x9c8c, 0x45e9, { 0x9f, 0x35, 0xb6, 0x6b, 0x88, 0x71, 0xa5, 0x4 } };
static const GUID cfg_stamper_002 = { 0x159d9072, 0xf3fc, 0x4e6a, { 0x9d, 0x7e, 0x75, 0x22, 0x26, 0xb1, 0x70, 0xc5 } };
static const GUID cfg_stamper_003 = { 0x12087db, 0x514b, 0x4062, { 0x98, 0x11, 0xf4, 0xeb, 0xd4, 0xac, 0xac, 0x65 } };

static cfg_int cfg_stamp( cfg_stamper_003, 0 );
static cfg_string cfg_stamp_action( cfg_stamper_000, "" );
static cfg_guid cfg_stamp_command( cfg_stamper_001, pfc::guid_null );
static cfg_guid cfg_stamp_subcommand( cfg_stamper_002, pfc::guid_null );

static const GUID guid_stamper_preferences = { 0xdcae7227, 0x2fb3, 0x44a1, { 0xb0, 0x94, 0x41, 0x6a, 0x1a, 0x98, 0xda, 0xf0 } };

bool_map bool_stamp_var_map[] = 
{
   { IDC_CHECK_PROCESS_FILES, &cfg_stamp },
};

script_map context_stamp_var_map[] =
{
   { IDC_COMBO_COMMAND, &cfg_stamp_action, &cfg_stamp_command, &cfg_stamp_subcommand },
};

class stamper_preferences : public preferences_page, public cwb_menu_helpers
{  
   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         setup_bool_maps( wnd, bool_stamp_var_map, tabsize(bool_stamp_var_map) );           
         setup_script_maps( wnd, context_stamp_var_map, tabsize(context_stamp_var_map) );

         EnableWindow( GetDlgItem( wnd, IDC_COMBO_COMMAND ), cfg_stamp ? TRUE : FALSE );
         break;

      case WM_COMMAND:   
         process_script_map( wnd, wp, context_stamp_var_map, tabsize(context_stamp_var_map) );

         if ( process_bool_map( wnd, wp, bool_stamp_var_map, tabsize(bool_stamp_var_map) ) )
         {
            EnableWindow( GetDlgItem( wnd, IDC_COMBO_COMMAND ), cfg_stamp ? TRUE : FALSE );
         }
         break;
      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_STAMP_PREFERENCES, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "New File Tagger";
   }

   virtual GUID get_guid()
   {
      return guid_stamper_preferences;
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

static service_factory_single_t<stamper_preferences> g_stamper_pref;


class new_file_stamper : public library_callback
{
   public:
	//! Called when new items are added to the Media Library.
	virtual void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) 
   {
      if ( !core_api::is_initializing() )
      {
         if ( cfg_stamp )
         {
            console::printf( "Processing new files with %s", cfg_stamp_action.get_ptr() );
            menu_helpers::run_command_context( cfg_stamp_command, cfg_stamp_subcommand, p_data );
         }
      }
   }

	//! Called when some items have been removed from the Media Library.
	virtual void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
   {
   }

	//! Called when some items in the Media Library have been modified.
	virtual void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
   {
   }
};

//static service_factory_single_t<new_file_stamper> g_stamper;
library_callback_factory_t<new_file_stamper> g_stamper;