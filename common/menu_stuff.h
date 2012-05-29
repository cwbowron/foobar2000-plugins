#pragma once

typedef struct { int id; cfg_string *var; } text_box_map;
typedef struct { int id; cfg_string *var; } context_item_map; 
typedef struct { int id; cfg_int *var; } bool_map;
typedef struct { int id; cfg_int *var; char **labels; } list_map;
typedef struct { int id; cfg_int *var; } int_map;
typedef struct { int id; cfg_int *var; } color_map;

typedef struct { int id; cfg_string *var; cfg_guid * command_guid; cfg_guid * subcommand_guid; } script_map; 

typedef struct { int id; pfc::string8 *var; GUID * command_guid; GUID * subcommand_guid; } script_map_local; 

static pfc::list_t<pfc::string8> g_context_list;
static pfc::list_t<GUID> g_command_guids;
static pfc::list_t<GUID> g_subcommand_guids;

class cwb_menu_helpers
{
public:
   static bool pick_color( HWND wnd, cfg_int & out, COLORREF custom = 0 )
   {
      bool rv = false;
      COLORREF COLOR = out;
      COLORREF COLORS[16] = {custom,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

      if ( uChooseColor( &COLOR, wnd, &COLORS[0] ) )
      {
         out = COLOR;
         rv = true;
      }
      return rv;
   }

   static void setup_text_boxes( HWND wnd, text_box_map * map, int m )
   {
      // Setup ALL Text Boxes
      for ( int n = 0; n < m; n++ )
      {
         uSetDlgItemText( wnd, map[n].id, map[n].var->get_ptr() );
      }
   }

   static void setup_int_maps( HWND wnd, int_map * map, int m )
   {
      for ( int n = 0; n < m; n++ )
      {
         uSetDlgItemText( wnd, map[n].id, pfc::string_printf("%d", map[n].var->get_value() ) );
      }
   }

   static void setup_color_maps( HWND hwnd, color_map * map, int m )
   {
   }

   static void get_context_list( contextmenu_item_node * parent, pfc::list_t<pfc::string8> & strings, 
      const metadb_handle_list & handles, const char * path, const GUID & command_guid )
   {
      if ( parent )
      {
         switch ( parent->get_type() )
         {
         case contextmenu_item_node::TYPE_COMMAND:
            {
               unsigned flags;         
               pfc::string8 name;
               parent->get_display_data( name, flags, handles, contextmenu_item::caller_undefined );

               // console::printf( "COMMAND: %s [%d]", name.get_ptr(), flags );

               if ( flags && contextmenu_item_node::FLAG_DISABLED )
               {
                  // console::printf( "DISABLED" );
               }
               else if ( flags && contextmenu_item_node::FLAG_GRAYED )
               {
                  // console::printf( "GRAYED" );
               }
               else
               {
                  pfc::string8 full_path = path;
                  if ( full_path.is_empty() == false )
                  {
                     full_path.add_string( "/" );
                  }
                  full_path.add_string( name );
                  strings.add_item( full_path );

                  g_command_guids.add_item( command_guid );
                  g_subcommand_guids.add_item( parent->get_guid() );

                  pfc::print_guid g1( command_guid );
                  pfc::print_guid g2( parent->get_guid() );

                  // console::printf( "%s [%s - %s]", full_path.get_ptr(), g1.get_ptr(), g2.get_ptr() );
               }
            }
            break;

         case contextmenu_item_node::TYPE_POPUP:
            {
               t_size child_idx;
               t_size child_num = parent->get_children_count();

               unsigned flags;         
               pfc::string8 name;
               parent->get_display_data( name, flags, handles, contextmenu_item::caller_undefined );

               if ( flags < 2 )
               {
                  pfc::string8 full_path = path;
                  if ( full_path.is_empty() == false )
                     full_path.add_string( "/" );
                  full_path.add_string( name );

                  for( child_idx = 0; child_idx < child_num; child_idx++ )
                  {
                     contextmenu_item_node * child = parent->get_child(child_idx);
                     if (child)
                     {
                        get_context_list( child, strings, handles, full_path, command_guid );
                     }
                  }
               }
            }
            break;
         }
      }
   }


   static void get_context_list_w00t( contextmenu_item * p, pfc::list_t<pfc::string8> & items, const metadb_handle_list & handles )
   {
      for ( int i = 0; i < p->get_num_items(); i++ )
      {
         pfc::string8 path;
         p->get_item_default_path( i, path );
         // console::printf( "Path: %s", path.get_ptr() );
         contextmenu_item_node_root * r = p->instantiate_item( i, handles, contextmenu_item::caller_undefined );

         if ( r )
         {
            if ( r->get_type() == contextmenu_item_node::TYPE_COMMAND )
            {
               pfc::string8_fastalloc menuPath;
               pfc::string8_fastalloc menuName;

               p->get_item_default_path( i, menuPath );
               p->get_item_name( i, menuName );

               if ( !menuPath.is_empty() )
               {
                  menuPath.add_string( "/" );
               }
               menuPath.add_string( menuName );

               t_size itm_index = items.add_item( menuPath );
               t_size cmd_index = g_command_guids.add_item( p->get_item_guid( i ) );
               t_size sub_index = g_subcommand_guids.add_item( pfc::guid_null );            
            }
            else 
            {
               GUID command_guid = p->get_item_guid( i );
               get_context_list( r, items, handles, path, command_guid );
            }
         }
      }
   }


   static void setup_script_maps( HWND wnd, script_map * map, int m )
   {
      g_context_list.remove_all();
      g_command_guids.remove_all();
      g_subcommand_guids.remove_all();

      service_enum_t<contextmenu_item> menus;
      service_ptr_t<contextmenu_item> item;

      metadb_handle_list handles;
      metadb_handle_ptr h;
      static_api_ptr_t<metadb> db;

      if ( false == metadb::g_get_random_handle( h ) )
      {        
         db->handle_create( h, playable_location_impl( "silence://1", 0 ) );
      }

      handles.add_item( h );

      while ( menus.next( item ) )
      {
         get_context_list_w00t( item.get_ptr(), g_context_list, handles );
      }

      for ( int n = 0; n < m; n++ )
      {
         int id = map[n].id;               
         HWND hwnd_control = GetDlgItem( wnd, id );

         SendMessage( hwnd_control, CB_RESETCONTENT, 0, 0 );

         // add empty string option...
         uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, "" );            

         for ( int i = 0; i < g_context_list.get_count(); i++ )
         {
            int nPos = uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, g_context_list.get_item( i ) );

            SendDlgItemMessage( wnd, id, CB_SETITEMDATA, nPos, i );
         }

         uSendDlgItemMessageText( wnd, id, CB_SELECTSTRING, -1, (*map[n].var) );
      }
   }

   static void setup_local_script_maps( HWND wnd, script_map_local * map, int m )
   {
      g_context_list.remove_all();
      g_command_guids.remove_all();
      g_subcommand_guids.remove_all();

      service_enum_t<contextmenu_item> menus;
      service_ptr_t<contextmenu_item> item;

      metadb_handle_list handles;
      metadb_handle_ptr h;
      static_api_ptr_t<metadb> db;

      if ( false == metadb::g_get_random_handle( h ) )
      {        
         db->handle_create( h, playable_location_impl( "silence://1", 0 ) );
      }

      handles.add_item( h );

      while ( menus.next( item ) )
      {
         get_context_list_w00t( item.get_ptr(), g_context_list, handles );
      }

      for ( int n = 0; n < m; n++ )
      {
         int id = map[n].id;               
         HWND hwnd_control = GetDlgItem( wnd, id );

         SendMessage( hwnd_control, CB_RESETCONTENT, 0, 0 );

         // add empty string option...
         uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, "" );            

         for ( int i = 0; i < g_context_list.get_count(); i++ )
         {
            int nPos = uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, g_context_list.get_item( i ) );

            SendDlgItemMessage( wnd, id, CB_SETITEMDATA, nPos, i );
         }

         uSendDlgItemMessageText( wnd, id, CB_SELECTSTRING, -1, (*map[n].var) );
      }
   }



   static void setup_context_item_maps( HWND wnd, context_item_map * map, int m )
   {
      for ( int n = 0; n < m; n++ )
      {
         int id = map[n].id;               
         HWND hwnd_control = GetDlgItem( wnd, id );

         SendMessage( hwnd_control, CB_RESETCONTENT, 0,0 );

         // add empty string option...
         uSendDlgItemMessageText( wnd, id, CB_ADDSTRING, 0, "" );            

         /*
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
         */

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

   static bool setup_bool_maps( HWND wnd, bool_map *map, int m )
   {
      for ( int n = 0; n < m; n++ )
      {
         CheckDlgButton( wnd, map[n].id, (map[n].var->get_value()) ? BST_CHECKED : BST_UNCHECKED ) ;            
      }
      return true;
   }

   static bool process_text_boxes( HWND wnd, WPARAM wp, text_box_map * map, int m )
   {
      if ( ( wp >> 16 ) == EN_CHANGE )
      {
         int edit_control = wp & 0xffff;
         for ( int n = 0; n < m; n++ )
         {
            if (map[n].id == edit_control )
            {
               pfc::string8 woop;
               uGetDlgItemText( wnd, map[n].id, woop );
               (*map[n].var) = woop;
               return true;
            }
         }
      }
      return false;
   }

   static bool process_int_maps( HWND wnd, WPARAM wp, int_map * map, int m )
   {
      if ( ( wp >> 16 ) == EN_CHANGE )
      {
         int edit_control = wp & 0xffff;
         for ( int n = 0; n < m; n++ )
         {
            if (map[n].id == edit_control )
            {
               pfc::string8 woop;
               uGetDlgItemText( wnd, map[n].id, woop );
               (*map[n].var) = atoi( woop );
               return true;
            }
         }
      }
      return false;
   }

   static bool process_script_map( HWND wnd, WPARAM wp, script_map * map, int m )
   {
      if ( wp >> 16 == CBN_SELCHANGE )
      {
         int the_list = wp & 0xffff;
         int n;

         for ( n = 0; n < m; n++ )
         {
            if ( map[n].id == the_list )
            {
               int s = SendDlgItemMessage( wnd, the_list, CB_GETCURSEL, 0, 0 );
               if ( s != CB_ERR )
               {
                  wchar_t tmp_buffer[1024];
                  map[n].var->reset();

                  SendMessage( GetDlgItem( wnd, the_list ), CB_GETLBTEXT, s, (LPARAM) tmp_buffer );

                  map[n].var->set_string( pfc::stringcvt::string_utf8_from_wide( tmp_buffer ) );

                  int nIndex = SendDlgItemMessage( wnd, the_list, CB_GETITEMDATA, s, 0 );

                  if ( map[n].var->is_empty() )
                  {
                     *map[n].command_guid = pfc::guid_null;
                     *map[n].subcommand_guid = pfc::guid_null;
                  }
                  else
                  {
                     *map[n].command_guid = g_command_guids[nIndex];
                     *map[n].subcommand_guid = g_subcommand_guids[nIndex];
                  }

                  if ( false )
                  {
                     pfc::print_guid g1( *map[n].command_guid );
                     pfc::print_guid g2( *map[n].subcommand_guid );
                     console::printf( "SELECTION: %s", map[n].var->get_ptr() );
                     console::printf( "ComboBox List Index: %d", s );
                     console::printf( "Array Index = %d", nIndex );
                     console::printf( "CMD: %s", g1.get_ptr() );
                     console::printf( "SUB: %s", g2.get_ptr() );

                     GUID guid;
                     if ( menu_helpers::find_command_by_name( map[n].var->get_ptr(), guid ) )
                     {
                        pfc::print_guid g3( guid );
                        console::printf( "find_by_command: %s", g3.get_ptr() );
                     }
                     else
                     {
                        console::printf( "find_by_command: NIL" );
                     }
                  }
               }
               return true;
            }
         }
      }
      return false;
   }

   static void script_map_local_on_ok( HWND wnd, script_map_local * map, int m )
   {
      for ( int n = 0; n < m; n++ )
      {
         int the_list = map[n].id;

         int s = SendDlgItemMessage( wnd, the_list, CB_GETCURSEL, 0, 0 );

         if ( s != CB_ERR )
         {
            wchar_t tmp_buffer[1024];
            map[n].var->reset();

            SendMessage( GetDlgItem( wnd, the_list ), CB_GETLBTEXT, s, (LPARAM) tmp_buffer );

            map[n].var->set_string( pfc::stringcvt::string_utf8_from_wide( tmp_buffer ) );

            int nIndex = SendDlgItemMessage( wnd, the_list, CB_GETITEMDATA, s, 0 );

            if ( map[n].var->is_empty() )
            {
               *map[n].command_guid = pfc::guid_null;
               *map[n].subcommand_guid = pfc::guid_null;
            }
            else
            {
               *map[n].command_guid = g_command_guids[nIndex];
               *map[n].subcommand_guid = g_subcommand_guids[nIndex];
            }

            /*
            if ( false )
            {
               pfc::print_guid g1( *map[n].command_guid );
               pfc::print_guid g2( *map[n].subcommand_guid );
               console::printf( "SELECTION: %s", map[n].var->get_ptr() );
               console::printf( "ComboBox List Index: %d", s );
               console::printf( "Array Index = %d", nIndex );
               console::printf( "CMD: %s", g1.get_ptr() );
               console::printf( "SUB: %s", g2.get_ptr() );

               GUID guid;
               if ( menu_helpers::find_command_by_name( map[n].var->get_ptr(), guid ) )
               {
                  pfc::print_guid g3( guid );
                  console::printf( "find_by_command: %s", g3.get_ptr() );
               }
               else
               {
                  console::printf( "find_by_command: NIL" );
               }
            }
            */
         }
      }    
   }

   static bool process_context_item_map( HWND wnd, WPARAM wp, context_item_map * map, int m )
   {
      if ( wp >> 16 == CBN_SELCHANGE )
      {
         int the_list = wp & 0xffff;

         for ( int n = 0; n < m; n++ )
         {
            if ( map[n].id == the_list )
            {
               int s = SendDlgItemMessage( wnd, the_list, CB_GETCURSEL, 0, 0 );
               if ( s != CB_ERR )
               {
                  wchar_t tmp_buffer[1024];
                  map[n].var->reset();

                  SendMessage( GetDlgItem( wnd, the_list ),
                     CB_GETLBTEXT, s, (LPARAM) tmp_buffer );

                  map[n].var->set_string( pfc::stringcvt::string_utf8_from_wide( tmp_buffer ) );
               }
               return true;
            }
         }
      }
      return false;
   }

   static bool process_bool_map( HWND wnd, WPARAM wp, bool_map * map, int m )
   {
      if ( wp >> 16 == BN_CLICKED )
      {
         int button = wp & 0xffff;

         for ( int n = 0; n < m; n++ )
         {
            if ( map[n].id == button )
            {
               bool val = ( BST_CHECKED == IsDlgButtonChecked( wnd, map[n].id ) );
               (*map[n].var) = val;
               return true;
            }
         }
      }
      return false;
   }

   static bool process_color_map( HWND wnd, WPARAM wp, color_map * map, int m )
   {
      if ( wp >> 16 == BN_CLICKED )
      {
         int button = wp & 0xffff;

         for ( int n = 0; n < m; n++ )
         {         
            if ( map[n].id == button )
            {
               pick_color( wnd, *map[n].var );
               return true;
            }     
         }
      }
      return false;
   }
};
