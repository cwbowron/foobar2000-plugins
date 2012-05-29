#pragma once

#ifndef META_QUERY_START
#define META_QUERY_START		"%<"
#define META_QUERY_STOP			">%"
#endif

bool string_get_function_param( pfc::string_base & out, const char *in, const char *tag, int *first = NULL, int *last = NULL );


static void string_split( const char *in, const char *delim, pfc::ptr_list_t<pfc::string8> & out )
{
   pfc::string8 str = in;
   t_size start = 0;
   int count = 0;
   int pos;

   start = 0;
   count = 0;

   while ( ( pos = str.find_first( delim, start ) ) >=0 )
   {		
      pfc::string8 *blip = new pfc::string8( str.get_ptr() + start, pos - start );
      count ++;
      start = pos + strlen( delim );
      out.add_item( blip );
   }

   pfc::string8 *last = new pfc::string8( str.get_ptr() + start );
   out.add_item( last );
}

static int string_replace( pfc::string_base & out, const char * in, int start, int end, const char * replacement )
{
   out.reset();

   int max = strlen( in );
   int n;

   if ( start > max ) start = max;

   for ( n = 0; n < start; n++ )
   {
      out.add_byte( in[n] );
   }

   out.add_string( replacement );

   for ( n = end; n < max; n++ )
   {
      out.add_byte( in[n] );
   }
   return true;
}

static int string_replace( pfc::string_base & out, 
                   const char * in, 
                   const char * replaceme, 
                   const char * withme,
                   int start = 0 )
{
   const char * first = strstr( in + start, replaceme );

   if ( first )
   {
      out.set_string( in, first - in );
      out.add_string( withme );
      out.add_string( first + strlen( replaceme ) );
      return first - in + strlen( withme );
   }
   else
   {
      out.set_string( in );
      return -1;
   }
}


static bool string_remove_tag_simple( pfc::string_base & out, const char * in, const char * tag )
{
   return ( string_replace( out, in, tag, "" ) != -1 );
}

static bool string_remove_tag_complex( pfc::string_base & out, const char * in, const char * tag )
{
   int start,end;
   pfc::string8 blip;

   if ( string_get_function_param( blip, in, tag, &start, &end ) )
   {
      string_replace( out, in, start, end+1, "" );
      return true;
   }
   else
   {
      return false;
   }
}

static int find_close_paren( const char * str, int start )
{
   const char delimit_open = '<';
   const char delimit_clse = '>';

	int i;
	int paren_total = 0;

   int len = strlen( str );

	for ( i = start; start < len; i++ )
	{
		if ( str[i] == 0 )
			break;

		if ( str[i] == delimit_open )
		{
			paren_total++;
		}

		if ( str[i] == delimit_clse )
		{
			paren_total--;
			if ( paren_total < 0 )
				return i;
		}
	}
	return -1;
}

static bool string_substr( pfc::string_base & out, const char * in, int start, int end = ~0 )
{
   if ( end == ~0 )
   {
      out.set_string( in + start, end );
   }
   else
   {
      out.set_string( in + start, end - start );
   }
   return true;
}

static bool string_get_function_param( 
   pfc::string_base & out, 
   const char *in,
   const char *tag, 
	int *first , 
   int *last  )
{
	const char *start_ptr = strstr( in, tag );
	
	if ( !start_ptr )
		return false;

   int start = start_ptr - in;
	
	int l_paren = start + strlen( tag ) + 1;
	
	int r_paren = find_close_paren( in, l_paren );

	if ( r_paren < 0 )
	{
      console::printf("Unbalanced Parenthesis '%s' [string_sub::get_function_param]", in );
		return 0;
	}
	
	string_substr( out, in, l_paren, r_paren );

	if ( first ) 
      *first = start;
	if ( last ) 
      *last = r_paren;

	return 1;
}

static pfc::string8 quoted_string( const char * input )
{
   if ( strlen( input ) == 0 )
   {
      return "nil";
   }
   else
   {
      pfc::string8 result = "\"";

      int n, m = strlen( input );
      for ( n = 0; n < m; n++)
      {
         if ( input[n] == '\"' )
         {
            result.add_string( "\"\"" );
         }
         else
         {
            result.add_byte( input[n] );
         }
      }

      result.add_char( '\"' );
      return result;
   }
}

static bool string_make_format_friendly( pfc::string_base & out, const char * in )
{
   bool affected = false;
	static const char replaced_characters[] = "'$%[]()@";

	int i = 0;
   int in_length = strlen( in );

   out.reset();

   for ( i = 0; i < in_length; i++ )
   {
      if ( strchr( replaced_characters, in[i] ) )
      {
         affected = true;
         out.add_string( pfc::string_printf( "$char(%d)", in[i] ) );
      }
      else
      {
         out.add_byte( in[i] );
      }
   }

   return affected;
}


// returns index of next character after replacement or -1 if no replacement
class query_ready_string : public pfc::string8 
{
public:
   query_ready_string() : pfc::string8()
   {
   }

   query_ready_string( const char * in ) : pfc::string8() 
   {
      convert( in );
   }

   void convert( const char * in )
   {
      pfc::string8_fastalloc tmp( in );

      if ( tmp[tmp.get_length()-1] != '|' )
      {
         tmp.add_byte( '|' );
      }

      while ( string_replace( *this, tmp, "||", "|" ) >= 0 )
      {
         tmp.set_string( get_ptr() );
      }

      int n;
      int m = length();

      for ( n = 0; n < m; n++ )
      {
         if ( m_data[n] == '|' )
            m_data[n] = '\0';
      }
   }
};

static bool has_tags( const char * s )
{
   if ( strstr( s, "%" ) )
      return true;
   if ( strstr( s, "$" ) )
      return true;
   return false;
}

static bool generate_meta_query_strings( pfc::string_base & in, metadb_handle_ptr handle, pfc::ptr_list_t<pfc::string8> & strings, bool add_if_empty = false )
{
   if ( !strstr ( in, META_QUERY_START ) )
   {
      strings.add_item( new pfc::string8( in ) );
      return false;
   }
   else
   {
      int s = in.find_first( META_QUERY_START );             
      int e = in.find_first( META_QUERY_STOP );

      if ( s == infinite || e == infinite ) 
      {
         return false;
      }
      else
      {
         pfc::string8_fastalloc t2;
         pfc::string8_fastalloc t1;

         string_substr( t1, in, s + strlen(META_QUERY_START), e );

         if ( strstr( t1, "," ) ) 
         {
            pfc::ptr_list_t<pfc::string8> tags;

            string_split( t1, ",", tags );

            bool res = false;
            for ( int tag_index = 0; tag_index < tags.get_count(); tag_index++ )
            {
               string_replace( t2, in, s, e + 2, 
                  pfc::string_printf( "%s%s%s",
                     META_QUERY_START,
                     tags[tag_index]->get_ptr(),
                     META_QUERY_STOP ) );
              
               if ( generate_meta_query_strings( t2, handle, strings, add_if_empty ) )
               {
                  res = true;
               }
            }
            return res;
         }
         else
         {
            //const file_info * info;
            file_info_impl info;

            handle->get_info( info );
            //handle->get_info_locked( info );

            bool res = false;

            int tag_index = info.meta_find( t1 );

            if ( add_if_empty || tag_index != infinite )
            {
               int max_meta;
               if ( tag_index != infinite )
               {
                  res = true;
                  max_meta = info.meta_enum_value_count( tag_index );                  
               }
               else
               {
                  max_meta = 1;
               }

               for ( int meta_index = 0; meta_index < max_meta; meta_index++ )
               {
                  /*
                  string_replace( t2, in, s, e + 2, 
                     info.meta_enum_value( tag_index, meta_index ) );
                     */
                  string_replace( t2, in, s, e+2, 
                     pfc::string_printf( "$meta(%s,%d)", (const char *)t1, meta_index ) );

                  generate_meta_query_strings( t2, handle, strings, add_if_empty );
               }
            }

            return res;
         }
      }
   }
}
