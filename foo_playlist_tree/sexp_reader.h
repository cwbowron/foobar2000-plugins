#pragma once

node * read_sexp( FILE * );

int is_whitespace( char ch )
{
   if ( ch == ' ' ) return 1;
   if ( ch == '\t') return 1;
   if ( ch == '\r') return 1;
   if ( ch == '\n') return 1;
   if ( ch == '\0') return 1;
   return 0;
}

int get_next_token( FILE * f, pfc::string8 & result )
{
   static char internal_buffer[1024];
   static int buffer_runner = -1;

   if ( ( buffer_runner < 0 ) || ( buffer_runner >= strlen( internal_buffer) ) )
   {
      if ( feof( f ) ) return 0;

      memset( internal_buffer, 0, sizeof(internal_buffer) );
      fgets( internal_buffer, sizeof(internal_buffer), f );
      buffer_runner = 0;
   }

   // zoom thru whitespace
   while ( is_whitespace( internal_buffer[buffer_runner] ) )
   {
      buffer_runner++;

      if ( buffer_runner >= strlen( internal_buffer ) )
      {
         if ( feof( f ) ) return 0;

         memset( internal_buffer, 0, sizeof(internal_buffer) );
         fgets( internal_buffer, sizeof(internal_buffer), f );
         buffer_runner = 0;
      }
   }

   // assume we are not looking at whitespace
   if ( internal_buffer[buffer_runner] == '(' )
   {
      buffer_runner++;
      result.set_string( "(" );
   }
   else if ( internal_buffer[buffer_runner] == ')' )
   {
      buffer_runner++;
      result.set_string( ")" );
   }
   else if ( internal_buffer[buffer_runner] == '\"' )
   {
      // READING STRING
      result.reset();

      buffer_runner++;


      //while ( internal_buffer[buffer_runner] != '\"' )
      for (;;)
      {
         char ch = internal_buffer[buffer_runner];

         if ( ch == '\"' )
         {
            if ( internal_buffer[buffer_runner+1] == '\"')
            {
               buffer_runner++;
            }
            else
            {
               break;
            }
         }

         result.add_byte( ch );
         buffer_runner++;

         if ( buffer_runner >= strlen( internal_buffer ) )
         {
            if ( feof ( f ) ) return 0;

            memset( internal_buffer, 0, sizeof(internal_buffer) );
            fgets( internal_buffer, sizeof(internal_buffer), f );
            buffer_runner = 0;
         }
      }

//      uMessageBox( NULL, result, "Token", MB_OK );

      buffer_runner++;
   }
   else 
   {
      // READING SYMBOL
      int start = buffer_runner;

      while ( 1 )
      {
         if ( is_whitespace( internal_buffer[buffer_runner] ) )
            break;

         if ( internal_buffer[buffer_runner] == ')' )
            break;

         buffer_runner++;

      }

      result.set_string( &internal_buffer[ start ], buffer_runner - start );

      if ( strcmp( result, "nil" ) == 0 )
      {
         result.reset();
      }
   }

#if 0
   FILE * ftrace = fopen( "c:\\temp\\playlist_tree.log", "a" );
   fprintf( ftrace, "position: %d, token: %s\n", ftell(ftrace), (const char *)result );
   fclose( ftrace);
#endif
   // console::info( string_printf( "Token: '%s'", (const char *) result ) );
   return 1;
}

int read_sexp_children( FILE *f, node *parent )
{
   pfc::string8 token;

   get_next_token( f, token );

   if ( strcmp( token, "(" ) != 0 )
   {
      console::warning( pfc::string_printf( "Error reading playlist file near character %d - token %s - expected (", ftell( f ), (const char *)token));
      return -1;
   }

   int count = 0;
   node *n;

   while ( n = read_sexp( f ) )
   {
      parent->add_child( n );
      count++;
   }

   return count;
}

enum { SEXP_TYPE_UNKNOWN, SEXP_TYPE_LEAF, SEXP_TYPE_FOLDER, SEXP_TYPE_QUERY };

node * read_sexp( FILE *f )
{
   pfc::string8 token;

   get_next_token( f, token );

   if ( strcmp( token, ")" ) == 0 )
   {
      return NULL;
   }

   if ( strcmp( token, "(" ) != 0 )
   {
      console::warning( 
         pfc::string_printf( "Error reading playlist tree file near character %d - token '%s' - expected (", 
            ftell( f ), (const char *) token ));
      return NULL;
   }

   get_next_token( f, token );

   int type = SEXP_TYPE_UNKNOWN;

   if ( strcmp( token, "LEAF" ) == 0 ) type = SEXP_TYPE_LEAF;
   else if ( strcmp( token, "FOLDER" ) == 0 ) type = SEXP_TYPE_FOLDER;
   else if ( strcmp( token, "QUERY" ) == 0 ) type = SEXP_TYPE_QUERY;

   if ( !type )
   {
      console::warning( 
         pfc::string_printf( "Error reading playlist tree file near character %d - token %s", 
            ftell( f ), (const char *)token ));
      return NULL;
   }

   pfc::string8 name;
   get_next_token( f, name );
   node * result;

   // leaf
   if ( type == SEXP_TYPE_LEAF ) 
   {
      pfc::string8 path;
      int subsong = 0;

      get_next_token( f, path );

      while ( get_next_token( f, token ) )
      {
         if ( strcmp( token, ")" ) == 0)
         {
            break;
         }
         if ( strcmp( token, ":SUBSONG" ) == 0 )
         {
            get_next_token( f, token );
            subsong = atoi( token );
         }
         else
         {
            console::warning( pfc::string_printf( "Unknown Token: '%s', near character %d", 
               (const char *) token,
               ftell( f )));
         }
      }

      static_api_ptr_t<metadb> db;

      metadb_handle_ptr h;
      db->handle_create( h, make_playable_location( path, subsong ) );
      result = new leaf_node( name, h );

      //h->handle_release();
   }
   else if ( type == SEXP_TYPE_FOLDER )
   {
      get_next_token( f, token );

      result = new folder_node( name, atoi( token ) );

      while ( get_next_token( f, token ) )
      {
         if ( strcmp( token, ")" ) == 0 ) break;

         if ( strcmp( token, ":CONTENTS" ) == 0 )
         {
            read_sexp_children( f, result );
         }
      }
   }
   else if ( type == SEXP_TYPE_QUERY )
   {
      query_node *result = new query_node( name );

      get_next_token( f, token );

      result->m_expanded = atoi( token );

      get_next_token( f, result->m_source );
      get_next_token( f, result->m_query_criteria );
      get_next_token( f, result->m_query_format );
      get_next_token( f, result->m_query_handle_sort_criteria );    
      //get_next_token( f, result->m_sort_criteria );
      get_next_token( f, token );

      result->m_sort_after_populate = ( strcmp( token, "0" ) != 0 );

      while ( get_next_token( f, token ) )
      {
         if ( strcmp( token, ")" ) == 0 ) 
         {
            break;
         }
         else if ( strcmp( token, ":MAX-TRACKS" ) == 0 )
         {
            get_next_token( f, token );
            result->m_query_limit = atoi( token );
            result->m_limit_criteria = MAX_CRIT_TRACKS;
         }
         else if ( strcmp( token, ":MAX-LENGTH" ) == 0 )
         {
            get_next_token( f, token );
            result->m_query_limit = atoi( token );
            result->m_limit_criteria = MAX_CRIT_LENGTH;
         }
         else if ( strcmp( token, ":MAX-SIZE" ) == 0 )
         {
            get_next_token( f, token );
            result->m_query_limit = atoi( token );
            result->m_limit_criteria = MAX_CRIT_SIZE;
         }
         else if ( strcmp( token, ":MAX-FOLDERS" ) == 0 )
         {
            get_next_token( f, token );
            result->m_query_limit = atoi( token );
            result->m_limit_criteria = MAX_CRIT_SUBFOLDER;
         }
         else if ( strcmp( token, ":REVERSE" ) == 0 )
         {
            get_next_token( f, token );

            if ( (strcmp( token, "t" ) == 0 ) || (strcmp( token, "1" ) == 0 ) )
            {
               result->m_reverse_populate_query = 1;
            }
            else
            {
               result->m_reverse_populate_query = 0;
            }
         }
         else if ( strcmp( token, ":CONTENTS" ) == 0 )
         {
            read_sexp_children( f, result );
         }
         else if ( strcmp( token, ":GUID" ) == 0 )
         {
            get_next_token( f, token );
            GUID guid_temp = pfc::GUID_from_text( token );
            
            // console::printf( "GUID = %s", token.get_ptr() );

            if ( guid_temp != pfc::guid_null )
            {
               result->m_guid = guid_temp;
            }
            /*
            else
            {
               console::printf( "Ignoring NULL GUID" );
            }
            */
         }
         else 
         {
            console::warning( 
               pfc::string_printf( "unexpected token %s, near character %d", 
                  (const char *)token, ftell( f ) ) );
         }
      }

      return result;
   }
   else
   {
      console::warning( 
         pfc::string_printf( "Unexpected Token Near character %d", 
            ftell( f ) ) );
   }

   return result;
}


static node * read_sexp_file( const char * path )
{
   FILE * f = fopen( path, "r" );

   if ( !f ) 
   {
      console::warning( pfc::string_printf( "opening %s failed", path ) );
      return NULL;
   }

   return read_sexp( f );
}

static node * do_load_helper( const char * file )
{
	FILE *playlist;
			
   pfc::stringcvt::string_wide_from_utf8 wide_file( file );

	playlist = _wfopen( wide_file, _T("r") );

	if ( playlist == NULL ) 
	{
      console::warning( pfc::string_printf( "Opening File Failed" ) );
		return NULL;
	}

   node * n = read_sexp( playlist );
   
   return n;
}

node * load( pfc::string8 * out_name )
{
   /*
	TCHAR fileName[MAX_PATH];

	TCHAR *filters = 
      _T("Playlist Tree SEXP (*.") _T(PLAYLIST_TREE_EXTENSION) _T(")\0")
      _T("*.") _T(PLAYLIST_TREE_EXTENSION) _T("\0")
		_T("All files (*.*)\0")
      _T("*.*\0")
      _T("\0");

	OPENFILENAME blip;
	memset((void*)&blip, 0, sizeof(blip));
	memset(fileName, 0, sizeof(fileName));
	blip.lpstrFilter = filters;
	blip.lStructSize = sizeof(OPENFILENAME); 
	blip.lpstrFile = fileName;
	blip.nMaxFile = sizeof(fileName)/ sizeof(*fileName); 
	blip.lpstrTitle = _T("Open...");

	blip.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
   */

   pfc::string8 filename;
   if ( get_save_name( filename, false, true ) )
	{
      node * n = do_load_helper( filename );

		if ( out_name )
		{
         out_name->set_string( filename );
		}

		return n;
	}
	return NULL;
}

