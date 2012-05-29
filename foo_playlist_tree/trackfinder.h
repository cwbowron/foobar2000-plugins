#pragma once

static const GUID guid_tf_context = { 0x10c60205, 0x8404, 0x4c63, { 0x89, 0xbd, 0x76, 0x1b, 0xba, 0x97, 0x11, 0xb8 } };


#if 1
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

#endif

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

	for (i = 0; i < list.get_count(); i++)
	{
		for ( j = 0; j < max_j; j++)
		{
			list[i]->format_title( NULL, handle_info, 
               (const char *)track_formats[j]->get_ptr(), NULL );
			 			  
         query_ready_string delimited( handle_info );

			//node * temp = qt_tree->add_delimited_child( delimited, list[i] );
         simple_node * temp = qt_tree->add_delimited_child( delimited, list[i] );
		}
	}

	qt_tree->sort( true );
	track_formats.delete_all();

	return qt_tree;
}

int process_trackfinder_selection( node *tree, int ID, pfc::ptr_list_t<pfc::string8> * actions )
{
   if ( actions->get_count() == 1 )
   {
      if ( tree->m_id == ID )
      {
         run_context_command( actions->get_item(0)->get_ptr(), NULL, tree );
         return true;
      }
   }
   else
   {
      int n = ID - tree->m_id;

      if ( ( n >= 0 ) && ( n < actions->get_count() ) )
      {   
         run_context_command( actions->get_item(n)->get_ptr(), NULL, tree );
         return true;
      }
   }

   if ( !tree->is_leaf() )
   {
      folder_node * fn = (folder_node*) tree;
      for ( int i = 0; i < fn->get_num_children(); i++ )
      {
         if ( process_trackfinder_selection( fn->m_children[i], ID, actions ) )
         {
            return true;
         }
      }
   }
   return false;
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

#if 1
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
      run_context_command( g_last_tf_command, &handles, NULL );
      

   }

   DestroyMenu( m2 );

#else
   int ID = 1;
   HMENU m = generate_node_menu_complex( n, ID, true, &actions );
   console::printf( "trackfinder items: %d", ID - 1 );


   HWND parent;

   parent = core_api::get_main_window();  
   
   int cmd = TrackPopupMenuEx( m, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, 
         pt.x, pt.y, parent, 0 );

   if ( cmd )
   {
      process_trackfinder_selection( n, cmd, &actions );
   }
   else
   {
      DWORD dw = GetLastError(); 
      if ( dw != 0 )
      {
         LPVOID lpMsgBuf;
         FormatMessage(
           FORMAT_MESSAGE_ALLOCATE_BUFFER | 
           FORMAT_MESSAGE_FROM_SYSTEM,
           NULL,
           dw,
           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
           (LPTSTR) &lpMsgBuf,
           0, NULL );

         pfc::stringcvt::string_utf8_from_wide msg( (TCHAR*)lpMsgBuf );

         pfc::string_printf warnmsg( "Track Finder failed: %s", msg.get_ptr() );
         console::warning( warnmsg );
         LocalFree( lpMsgBuf );
      }
   }

   DestroyMenu( m );
#endif

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