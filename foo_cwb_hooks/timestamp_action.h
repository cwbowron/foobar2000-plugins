#pragma once

///
static const GUID cfg_var_000 = { 0xb778d58f, 0x813e, 0x4306, { 0xba, 0x2, 0xbb, 0x8a, 0xd7, 0x20, 0x7c, 0xf9 } };
static const GUID cfg_var_001 = { 0xa0bb8d40, 0x38ad, 0x4859, { 0xa5, 0x88, 0xf6, 0xdb, 0x23, 0x1a, 0xa8, 0x4b } };

static cfg_string cfg_last_tag( cfg_var_000, "TIMESTAMP" );
static cfg_string cfg_creation_tag( cfg_var_001, "DATE_CREATED" );
///

void get_current_time( pfc::string_base & p_out )
{
   static CHAR date_string[30];
   SYSTEMTIME st;
   GetLocalTime(&st);
   sprintf(date_string, "%d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay,
	   st.wHour, st.wMinute, st.wSecond);

   p_out.set_string( date_string );
}

bool get_file_creation_time( const playable_location & p_location, pfc::string_base & p_out )
{
   pfc::string8 filename;
   FILETIME creation;
   SYSTEMTIME stUTC, stLocal;

   if ( strstr( p_location.get_path(), "file://") )
   {
      filesystem::g_get_display_path( p_location.get_path(), filename );

      HANDLE file_handle = uCreateFile( filename, FILE_READ_ATTRIBUTES,
         FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

      if ( file_handle != INVALID_HANDLE_VALUE )
      {
         GetFileTime( file_handle, &creation, NULL, NULL );
         CloseHandle( file_handle );
       
         // Convert the last-write time to local time.         
         FileTimeToSystemTime( &creation, &stUTC );
         SystemTimeToTzSpecificLocalTime( NULL, &stUTC, &stLocal );

         static CHAR date_string[30];
   
         sprintf(date_string, "%d-%02d-%02d %02d:%02d:%02d", stLocal.wYear, stLocal.wMonth, stLocal.wDay,
            stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
   
         p_out.set_string( date_string );
         return true;
      }
   }
   return false;
}

		   
static BOOL CALLBACK tag_selection_callback( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
   static pfc::string_base * in_out = NULL;
    
   switch ( msg )
   {
   case WM_INITDIALOG:
      {
         in_out = ( pfc::string_base * ) lp;        
         uSetDlgItemText( wnd, IDC_EDIT_TAG, in_out->get_ptr() );
         SetFocus( GetDlgItem( wnd, IDC_EDIT_TAG ) );
         return FALSE;
      }
      break;

	case WM_COMMAND:
		switch ( wp )
		{
		case IDOK:
   		uGetDlgItemText( wnd, IDC_EDIT_TAG, *in_out );
			// fallthrough
		case IDCANCEL:
			EndDialog( wnd, wp ); 
			break;
      }
   }
   return false;
}


class timestamp_action : public masstagger_action
{
public:
   pfc::string8 m_tag;

	/**
	 * Get name to display on list of available actions.
	 * \param p_out store name in here
	 */
	virtual void get_name(pfc::string_base & p_out)
   {
      p_out.set_string( "Stamp Current Date and Time..." );
   }

	/**
	 * Initialize and show configuration with default values, if appropriate.
	 * \param p_parent handle to parent window
	 * \returns true iff succesful
	 */
	virtual bool initialize(HWND p_parent)
   {
      m_tag.set_string( cfg_last_tag );
      if ( IDOK == uDialogBox( IDD_TAG_SELECTION, p_parent, tag_selection_callback, (LPARAM) &m_tag ) )
      {
         cfg_last_tag = m_tag;
         return true;
      }
      return false;
   }

	/**
	 * Initialize and get configuration from parameter.
	 * \param p_data a zero-terminated string containing previously stored configuration data.
	 * \returns true iff succesful
	 */
	virtual bool initialize_fromconfig(const char * p_data)
   {
      m_tag.set_string( p_data );
      return true;
   }

	/**
	 * Show configuration with current values.
	 * \param parent handle to parent window
	 * \returns true if display string needs to be updated.
	 */
	virtual bool configure(HWND p_parent)
   {
      if ( IDOK == uDialogBox( IDD_TAG_SELECTION, p_parent, tag_selection_callback, (LPARAM) &m_tag ) )
      {
         cfg_last_tag = m_tag;
         return true;
      }
      return false;
   }

	/**
	 * Get name to display on list of configured actions.
	 * You should include value of settings, if possible.
	 * \param p_name store name in here
	 */
	virtual void get_display_string(pfc::string_base & p_name)
   {
      p_name.set_string( "Time Stamp: " );
      p_name.add_string( m_tag );
   }

	/**
	 * Get current settings as zero-terminated string.
	 * \param p_data store settings in here
	 */
	virtual void get_config(pfc::string_base & p_data)
   {
      p_data.set_string( m_tag );
   }

	/**
	 * Apply action on file info.
	 * \param p_location location of current item
	 * \param p_info file info of current item, contains modifications from previous actions
	 * \param p_index zero-based index of current item in list of processed items
	 * \param p_count number of processed items
	 */
	virtual void run(const playable_location & p_location, file_info * p_info, t_size p_index, t_size p_count)
   {
      pfc::string8 timestamp;

      get_current_time( timestamp );

      p_info->meta_set( m_tag, timestamp );
   }

   virtual const GUID & get_guid()
   {
      static const GUID guid = { 0x8e883ae2, 0x63dc, 0x446d, { 0x9b, 0xe9, 0xc8, 0x41, 0x7e, 0xd3, 0x93, 0x9f } };
      return guid;
   }
};

static service_factory_single_t<timestamp_action> g_timestamp_action;


class created_stamp_action : public masstagger_action
{
public:
   pfc::string8 m_tag;

	/**
	 * Get name to display on list of available actions.
	 * \param p_out store name in here
	 */
	virtual void get_name(pfc::string_base & p_out)
   {
      p_out.set_string( "Stamp File Creation Time..." );
   }

	/**
	 * Initialize and show configuration with default values, if appropriate.
	 * \param p_parent handle to parent window
	 * \returns true iff succesful
	 */
	virtual bool initialize(HWND p_parent)
   {
      m_tag.set_string( cfg_creation_tag );
      if ( IDOK == uDialogBox( IDD_TAG_SELECTION, p_parent, tag_selection_callback, (LPARAM) &m_tag ) )
      {
         cfg_creation_tag = m_tag;
         return true;
      }
      return false;
   }

	/**
	 * Initialize and get configuration from parameter.
	 * \param p_data a zero-terminated string containing previously stored configuration data.
	 * \returns true iff succesful
	 */
	virtual bool initialize_fromconfig(const char * p_data)
   {
      m_tag.set_string( p_data );
      return true;
   }

	/**
	 * Show configuration with current values.
	 * \param parent handle to parent window
	 * \returns true if display string needs to be updated.
	 */
	virtual bool configure(HWND p_parent)
   {
      if ( IDOK == uDialogBox( IDD_TAG_SELECTION, p_parent, tag_selection_callback, (LPARAM) &m_tag ) )
      {
         cfg_creation_tag = m_tag;
         return true;
      }
      return false;
   }

	/**
	 * Get name to display on list of configured actions.
	 * You should include value of settings, if possible.
	 * \param p_name store name in here
	 */
	virtual void get_display_string(pfc::string_base & p_name)
   {
      p_name.set_string( "File Creation Stamp: " );
      p_name.add_string( m_tag );
   }

	/**
	 * Get current settings as zero-terminated string.
	 * \param p_data store settings in here
	 */
	virtual void get_config(pfc::string_base & p_data)
   {
      p_data.set_string( m_tag );
   }

	/**
	 * Apply action on file info.
	 * \param p_location location of current item
	 * \param p_info file info of current item, contains modifications from previous actions
	 * \param p_index zero-based index of current item in list of processed items
	 * \param p_count number of processed items
	 */
	virtual void run(const playable_location & p_location, file_info * p_info, t_size p_index, t_size p_count)
   {
      pfc::string8 timestamp;

      if ( get_file_creation_time( p_location, timestamp ) )
      {
         p_info->meta_set( m_tag, timestamp );
      }
   }

   virtual const GUID & get_guid()
   {
      static const GUID guid = { 0x251f2d55, 0x84eb, 0x487e, { 0x9a, 0xa3, 0xb0, 0x4e, 0x17, 0x23, 0x36, 0x0 } };
      return guid;
   }
};

static service_factory_single_t<created_stamp_action> g_created_stamp_action;
