enum { GRP_PLAYLISTTREE, GRP_SELECTION, GRP_FILE, GRP_ROOT, GRP_REFRESH, GRP_LAST };

static const GUID guid_group_selection    = { 0x67ca91c3, 0x55aa, 0x4bcf, { 0xb5, 0xc0, 0xb9, 0xbd, 0x5a, 0xb6, 0x86, 0x3d } };
static const GUID guid_group_playlisttree = { 0xb0f269e, 0x7455, 0x4cef, { 0x9f, 0xee, 0x1d, 0xc2, 0x17, 0xef, 0xf6, 0xd3 } };
static const GUID guid_group_file         = { 0x12ba3c8a, 0x6974, 0x4636, { 0x8e, 0x5f, 0x1f, 0x20, 0xa3, 0xe1, 0x5c, 0x20 } };
static const GUID guid_group_root         = { 0xc8851205, 0x26b0, 0x4d04, { 0x97, 0x3a, 0x9c, 0xd2, 0x1, 0xa4, 0x3e, 0x37 } };
static const GUID guid_group_refresh      = { 0x58b0f4ee, 0xcc8, 0x4265, { 0x94, 0xce, 0x20, 0x9a, 0x4, 0xec, 0xaa, 0x61 } };

enum { 
   MM_NULL,
   MM_SELECTION_EDIT, 
   MM_SELECTION_REPOPULATE, 
   MM_FILE_OPEN,
   MM_FILE_SAVE,
   MM_SELECTION_NEW_FOLDER,
   MM_SELECTION_NEW_QUERY,
   MM_SELECTION_SORT,
   MM_SELECTION_SORT_RECURSIVELY,
   MM_SELECTION_REMOVE,
   MM_SELECTION_RENAME_ARTIST,
   MM_SELECTION_RENAME_ALBUM,
   MM_SELECTION_RENAME_ALBUM_ARTIST,
   MM_SELECTION_CLEAR_QUERY,
   MM_ROOT_NEW_FOLDER,
   MM_ROOT_NEW_QUERY, 
   MM_ROOT_SORT_ALL, 
   MM_ROOT_REPOPULATE_ALL,
   MM_ROOT_EXPORT_ALL,
   MM_ROOT_EXPORT_VISIBLE,
   MM_SELECTION_SAVEAS,
   MM_SELECTION_LOADCHILD,
   MM_SELECTION_EXPORT_QUERY,
   MM_SELECTION_CONTEXT_LOCAL,
   MM_SELECTION_CONTEXT_GLOBAL,
   MM_ROOT_COLLAPSE,
   //MM_TRACKFINDER_PLAYLIST,

//   MM_SIZE_RESTORE,
   MM_LAST 
};


void mm_command_process( int p_index, node * n )
{
   node * selected_node = NULL;

   // options that do not require a tree
   // switch ( p_index )
   // {
      /*
   case MM_TRACKFINDER_PLAYLIST:
      {
            static_api_ptr_t<playlist_manager> m;
            metadb_handle_list handles;
            m->activeplaylist_get_all_items( handles );
            track_finder( handles );
      }
      break;
      */

      /*
   case MM_SIZE_RESTORE:
      {
         RECT rc;
         GetWindowRect( core_api::get_main_window(), &rc );
         SetWindowPos( core_api::get_main_window(), 0, rc.left, rc.top, 100, 100, SWP_NOZORDER );
         break;
      }
      */
   // }

   if ( g_last_active_tree )
   {
      // optons requiring a selection
      if ( n )
      {
         selected_node = n;
      }
      else
      {
         selected_node = g_last_active_tree->get_selection();
      }

      if ( selected_node )
      {
         query_node * qn = dynamic_cast<query_node*>( selected_node );
         
         if ( qn )
         {
            switch ( p_index )
            {
            case MM_SELECTION_EXPORT_QUERY:
               qn->export_query();
               break;

            case MM_SELECTION_CLEAR_QUERY:
               qn->free_children();
               qn->refresh_subtree();
               break;
            }
         }
         folder_node * fn = dynamic_cast<folder_node*>( selected_node );

         // items that require a folder... 
         if ( fn )
         {
            switch ( p_index )
            {
            case MM_SELECTION_RENAME_ARTIST:
               fn->rename_as_tag( "%artist%" );
               break;

            case MM_SELECTION_RENAME_ALBUM:
               fn->rename_as_tag( "%album%" );
               break;

            case MM_SELECTION_RENAME_ALBUM_ARTIST:
               fn->rename_as_tag( "%album artist%" );
               break;

            case MM_SELECTION_SAVEAS:
               fn->save();
               break;

            case MM_SELECTION_LOADCHILD:
               {
                  node * n = load( NULL );

                  if ( n ) 
                  {
                     fn->add_child( n );
                  }
               }
               break;

            case MM_SELECTION_SORT:
               fn->sort( false );
               fn->refresh_subtree();
               break;

            case MM_SELECTION_SORT_RECURSIVELY:
               fn->sort( true );
               fn->refresh_subtree();
               break;

            case MM_SELECTION_REPOPULATE:
               SendMessage( fn->m_hwnd_tree, WM_SETREDRAW, FALSE, 0 );
               fn->repopulate();
               fn->refresh_subtree();
               fn->refresh_tree_label();
               SendMessage( fn->m_hwnd_tree, WM_SETREDRAW, TRUE, 0 );
               InvalidateRect( fn->m_hwnd_tree, NULL, FALSE );
               break;

            case MM_SELECTION_NEW_QUERY:
               lazy_add_user_query( fn );               
               break;

            case MM_SELECTION_NEW_FOLDER:
               {
                  folder_node * f = new folder_node( "New Folder" );
                  fn->add_child( f );

                  f->ensure_in_tree();
                  f->force_tree_selection();

                  if ( !f->edit() )
                  {
                     delete f;
                  }
               }
               break;
            }
         }

         switch ( p_index )
         {
         case MM_SELECTION_CONTEXT_LOCAL:
            selected_node->do_node_context_menu( );
            break;

         case MM_SELECTION_CONTEXT_GLOBAL:
            selected_node->do_foobar2000_context_menu( );
            break;

         case MM_SELECTION_REMOVE:
            if ( selected_node->m_parent )
            {
               on_node_removing( selected_node );
               delete selected_node;
               update_nodes_if_necessary();
            }
            else
            {
               MessageBox( g_last_active_tree->get_wnd(), 
                  _T("The root playlist tree node cannot be removed.  If you do not ")
                  _T("want it to be displayed, you can hide it by checking 'hide root' ")
                  _T("in the Playlist Tree preferences"),
                  _T("foo_playlist_tree"), MB_OK | MB_ICONSTOP );
            }
            break;

         case MM_SELECTION_EDIT:
            selected_node->edit();
            break;             
         }
      }

      // non selection options
      switch ( p_index )
      {
      case MM_ROOT_EXPORT_ALL:
         {
            pfc::string8 path;

            if ( get_save_name( path, false, false, true ) )
            {
               pfc::stringcvt::string_wide_from_utf8 wide_file( path );

	            FILE * f = _wfopen( wide_file, _T("w+") );

               if ( f )
               {
                  g_last_active_tree->m_root->write_text( f, false );
               }

               fclose( f );
            }
         }
         break;

      case MM_ROOT_EXPORT_VISIBLE:
         {
            pfc::string8 path;

            if ( get_save_name( path, false, false, true ) )
            {
               pfc::stringcvt::string_wide_from_utf8 wide_file( path );

	            FILE * f = _wfopen( wide_file, _T("w+") );

               if ( f )
               {
                  g_last_active_tree->m_root->write_text( f, true );
               }

               fclose( f );
            }
         }
         break;

      case MM_ROOT_COLLAPSE:
         g_last_active_tree->m_root->collapse( true );
         g_last_active_tree->setup_tree();
         break;

      case MM_ROOT_SORT_ALL:
         g_last_active_tree->m_root->sort( true );
         g_last_active_tree->setup_tree();
         break;

      case MM_ROOT_REPOPULATE_ALL:
         g_last_active_tree->m_root->repopulate();
         g_last_active_tree->setup_tree();
         break;

      case MM_ROOT_NEW_FOLDER:
         {
            folder_node * f = new folder_node( "New Folder" );
            g_last_active_tree->m_root->add_child( f );

            f->ensure_in_tree();
            f->force_tree_selection();

            if ( !f->edit() )
            {
               delete f;
            }
         }
         break;

      case MM_ROOT_NEW_QUERY:
         lazy_add_user_query( g_last_active_tree->m_root );
         break;

      case MM_FILE_OPEN:
         {
            folder_node * n = (folder_node *)load( NULL );
            if ( n )
            {
               g_last_active_tree->set_root( n );                     
               g_last_active_tree->setup_tree();
            }
         }
         break;

      case MM_FILE_SAVE:
         g_last_active_tree->m_root->save();
         break;
      }
   }
}

#define TYPE_LEAF    0x01
#define TYPE_FOLDER  0x02
#define TYPE_QUERY   0x04
#define TYPE_FQ      (TYPE_FOLDER|TYPE_QUERY)
#define TYPE_ALL     (TYPE_LEAF|TYPE_FOLDER|TYPE_QUERY)
#define TYPE_NONE    0

struct {
   int id;
   int group_id;
   char * label;
   char * desc;
   char * local_path;
   int type;
   GUID guid;   
   int local_index;
} mm_map[] = 
{
   { MM_SELECTION_SAVEAS,
   GRP_SELECTION,
   "Save As...",
   "Save the selected node",
   "File|Save As...",
   TYPE_FQ,
   { 0x6b17636c, 0x6e64, 0x46fe, { 0xac, 0x28, 0x15, 0xde, 0x74, 0x4d, 0xc9, 0x69 } } },

   { MM_SELECTION_LOADCHILD,
   GRP_SELECTION,
   "Load Child...",
   "Load a child from a file",
   "File|Load Child...",
   TYPE_FOLDER,
   { 0x39d8ff16, 0x74c4, 0x4609, { 0xaa, 0xc1, 0xc5, 0x50, 0x96, 0x16, 0xbf, 0xbd } } },

   { MM_SELECTION_EXPORT_QUERY,
   GRP_SELECTION,
   "Export Query...",
   "Export a query without the children nodes",
   "File|Export Query...",
   TYPE_QUERY,
   { 0xa3d07373, 0x8f3d, 0x472a, { 0x91, 0x34, 0xe5, 0xaf, 0x55, 0x6b, 0x19, 0x1d } } },
   
   { MM_SELECTION_NEW_FOLDER,
   GRP_SELECTION,
   "New Folder...",
   "Create a new folder inside the selected node",
   "New|Folder...",
   TYPE_FQ,
   { 0xba2650db, 0x2e6b, 0x483d, { 0x90, 0x4e, 0x63, 0xe0, 0x28, 0x33, 0x18, 0xf6 } } },

   { MM_SELECTION_NEW_QUERY,
   GRP_SELECTION,
   "New Query...",
   "Create a new query inside the selected node",
   "New|Query...",
   TYPE_FQ,
   { 0x8e0a3210, 0xfe9f, 0x4e02, { 0x86, 0x82, 0xae, 0x80, 0xb3, 0xaa, 0x99, 0x96 } } },

   { MM_SELECTION_SORT,
   GRP_SELECTION,
   "Sort",
   "Sort the children of the selected node",
   "Sort|Children",
   TYPE_FQ,
   { 0x13453ee4, 0x503e, 0x491d, { 0xbe, 0x15, 0x23, 0x8a, 0x8a, 0x7d, 0x42, 0x13 } } },

   { MM_SELECTION_SORT_RECURSIVELY,
   GRP_SELECTION,
   "Sort Recursively",
   "Sort the children of the selected node recursively",
   "Sort|Recursively",
   TYPE_FQ,
   { 0x1dd64906, 0xff02, 0x41b5, { 0x96, 0xae, 0x84, 0x9c, 0x7a, 0xd1, 0x6b, 0xd6 } } },

   { MM_SELECTION_RENAME_ARTIST,
   GRP_SELECTION,
   "Rename as Artist",
   "Rename the selected node as the artist of the first track",
   "Rename|As Artist",
   TYPE_FQ,
   { 0xdf20f136, 0xd53c, 0x4703, { 0x8e, 0xe, 0x1, 0xfb, 0xdc, 0x7b, 0x72, 0x67 } } },

   { MM_SELECTION_RENAME_ALBUM,
   GRP_SELECTION,
   "Rename as Album",
   "Rename the selected node as the album of the first track",
   "Rename|As Album",
   TYPE_FQ,
   { 0xbbe15094, 0xe075, 0x4c40, { 0x8d, 0x24, 0x91, 0x15, 0xe3, 0xff, 0x1c, 0xb2 } } },

   { MM_SELECTION_RENAME_ALBUM_ARTIST,
   GRP_SELECTION,
   "Rename as Album Artist",
   "Rename the selected node as the '%album artist%' of the first track",
   "Rename|As Album Artist",
   TYPE_FQ,
   { 0xcdc9e14e, 0x1d66, 0x44dc, { 0xb8, 0x6e, 0x9e, 0xe0, 0xa9, 0x68, 0xdd, 0x67 } } },

   { MM_SELECTION_EDIT,
   GRP_SELECTION,
   "Edit...",
   "Edit selected node",
   "Edit...",
   TYPE_FQ,
   { 0x516048b9, 0x2ba0, 0x4a89, { 0x9c, 0xa3, 0x33, 0xf2, 0xd1, 0x28, 0x17, 0x62 } } },

   { MM_SELECTION_REPOPULATE,   
   GRP_SELECTION,
   "Refresh Queries",
   "Refresh Queries",
   "Refresh Queries",
   TYPE_FQ,
   { 0xf00cdc42, 0x3a25, 0x4d2a, { 0xaf, 0xf3, 0x93, 0xd9, 0x37, 0xd2, 0x56, 0xc9 } } },
     
   { MM_SELECTION_CLEAR_QUERY,
   GRP_SELECTION,
   "Clear Query Results",
   "Clear the children of a query",
   "Clear Query Results",
   TYPE_QUERY,
   { 0x872c9547, 0x33a1, 0x4281, { 0xa8, 0xc6, 0xa3, 0x93, 0x3d, 0xf2, 0x1e, 0x88 } } },

   { MM_FILE_OPEN,
   GRP_FILE,
   "Open...",
   "Open a playlist tree collection",
   "",
   TYPE_NONE,
   { 0xa0d3d04c, 0xe941, 0x43e6, { 0xad, 0x18, 0x43, 0x4e, 0xb4, 0x15, 0xdc, 0xc9 } } },

   { MM_FILE_SAVE,
   GRP_FILE,
   "Save...", 
   "Save a playlist tree collection",
   "",
   TYPE_NONE,
   { 0x712d23e3, 0x3d03, 0x4e52, { 0x8d, 0xc2, 0xed, 0xf2, 0x8d, 0x9a, 0x14, 0x53 } } },

   { MM_ROOT_NEW_FOLDER,
   GRP_ROOT,
   "New Folder...",
   "Create a new folder inside the root node",
   "",
   TYPE_NONE,
   { 0x22925d37, 0x63d, 0x469a, { 0xb3, 0xd1, 0xed, 0x7e, 0xdb, 0xc6, 0xc3, 0x95 } } },

   { MM_ROOT_NEW_QUERY,
   GRP_ROOT,
   "New Query...",
   "Create a new query inside the root node",
   "",
   TYPE_NONE,
   { 0xda520775, 0x52d2, 0x4838, { 0xb9, 0x22, 0xe1, 0xf2, 0xd9, 0x19, 0x4e, 0x8 } } },  

   { MM_ROOT_SORT_ALL,
   GRP_ROOT,
   "Sort All",
   "Sorts all the folders in the tree",
   "",
   TYPE_NONE,
   { 0xeee441e1, 0x60e4, 0x4f66, { 0x8c, 0x50, 0xbc, 0x70, 0xe9, 0x93, 0x6e, 0xb9 } } },

   { MM_ROOT_REPOPULATE_ALL,
   GRP_ROOT,
   "Refresh All Queries",
   "Refresh All Queries in the Entire Tree",
   "",
   TYPE_NONE,
   { 0x979e94d8, 0x728d, 0x467b, { 0x8f, 0xe4, 0xfe, 0x59, 0x1d, 0x33, 0x6c, 0x55 } } },
   
   { MM_SELECTION_REMOVE,
   GRP_SELECTION,
   "Remove",
   "Remove the selected node",
   "Remove",
   TYPE_ALL,
   { 0x581cfbb1, 0xfc45, 0x4896, { 0xaf, 0x80, 0x81, 0x4c, 0x60, 0xa1, 0x71, 0x74 } } },

   { MM_SELECTION_CONTEXT_LOCAL,
   GRP_SELECTION,
   "Local Context Menu...",
   "Shows the playlist tree context menu for the selected item",
   "Local Context Menu",
   TYPE_NONE, 
   { 0x1de073fb, 0xb006, 0x4ffd, { 0xa6, 0x98, 0x3, 0xc7, 0x79, 0xa1, 0x3a, 0xcd } } },

   { MM_SELECTION_CONTEXT_GLOBAL,
   GRP_SELECTION,
   "foobar2000 Context Menu...",
   "Shows the foobar2000 Context Menu for the selected item",
   "foobar2000 Context Menu",
   TYPE_NONE,
   { 0xbac47a85, 0xcb9f, 0x4185, { 0x96, 0x12, 0xa5, 0xc0, 0xf9, 0xce, 0x75, 0x3f } } },

   { MM_ROOT_COLLAPSE,
   GRP_ROOT,
   "Collapse Tree",
   "Collapses all nodes of the last active tree",
   "",
   TYPE_NONE,
   { 0x42cfdf2d, 0x8fd2, 0x416a, { 0xa8, 0xee, 0xd6, 0x81, 0x10, 0xe5, 0xdd, 0xa5 } } },

   /*
   { MM_TRACKFINDER_PLAYLIST,
   GRP_PLAYLISTTREE,
   "Track Finder (Playlist)",
   "Runs the track finder on the current playlist",
   "",
   TYPE_NONE,
   { 0xb46c3f53, 0x1ce0, 0x4a40, { 0x86, 0xbf, 0x9c, 0x1e, 0x5f, 0xf9, 0xc5, 0x5e } } },
   */

   { MM_ROOT_EXPORT_ALL,
   GRP_ROOT,
   "Export (All)",
   "Export all nodes to a text file",
   "",
   TYPE_NONE,
   { 0x37dd1b04, 0x671b, 0x4f48, { 0xbe, 0xeb, 0x78, 0x59, 0x3f, 0x4b, 0x3f, 0xd2 } } },

   { MM_ROOT_EXPORT_VISIBLE,
   GRP_ROOT,
   "Export (Visible)",
   "Export visible nodes to a text file",
   "",
   TYPE_NONE,
   { 0x84785077, 0x435f, 0x49ca, { 0xbb, 0xb0, 0xf3, 0x91, 0x4d, 0xec, 0x84, 0xb8 } } },

   /*
   { MM_SIZE_RESTORE,
   GRP_PLAYLISTTREE,
   "Restore Size",
   "Restore Size",
   "",
   TYPE_NONE,
   { 0x7955eba, 0x69a9, 0x4f28, { 0xad, 0xbf, 0xa9, 0xb0, 0xd6, 0xaf, 0x6f, 0xda } } },
   */
};

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

//static mainmenu_commands_factory_t<generic_group> g_group_selection( "Selection", guid_group_selection, mainmenu_groups::library );
static service_factory_single_t<generic_group> g_group_tree( "Playlist Tree", guid_group_playlisttree, mainmenu_groups::library );
static service_factory_single_t<generic_group> g_group_file( "File", guid_group_file, guid_group_playlisttree, mainmenu_commands::sort_priority_base );
static service_factory_single_t<generic_group> g_group_selection( "Selection", guid_group_selection, guid_group_playlisttree, mainmenu_commands::sort_priority_base );
static service_factory_single_t<generic_group> g_group_root( "Root", guid_group_root, guid_group_playlisttree, mainmenu_commands::sort_priority_base );
static service_factory_single_t<generic_group> g_group_refresh( "Refresh", guid_group_refresh, guid_group_playlisttree, mainmenu_commands::sort_priority_base );

class generic_group_items : public mainmenu_commands
{
public:
   int m_group_id;
   GUID m_parent;
   int  m_count;

   generic_group_items( int group_id, GUID parent )
   {
      m_group_id = group_id;
      m_parent = parent;
      
      m_count = 0;
      for ( int n = 0; n < tabsize( mm_map ); n++ )
      {
         if ( mm_map[n].group_id == group_id )
         {
            mm_map[n].local_index = m_count;
            m_count++;
         }
      }
   }

   int local_index_2_map_index( int p_index )
   {
      for ( int n = 0; n < tabsize( mm_map ); n++ )
      {
         if ( mm_map[n].group_id == m_group_id ) 
         {
            if ( mm_map[n].local_index == p_index )
            {
               return n;
            }
         }
      }

      return -1;
   }

   t_uint get_command_count()
   {
      return m_count;
   }

   virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags) 
   {
      if ( p_index < m_count )
      {
         int map_index = local_index_2_map_index( p_index );
   
         if ( mm_map[map_index].group_id == GRP_SELECTION )
         {
            p_flags = flag_disabled;

            if ( g_last_active_tree )
            {                  
               node * selected_node = g_last_active_tree->get_selection();

               if ( selected_node )
               {
                  if ( ( mm_map[map_index].type == TYPE_NONE ) )
                  {
                     p_flags = 0;
                  }
                  if ( ( mm_map[map_index].type & TYPE_LEAF ) && ( selected_node->is_leaf() ) )
                  {
                     p_flags = 0;
                  }
                  else if ( ( mm_map[map_index].type & TYPE_FOLDER ) && ( selected_node->is_folder() ) )
                  {
                     p_flags = 0;
                  }
                  else if ( ( mm_map[map_index].type & TYPE_QUERY ) && ( selected_node->is_query() ) )
                  {
                     p_flags = 0;
                  }
               }
            }
         }
         else
         {
            p_flags = 0;
         }
         get_name(p_index,p_text);
         return true;
      }
      return false;
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      if ( p_index < m_count )
      {
         int map_index = local_index_2_map_index( p_index );

         if ( map_index >= 0 )
         {
            return mm_map[map_index].guid;
         }
      }
      return pfc::guid_null;
   }

   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      if ( p_index < m_count )
      {
         int map_index = local_index_2_map_index( p_index );

         if ( map_index >= 0 )
         {
            p_out.set_string( mm_map[map_index].label );
         }
      }
   }

   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      if ( p_index < m_count )
      {
         int map_index = local_index_2_map_index( p_index );

         if ( map_index >= 0 )
         {
            p_out.set_string( mm_map[map_index].desc );
            return true;
         }
      }
      return false;
   }

   virtual GUID get_parent()
   {
      return m_parent;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      if ( p_index < m_count )
      {
         int map_index = local_index_2_map_index( p_index );

         if ( map_index >= 0 )
         {
            mm_command_process( mm_map[map_index].id );
         }
      }
   }
};

static service_factory_single_t<generic_group_items> g_group_file_items( GRP_FILE, guid_group_file );
static service_factory_single_t<generic_group_items> g_group_root_items( GRP_ROOT, guid_group_root );
static service_factory_single_t<generic_group_items> g_group_selection_items( GRP_SELECTION, guid_group_selection );
static service_factory_single_t<generic_group_items> g_group_playlisttree_items( GRP_PLAYLISTTREE, guid_group_playlisttree );



// {D8600091-416B-461c-8E5F-DA78B5E25F4A}
static const GUID guid_refresh_command = { 0xd8600091, 0x416b, 0x0000, { 0x8e, 0x5f, 0xda, 0x78, 0xb5, 0xe2, 0x5f, 0x43 } };


class refresh_menu_items : public mainmenu_commands
{
private:
   void collect_queries( playlist_tree_panel * tree, node * start, int tree_index )
   {
      if ( start->is_query() )
      {
         query_node * qn = (query_node*) start;

         if ( strstr( qn->m_source, TAG_SCHEME ) )
         {
            pfc::string8 t;
            pfc::string8 name;
            qn->get_display_name( t );
            name = pfc::string_printf( "tree %d: %s", tree_index, t.get_ptr() );
            m_ar_query_names.add_item( name );
            m_ar_query_nodes.add_item( qn );
         }
      }
      else if ( start->is_folder() )
      {
         folder_node * fn = (folder_node*) start;
         for ( int n = 0; n < fn->get_num_children(); n++ )
         {
            collect_queries( tree, fn->m_children[n], tree_index );
         }
      }
   }

   pfc::list_t<query_node*> m_ar_query_nodes;
   pfc::list_t<pfc::string8> m_ar_query_names;

public:
   t_uint get_command_count()
   {           
      m_ar_query_nodes.remove_all();
      m_ar_query_names.remove_all();

      for ( int i = 0; i < g_active_trees.get_count(); i++ )
      {  
         collect_queries( g_active_trees[i], g_active_trees[i]->m_root, i );
      }

      int z = m_ar_query_nodes.get_count();
      // return m_ar_query_nodes.get_count();
      return z;
   }

   virtual GUID get_command(t_uint32 p_index)
   {
      if ( p_index < m_ar_query_nodes.get_count() )
      {
         return m_ar_query_nodes[p_index]->m_guid;
      }
      return pfc::guid_null;
   }

   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      p_out.set_string( m_ar_query_names[p_index] );
   }

   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      p_out.set_string( "Refresh query" );
      return true;
   }

   virtual GUID get_parent()
   {
      return guid_group_refresh;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      m_ar_query_nodes[p_index]->no_flicker_repopulate();
   }
};

static service_factory_single_t<refresh_menu_items> g_group_refresh_items;
