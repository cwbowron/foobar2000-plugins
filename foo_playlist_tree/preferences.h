#pragma once

#include "../common/menu_stuff.h"

void setup_context_item_maps_pt( HWND wnd, context_item_map * map, int m )
{
   for ( int n = 0; n < m; n++ )
   {
      int id = map[n].id;               
      HWND hwnd_control = GetDlgItem( wnd, id );

      SendMessage( hwnd_control, CB_RESETCONTENT, 0,0 );

      // add empty string option...
      uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, "" );            

      pfc::string8_fastalloc tmp;
      for ( int z = 0; z < tabsize(mm_map); z ++ )     
      {
         if ( strcmp( mm_map[z].local_path, "" ) != 0 )
         {
            tmp.set_string( "[local] " );
            tmp.add_string( mm_map[z].local_path );
            uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, tmp );
         }
      }

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
            uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, menuPath );
         }

         uSendDlgItemMessageText( wnd, id, CB_SELECTSTRING, -1, (*map[n].var) );
      }
   }

}

static const GUID guid_preferences = { 0x85d7363c, 0xf8dd, 0x46c7, { 0xbd, 0x4a, 0xbe, 0xf2, 0x43, 0x3, 0xbf, 0x8a } };
static const GUID guid_preferences_mouse = { 0x2977da50, 0x9771, 0x46c8, { 0x83, 0x7e, 0x3e, 0xed, 0x7b, 0xc2, 0xc4, 0x13 } };
static const GUID guid_preferences_scheme = { 0xb4b84026, 0x27d1, 0x42e7, { 0xa7, 0xfd, 0xee, 0xd5, 0x5a, 0xc3, 0x62, 0xe3 } };

text_box_map var_map[] =
{  
   { IDC_EDIT_QUERY_FORMAT, &cfg_query_display },
   { IDC_EDIT_FILE_FORMAT, &cfg_file_format },
   { IDC_EDIT_LIBRARY_PLAYLIST, &cfg_library_playlist },
   { IDC_EDIT_FOLDER_FORMAT, &cfg_folder_display },
   { IDC_EDIT_MORE_BITMAPS, &cfg_bitmaps_more },  
   { IDC_ICON_PATH, &cfg_icon_path },
};    

context_item_map context_menu_map[] = 
{
   //{ IDC_SELECTION_ACTION_1, &cfg_selection_action_1 },
   //{ IDC_SELECTION_ACTION_2, &cfg_selection_action_2 },
   { IDC_DOUBLE_CLICK, &cfg_doubleclick },
   { IDC_RIGHTCLICK_ACTION, &cfg_rightclick },
   { IDC_RIGHTCLICK_ACTION_SHIFT, &cfg_rightclick_shift },
   { IDC_MIDDLECLICK, &cfg_middleclick },
   { IDC_ENTER, &cfg_enter },
   { IDC_SPACE, &cfg_space },
};

int_map int_var_map_mouse[] = 
{
   { IDC_SELECTION_ACTION_LIMIT, &cfg_selection_action_limit },
};

bool_map bool_var_map[] = 
{
   { IDC_ENABLE_BITMAPS, &cfg_bitmaps_enabled },
   { IDC_HIDELEAVES, &cfg_hideleaves },
   { IDC_HIDEROOT, &cfg_hideroot },
   { IDC_NOHSCROLL, &cfg_nohscroll },
   { IDC_HIDELINES, &cfg_hidelines },
   { IDC_HIDEBUTTONS, &cfg_hidebuttons },
   { IDC_LOADSYSTEMBITMAPS, &cfg_bitmaps_loadsystem },
   { IDC_ACTIVATE_LIBRARY_PLAYLIST, &cfg_activate_library_playlist },
   { IDC_REMOVE_LAST_PLAYLIST, &cfg_remove_last_playlist },
   { IDC_ALLOW_RGB, &cfg_custom_colors },
   { IDC_DEFAULT_QUERIES, &cfg_use_default_queries },
};

bool_map bool_var_map_mouse[] = 
{  
   { IDC_REFRESH_QUERIES, &cfg_repopulate_doubleclick },
   { IDC_PROCESS_SHORTCUTS, &cfg_process_keydown },
};

list_map list_var_map[] =
{
   { IDC_EDGE_STYLE, &cfg_edge_style, g_edge_styles }
};

color_map color_var_map[] =
{
   { IDC_BTN_SELECTION_TEXT_FOCUS, &cfg_selection_text },
   { IDC_BTN_SELECTION_TEXT_NONFOCUS, &cfg_selection_text_nofocus },
   { IDC_BTN_SELECTION_FOCUSED, &cfg_color_selection_focused },
   { IDC_BTN_SELECTION_NONFOCUSED, &cfg_color_selection_nonfocused },
   { IDC_BTN_LINE_COLOR, &cfg_line_color },
   { IDC_BTN_TEXT_COLOR, &cfg_text_color },
   { IDC_BTN_BACK_COLOR, &cfg_back_color },
};

script_map script_var_map[] =
{
   { IDC_SELECTION_ACTION_1, &cfg_selection_action_1, &cfg_selection_action1_guid_command, &cfg_selection_action1_guid_subcommand },
   { IDC_SELECTION_ACTION_2, &cfg_selection_action_2, &cfg_selection_action2_guid_command, &cfg_selection_action2_guid_subcommand },
};

text_box_map scheme_edit_map[] =
{  
   { IDC_EDIT_SCHEME_STARTUP, &cfg_scheme_startup },
};

class playlist_tree_preferences : public preferences_page, public cwb_menu_helpers
{  
   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            int n, m;

            setup_text_boxes( wnd, var_map, tabsize(var_map) );
            setup_bool_maps( wnd, bool_var_map, tabsize(bool_var_map) );

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

            
         }
         break;

      case WM_COMMAND:
         process_text_boxes( wnd, wp, var_map, tabsize(var_map) );

         if ( process_bool_map( wnd, wp, bool_var_map, tabsize(bool_var_map) ) )
         {
            playlist_tree_panel::g_update_appearance();
         }
         else if ( process_color_map( wnd, wp, color_var_map, tabsize( color_var_map ) ) )
         {
            playlist_tree_panel::g_update_appearance();
         }

         if ( wp >> 16 == CBN_SELCHANGE )
         {            
            int the_list = wp & 0xffff;
            int n;
            int m = tabsize( list_var_map );

            for ( n = 0; n < m; n++ )
            {
               if ( list_var_map[n].id == the_list )
               {
                  int s = SendDlgItemMessage( wnd, the_list, CB_GETCURSEL, 0, 0 );
                  if ( s != CB_ERR )
                  {
                     ( *list_var_map[n].var ) = s;
                     playlist_tree_panel::g_update_appearance();
                  }
                  break;
               }
            } 
         }

         switch ( wp )
         {
      case BN_CLICKED<<16|IDC_BTN_LEAF:
         {
            if ( g_active_trees.get_count() > 0 )
            {
               int ic = select_icon( wnd, g_active_trees[0]->m_imagelist );

               if ( ic >= 0 )
               {
                  cfg_bitmap_leaf = ic;
               }               
            }
            else
            {
               uMessageBox( wnd, "Playlist Tree must be active to select icons.", "Playlist Tree", MB_OK );
            }         
         }          
         break;

      case BN_CLICKED<<16|IDC_BTN_FOLDER:
         {
            if ( g_active_trees.get_count() > 0 )
            {
               int ic = select_icon( wnd, g_active_trees[0]->m_imagelist );

               if ( ic >= 0 )
               {
                  cfg_bitmap_folder = ic;
               }               
            }
            else
            {
               uMessageBox( wnd, "Playlist Tree must be active to select icons.", "Playlist Tree", MB_OK );
            }         
            break;
         }
               
      case BN_CLICKED<<16|IDC_HIDELEAVES:
      case BN_CLICKED<<16|IDC_HIDEROOT:
         {
            for ( int n = 0; n < g_active_trees.get_count(); n++ )
            {
               g_active_trees[n]->setup_tree();
            }
         }
         break;

      case BN_CLICKED<<16|IDC_BTN_QUERY:
         {
            if ( g_active_trees.get_count() > 0 )
            {
               int ic = select_icon( wnd, g_active_trees[0]->m_imagelist );

               if ( ic >= 0 )
               {
                  cfg_bitmap_query = ic;
               }               
            }
            else
            {
               uMessageBox( wnd, "Playlist Tree must be active to select icons.", "Playlist Tree", MB_OK );
            }         
            break;
         }

         case BN_CLICKED<<16|IDC_BTN_FONT:
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
                  playlist_tree_panel::g_update_appearance();					
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
      return "Playlist Tree Panel";
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

static service_factory_single_t<playlist_tree_preferences> g_pref;

class mouse_preferences : public preferences_page, public cwb_menu_helpers
{  
   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         setup_context_item_maps_pt( wnd, context_menu_map, tabsize(context_menu_map) );
         setup_bool_maps( wnd, bool_var_map_mouse, tabsize(bool_var_map_mouse) );
         setup_int_maps( wnd, int_var_map_mouse, tabsize(int_var_map_mouse) );
         setup_script_maps( wnd, script_var_map, tabsize(script_var_map) );
         break;

      case WM_COMMAND:
         process_context_item_map( wnd, wp, context_menu_map, tabsize(context_menu_map) );
         process_bool_map( wnd, wp, bool_var_map_mouse, tabsize(bool_var_map_mouse) );
         process_int_maps( wnd, wp, int_var_map_mouse, tabsize(int_var_map_mouse) );
         process_script_map( wnd, wp, script_var_map, tabsize(script_var_map) );
         break;
      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_CONFIG_MOUSE, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "Mouse & Keyboard";
   }

   virtual GUID get_guid()
   {
      return guid_preferences_mouse;
   }

   virtual GUID get_parent_guid()
   {
      return guid_preferences;
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

static service_factory_single_t<mouse_preferences> g_pref_mouse;


class scheme_preferences : public preferences_page, public cwb_menu_helpers
{  
   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         setup_text_boxes( wnd, scheme_edit_map, tabsize(scheme_edit_map) );
         break;

      case WM_COMMAND:
         process_text_boxes( wnd, wp, scheme_edit_map, tabsize(scheme_edit_map) );
        
         switch ( wp )
         {
         case BN_CLICKED<<16|IDC_BUTTON_SCHEME_EVAL_NOW:
            if ( !g_scheme_installed )
            {
               install_scheme_globals();
            }
            else
            {
               scm_eval_startup();
            }
            break;
         }
         break;

      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_CONFIG_SCHEME, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "Scheme";
   }

   virtual GUID get_guid()
   {
      return guid_preferences_scheme;
   }

   virtual GUID get_parent_guid()
   {
      return guid_preferences;
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

static service_factory_single_t<scheme_preferences> g_pref_scheme;
