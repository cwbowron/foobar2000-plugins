

#define scheme_make_handle(h)             scheme_make_cptr(h,g_scheme_handle_type)
#define scheme_make_node(n)               scheme_make_cptr(n,g_scheme_node_type)

#define SCHEME_HANDLEP(o)                 (SCHEME_CPTRP(o)&&(SCHEME_CPTR_TYPE(o)==g_scheme_handle_type))
#define SCHEME_NODEP(o)                   (SCHEME_CPTRP(o)&&(SCHEME_CPTR_TYPE(o)==g_scheme_node_type))

#define GUARANTEE_CPTR(fname, argnum)     GUARANTEE_TYPE (fname, argnum, SCHEME_CPTRP, "cpointer")
#define GUARANTEE_HANDLE(fname, argnum)   GUARANTEE_TYPE (fname, argnum, SCHEME_HANDLEP, "handle")
#define GUARANTEE_NODE(fname, argnum)     GUARANTEE_TYPE (fname, argnum, SCHEME_NODEP, "tree-node")

#define SCHEME_HANDLE_VAL(o)              ((metadb_handle*)SCHEME_CPTR_VAL(o))

#define scheme_boolify(e)                 ((e) ? scheme_true : scheme_false)

/// 
void scm_string_to_utf8( Scheme_Object * s, pfc::string_base & p_out )
{
   static char buf[1024];
   
   if ( SCHEME_CHAR_STRINGP( s ) )
   {
      scheme_utf8_encode_to_buffer( SCHEME_CHAR_STR_VAL(s), SCHEME_CHAR_STRLEN_VAL(s), buf, 1024 );
      p_out.set_string( buf );
   }
   else if ( SCHEME_PATHP( s ) )
   {
      scm_string_to_utf8( scheme_path_to_char_string( s ), p_out );
   }
   else
   {
      long len;
      const char * str = scheme_write_to_string( s, &len );
      scheme_signal_error( "char-string->utf8: Error - expected string, got: %s", str );
   }
}

typedef struct {
   pfc::string8 * str_prompt;
   Scheme_Object * the_list;
   int * response;
} user_list_info;

typedef struct {
   pfc::string8 * str_prompt;
   pfc::string8 * str_response;
} user_string_info;

static BOOL CALLBACK prompt_user_string_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   static user_string_info * p_info = NULL;

   switch( msg )
   {
   case WM_INITDIALOG:
      {
         p_info = (user_string_info*)lp;
         uSetWindowText( GetDlgItem( wnd, IDC_STATIC_PROMPT ), p_info->str_prompt->get_ptr() );
         uSetWindowText( wnd, "Playlist Tree" );
         SetFocus( GetDlgItem( wnd, IDC_EDIT_RESPONSE ) );
      }
      break;

   case WM_COMMAND:
      switch ( wp )
      {
      case IDOK:
         uGetWindowText( GetDlgItem( wnd, IDC_EDIT_RESPONSE ), *(p_info->str_response) );
         /* fall through */
      case IDCANCEL:
         EndDialog( wnd, wp ); 
         break;
      }
   }
   return 0;
}

static BOOL CALLBACK prompt_user_list_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   static user_list_info * p_info = NULL;

   switch( msg )
   {
   case WM_INITDIALOG:
      {
         p_info = (user_list_info*)lp;
         uSetWindowText( GetDlgItem( wnd, IDC_STATIC_PROMPT ), p_info->str_prompt->get_ptr() );
         uSetWindowText( wnd, "Playlist Tree" );

         HWND drop_list = GetDlgItem( wnd, IDC_LIST );
         Scheme_Object * list = p_info->the_list;

         while ( ! SCHEME_NULLP( list ) )
         {
            pfc::string8 str;
            scm_string_to_utf8( SCHEME_CAR(list), str );
            uSendDlgItemMessageText( wnd, IDC_LIST, CB_ADDSTRING, 0, str );
            list = scheme_cdr( list );
         }  

         SetFocus( drop_list );
      }
      break;

   case WM_COMMAND:
      switch ( wp )
      {
      case IDOK:
         *p_info->response = SendMessage( GetDlgItem( wnd, IDC_LIST ), CB_GETCURSEL, 0, 0 );
         /* fall through */
      case IDCANCEL:
         EndDialog( wnd, wp ); 
         break;
      }
   }
   return 0;
}

///
static Scheme_Object *exn_catching_apply, *exn_p, *exn_message;

void init_exn_catching_apply()
{
  if (!exn_catching_apply) {
    char *e = 
      "(lambda (thunk) "
        "(with-handlers ([void (lambda (exn) (cons #f exn))]) "
          "(cons #t (thunk))))";
    /* make sure we have a namespace with the standard bindings: */
    Scheme_Env *env = (Scheme_Env *)scheme_make_namespace(0, NULL);

    scheme_register_extension_global(&exn_catching_apply, sizeof(Scheme_Object *));
    scheme_register_extension_global(&exn_p, sizeof(Scheme_Object *));
    scheme_register_extension_global(&exn_message, sizeof(Scheme_Object *));
    
    exn_catching_apply = scheme_eval_string(e, env);
    exn_p = scheme_lookup_global(scheme_intern_symbol("exn?"), env);
    exn_message = scheme_lookup_global(scheme_intern_symbol("exn-message"), env);
  }
}

/* This function applies a thunk, returning the Scheme value if there's no exception, 
   otherwise returning NULL and setting *exn to the raised value (usually an exn 
   structure). */
 Scheme_Object *_apply_thunk_catch_exceptions(Scheme_Object *f, Scheme_Object **exn)
{
  Scheme_Object *v;

  init_exn_catching_apply();
  
  v = _scheme_apply(exn_catching_apply, 1, &f);
  /* v is a pair: (cons #t value) or (cons #f exn) */

  if (SCHEME_TRUEP(SCHEME_CAR(v)))
    return SCHEME_CDR(v);
  else {
    *exn = SCHEME_CDR(v);
    return NULL;
  }
}

 Scheme_Object *extract_exn_message(Scheme_Object *v)
{
  init_exn_catching_apply();

  if (SCHEME_TRUEP(_scheme_apply(exn_p, 1, &v)))
    return _scheme_apply(exn_message, 1, &v);
  else
    return NULL; /* Not an exn structure */
}

static Scheme_Object *do_eval(void *s, int noargc, Scheme_Object **noargv )
{  
   // return scheme_eval_string((char *)s, scheme_get_env(scheme_current_config()) );
   return scheme_eval_string_all((char *)s, scheme_get_env(scheme_current_config()), 1 );
}

static Scheme_Object *eval_string_or_get_exn_message(char *s)
{
  Scheme_Object *v, *exn;

  v = _apply_thunk_catch_exceptions( (Scheme_Object*)scheme_make_closed_prim(do_eval, s), &exn);
  /* Got a value? */
  if (v)
    return v;

  v = extract_exn_message(exn);
  /* Got an exn? */
  if (v)
    return v;

  /* `raise' was called on some arbitrary value */
  return exn;
}
///

static Scheme_Object * scm_console_print( int argc, Scheme_Object *argv[] )
{      
   long len;
   
   for ( int n = 0; n < argc; n++ )
   {               
      const char * str = scheme_display_to_string( argv[n], &len );
      pfc::string8 s( str, len );
      console::print( s );   
   }
   return scheme_void;
}

Scheme_Object * scm_now_playing( int argc, Scheme_Object *argv[] )
{
   metadb_handle_ptr handle;
   static_api_ptr_t<playback_control> srv;
   
   if ( srv->get_now_playing( handle ) )
   {
      return scheme_make_handle( handle.get_ptr() );
   }
   return scheme_false;
}

Scheme_Object * scm_prompt_user_string( int argc, Scheme_Object *argv[] )
{
   GUARANTEE_CHAR_STRING( "prompt-user-string", 0 );
   
   pfc::string8 str_prompt;
   pfc::string8 str_response;
   scm_string_to_utf8( argv[0], str_prompt );

   user_string_info info = { &str_prompt, &str_response };

   uDialogBox( IDD_SCHEME_STRING, core_api::get_main_window(), prompt_user_string_dialog_proc, (LPARAM) &info );
   return scheme_make_locale_string( str_response.get_ptr() );
}

Scheme_Object * scm_prompt_user_list( int argc, Scheme_Object *argv[] )
{
   GUARANTEE_CHAR_STRING( "prompt-user-list", 0 );
   GUARANTEE_PAIR( "prompt-user-list", 1 );
   
   pfc::string8 str_prompt;
   int response = -1;
   scm_string_to_utf8( argv[0], str_prompt );

   user_list_info info = { &str_prompt, argv[1], &response };

   uDialogBox( IDD_SCHEME_DROPLIST, core_api::get_main_window(), prompt_user_list_dialog_proc, (LPARAM) &info );

   return scheme_make_integer( response );
}

Scheme_Object * scm_format_title( int argc, Scheme_Object *argv[] )
{
   GUARANTEE_CPTR( "format-title", 0 );
   GUARANTEE_CHAR_STRING( "format-title", 1 );

   pfc::string8 str;
   pfc::string8 formatted;

   metadb_handle * h;

   scm_string_to_utf8( argv[1], str );
   h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );

   h->format_title_legacy( NULL, formatted, str, NULL );
   return scheme_make_locale_string( formatted.get_ptr() );
}

Scheme_Object * scm_for_each_db_entry( int argc, Scheme_Object *argv[] )
{
   GUARANTEE_PROCEDURE( "for-each-db-entry", 0 );
       
   search_tools::search_filter filter;
   pfc::string8 filter_string;
   pfc::string8 sort_string;
   bool apply_filter_p = false;
   bool apply_sort_p = false;

   if ( argc > 1 )
   {
      if ( !SCHEME_FALSEP( argv[1] ) )
      {
         GUARANTEE_CHAR_STRING( "for-each-db-entry", 1 );
         apply_filter_p = true;
         scm_string_to_utf8( argv[1], filter_string );
      }

      if ( argc > 2 )
      {
         if ( !SCHEME_FALSEP( argv[2] ) )
         {
            GUARANTEE_CHAR_STRING( "for-each-db-entry", 2 );
            apply_sort_p = true;
            scm_string_to_utf8( argv[2], sort_string );
         }
      }
   }

   metadb_handle_list handles;
   static_api_ptr_t<library_manager> db;         
   db->get_all_items( handles );

   if ( apply_filter_p )
   {
      console::printf( "for-each-db-entry: Filter by: %s", filter_string.get_ptr() );
      filter.init( filter_string );
   }

   if ( apply_sort_p )
   {
      console::printf( "for-each-db-entry: Sort by: %s", sort_string.get_ptr() );
      handles.sort_by_format( sort_string, NULL );
   }

   int m = handles.get_count();

   for ( int n = 0; n < m; n++ )
   {
      if ( !apply_filter_p || filter.test( handles[n] ) )
      {
         Scheme_Object * p = scheme_make_handle( handles[n].get_ptr() );
         Scheme_Object * ar[1] = { p };
         scheme_apply( argv[0], 1, ar );
      }
   }   
   return scheme_void;
}

Scheme_Object * scm_for_each_playlist( int argc, Scheme_Object **argv )
{  
   GUARANTEE_PROCEDURE( "for-each-playlist", 0 );

   static_api_ptr_t<playlist_manager> man;

   for ( int n = 0; n <man->get_playlist_count(); n++ )
   {
      pfc::string8 p_name_string;
      man->playlist_get_name( n, p_name_string );
      Scheme_Object * p_name = scheme_make_locale_string( p_name_string );
      scheme_apply( argv[0], 1, &p_name );
   }
   return scheme_void;
}

Scheme_Object * scm_for_each_playlist_entry( int argc, Scheme_Object **argv )
{
   GUARANTEE_PROCEDURE( "for-each-playlist-entry", 1 );

   static_api_ptr_t<playlist_manager> man;
   t_size p_index = 0;

   if ( SCHEME_INTP( argv[0] ) )
   {
      p_index = SCHEME_INT_VAL( argv[0] );
   }
   else
   {
      GUARANTEE_TYPE ("for-each-playlist-entry", 0, SCHEME_CHAR_STRINGP, "string or index");
      pfc::string8 playlist_string;
      scm_string_to_utf8( argv[0], playlist_string );
      p_index = man->find_playlist( playlist_string, infinite );
   }

   if ( p_index != infinite )
   {
      metadb_handle_list handles;
      man->playlist_get_all_items( p_index, handles );

      for (int n = 0; n < handles.get_count(); n++ )
      {
         Scheme_Object * p = scheme_make_handle( handles[n].get_ptr() );
         scheme_apply( argv[1], 1, &p );
      }

      return scheme_true;
   }
   
   return scheme_false;   
}

Scheme_Object * scm_for_each_node_entry( int argc, Scheme_Object **argv )
{
   GUARANTEE_CHAR_STRING( "for-each-node-entry", 0 );
   GUARANTEE_PROCEDURE( "for-each-node-entry", 1 );

   pfc::string8 node_name;
   scm_string_to_utf8( argv[0], node_name );
   metadb_handle_list handles;

   if ( get_entries_by_name( g_query_node->get_root(), node_name, handles ) )
   {
      for (int n = 0; n < handles.get_count(); n++ )
      {
         Scheme_Object * p = scheme_make_handle( handles[n].get_ptr() );
         scheme_apply( argv[1], 1, &p );
      }
      return scheme_true;
   }
   
   scheme_signal_error( "Node not found: %s", node_name.get_ptr() );
   return scheme_false;   
}

Scheme_Object * scm_for_each_queue_entry( int argc, Scheme_Object **argv )
{
   GUARANTEE_PROCEDURE( "for-each-queue-entry", 0 );

   metadb_handle_list handles;

   if ( get_entries_queue( handles ) )
   {
      for (int n = 0; n < handles.get_count(); n++ )
      {
         Scheme_Object * p = scheme_make_handle( handles[n].get_ptr() );
         scheme_apply( argv[0], 1, &p );
      }
      return scheme_true;
   }
   
   return scheme_false;   
}

Scheme_Object * scm_meta_list( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_HANDLE( "meta-list", 0 );
   GUARANTEE_CHAR_STRING( "meta-list", 1 );
   
   metadb_handle * h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );
   Scheme_Object * the_list = scheme_null;

   pfc::string8 tag;
   scm_string_to_utf8( argv[1], tag );

   file_info_impl info;
   h->get_info( info );

   if ( info.meta_exists( tag ) )
   {
      for ( int n = 0; n < info.meta_get_count_by_name( tag ); n++ )
      {
         Scheme_Object * item = scheme_make_locale_string( info.meta_get( tag, n ) );
         the_list = scheme_make_pair( item, the_list );
      }
   }

   return the_list;
}

Scheme_Object * scm_get_file_size( int argc, Scheme_Object **argv )
{
   GUARANTEE_HANDLE( "get-file-size", 0 );
   metadb_handle * h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );
   return scheme_make_integer( h->get_filesize() );
}

Scheme_Object * scm_get_length( int argc, Scheme_Object **argv )
{
   GUARANTEE_HANDLE( "get-length", 0 );
   metadb_handle * h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );
   return scheme_make_integer( h->get_length() );
}

Scheme_Object * scm_get_path( int argc, Scheme_Object **argv )
{
   GUARANTEE_HANDLE( "get-path", 0 );
   metadb_handle * h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );
   return scheme_make_locale_string( h->get_path() );
}

Scheme_Object * scm_get_subsong_index( int argc, Scheme_Object **argv )
{
   GUARANTEE_HANDLE( "get-subsong-index", 0 );
   metadb_handle * h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );
   return scheme_make_integer( h->get_subsong_index() );
}


Scheme_Object * scm_add_node( int argc, Scheme_Object *argv[] )
{
   GUARANTEE_HANDLE( "add-node", 0 ); 
   GUARANTEE_PAIR( "add-node", 1 );
   
   metadb_handle * h = (metadb_handle*)SCHEME_CPTR_VAL( argv[0] );
   pfc::string8 fmt;
   query_ready_string delimited_string;

   Scheme_Object * list = argv[1];

   while ( ! SCHEME_NULLP( list ) )
   {
      pfc::string8 folder;
      scm_string_to_utf8( SCHEME_CAR(list), folder );
      fmt.add_string( folder );
      fmt.add_string( "|" );

      list = scheme_cdr( list );
   }  

   delimited_string.convert( fmt );

   if ( !g_query_node )
   {
      scheme_signal_error( "add-node called outside of query node" );
   }

   g_query_node->add_delimited_child( delimited_string, h );

   return scheme_void;   
}

// argv[0] = LABEL : string
// argv[1] = SOURCE : string
// argv[2] = CRITERIA : string
// argv[3] = FORMAT : string
// argv[4] = POPULATION ORDER : string [optional]
// argv[5] = SORT BY DISPLAY : boolean [optional] 
// argv[6] = maximum value : integer [optional]
// argv[7] = maximum type : integer [optional]
Scheme_Object *scm_add_query_node( int argc, Scheme_Object *argv[] )
{  
   GUARANTEE_CHAR_STRING( "add-query-node", 0 );
   GUARANTEE_CHAR_STRING( "add-query-node", 1 );
   GUARANTEE_CHAR_STRING( "add-query-node", 2 );
   GUARANTEE_CHAR_STRING( "add-query-node", 3 );

   if ( argc > 4 )
      GUARANTEE_CHAR_STRING( "add-query-node", 4 );
   if ( argc > 5 )
      GUARANTEE_BOOL( "add-query-node", 5 );
   if ( argc > 6 )
      GUARANTEE_INTEGER( "add-query-node", 6);
   if ( argc > 7 )
      GUARANTEE_INTEGER( "add-query_node", 7 );

   pfc::string8 strings[5];

   for ( int i = 0; i < min( argc, 5 ); i++ )
   {
      scm_string_to_utf8( argv[i], strings[i] );
   }

   bool b_sort_after_pop = false;
   if ( argc > 5 && SCHEME_TRUEP( argv[5] ) )
   {
      b_sort_after_pop = true;
   }
   
   if ( !g_query_node )
   {
      scheme_signal_error( "add-node called outside of query node" );
   }

   query_node * n = new query_node( strings[0], false, strings[1], strings[2], strings[3], strings[4], b_sort_after_pop );
   g_query_node->add_child( n );

   if ( argc > 6 )
   {
      int max_val = SCHEME_INT_VAL( argv[6] );
      int max_type = (argc > 7) ? SCHEME_INT_VAL( argv[7] ) : 0;

      n->m_limit_criteria   = max_type;
      n->m_query_limit      = max_val;
   }

   return scheme_make_node( n );
}

Scheme_Object * scm_refresh_query( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_NODE( "refresh-query", 0 );

   query_node * n = (query_node*) SCHEME_CPTR_VAL(argv[0]);
   n->repopulate();
   return scheme_true;
}

/*
Scheme_Object * scm_systemtime_to_list( SYSTEMTIME * sys )
{
   Scheme_Object * the_list = scheme_null;
   the_list = scheme_make_pair( scheme_make_integer( sys->wSecond ), the_list );
   the_list = scheme_make_pair( scheme_make_integer( sys->wMinute ), the_list );
   the_list = scheme_make_pair( scheme_make_integer( sys->wHour ), the_list );
   the_list = scheme_make_pair( scheme_make_integer( sys->wDay ), the_list );
   the_list = scheme_make_pair( scheme_make_integer( sys->wMonth ), the_list );
   the_list = scheme_make_pair( scheme_make_integer( sys->wYear ), the_list );
   return the_list;
}
*/

// copied from mzscheme
static time_t convert_date(const FILETIME *ft)
{
  LONGLONG l, delta;
  FILETIME ft2;
  SYSTEMTIME st;
  TIME_ZONE_INFORMATION tz;

  /* FindFirstFile incorrectly shifts for daylight saving. It
     subtracts an hour to get to UTC when daylight saving is in effect
     now, even when daylight saving was not in effect when the file
     was saved.  Counteract the difference. There's a race condition
     here, because we might cross the daylight-saving boundary between
     the time that FindFirstFile runs and GetTimeZoneInformation
     runs. Cross your fingers... */
  FileTimeToLocalFileTime(ft, &ft2);
  FileTimeToSystemTime(&ft2, &st);
  
  delta = 0;
  if (GetTimeZoneInformation(&tz) == TIME_ZONE_ID_DAYLIGHT) {
    /* Daylight saving is in effect now, so there may be a bad
       shift. Check the file's date. */
    int start_day_of_month, end_day_of_month, first_day_of_week, diff, end_shift;

    /* Valid only when the months match: */
    first_day_of_week = (st.wDayOfWeek - (st.wDay - 1 - (((st.wDay - 1) / 7) * 7)));
    if (first_day_of_week < 0)
      first_day_of_week += 7;

    diff = (tz.DaylightDate.wDayOfWeek - first_day_of_week);
    if (diff < 0)
      diff += 7;
    start_day_of_month = 1 + (((tz.DaylightDate.wDay - 1) * 7)
			      + diff);
	
    diff = (tz.StandardDate.wDayOfWeek - first_day_of_week);
    if (diff < 0)
      diff += 7;
    end_day_of_month = 1 + (((tz.StandardDate.wDay - 1) * 7)
			    + diff);

    /* Count ambigious range (when the clock goes back) as
       in standard time. We assume that subtracting the 
       ambiguous range does not go back into the previous day,
       and that the shift is a multiple of an hour. */
    end_shift = ((tz.StandardBias - tz.DaylightBias) / 60);

    if ((st.wMonth < tz.DaylightDate.wMonth)
	|| ((st.wMonth == tz.DaylightDate.wMonth)
	    && ((st.wDay < start_day_of_month)
		|| ((st.wDay == start_day_of_month)
		    && (st.wHour < tz.DaylightDate.wHour))))) {
      /* Daylight saving had not yet started. */
      delta = ((tz.StandardBias - tz.DaylightBias) * 60);
    } else if ((st.wMonth > tz.StandardDate.wMonth)
	       || ((st.wMonth == tz.StandardDate.wMonth)
		   && ((st.wDay > end_day_of_month)
		       || ((st.wDay == end_day_of_month)
			   && (st.wHour >= (tz.StandardDate.wHour
					    - end_shift)))))) {
      /* Daylight saving was already over. */
      delta = ((tz.StandardBias - tz.DaylightBias) * 60);
    }
  }

  l = ((((LONGLONG)ft->dwHighDateTime << 32) | ft->dwLowDateTime)
       - (((LONGLONG)0x019DB1DE << 32) | 0xD53E8000));
  l /= 10000000;
  l += delta;

  return (time_t)l;
}

Scheme_Object * scm_file_information( int argc, Scheme_Object * argv[], const char * fname, int field )
{
   GUARANTEE_TYPE( fname, 0, SCHEME_PATH_STRINGP, "path or string" );

   pfc::string8 filename;  
   scm_string_to_utf8( argv[0], filename );
   
   HANDLE file_handle = uCreateFile( filename, FILE_READ_ATTRIBUTES,
      FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

   if ( file_handle != INVALID_HANDLE_VALUE )
   {
      FILETIME ft;

      if ( field == 0 )
      {
         GetFileTime( file_handle, &ft, NULL, NULL );
      }
      else if ( field == 1 )
      {
         GetFileTime( file_handle, NULL, &ft, NULL );
      }
      else
      {
         GetFileTime( file_handle, NULL, NULL, &ft );
      }

      CloseHandle( file_handle );

      return scheme_make_integer_value_from_unsigned( convert_date( &ft ) );
   }
   else
   {
      return scheme_false;
   }
}

Scheme_Object * scm_file_last_write( int argc, Scheme_Object * argv[] )
{
   return scm_file_information( argc, argv, "file-last-write", 2 );
}

Scheme_Object * scm_file_last_access( int argc, Scheme_Object * argv[] )
{
   return scm_file_information( argc, argv, "file-last-access", 1 );
}

Scheme_Object * scm_file_creation( int argc, Scheme_Object * argv[] )
{
   return scm_file_information( argc, argv, "file-creation", 0 );
}

Scheme_Object * scm_handle_apply( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_PROCEDURE( "handle-apply", 0 );
   GUARANTEE_CHAR_STRING( "handle-apply", 1 );
 

   int subsong_index = 0;

   if ( argc > 2 )
   {
      GUARANTEE_INTEGER( "handle-apply", 2 );
      subsong_index = SCHEME_INT_VAL(argv[2]);
   }

   pfc::string8 path;
   scm_string_to_utf8( argv[1], path );
   metadb_handle_ptr handle;

   static_api_ptr_t<metadb> db;         
   db->handle_create( handle, make_playable_location( file_path_canonical(path), subsong_index ) );

   if ( handle.is_valid() )
   {   
      Scheme_Object * p = scheme_make_handle( handle.get_ptr() );
      return scheme_apply( argv[0], 1, &p );
   }
   return scheme_false;
}

Scheme_Object * scm_add_to_playlist( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_HANDLE( "add-to-playlist", 0 );

   static_api_ptr_t<playlist_manager> pm;
   int nPlaylist = pm->get_active_playlist();

   if ( argc > 1 )
   {
      nPlaylist = SCHEME_INT_VAL( argv[1] );
      GUARANTEE_INTEGER( "add-to-playlist", 1 );
   }

   metadb_handle * h = SCHEME_HANDLE_VAL( argv[0] );

   t_size index = infinite;
   if ( argc > 2 )
   {
      GUARANTEE_INTEGER( "add-to-playlist", 2 );
      index = SCHEME_INT_VAL( argv[2] );
   }  

   metadb_handle_list list;
   list.add_item( h );
   
   bit_array_false b;
   
   return scheme_make_integer( pm->playlist_insert_items( nPlaylist, index, list, b ) );
}

Scheme_Object * scm_clear_playlist( int argc, Scheme_Object * argv[] )
{
   static_api_ptr_t<playlist_manager> pm;
   t_size t_index = pm->get_active_playlist();

   if ( argc > 0 )
   {
      GUARANTEE_INTEGER( "clear-playlist", 0 );
      t_index = SCHEME_INT_VAL( argv[0] );
   }

   pm->playlist_clear( t_index );
   return scheme_void;
}

Scheme_Object * scm_find_or_create_playlist( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_CHAR_STRING( "find-or-create-playlist", 0 );

   pfc::string8 str;
   scm_string_to_utf8( argv[0], str );
   static_api_ptr_t<playlist_manager> pm;
   t_size index = pm->find_or_create_playlist( str, ~0 );
   return scheme_make_integer( index );
}

Scheme_Object * scm_activate_playlist( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_INTEGER( "activate-playlist", 0 );
   static_api_ptr_t<playlist_manager> pm;
   pm->set_active_playlist( SCHEME_INT_VAL( argv[0] ) );
   return scheme_void;
}

Scheme_Object * scm_play_from_playlist( int argc, Scheme_Object * argv[] )
{
   static_api_ptr_t<playlist_manager> pm;
   static_api_ptr_t<playback_control> pc;
   t_size t_playlist = pm->get_active_playlist();

   if ( argc > 0 )
   {
      GUARANTEE_INTEGER( "play-from-playlist", 0 );
      t_playlist = SCHEME_INT_VAL( argv[0] );
   }

   t_size t_index = 0;
   if ( argc > 1 )
   {
      GUARANTEE_INTEGER( "play-from-playlist", 1 );
      t_index = SCHEME_INT_VAL( argv[1] );
   }

   pm->set_playing_playlist( t_playlist );
   if ( t_index > 0 )
   {
      pm->playlist_set_focus_item( t_playlist, t_index );
      pc->start( playback_control::track_command_settrack );
   }
   else
   {
      pc->start();
   }
   return scheme_void;
}

Scheme_Object * scm_active_playlist( int argc, Scheme_Object * argv[] )
{
   static_api_ptr_t<playlist_manager> pm;
   return scheme_make_integer( pm->get_active_playlist() );
}

Scheme_Object * scm_playing_playlist( int argc, Scheme_Object * argv[] )
{
   static_api_ptr_t<playlist_manager> pm;
   return scheme_make_integer( pm->get_playing_playlist() );
}

Scheme_Object * scm_mainmenu( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_CHAR_STRING( "mainmenu", 0 );
   
   GUID guid_cmd;
   pfc::string8 str_cmd;
   scm_string_to_utf8( argv[0], str_cmd );

   if ( mainmenu_commands::g_find_by_name( str_cmd, guid_cmd ) )
   {
      mainmenu_commands::g_execute( guid_cmd );
      return scheme_true;
   }
   else
   {
      return scheme_false;
   }
}

void scm_scheme_list_to_metadb_list( Scheme_Object * the_list, metadb_handle_list & handles )
{
   if ( !SCHEME_NULLP( the_list ) )
   {
      Scheme_Object * car = scheme_car( the_list );
      if ( !SCHEME_HANDLEP( car ) )
      {
         scheme_wrong_type( "scheme-list->metadb-list", "metadb-handle", -1, 0, &car );
      }
      metadb_handle * h = SCHEME_HANDLE_VAL( car );
      handles.add_item( h );
      scm_scheme_list_to_metadb_list( scheme_cdr( the_list ), handles );
   }
}

Scheme_Object * scm_contextmenu( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_CHAR_STRING( "contextmenu", 0 );
   GUARANTEE_PAIR( "contextmenu", 1 );

   pfc::string8 str_cmd;
   scm_string_to_utf8( argv[0], str_cmd );
   metadb_handle_list handles;
   
   scm_scheme_list_to_metadb_list( argv[1], handles );

   return run_context_command( str_cmd, &handles ) ? scheme_true : scheme_false;
}

Scheme_Object * scm_playlist_contextmenu( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_CHAR_STRING( "playlist-contextmenu", 0 );
   pfc::string8 str_cmd;
   scm_string_to_utf8( argv[0], str_cmd );

   static_api_ptr_t<playlist_manager> pm;
   t_size t_playlist = pm->get_active_playlist();

   if ( argc > 1 )
   {
      GUARANTEE_INTEGER( "playlist-contextmenu", 1 );
      t_playlist = SCHEME_INT_VAL( argv[1] );
   }

   metadb_handle_list handles;
   pm->playlist_get_all_items( t_playlist, handles );
   
   return run_context_command( str_cmd, &handles ) ? scheme_true : scheme_false;
}

Scheme_Object * scm_in_libraryp( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_HANDLE( "in-library?", 0 );
   metadb_handle * h = SCHEME_HANDLE_VAL( argv[0] );

   static_api_ptr_t<library_manager> lib;

   return lib->is_item_in_library( h ) ? scheme_true : scheme_false;
}

Scheme_Object * scm_test_handle( int argc, Scheme_Object * argv[] )
{
   GUARANTEE_HANDLE( "test-handle", 0 );
   GUARANTEE_CHAR_STRING( "test-handle", 1 );

   search_tools::search_filter filter;
   pfc::string8 str_filter;
   scm_string_to_utf8( argv[1], str_filter );
   metadb_handle * h = SCHEME_HANDLE_VAL( argv[0] );

   filter.init( str_filter );
   
   return filter.test( h ) ? scheme_true : scheme_false;
}

// for the output port
long playlist_tree_write_bytes_fun( Scheme_Output_Port * port, const char * buffer, long offset, long size, int rarely_block, int enable_break )
{
   pfc::string8 utf8( buffer + offset, size );
   console::print( utf8 );
   return size;
}
int playlist_tree_char_ready_fun( Scheme_Output_Port * port )
{
   return 1;
}
void playlist_tree_close_fun( Scheme_Output_Port *port )
{
}
void playlist_tree_need_wakeup_fun( Scheme_Output_Port *port, void *fds )
{
}
// done with the output port

void scm_eval_startup()
{
   char * scheme_result_string;
   long scheme_result_length;

   Scheme_Object * v = eval_string_or_get_exn_message( (char*)cfg_scheme_startup.get_ptr() );
   // Scheme_Object * scheme_string = scheme_make_locale_string( cfg_scheme_startup );
   // Scheme_Object * v = scheme_eval_multi( scheme_string, g_scheme_environment );
   // Scheme_Object * v = scheme_eval_string_all( cfg_scheme_startup, g_scheme_environment, 1 );
   scheme_result_string = scheme_write_to_string( v, &scheme_result_length );
   pfc::string8 s8_result( scheme_result_string, scheme_result_length );
   console::printf( "->%s", s8_result.get_ptr() );
}

static Scheme_Output_Port * g_console_output_port = NULL;

#define ADD_PRIMITIVE(fn, sname,mina,maxa)   (scheme_add_global( sname, scheme_make_prim_w_arity(fn,sname,mina,maxa),g_scheme_environment))

void install_scheme_globals()
{  
   if ( !g_scheme_installed )
   {
      g_scheme_handle_type = scheme_make_locale_string( "handle" );
      g_scheme_node_type = scheme_make_locale_string( "tree-node" );

      g_console_output_port = scheme_make_output_port( scheme_make_locale_string( "console-port"),
         NULL, scheme_make_locale_string( "console"),
         scheme_write_evt_via_write,
         playlist_tree_write_bytes_fun,
         playlist_tree_char_ready_fun,
         playlist_tree_close_fun,
         playlist_tree_need_wakeup_fun,
         NULL,
         NULL, 
         0 );
           
      scheme_set_param( scheme_current_config(), MZCONFIG_OUTPUT_PORT, (Scheme_Object*)g_console_output_port );
      scheme_set_param( scheme_current_config(), MZCONFIG_ERROR_PORT, (Scheme_Object*)g_console_output_port );

      ADD_PRIMITIVE( scm_console_print, "console-print", 1, -1 );
      ADD_PRIMITIVE( scm_now_playing, "now-playing", 0, 0 );
      ADD_PRIMITIVE( scm_format_title, "format-title", 2, 2 );
      ADD_PRIMITIVE( scm_add_node, "add-node", 2, 2 );
      ADD_PRIMITIVE( scm_meta_list, "meta-list", 2, 2 );
      ADD_PRIMITIVE( scm_get_path, "get-path", 1, 1 );
      ADD_PRIMITIVE( scm_get_subsong_index, "get-subsong-index", 1, 1 );      
      ADD_PRIMITIVE( scm_get_length, "get-length", 1, 1 );
      ADD_PRIMITIVE( scm_get_file_size, "get-file-size", 1, 1 );
      ADD_PRIMITIVE( scm_for_each_playlist, "for-each-playlist", 1, 1 );
      ADD_PRIMITIVE( scm_for_each_node_entry, "for-each-node-entry", 2, 2 );
      ADD_PRIMITIVE( scm_for_each_playlist_entry, "for-each-playlist-entry", 2, 2 );
      ADD_PRIMITIVE( scm_for_each_db_entry, "for-each-db-entry", 1, 3 );
      ADD_PRIMITIVE( scm_for_each_queue_entry, "for-each-queue-entry", 1, 3 );

      ADD_PRIMITIVE( scm_add_query_node, "add-query-node", 4, 8 );
      ADD_PRIMITIVE( scm_refresh_query, "refresh-query", 1, 1 );
      ADD_PRIMITIVE( scm_handle_apply, "handle-apply", 2, 3 );  

      // File information
      ADD_PRIMITIVE( scm_file_creation, "file-creation", 1, 1 );  
      ADD_PRIMITIVE( scm_file_last_access, "file-last-access", 1, 1 );  
      ADD_PRIMITIVE( scm_file_last_write, "file-last-write", 1, 1 );  

      // Playlist interaction
      ADD_PRIMITIVE( scm_clear_playlist, "clear-playlist", 0, 1 );
      ADD_PRIMITIVE( scm_add_to_playlist, "add-to-playlist", 1, 3 );
      ADD_PRIMITIVE( scm_find_or_create_playlist, "find-or-create-playlist", 1, 1 );
      ADD_PRIMITIVE( scm_activate_playlist, "activate-playlist", 1, 1 );
      ADD_PRIMITIVE( scm_play_from_playlist, "play-from-playlist", 0, 2 );
      ADD_PRIMITIVE( scm_active_playlist, "active-playlist", 0, 0 );
      ADD_PRIMITIVE( scm_playing_playlist, "playing-playlist", 0, 0 );

      // Menus
      ADD_PRIMITIVE( scm_mainmenu, "mainmenu", 1, 1 );
      ADD_PRIMITIVE( scm_playlist_contextmenu, "playlist-contextmenu", 1, 2 );
      ADD_PRIMITIVE( scm_contextmenu, "contextmenu", 2, 2 );

      ADD_PRIMITIVE( scm_in_libraryp, "in-library?", 1, 1 );
      ADD_PRIMITIVE( scm_test_handle, "test-handle", 2, 2 );

      // user interaction... 
      ADD_PRIMITIVE( scm_prompt_user_string, "prompt-user-string", 1, 1 );
      ADD_PRIMITIVE( scm_prompt_user_list, "prompt-user-list", 2, 2 );

      scheme_add_global( "tree-max-tracks", scheme_make_integer( MAX_CRIT_TRACKS ), g_scheme_environment );
      scheme_add_global( "tree-max-minutes", scheme_make_integer( MAX_CRIT_LENGTH ), g_scheme_environment );
      scheme_add_global( "tree-max-megs", scheme_make_integer( MAX_CRIT_SIZE ), g_scheme_environment );
      scheme_add_global( "tree-max-subfolders", scheme_make_integer( MAX_CRIT_SUBFOLDER ), g_scheme_environment );

      scm_eval_startup();
      g_scheme_installed = true;
   }
}
///
