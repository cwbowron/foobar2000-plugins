#pragma once

// query source designators
#define TAG_DROP				"@drop"
#define TAG_NODE				"@node"
#define TAG_PLAYLIST			"@playlist"
#define TAG_PLAYLISTS      "@playlists"
#define TAG_DATABASE       "@database"
#define TAG_SCHEME         "@scheme"

#define TAG_DEFAULT        "@default"

#define TAG_AND            "@and"
#define TAG_OR             "@or"
#define TAG_NOT            "@not"

query_node * g_query_node;

enum {MAX_CRIT_TRACKS, MAX_CRIT_SIZE, MAX_CRIT_LENGTH, MAX_CRIT_SUBFOLDER };

void display_elapsed_time( const char * msg, double t )
{
   static char w00t[100];
   sprintf( w00t, "%s: %f", msg, t );
   console::printf( w00t );
}

bool get_entries_by_directory( const char *directory, metadb_handle_list & entries )
{         
   DWORD dwAtt = uGetFileAttributes( directory );

   //if ( uFileExists( directory ) )
   if ( dwAtt != INVALID_FILE_ATTRIBUTES )
   {
      abort_callback_impl abort;
      playlist_loader_callback_impl loader( abort );
      playlist_loader::g_process_path_ex( directory, loader );

      entries.add_items( loader.get_data() );
      entries.remove_duplicates();
      return true;
   }

   console::warning( pfc::string_printf( "File not found: %s", directory ) );
   return false;
}

bool is_empty_string( const char * s )
{
   int n;
   int m = strlen( s );

   bool result = false;

   for ( n = 0; n < m; n++ )
   {
      if ( s[n] != ' ' ) return false;
   }
   return true;
}

bool get_entries_queue( metadb_handle_list & entries )
{
   pfc::list_t<t_playback_queue_item> queue;

   static_api_ptr_t<playlist_manager> man;
   man->queue_get_contents( queue );

   for ( int i = 0; i < queue.get_count(); i++ )
   {
      entries.add_item( queue[i].m_handle );
   }
   return true;
}

bool get_entries_by_name( node * root, const char *name, metadb_handle_list & entries )
{
   node *test_node = root->find_by_name( name );

   if ( test_node )
   {
      test_node->get_entries( entries );
      return true;
   }
   else
   {
      console::warning( pfc::string_printf( "Node %s not found", (const char *)name ));
      return false;
   }
}

bool get_playlist_entries_by_name( const char *name, metadb_handle_list & entries )
{
   static_api_ptr_t<playlist_manager> man;
   bit_array_false f;
   int n = man->find_playlist( name, ~0 );

   if ( n != ~0 )
   {
      man->playlist_get_all_items( n, entries );
      return true;
   }

   console::printf( "playlist not found: %s", name );
   return false;
}

#include <stdarg.h>

int min_non_negative( int n, ... )
{
   int r = -1;
   va_list ap;
   va_start( ap, n );

   for ( int i = 0; i < n; i++ )
   {
      int x = va_arg( ap, int );

      if ( ( x >= 0 ) && ( ( r == -1 ) || ( x < r ) ) )
      {
         r = x;
      }
   }
   va_end( ap );

   return r;
}

class query_node : public folder_node
{
public:
   pfc::string8 m_query_criteria;
   pfc::string8 m_query_handle_sort_criteria;
   pfc::string8 m_query_format;
   pfc::string8 m_source;

   int m_query_limit;
   int m_limit_criteria;
   int m_reverse_populate_query;   

   bool m_sort_after_populate;
   bool m_refresh_on_new_track;

   search_tools::search_filter m_filter;
   GUID m_guid;

   query_node( const char *name,
      bool expanded = false,
      const char *source = NULL,
      const char *criteria = NULL,
      const char *format = NULL,
      const char *pop_sort = NULL,
      //const char *display_sort = NULL
      bool sort_after_populate = false
      ) : folder_node( name, expanded )
   {
      m_query_criteria.set_string( criteria );

      m_query_limit = 0;
      m_limit_criteria = 0;
      m_reverse_populate_query = 0;

      if ( source && strlen( source ) > 0 )
      {
         m_source.set_string( source );
      }
      else
      {
         m_source.set_string( TAG_DATABASE );
      }
      if ( format )
      {
         m_query_format.set_string( format );
      }
      else
      {
         m_query_format.set_string( cfg_query_display );
      }

      if ( pop_sort )
      {
         m_query_handle_sort_criteria.set_string( pop_sort );
      }
      else
      {
         m_query_handle_sort_criteria.set_string( cfg_default_query_sort );
      }

      /*
      if ( display_sort )
      {
      m_sort_criteria.set_string( display_sort );
      }
      */
      m_sort_after_populate = sort_after_populate;

      m_refresh_on_new_track = false;

      ::UuidCreate( &m_guid );
   }

   virtual ~query_node()
   {
   }

   virtual bool is_folder() const
   {
      return false;
   }

   virtual node *copy()
   {
      query_node *dup = new query_node( m_label, m_expanded,
         m_source, m_query_criteria, m_query_format, 
         m_query_handle_sort_criteria, /*m_sort_criteria*/ m_sort_after_populate );

      dup->m_limit_criteria = m_limit_criteria;
      dup->m_query_limit = m_query_limit;
      dup->m_reverse_populate_query = m_reverse_populate_query;
      dup->m_refresh_on_new_track = m_refresh_on_new_track;

      for (unsigned i = 0; i < get_num_children(); i++)
      {
         dup->add_child( m_children[i]->copy() );
      }

      return dup;
   }

   bool export_query()
   {
try_again:;
      pfc::string8 file_name;

      if ( get_save_name( file_name, true ) ) 
      {
         pfc::stringcvt::string_wide_from_utf8 wide_name( file_name );

         FILE *out = _wfopen( wide_name, _T("w+") );

         if ( out == NULL )
         {
            console::warning( pfc::string_printf("Error opening %s for writing", 
               (const char * ) file_name ) );
            goto try_again;
         }

         write_query( out, 0, false);
         fclose( out );

         return true;
      } 
      return false;
   }

   virtual bool is_query() const { return true; }

   // return true if the query can be populated in the background
   // false if not...
   // $drop queries must be populated from main thread
   virtual bool is_backgroundable()
   {
      if ( strstr( m_query_criteria, TAG_DROP ))
      {
         return false;
      }
      return true;
   }

   void get_query_format( pfc::string_base & out )
   {
      process_quote( out, m_query_format );
   }

   void get_query_string( pfc::string8 & result )
   {
      process_local_tags( result, m_query_criteria );
   }

   void sort_handles( metadb_handle_list & results )
   {
      if ( m_query_handle_sort_criteria.length() > 0 )
      {
         if ( strstr( m_query_handle_sort_criteria, TAG_DEFAULT ))
         {
            results.sort_by_format( m_query_format, NULL );
         }
         else
         {
            results.sort_by_format( m_query_handle_sort_criteria, NULL );
         }
      }	
   }

   void get_matching_handles( const char * str, metadb_handle_list & results )
   {
      pfc::string8 source = m_source;
      pfc::string8 tmp;

      int aa = source.find_first( TAG_DROP );
      int bb = source.find_first( TAG_NODE );
      int cc = source.find_first( TAG_PLAYLIST );
      int dd = source.find_first( TAG_DATABASE );
      int ee = source.find_first( TAG_QUEUE );

      int a = source.find_first( TAG_AND );
      int b = source.find_first( TAG_OR );
      int c = source.find_first( TAG_NOT );

      int nn = min_non_negative( 5, aa, bb, cc, dd, ee );
      int n = min_non_negative( 3, a, b, c );

      // If an operator (@and, @or, @not) comes before the designator, assume full database 
      if ( ( n >= 0 ) && ( n < nn ) )
      {
         nn = -1;
      }

      // no designator specified, use entire database... 
      if ( nn == - 1)
      {
         static_api_ptr_t<library_manager> db;
         db->get_all_items( results );
      }
      else
      {
         pfc::string8 filename;
         int start, end;

         if ( nn == aa )
         {
            if ( !string_get_function_param( filename, source, TAG_DROP, &start, &end ) )
               return;

            get_entries_by_directory( filename, results );

            if ( string_replace( tmp, source, start, end + 1, "" ) >= 0 )
            {
               source = tmp;
            }
         }
         else if ( nn == bb )
         {
            if ( string_get_function_param( filename, source, TAG_NODE, &start, &end ) )
            {
               get_entries_by_name( get_root(), filename, results );
               if ( string_replace( tmp, source, start, end + 1, "" ) >= 0 )
               {
                  source = tmp;
               }
            }
         }
         else if ( nn == cc )
         {
            if ( !string_get_function_param( filename, source, TAG_PLAYLIST, &start, &end ) )
               return;

            get_playlist_entries_by_name( filename, results );

            if ( string_replace( tmp, source, start, end + 1, "" ) >= 0 )
            {
               source = tmp;
            }
         }
         else if ( nn == dd )
         {
            if ( string_remove_tag_simple( tmp, source, TAG_DATABASE ) )
            {
               source = tmp;
            }

            static_api_ptr_t<library_manager> db;         
            db->get_all_items( results );
         }
         else if ( nn == ee )
         {
            if ( string_remove_tag_simple( tmp, source, TAG_QUEUE ) )
            {
               source = tmp;
            }

            get_entries_queue( results );
         }
      }

      while ( 1 )
      {
         a = source.find_first( TAG_AND );
         b = source.find_first( TAG_OR );
         c = source.find_first( TAG_NOT );

         int m = min_non_negative( 3, a, b, c );

         if ( m < 0 ) 
            break;

         int w = source.find_first( TAG_DROP );
         int x = source.find_first( TAG_NODE );
         int y = source.find_first( TAG_PLAYLIST );

         int z = min_non_negative( 3, w, x, y );

         if ( z < 0 )
         {
            console::warning( "Missing @node or @playlist in query criteria" );
            break;
         }

         pfc::string8 node_name;
         metadb_handle_list entries;
         int first_char, last_char;

         if ( z == w  )
         {
            if ( string_get_function_param( node_name, source, TAG_DROP, &first_char, &last_char  ) )
            {
               get_entries_by_directory( node_name, entries );
               string_replace( tmp, source, first_char, last_char+1, "" );
               source = tmp;
            }
            else break;
         }
         // NODE
         else if ( z == x )
         {
            if ( string_get_function_param( node_name, source, TAG_NODE, &first_char, &last_char  ) )
            {
               get_entries_by_name( get_root(), node_name, entries );
               string_replace( tmp, source, first_char, last_char+1, "" );
               source = tmp;
            }
            else break;
         }
         // PLAYLIST
         else
         {
            if ( string_get_function_param( node_name, source, TAG_PLAYLIST, &first_char, &last_char  ) )
            {              
               get_playlist_entries_by_name( node_name, entries );
               string_replace( tmp, source, first_char, last_char+1, "" );
               source = tmp;
            }
            else break;
         }

         // AND
         if ( m == a )
         {
            string_remove_tag_simple( tmp, source, TAG_AND );
            source = tmp;

            for ( int i = results.get_count() - 1; i >= 0 ; i-- )
            {
               if ( !entries.have_item( results[i] ) )
               {
                  metadb_handle_ptr h = results.remove_by_idx( i );
                  //h->handle_release();                  
               }      			
            }
         }
         // OR
         else if ( m == b )
         {
            string_remove_tag_simple( tmp, source, TAG_OR );
            source = tmp;

            for ( int i = 0; i < entries.get_count(); i++ )
            {
               if ( !results.have_item( entries[i] ) )
               {
                  results.add_item( entries[i] );
               }
            }
         }
         // NOT
         else 
         {
            string_remove_tag_simple( tmp, source, TAG_NOT );
            source = tmp;
            for ( int i = results.get_count() - 1; i >= 0 ; i-- )
            {
               if ( entries.have_item( results[i] ) )
               {
                  metadb_handle_ptr h = results.remove_by_idx(i);
                  //h->handle_release();
               }
            }
         }

         // entries.delete_all();
      }

      unsigned count = results.get_count();

      pfc::string8 query_criteria = str;

      //query_criteria.trim_leading_whitespace();

      // console::printf( "criteria: %s", (const char * ) query_criteria );

      if ( !is_empty_string( query_criteria ) ) 
      {
         // console::printf( "not empty" );

         if ( m_filter.init( query_criteria ) )
         {
            bit_array_bittable mask( count );
            unsigned n;
            for( n = 0; n < count; n++ )
            {
               bool val = m_filter.test( results[n] );
               mask.set( n, val );
            }

            //results.filter_mask_del( mask );
            results.filter_mask( mask );
         }
      }
   }

   void trim_matches( metadb_handle_list & list )
   {
      if ( m_query_limit == 0)
         return;

      if ( m_limit_criteria >= MAX_CRIT_SUBFOLDER )
         return;

      unsigned count = list.get_count();
      unsigned i;

      bit_array_bittable mask( count );

      for ( i = 0; i < count; i++ )
      {
         mask.set( i, false );
      }

      __int64 size = 0;
      double length = 0.0;

      for ( i = 0; i < count; i++ )
      {
         unsigned m = m_reverse_populate_query ?  count - i - 1 : i;

         size += list[m]->get_filesize();
         length += list[m]->get_length();

         if ((m_limit_criteria == MAX_CRIT_TRACKS) && (i>=m_query_limit))
            break;
         if ((m_limit_criteria == MAX_CRIT_LENGTH) && (length>m_query_limit*60))
            break;
         if ((m_limit_criteria == MAX_CRIT_SIZE) && (size>m_query_limit*1024*1024))
            break;

         mask.set(m, true);
      }

      list.filter_mask( mask );
   }

   /*
   void clear_children()
   {
   clear_subtree();		
   m_children.delete_all();
   }
   */



   virtual void apply_format_and_add( const pfc::list_base_const_t<metadb_handle_ptr> & list, const pfc::string8 & query_display, bool reverse )
   {
      pfc::string8 display_format;
      query_ready_string delimited_string;

      g_format_hook->set_node( this );

      if ( strstr( query_display, META_QUERY_START ) )
      {
         for ( int n = 0; n < list.get_count(); n++ )
         {         
            int m = reverse ? list.get_count() - n - 1 : n;

            //metadb_handle_lock handle_lock( list[m] );

            pfc::ptr_list_t<pfc::string8> strings;
            pfc::string8_fastalloc in_format = query_display;

            generate_meta_query_strings( in_format, list[m], strings );

            for ( int ix = 0; ix < strings.get_count(); ix++ )
            {
               service_ptr_t<titleformat_object> script;
               static_api_ptr_t<titleformat_compiler>()->compile_safe(script,strings[ix]->get_ptr());

               // list[m]->format_title( g_format_hook, display_format, script, NULL );
               list[m]->format_title( NULL, display_format, script, NULL );
               delimited_string.convert( display_format );
               add_delimited_child( delimited_string, list[m] );
            }

            strings.delete_all();
         }       
      }
      else
      {
         service_ptr_t<titleformat_object> script;

         if ( static_api_ptr_t<titleformat_compiler>()->compile(script,query_display) )
         {
            for ( int n = 0; n < list.get_count(); n++ )
            {
               int m = reverse ? list.get_count() - n - 1 : n;
               // list[m]->format_title( g_format_hook, display_format, script, NULL );
               list[m]->format_title( NULL, display_format, script, NULL );
               delimited_string.convert( display_format );
               add_delimited_child( delimited_string, list[m] );
            }
         }
         else
         {
            console::warning( "error in query results format" );
         }
      }
   }

   virtual void no_flicker_repopulate()
   {
      HWND wnd = GetParent( m_hwnd_tree );
      SendMessage( wnd, WM_SETREDRAW, FALSE, 0 );
      repopulate();
      refresh_tree_label();
      refresh_subtree();
      SendMessage( wnd, WM_SETREDRAW, TRUE, 0 );
      InvalidateRect( wnd, NULL, TRUE );
   }

   virtual bool repopulate_auto( int reason, WPARAM wp = 0, LPARAM lp = 0 )
   {
      if ( g_refresh_lock )
         return false;

      switch ( reason )
      {
      case repop_queue_change:
         {
            if ( m_refresh_on_new_track )
            {
               if ( strstr(  m_source, TAG_QUEUE ) )
               {
                  no_flicker_repopulate();
                  return true;
               }
            }
         }
         break;

      case repop_playlist_change:           
         if ( strstr( m_source, TAG_PLAYLISTS ) )
         {
            bool ret = false;
            for ( int i = 0; i < m_children.get_count(); i++ )
            {
               if ( m_children[i]->repopulate_auto( reason, wp, lp ) )
                  ret = true;
            }
            return ret;
         }
         else if ( m_refresh_on_new_track )
         {        
            if ( strstr(  m_source, TAG_PLAYLIST ) )              
            {
               pfc::string8 str_playlist;
               static_api_ptr_t<playlist_manager> pm;
               pm->playlist_get_name( wp, str_playlist );

               if ( strstr( m_source, str_playlist ) )
               {
                  no_flicker_repopulate();
                  return true;
               }
            }
         }        
         break;

      case repop_playlists_change:
         if ( m_refresh_on_new_track )
         {
            if ( strstr( m_source, TAG_PLAYLISTS ) )
            {
               no_flicker_repopulate();
               return true;
            }
         }
         break;

      case repop_new_track:
         if ( m_refresh_on_new_track )
         {
            if ( strstr( m_source, TAG_PLAYLISTS ) )
               break;

            if ( strstr( m_source, TAG_PLAYLIST ) )
               break;

            if ( strstr( m_source, TAG_QUEUE ) )
               break;

            no_flicker_repopulate();
            return true;
         }
         break;

      }
      return false;
   }

   virtual void repopulate()
   { 
      /*
      if ( is_hidden() && cfg_refresh_hidden_content == 0 )
      {
      return;
      }
      */

      if ( !is_backgroundable() && !core_api::assert_main_thread() )
      {
         console::printf( "cannot refresh %s in background", (const char *) m_label );
         return;
      }

      
      if ( strstr( m_query_criteria, TAG_PLAYING ) )
      {
         metadb_handle_ptr handle;
         static_api_ptr_t<playback_control> srv;
         
         if ( !srv->get_now_playing( handle ) )
         {
            console::warning( "Not refreshing: Nothing Playing\n");
            return;
         }
      }

      free_children();

      pfc::string8 test_string;
      pfc::string8 query_display;
      
      get_query_format( query_display );
      get_query_string( test_string );


      // @playlists
      if ( strstr( m_source, TAG_PLAYLISTS ) ) 
      {
         static_api_ptr_t<playlist_manager> sw;

         int m = sw->get_playlist_count();

         for ( int n = 0; n < m; n++ )
         {
            pfc::string8 name;

            sw->playlist_get_name( n, name );

            node * ch = add_child( 
               new query_node( 
               name,
               false,
               pfc::string_printf( "%s<%s>", TAG_PLAYLIST, (const char *) name ),
               m_query_criteria, 
               m_query_format
               ) );

            ch->repopulate();
         }
      }
      // @drop query in @default format
      else if ( strstr( m_source, TAG_DROP ) && strstr( query_display, TAG_DEFAULT ) )
      {
         pfc::string8 filename;

         if ( string_get_function_param( filename, m_source, TAG_DROP ) )
         {
            node *dropped = process_dropped_file( filename, true );
         }
      }
      else if ( strstr( m_source, TAG_SCHEME ) )
      {                        
         char * scheme_result_string;
         long scheme_result_length;
          
         g_query_node = this;

         install_scheme_globals();

         Scheme_Object * v = eval_string_or_get_exn_message( (char*)m_query_format.get_ptr() );

         scheme_result_string = scheme_write_to_string( v, &scheme_result_length );
         pfc::string8 s8_result( scheme_result_string, scheme_result_length );
         console::printf( "->%s", s8_result.get_ptr() );

         g_query_node = NULL;

      }
      // normal query
      else     
      {
         metadb_handle_list list;
#ifdef PROFILING
         pfc::hires_timer timer;
         pfc::string8 label;
         get_display_name( label );
         console::printf( "refreshing %s", (const char *) label );
         timer.start();
#endif

         get_matching_handles( test_string, list );

         list.remove_duplicates();

#ifdef PROFILING
         display_elapsed_time( "  get_matching_handles", timer.query() );
         timer.start();
#endif

         sort_handles( list );

#ifdef PROFILING
         display_elapsed_time( "          sort_handles", timer.query() );
         timer.start();
#endif

         trim_matches( list );

#ifdef PROFILING
         display_elapsed_time( "          trim_matches", timer.query() );
         timer.start();
#endif

         apply_format_and_add( list, query_display, m_reverse_populate_query );         
         
#ifdef PROFILING
         display_elapsed_time( "  apply_format_and_add", timer.query() );
#endif
      }

      // trim to x subfolders
      if ( ( m_limit_criteria == MAX_CRIT_SUBFOLDER ) && ( m_query_limit > 0 ) )
      {
         for ( int i = get_num_children() - 1; i >= m_query_limit; i-- )
         {
            m_children.delete_by_idx( i );
         }
      }

      // sort the children if necessary      
      if ( m_sort_after_populate )
      {
         sort( true );
      }
   }

   virtual void write_sexp( FILE * f, int offset = 0 )
   {
      write_query( f, offset, true );
   }

   virtual void write_query( FILE *f, int offset, bool include_children )
   {
      for ( int j = 0; j < offset; j++ )
      {
         fprintf( f, "\t" );
      }

      fprintf( f, "(QUERY %s %d %s %s %s %s %d", 
         (const char *) quoted_string( m_label ),
         m_expanded,
         (const char *) quoted_string( m_source ), 
         (const char *) quoted_string( m_query_criteria ), 
         (const char *) quoted_string( m_query_format ),
         (const char *) quoted_string( m_query_handle_sort_criteria ),
         //(const char *) quoted_string( m_sort_criteria ) 
         m_sort_after_populate
         );

      if ( m_reverse_populate_query )
      {
         fprintf( f, " :REVERSE t");
      }

      if ( m_query_limit )
      {
         switch ( m_limit_criteria )
         {
         case MAX_CRIT_TRACKS:
            fprintf( f, " :MAX-TRACKS %d", m_query_limit );
            break;
         case MAX_CRIT_SIZE:
            fprintf( f, " :MAX-SIZE %d", m_query_limit );
            break;
         case MAX_CRIT_LENGTH:
            fprintf( f, " :MAX-LENGTH %d", m_query_limit );
            break;
         case MAX_CRIT_SUBFOLDER:
            fprintf( f, " :MAX-FOLDERS %d", m_query_limit );
            break;
         }
      }

      pfc::print_guid str_guid( m_guid );
      fprintf( f, " :GUID %s", str_guid.get_ptr() );

      if ( strstr( m_source, TAG_QUEUE ) || ( include_children == false ) )
      {
         // dont write children on queue node
      }
      else
      {
         write_sexp_children( f, offset + 1 );
      }

      fprintf( f, ")\n");
   }

   virtual void reset_tag_variables()
   {
      folder_node::reset_tag_variables();
      m_refresh_on_new_track = false;
   }

   virtual bool process_local_tags( pfc::string_base & out, const char * in )
   {
      bool result = folder_node::process_local_tags( out, in );

      if ( strstr( out, TAG_REFRESH ) )
      {
         result = true;
         m_refresh_on_new_track = true;
         pfc::string8 tmp;

         while ( string_replace( tmp, out, TAG_REFRESH, "" ) >= 0 )
         {
            out.set_string( tmp );
         }
      }
       
      return result;
   }

   virtual int get_default_bitmap_index() { return cfg_bitmap_query; }

   static BOOL CALLBACK queryEditProc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      static query_node *s_query    = NULL;
      static HICON icon_normal      = NULL;
      static HIMAGELIST imagelist   = NULL;
      static pfc::string8 value_string;

      static struct { int item; char * tag; } label_map[] = 
      {
         {IDC_CHECK_NOBROWSE, TAG_NOBROWSE},
         {IDC_CHECK_NOSAVE, TAG_NOSAVE},
         {IDC_CHECK_AUTOREFRESH, TAG_REFRESH}
      };

      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            pfc::string8 display_name;

            s_query = (query_node*) lp;

            s_query->get_display_name( display_name );
            value_string.set_string( s_query->m_label );

            pfc::string8_fastalloc tmp;

            while ( string_remove_tag_complex( tmp, value_string, TAG_ICON ) )
            {
               value_string.set_string( tmp );
            }

            while ( string_remove_tag_simple( tmp, value_string, TAG_REFRESH ) )
            {
               value_string.set_string( tmp );
            }


            /*
            int check_max = sizeof(label_map)/sizeof(label_map[0]);
            for (int i=0;i<check_max;i++)
            {
            if (value_string.find_first(label_map[i].tag)>=0)
            {
            CheckDlgButton(wnd, label_map[i].item, BST_CHECKED);

            while (value_string.remove_tag(label_map[i].tag, false))
            ; // do nothing
            }
            }
            */

            CheckDlgButton( wnd, IDC_CHECK_AUTOREFRESH, s_query->m_refresh_on_new_track ? BST_CHECKED : BST_UNCHECKED );

            uSetDlgItemText(wnd, IDC_LABEL, value_string);

            imagelist = TreeView_GetImageList( s_query->m_hwnd_tree, TVSIL_NORMAL );

			   if ( imagelist )
			   {
				   icon_normal = ImageList_GetIcon( imagelist, s_query->get_bitmap_index(), ILD_TRANSPARENT );
				   SendDlgItemMessage( wnd, IDC_ICON_NORMAL, STM_SETIMAGE, (WPARAM) IMAGE_ICON,(LPARAM) icon_normal );
			   }
   
				   if (s_query->m_query_criteria.length() == 0)
					   uSetDlgItemText(wnd, IDC_QUERY, "");
				   else
					   uSetDlgItemText(wnd, IDC_QUERY, s_query->m_query_criteria);

				   uSetDlgItemText(wnd, IDC_FORMAT, s_query->m_query_format );
               uSetDlgItemText(wnd, IDC_MAX, pfc::string_printf("%d", s_query->m_query_limit));
				   uSetDlgItemText(wnd, IDC_SORT_CRITERIA, s_query->m_query_handle_sort_criteria);
				   //uSetDlgItemText(wnd, IDC_SORT_CRITERIA_DISPLAY, s_query->m_sort_criteria);

               uSetDlgItemText(wnd, IDC_EDIT_SOURCE, s_query->m_source );

				   CheckDlgButton(wnd, IDC_REVERSE, s_query->m_reverse_populate_query ? BST_CHECKED: BST_UNCHECKED);

               CheckDlgButton(wnd, IDC_SORT_AFTER, s_query->m_sort_after_populate ? BST_CHECKED: BST_UNCHECKED);

				   switch (s_query->m_limit_criteria)
				   {
				   case MAX_CRIT_TRACKS:
					   CheckRadioButton(wnd, IDC_TRACKS, IDC_SUBFOLDERS, IDC_TRACKS);
					   break;
				   case MAX_CRIT_LENGTH:
					   CheckRadioButton(wnd, IDC_TRACKS, IDC_SUBFOLDERS, IDC_MINUTES);
					   break;
				   case MAX_CRIT_SIZE:
					   CheckRadioButton(wnd, IDC_TRACKS, IDC_SUBFOLDERS, IDC_MEGS);
					   break;
				   case MAX_CRIT_SUBFOLDER:
					   CheckRadioButton(wnd, IDC_TRACKS, IDC_SUBFOLDERS, IDC_SUBFOLDERS);
					   break;
				   }

               uSetWindowText(wnd, 
                  pfc::string_printf("Query::%s",(const char *)display_name));

				   return 0;
			   }

		   case WM_COMMAND:
			   switch (wp)
			   {
			   case IDC_ICON_RESET:
				   if ( !imagelist ) 
					   break;

				   s_query->m_custom_icon = -1;

				   if ( icon_normal ) 
               {
                  DestroyIcon( icon_normal );
               }

               icon_normal = ImageList_GetIcon( imagelist, s_query->get_bitmap_index(), ILD_TRANSPARENT );	   
				   SendDlgItemMessage( wnd, IDC_ICON_NORMAL, STM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) icon_normal );				   

				   break;
   			
			   case IDC_SET_NORMAL:
				   if ( imagelist )
				   {
					   int res = select_icon( wnd, imagelist );
					   if ( res >= 0 )
					   {
						   if ( icon_normal )
                     {
                        DestroyIcon(icon_normal);
                     }
						   s_query->m_custom_icon = res;
						   icon_normal = ImageList_GetIcon( imagelist, s_query->get_bitmap_index(), ILD_TRANSPARENT );
						   SendDlgItemMessage( wnd, IDC_ICON_NORMAL, STM_SETIMAGE, (WPARAM) IMAGE_ICON,(LPARAM) icon_normal );				
					   }
				   }
				   break;

			   case IDOK:
				   {
					   // g_tree_modified = 1;

					   uGetDlgItemText( wnd, IDC_LABEL, s_query->m_label );

                  
					   if ( s_query->m_custom_icon >= 0 )
					   {
						   s_query->m_label += 
                        pfc::string_printf( "%s<%d>", TAG_ICON, s_query->m_custom_icon );
					   }

                  
                  if ( IsDlgButtonChecked( wnd, IDC_CHECK_AUTOREFRESH ) == BST_CHECKED )
                  {
                     console::printf( "Refresh enabled" );
                     s_query->m_label += TAG_REFRESH;
                  }

                  /*
					   int check_max = sizeof(label_map)/sizeof(label_map[0]);
					   for ( int i = 0; i < check_max; i++ )
					   {
						   if ( IsDlgButtonChecked(wnd, label_map[i].item) == BST_CHECKED )
						   {
							   s_query->m_value += label_map[i].tag;
						   }
					   }
                  */

					   s_query->m_query_criteria.reset();
					   s_query->m_query_format.reset();
					   s_query->m_query_handle_sort_criteria.reset();
					   //s_query->m_sort_criteria.reset();
                  s_query->m_source.reset();

					   uGetDlgItemText( wnd, IDC_QUERY, s_query->m_query_criteria );
					   uGetDlgItemText( wnd, IDC_FORMAT, s_query->m_query_format );			
					   uGetDlgItemText( wnd, IDC_SORT_CRITERIA, s_query->m_query_handle_sort_criteria );
					   //uGetDlgItemText( wnd, IDC_SORT_CRITERIA_DISPLAY, s_query->m_sort_criteria );
                  uGetDlgItemText( wnd, IDC_EDIT_SOURCE, s_query->m_source );

					   s_query->m_reverse_populate_query = ( IsDlgButtonChecked( wnd, IDC_REVERSE ) == BST_CHECKED );
                  s_query->m_sort_after_populate = ( IsDlgButtonChecked( wnd, IDC_SORT_AFTER ) == BST_CHECKED );

					   if ( IsDlgButtonChecked( wnd, IDC_TRACKS ) == BST_CHECKED )
						   s_query->m_limit_criteria = MAX_CRIT_TRACKS;
					   else if ( IsDlgButtonChecked(wnd, IDC_MINUTES ) == BST_CHECKED)
						   s_query->m_limit_criteria = MAX_CRIT_LENGTH;
					   else if ( IsDlgButtonChecked(wnd, IDC_MEGS ) == BST_CHECKED)
						   s_query->m_limit_criteria = MAX_CRIT_SIZE;
					   else if ( IsDlgButtonChecked(wnd, IDC_SUBFOLDERS ) == BST_CHECKED)
						   s_query->m_limit_criteria = MAX_CRIT_SUBFOLDER;

                  pfc::string8 max_string;
					   uGetDlgItemText( wnd, IDC_MAX, max_string );
					   sscanf( max_string, "%d", &s_query->m_query_limit );

                  s_query->refresh_tree_label();

                  if ( !shift_pressed() )
                  {
                     s_query->repopulate();
                     s_query->refresh_subtree();
                  }
				   }
				   /* fall through */
			   case IDCANCEL:
				   EndDialog( wnd, wp ); 
				   break;
			   }
	   }
	   return 0;
   }

   virtual bool edit()
	{
		int res = DialogBoxParam(
         core_api::get_my_instance(), 
         MAKEINTRESOURCE(IDD_QUERY), 
         m_hwnd_tree,
         queryEditProc, 
         (LPARAM) this );

		return res == IDOK;
	}
};

