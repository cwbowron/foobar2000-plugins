// PREFERENCES /////////////////////////////////////
#include "../common/menu_stuff.h"

static const GUID guid_preferences = { 0x4e35cce5, 0xbbcf, 0x49cc, { 0xbc, 0x8f, 0x20, 0x95, 0x1a, 0xd6, 0xac, 0xbc } };

bool_map bool_var_map[] = 
{
   { IDC_SHOW_ARROWS, &cfg_show_dock_arrows },
   { IDC_MIN_ON_MAIN_MIN, &cfg_minimize_on_main_minimize },
   { IDC_USE_EXPANSION_TAGZ, &cfg_use_expansion_tagz },
   { IDC_USE_TRANSPARENCY, &cfg_use_transparency },
};

int_map int_var_map[] = 
{
   { IDC_EDIT_OPACITY_ACTIVE, &cfg_opacity_active },
   { IDC_EDIT_OPACITY_INACTIVE, &cfg_opacity_inactive },
   { IDC_EDIT_SNAPPING_DISTANCE, &cfg_snapping_distance },
};

class dockable_panel_preferences : public preferences_page
{  
   static bool m_initialized;

   static void enable_shit( HWND wnd )
   {
      if ( cfg_use_transparency )
      {
         EnableWindow( GetDlgItem( wnd, IDC_EDIT_OPACITY_ACTIVE ), TRUE );
         EnableWindow( GetDlgItem( wnd, IDC_EDIT_OPACITY_INACTIVE ), TRUE );
         EnableWindow( GetDlgItem( wnd, IDC_SPIN_OPACITY_ACTIVE ), TRUE );
         EnableWindow( GetDlgItem( wnd, IDC_SPIN_OPACITY_INACTIVE ), TRUE );
      }
      else
      {
         EnableWindow( GetDlgItem( wnd, IDC_EDIT_OPACITY_ACTIVE ), FALSE );
         EnableWindow( GetDlgItem( wnd, IDC_EDIT_OPACITY_INACTIVE ), FALSE );
         EnableWindow( GetDlgItem( wnd, IDC_SPIN_OPACITY_ACTIVE ), FALSE );
         EnableWindow( GetDlgItem( wnd, IDC_SPIN_OPACITY_INACTIVE ), FALSE );
      }
   }

   static void set_spinner_status( HWND hwnd, int min, int max, int pos )
   {
      SendMessage( hwnd, UDM_SETRANGE, 0, MAKELONG(max,min) );
      SendMessage( hwnd, UDM_SETPOS, 0, MAKELONG(pos,0) );
   }

   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            m_initialized = true;

            int opacity_active =    cfg_opacity_active;
            int opacity_inactive =  cfg_opacity_inactive;
            
            set_spinner_status( GetDlgItem( wnd, IDC_SPIN_OPACITY_ACTIVE ), 0, 255, opacity_active );
            set_spinner_status( GetDlgItem( wnd, IDC_SPIN_OPACITY_INACTIVE ), 0, 255, opacity_inactive );
            set_spinner_status( GetDlgItem( wnd, IDC_SPIN_SNAPPING_DISTANCE), 0, 10, cfg_snapping_distance );

            setup_bool_maps( wnd, bool_var_map, tabsize(bool_var_map) );       
            setup_int_maps( wnd, int_var_map, tabsize(int_var_map) );

            enable_shit( wnd );
         }
         break;

      case WM_COMMAND:
         {
            if ( m_initialized )
            {
               bool b1 = process_bool_map( wnd, wp, bool_var_map, tabsize(bool_var_map) );         
               bool b2 = process_int_maps( wnd, wp, int_var_map, tabsize(int_var_map) );

               if ( b1 || b2 )
               {
                  for ( int n = 0; n < g_host_windows.get_count(); n++ )
                  {
                     g_host_windows[n]->update_title();
                     g_host_windows[n]->update_style();
                  }

                  enable_shit( wnd );
               }
            }
         }
         break;
      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      m_initialized = false;
      return uCreateDialog( IDD_PREFERENCES, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "Dockable Panels";
   }

   virtual GUID get_guid()
   {
      return guid_preferences;
   }

   virtual GUID get_parent_guid()
   {
      return preferences_page::guid_display;
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
bool dockable_panel_preferences::m_initialized = false;

static service_factory_single_t<dockable_panel_preferences> g_pref;
