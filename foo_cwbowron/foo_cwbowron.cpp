#define VERSION         ("0.0.5 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4996)
#pragma warning(disable:4312)
#pragma warning(disable:4244)

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "resource.h"
#include "../common/string_functions.h"
#include "../common/menu_stuff.h"

#include <gdiplus.h>
#include <GdiplusHeaders.h>
#include <Gdiplusinit.h>

#define URL_HELP                 "http://wiki.bowron.us/index.php/Foobar2000"
#define ALBUMART_FILENAME        "folder.jpg"
#define LABEL_FORMAT             "%album artist% - %album%"

#define ART_LOCATION_FORMAT      "$replace(%path%,%filename_ext%,)folder.jpg"
#define ACTION_COMMAND_PLAY      "Play in new playlist"

DECLARE_COMPONENT_VERSION( "Album Art Browser", VERSION, 
   "By Christopher Bowron <chris@bowron.us>\n"
   "http://wiki.bowron.us/index.php/Foobar2000" )

   
static const GUID g_album_browser_window_guid = { 0x933913ed, 0x1c20, 0x48f8, { 0xa6, 0x73, 0xd5, 0x81, 0xe8, 0x5e, 0x8b, 0xd0 } };

bool color_picker( HWND wnd, UINT * out, COLORREF custom = 0 )
{
	bool rv = false;
	COLORREF COLOR = *out;
	COLORREF COLORS[16] = {custom,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	if (uChooseColor(&COLOR, wnd, &COLORS[0]))
	{
		*out = COLOR;
		rv = true;
	}
	return rv;
}


   
class album_browser_window;

pfc::ptr_list_t<album_browser_window> g_album_browser_windows;

class NOVTABLE album_browser_window : public ui_extension::window
{
private:
   HWND                                   m_wnd;
   ui_extension::window_host_ptr          m_host;
   HIMAGELIST                             m_imagelist;
   pfc::list_t<pfc::string8>              m_image_paths;
   pfc::list_t<pfc::string8>              m_image_labels;
   int                                    m_image_cx;
   int                                    m_image_cy;
   pfc::ptr_list_t<metadb_handle_list>    m_handle_list_list;
   pfc::string8                           m_image_path_format;
   pfc::string8                           m_label_format;
   UINT                                   m_clr_back;
   UINT                                   m_clr_text;
   UINT                                   m_clr_tmp_back;
   UINT                                   m_clr_tmp_text;
   bool                                   m_sort;
   
   GUID                                   m_guid_selection_command;
   GUID                                   m_guid_selection_subcommand;
   pfc::string8                           m_str_selection_command;

   GUID                                   m_guid_double_click_command;
   GUID                                   m_guid_double_click_subcommand;
   pfc::string8                           m_str_double_click_command;

   pfc::string8                           m_default_image_path;

   static BOOL CALLBACK options_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:            
         {
            album_browser_window * p_browser = (album_browser_window*)lp;

            script_map_local map[] = 
            {
               { 
                  IDC_COMBO_SELECTION_ACTION, 
                  &p_browser->m_str_selection_command,
                  &p_browser->m_guid_selection_command,
                  &p_browser->m_guid_selection_subcommand
               },
               { 
                  IDC_COMBO_DOUBLE_CLICK_ACTION, 
                  &p_browser->m_str_double_click_command,
                  &p_browser->m_guid_double_click_command,
                  &p_browser->m_guid_double_click_subcommand
               },
            };

            cwb_menu_helpers::setup_local_script_maps( wnd, map, tabsize(map) );
            uSendDlgItemMessageText( wnd, IDC_COMBO_DOUBLE_CLICK_ACTION, CB_ADDSTRING, 0, ACTION_COMMAND_PLAY );            

            if ( 0 == stricmp( p_browser->m_str_double_click_command, ACTION_COMMAND_PLAY ) )
            {
               uSendDlgItemMessageText( wnd, IDC_COMBO_DOUBLE_CLICK_ACTION, CB_SELECTSTRING, -1, ACTION_COMMAND_PLAY );
            }


            p_browser->m_clr_tmp_back = p_browser->m_clr_back;
            p_browser->m_clr_tmp_text = p_browser->m_clr_text;
            
            uSetDlgItemText( wnd, IDC_EDIT_LABEL_FORMAT, p_browser->m_label_format );
            uSetDlgItemText( wnd, IDC_EDIT_ART_FORMAT, p_browser->m_image_path_format );
                       
            SetDlgItemInt( wnd, IDC_EDIT_THUMBNAIL_WIDTH, p_browser->m_image_cx, TRUE );
            SetDlgItemInt( wnd, IDC_EDIT_THUMBNAIL_HEIGHT, p_browser->m_image_cy, TRUE );

            SetWindowLong( wnd, GWL_USERDATA, lp );
         }
         break;

      case WM_COMMAND:
         {
            album_browser_window * p_browser = (album_browser_window*) GetWindowLong( wnd, GWL_USERDATA );

            switch ( wp )
            {
            case ( BN_CLICKED << 16 | IDC_BUTTON_TEXT ):
               color_picker( wnd, &p_browser->m_clr_tmp_text );
               InvalidateRect( wnd, NULL, TRUE );
               UpdateWindow( wnd );
               break;

            case ( BN_CLICKED << 16 | IDC_BUTTON_BACKGROUND ):
               color_picker( wnd, &p_browser->m_clr_tmp_back );
               InvalidateRect( wnd, NULL, TRUE );
               UpdateWindow( wnd );
               break;

            case IDOK:
               {
                  pfc::string8 str_tmp;
                  uGetDlgItemText( wnd, IDC_EDIT_LABEL_FORMAT, p_browser->m_label_format );
                  uGetDlgItemText( wnd, IDC_EDIT_ART_FORMAT, p_browser->m_image_path_format );
                  p_browser->m_clr_back = p_browser->m_clr_tmp_back;
                  p_browser->m_clr_text = p_browser->m_clr_tmp_text;
                  BOOL bTrans;
                  p_browser->m_image_cx = GetDlgItemInt( wnd, IDC_EDIT_THUMBNAIL_WIDTH, &bTrans, FALSE );
                  p_browser->m_image_cy = GetDlgItemInt( wnd, IDC_EDIT_THUMBNAIL_WIDTH, &bTrans, FALSE );

                  script_map_local map[] = 
                  {
                     { 
                        IDC_COMBO_SELECTION_ACTION, 
                        &p_browser->m_str_selection_command,
                        &p_browser->m_guid_selection_command,
                        &p_browser->m_guid_selection_subcommand
                     },
                     { 
                        IDC_COMBO_DOUBLE_CLICK_ACTION, 
                        &p_browser->m_str_double_click_command,
                        &p_browser->m_guid_double_click_command,
                        &p_browser->m_guid_double_click_subcommand
                     },
                  };

                  cwb_menu_helpers::script_map_local_on_ok( wnd, map, tabsize(map) );

                  /*
                  console::printf( "Selection Action: %s", p_browser->m_str_selection_command.get_ptr() );
                  pfc::print_guid g1( p_browser->m_guid_selection_command );
                  pfc::print_guid g2( p_browser->m_guid_selection_subcommand );
                  console::printf( "Command GUID: %s", g1.get_ptr() );
                  console::printf( "Subcommand GUID: %s", g2.get_ptr() );
                  */
               }
               // fall through

            case IDCANCEL:
               EndDialog( wnd, wp );
               break;
            }
         }
         break;

      case WM_NOTIFY:
         {
            album_browser_window * p_browser = (album_browser_window*) GetWindowLong( wnd, GWL_USERDATA );

            NMCUSTOMDRAW * pCustomDraw = (NMCUSTOMDRAW*)lp;

            switch ( pCustomDraw->dwDrawStage )
            {      
            case CDDS_PREPAINT:
               SetWindowLong( wnd, DWL_MSGRESULT, CDRF_NOTIFYPOSTPAINT );
               return TRUE;

            case CDDS_POSTPAINT:
               switch ( pCustomDraw->hdr.idFrom )
               {
               case IDC_BUTTON_FILL_BACK:
                  {
                     HBRUSH h = CreateSolidBrush( p_browser->m_clr_tmp_back );
                     ::FillRect( pCustomDraw->hdc, &pCustomDraw->rc, h );
                     DeleteObject( h );
                  }
                  break;

               case IDC_BUTTON_FILL_TEXT:
                  {
                     HBRUSH h = CreateSolidBrush( p_browser->m_clr_tmp_text );
                     ::FillRect( pCustomDraw->hdc, &pCustomDraw->rc, h );
                     DeleteObject( h );
                  }
                  break;
               }      
               break;
            }            
         }

         break;
      }      

      return FALSE;
   }

   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:            
         ((album_browser_window*)lp)->on_initdialog( wnd );
         break;

      case WM_SIZE:
         get_window( wnd )->recalc_layout();
         break;

      case WM_CONTEXTMENU:
         {
            if ( ::GetKeyState( VK_SHIFT ) & 0x8000 )
            {
               get_window( wnd )->do_options();
            }
            else
            {
               get_window( wnd )->do_context_menu();
            }
         }
         return TRUE;

      case WM_COMMAND:
         break;

      case WM_NOTIFY:
         {
            if ( wp == IDC_LIST_IMAGES )
            {
               NMHDR * pHeader = (NMHDR *) lp;
               switch ( pHeader->code )
               {                     
               case NM_DBLCLK:
                  {
                     NMITEMACTIVATE *lpnmitem = (NMITEMACTIVATE *) lp;
                     get_window( wnd )->on_double_click( lpnmitem->iItem );
                     return TRUE;
                  }
                  break;

               case LVN_ITEMCHANGED:
                  get_window( wnd )->on_item_changed( (NMLISTVIEW*) lp );
                  break;
                     
               case NM_CUSTOMDRAW:
                  {
                     LPNMLVCUSTOMDRAW pCustomDraw = (LPNMLVCUSTOMDRAW) lp;                        
                     LRESULT lResult = get_window( wnd )->on_custom_draw( pCustomDraw );
                     SetWindowLong( wnd, DWL_MSGRESULT, lResult );
                     return TRUE;
                  }
                  break;
               }
            }
         }
         break;

      }
            
      return FALSE;
   }

public:
   album_browser_window()
   {
      m_wnd                = NULL;
      m_host               = NULL;
      m_imagelist          = NULL;
      m_image_cx           = 50;
      m_image_cy           = 50;
      m_image_path_format  = ART_LOCATION_FORMAT;
      m_label_format       = LABEL_FORMAT;
      m_clr_back           = 0xFFFFFF;
      m_clr_text           = 0x000000;
      m_sort               = true;

      m_guid_selection_command         = pfc::guid_null;
      m_guid_selection_subcommand      = pfc::guid_null;
      m_guid_double_click_command      = pfc::guid_null;      
      m_guid_double_click_subcommand   = pfc::guid_null;

      m_str_selection_command          = "";
      m_str_double_click_command       = ACTION_COMMAND_PLAY;

      m_default_image_path             = "";
   }

   ~album_browser_window()
   {
      if ( m_imagelist )
      {
         ImageList_Destroy( m_imagelist );
         m_imagelist = NULL;
      }
   }

   void on_selection_changed()
   {
      metadb_handle_list handles;
      get_selected_items( handles );

      if ( m_guid_selection_command != pfc::guid_null )
      {
         menu_helpers::run_command_context( m_guid_selection_command, m_guid_selection_subcommand, handles );
      }
   }

   void on_item_changed( NMLISTVIEW * pChange )
   {
      bool bWasSelected = pChange->uOldState & LVIS_SELECTED;
      bool bIsSelected  = pChange->uNewState & LVIS_SELECTED;

      if ( !bWasSelected && bIsSelected )
      {
         on_selection_changed();
      }
      else if ( !bIsSelected && bWasSelected )
      {
         on_selection_changed();
      }
   }

   void on_double_click( int n )
   {
      if ( n >= 0 )
      {
         if ( 0 == stricmp_utf8( m_str_double_click_command, ACTION_COMMAND_PLAY ) )
         {
            static_api_ptr_t<playlist_manager> pm;

            pfc::string8 str_label;
            listview_helper::get_item_text( get_list(), n, 0, str_label );
            t_size playlist = pm->find_or_create_playlist( str_label, ~0 );
            pm->playlist_undo_backup( playlist );
            pm->playlist_clear( playlist );
            bit_array_false baf;
            pm->playlist_add_items( playlist, *m_handle_list_list[n], baf );
            pm->set_active_playlist( playlist );
            pm->set_playing_playlist( playlist );
            static_api_ptr_t<playback_control> playback;
            playback->play_stop();
            playback->play_start();
         }
         else if ( m_guid_double_click_command != pfc::guid_null )
         {
            menu_helpers::run_command_context( m_guid_double_click_command, m_guid_double_click_subcommand, *m_handle_list_list[n] );
         }
      }
   }

   LRESULT on_custom_draw( LPNMLVCUSTOMDRAW pCustomDraw )
   {
      LRESULT lResult = CDRF_DODEFAULT;

      switch( pCustomDraw->nmcd.dwDrawStage )
      {
      case CDDS_PREPAINT:
         lResult = CDRF_NOTIFYITEMDRAW;          // ask for item notifications.
         break;
      
      case CDDS_ITEMPREPAINT:
         lResult = CDRF_NOTIFYPOSTPAINT;
         break;

      case CDDS_ITEMPOSTPAINT:
         {
            int nIndex = pCustomDraw->nmcd.dwItemSpec;

            RECT rc;     
            ListView_GetItemRect( get_list(), nIndex, &rc, LVIR_ICON );

            RECT rcClient;
            GetClientRect( get_wnd(), &rcClient );

            int cx = m_image_cx;
            int cy = m_image_cy;

            int bound_cx = rc.right - rc.left;
            int bound_cy = rc.bottom - rc.top;

            int x = rc.left + ( (bound_cx - cx) / 2 );
            int y = rc.top + ( (bound_cy - cy) / 2 );

            if ( ( x + cx < rcClient.left ) || ( x > rcClient.right ) ||
                 ( y + cy < rcClient.top ) || ( y > rcClient.bottom ) )
            {
               // console::printf( "Skipping %d", nIndex );
            }
            else
            {
               Gdiplus::Bitmap * pBitmap = GetItemBitmap( nIndex );
               Gdiplus::Graphics g( pCustomDraw->nmcd.hdc );
               g.DrawImage( pBitmap, x, y, cx, cy );         
               delete pBitmap;

               UINT state = ListView_GetItemState( get_list(), nIndex, LVIS_SELECTED | LVIS_FOCUSED ); 

               if ( state & LVIS_SELECTED )
               {
                  RECT rcFocusRect = { x-1, y-1, x+cx+1, y+cy+1 };
                  ::DrawFocusRect( pCustomDraw->nmcd.hdc, &rcFocusRect );
               }
            }
         }
         break;

      }

      return lResult;
   }

   Gdiplus::Bitmap * GetItemBitmap( int nIndex )
   {
      pfc::stringcvt::string_wide_from_utf8 widePath( m_image_paths[ get_index(nIndex) ] );         
      return Gdiplus::Bitmap::FromFile( widePath );
   }         

   int AddImage ( const char * strPath, const char * str_label = NULL )
   {
      int nItems = ListView_GetItemCount( get_list() );

      int nIndex = listview_helper::insert_item( get_list(), nItems, str_label ? str_label : "", nItems );
      m_image_paths.add_item( strPath);
      m_image_labels.add_item( str_label ? str_label : "" );

      /*
      if ( ImageList_GetImageCount( m_imagelist ) == 0 )
      {
         pfc::stringcvt::string_wide_from_utf8 widePath( strPath );
         Gdiplus::Bitmap * foo = Gdiplus::Bitmap::FromFile( widePath );

         if ( foo )
         {
            HICON hIcon;
            foo->GetHICON( &hIcon );
            ImageList_AddIcon( m_imagelist, hIcon );
            DestroyIcon( hIcon );
            delete foo;
         }
         else
         {
            console::printf( "Failed to open %s", strPath );
         }
      }
      */

      return nIndex;
   }

   void ProcessAlbumArt( const char * strPath )
   {         
      AddImage( strPath );
   }

   void ProcessFindResult( const char * strPath, puFindFile pFind )
   {
      pfc::string8 fileName( pFind->GetFileName() );

      if ( pFind->GetFileName()[0] == '.' )
      {
      }
      else if ( pFind->IsDirectory() )
      {
         pfc::string8 strNewPath( strPath );
         strNewPath.add_string( pFind->GetFileName() );
         strNewPath.add_string( "\\" );
         TraverseDirectory( strNewPath );
      }
      else if ( 0 == stricmp_utf8( fileName, ALBUMART_FILENAME ) )
      {
         pfc::string8 strNewPath( strPath );
         strNewPath.add_string( pFind->GetFileName() );
         ProcessAlbumArt( strNewPath );
      }
   }

   void TraverseDirectory( const char * strPath )
   {
      pfc::string8 strWildCardPath( strPath );
      strWildCardPath.add_string( "*" );
      
      puFindFile pFind;

      if ( pFind = uFindFirstFile( strWildCardPath ) )      
      {
         do 
         {
            ProcessFindResult( strPath, pFind );
         } while ( pFind->FindNext() );
      }
   }


   static album_browser_window * get_window( HWND wnd )
   {
      return (album_browser_window*)GetWindowLong( wnd, GWL_USERDATA );
   }
	
   void clear_contents()
   {
      ListView_DeleteAllItems( get_list() );
      m_image_paths.remove_all();

      m_handle_list_list.delete_all();
   }

   int find_or_add_path( const char * path, metadb_handle_ptr p_handle )
   {
      for ( int n = 0; n < m_image_paths.get_count(); n++ )
      {
         if ( 0 == stricmp_utf8( m_image_paths[n], path ) )
         {
            return n;
         }
      }
       
      // The item was not found... 
      m_handle_list_list.add_item( new metadb_handle_list );
      pfc::string8 str_label;
      p_handle->format_title_legacy( NULL, str_label, m_label_format, NULL );
      return AddImage( path, str_label );
   }

   void browse( const pfc::list_base_const_t<metadb_handle_ptr> & p_data_in )
   {
      SendMessage( get_list(), WM_SETREDRAW, FALSE, 0 );

      clear_contents();      

      metadb_handle_list p_data = p_data_in;
      if ( m_sort )
      {
         p_data.sort_by_format( m_label_format, NULL );
      }

      for ( int n = 0; n < p_data.get_count(); n++ )
      {
         pfc::string8 image_path;
         p_data[n]->format_title_legacy( NULL, image_path, m_image_path_format, NULL );         
         
         image_path.replace_char( '/', '_' );
         image_path.replace_char( ':', '_', 2 );
         image_path.replace_char( '*', '_' );
         image_path.replace_char( '?', '_' );
         image_path.replace_char( '|', '_' );        
         image_path.replace_char( '<', '_' );
         image_path.replace_char( '>', '_' );
         image_path.replace_char( '"', '_' );

         if ( uFileExists( image_path ) )
         {            
            int nIndex = find_or_add_path( image_path, p_data[n] );
            m_handle_list_list[nIndex]->add_item( p_data[n] );
         }
      }

      SendMessage( get_list(), WM_SETREDRAW, TRUE, 0 );
   }

   int get_index( int n )
   {
      LVITEM item;
      ZeroMemory( &item, sizeof(item) );
      item.iItem  = n;
      item.mask   = LVIF_PARAM;
      ListView_GetItem( get_list(), &item );
      return (int) item.lParam;
   }

   void get_selected_items( metadb_handle_list & handles )
   {
      HWND list = get_list();
      for ( int n = 0; n < ListView_GetItemCount( list ); n++ )
      {            
         if ( ListView_GetItemState( list, n, LVIS_SELECTED ) )
         {
            handles.add_items( *m_handle_list_list[ get_index(n) ] );
         }
      }
   }

   void do_context_menu()
   {
      metadb_handle_list handles;
      get_selected_items( handles );
      static_api_ptr_t<contextmenu_manager> man;
      man->win32_run_menu_context( m_wnd, handles );          
   }

   virtual bool have_config_popup() const { return true; }
   virtual bool show_config_popup( HWND wnd_parent ) 
   {
      return do_options();
   }

   bool do_options()
   {
      if ( IDOK == uDialogBox( IDD_OPTIONS, get_wnd(), options_dialog_proc, (LPARAM) this ) )
      {
         recalc_layout();
         return true;
      }
      return false;      
   }

   void on_initdialog( HWND wnd )
   {
      m_wnd = wnd;
      SetWindowLong( wnd, GWL_USERDATA, (LPARAM)this );
      m_imagelist = ImageList_Create( m_image_cx, m_image_cy, ILC_COLOR32, 0, 1 );
      ListView_SetImageList( get_list(), m_imagelist, LVSIL_NORMAL );

      recalc_layout();      
   }

   void recalc_layout()
   {
      HWND wnd = m_wnd;
      HWND hwndList     = GetDlgItem( wnd, IDC_LIST_IMAGES );
      RECT rc;
      ::GetClientRect( wnd, &rc );

      if ( m_imagelist )
      {
         ImageList_Destroy( m_imagelist );
      }
      m_imagelist = ImageList_Create( m_image_cx, m_image_cy, ILC_COLOR32, 0, 1 );
      ListView_SetImageList( get_list(), m_imagelist, LVSIL_NORMAL );

      MoveWindow( hwndList, 0, 0, rc.right, rc.bottom, FALSE );
      ListView_Arrange( get_list(), LVA_DEFAULT );

      ImageList_SetBkColor( m_imagelist, m_clr_back );
      ListView_SetBkColor( get_list(), m_clr_back );
      ListView_SetTextBkColor( get_list(), m_clr_back );
      ListView_SetTextColor( get_list(), m_clr_text );

      DWORD dwStyle = GetWindowLong( get_list(), GWL_STYLE );
      SetWindowLong( get_list(), GWL_STYLE, dwStyle & ~(WS_VSCROLL) );

      InvalidateRect( wnd, NULL, TRUE );
      UpdateWindow( wnd );
   }

   HWND get_list()
   {
      return GetDlgItem( m_wnd, IDC_LIST_IMAGES );
   }

		/**
		* \brief Gets the category of the extension.
		* 
		* Categories you may use are "Toolbars", "Panels", "Splitters", "Playlist views" and "Visualisations"
		*
		* \param [out]	out		receives the category of the panel, utf-8 encoded
		*/
		virtual void get_category(pfc::string_base & out) const
      {
         out.set_string( "Panels" );
      }

		/**
		* \brief Gets the short, presumably more user-friendly than the name returned by get_name, name of the panel.
		*
		* \param [out]	out		receives the short name of the extension, e.g. "Order" instead
		*						of "Playback order", or "Playlists" instead of "Playlist switcher"
		* 
		* \return				true if the extension has a short name
		*/
		virtual bool get_short_name(pfc::string_base & out) const 
      {
         out.set_string( "Album Art Browser");
         return true;
      }; //short/friendly name, e.g. order vs. playback order, playlists vs. playlist switcher

		/**
		* \brief Gets the description of the extension.
		*
		* \param [out]	out		receives the description of the extension,
		*						e.g. "Drop-down list for displaying and changing the current playback order"
		* 
		* \return true if the extension has a description
		*/
		virtual bool get_description(pfc::string_base & out) const 
      {
         out.set_string( "Album Art Browser" );
         return true;
      }; 

		/**
		* \brief Gets the type of the extension.
		*
		* \return				a combination of ui_extension::type_* flags
		*
		* \see ui_extension::window_type_t
		*/
		virtual unsigned get_type() const 
      {
         return ui_extension::type_panel;
      }

		/**
		* \brief Get availability of the extension.
		*
		* This method is called before create_or_transfer() to test, if this call will be legal.
		* If this instance is already hosted, it should check whether the given host's GUID equals its
		* current host's GUID, and should return <code>false</code>, if it does. This is mostly important 
		* for single instance extensions.
		*
		* Extensions that support multiple instances can generally return <code>true</code>.
		*
		* \return whether this instance can be created in or moved to the given host
		*/
      virtual bool is_available(const ui_extension::window_host_ptr & p_host) const
      {
         // return ( m_wnd == NULL );
         return true;
      }

		/**
		* \brief Create or transfer extension window.
		*
		* Create your window here.
		*
		* In the case of single instance panels, if your window is already created, you must
		* (in the same order):
		* - Hide your window. i.e: \code ShowWindow(wnd, SW_HIDE) \endcode
		* - Set the parent window to to wnd_parent. I.e. \code SetParent(get_wnd(), wnd_parent) \endcode
		* - Move your window to the new window position. I.e.:
		*   \code SetWindowPos(get_wnd(), NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER); \endcode
		* - Call relinquish_ownership() on your current host.
		*
		* Other rules you should follow are:
		* - Ensure you are using the correct window styles. The window MUST have the WS_CHILD
		*   window style. It MUST NOT have the WS_POPUP, WS_CAPTION styles.
		* - The window must be created hidden.
		* - Use WS_EX_CONTROLPARENT if you have child windows that receive keyboard input,
		*   and you want them to be included in tab operations in the host window.
		* - Do not directly create a common control as your window. You must create
		*   a window to contain any common controls, and any other controls that
		*   communicate to the parent window via WM_COMMAND and WM_NOTIFY window messages.
		* - Under NO CIRCUMSTANCES may you subclass the host window.
		* - If you are not hosting any panels yourself, you may dialog manage your window if you wish.
		* - The window MUST have a dialog item ID of 0.
		*
		* \pre May only be called if is_available() returned true.
		*
		* \param [in]	wnd_parent		Handle to the window to use as the parent for your window
		* \param [in]	p_host			Pointer to the host that creates the extension.
		*								This parameter may not be NULL.
		* \param [in]	p_position		Initial position of the window
		* \return						Window handle of the panel window
		*/
      virtual HWND create_or_transfer_window(HWND wnd_parent, const ui_extension::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position = ui_helpers::window_position_null)
      {
         // console::printf( "tagger_window::create_or_transfer_window" );
         // g_tagger = this;
         // uMessageBox( NULL, "create_or_transfer_window", "foo_cwbowron", MB_OK );

         if ( m_wnd ) 
         {
            ShowWindow( m_wnd, SW_HIDE );
            SetParent( get_wnd(), wnd_parent );
            SetWindowPos( get_wnd(), NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER );
            m_host->relinquish_ownership( get_wnd() );
            return m_wnd;
         }
         else
         {
            m_host = p_host;
            m_wnd = uCreateDialog( IDD_ART_LIST, wnd_parent, dialog_proc, (LPARAM)this );
            g_album_browser_windows.add_item( this );
            return m_wnd;
         }
      }

		/**
		* \brief Destroys the extension window.
		*/
		virtual void destroy_window()
      {
         // console::printf( "tagger_window::destroy_window" );
         clear_contents();
         DestroyWindow( m_wnd );
         m_wnd    = NULL;
         g_album_browser_windows.remove_item( this );
      }

		/**
		* \brief Gets extension window handle.
		*
		* \pre May only be called on hosted extensions.
		*
		* \return						Window handle of the extension window
		*/
		virtual HWND get_wnd() const
      {
         return m_wnd;
      }

		/**
		* \brief Get unique ID of extension.
		*
		* This GUID is used to identify a specific extension.
		*
		* \return extension GUID
		*/
		virtual const GUID & get_extension_guid() const
      {
         return g_album_browser_window_guid;
      }

		/**
		* \brief Get a user-readable name of the extension.
		*
		* \warning
		* Do not use the name to identify extensions; use extension GUIDs instead.
		*
		* \param [out] out receives the name of the extension, e.g. "Spectrum analyser"
		*
		* \see get_extension_guid
		*/
		virtual void get_name(pfc::string_base & out) const 
      {
         out.set_string( "Album Art Browser" );
      }

		/**
		* \brief Set instance configuration data.
		*
		* \remarks 
		* - Only called before enabling/window creation.
		* - Must not be used by single instance extensions.
		* - You should also make sure you deal with the case of an empty stream
		* 
		* \throw Throws pfc::exception on failure
		*
		* \param [in]	p_reader		Pointer to configuration data stream
		* \param [in]	p_size			Size of data in stream
		* \param [in]	p_abort			Signals abort of operation
		*/
		virtual void set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort)
      {
         int version;

         p_reader->read( &version, sizeof(version), p_abort );
         
         if ( version > 0 )
         {
            p_reader->read( &m_image_cx, sizeof(m_image_cx), p_abort );
            p_reader->read( &m_image_cy, sizeof(m_image_cy), p_abort );
            p_reader->read( &m_clr_text, sizeof(m_clr_text), p_abort );
            p_reader->read( &m_clr_back, sizeof(m_clr_back), p_abort );
            p_reader->read_string( m_image_path_format, p_abort );
            p_reader->read_string( m_label_format, p_abort );

            if ( version > 1 )
            {
               p_reader->read( &m_sort, sizeof(m_sort), p_abort );

               p_reader->read_string( m_str_selection_command, p_abort );               
               p_reader->read( &m_guid_selection_command, sizeof(m_guid_selection_command), p_abort );
               p_reader->read( &m_guid_selection_subcommand, sizeof(m_guid_selection_subcommand), p_abort );
                              
               p_reader->read_string( m_str_double_click_command, p_abort );
               p_reader->read( &m_guid_double_click_command, sizeof(m_guid_double_click_command), p_abort );
               p_reader->read( &m_guid_double_click_subcommand, sizeof(m_guid_double_click_subcommand), p_abort );

               p_reader->read_string( m_default_image_path, p_abort );
            }
         }
      };

		/**
		* \brief Get instance configuration data.
		* \remarks
		* Must not be used by single instance extensions.
		*
		* \note  Consider compatibility with future versions of you own component when deciding
		*		 upon a data format. You may wish to change what is written by this function 
		*		 in the future. If you prepare for this in advance, you won't have to take 
		*		 measures such as changing your extension GUID to avoid incompatibility.
		* 
		* \throw Throws pfc::exception on failure
		*
		* \param [out]	p_writer		Pointer to stream receiving configuration data
		* \param [in]	p_abort			Signals abort of operation
		*/
		virtual void get_config(stream_writer * p_writer, abort_callback & p_abort) const 
      {
         int version = 2;

         // version 1
         p_writer->write( &version, sizeof(version), p_abort );
         p_writer->write( &m_image_cx, sizeof(m_image_cx), p_abort );
         p_writer->write( &m_image_cy, sizeof(m_image_cy), p_abort );
         p_writer->write( &m_clr_text, sizeof(m_clr_text), p_abort );
         p_writer->write( &m_clr_back, sizeof(m_clr_back), p_abort );
         p_writer->write_string( m_image_path_format, p_abort );
         p_writer->write_string( m_label_format, p_abort );

         // version 2...
         p_writer->write( &m_sort, sizeof(m_sort), p_abort );
         p_writer->write_string( m_str_selection_command, p_abort );
         p_writer->write( &m_guid_selection_command, sizeof(m_guid_selection_command), p_abort );
         p_writer->write( &m_guid_selection_subcommand, sizeof(m_guid_selection_subcommand), p_abort );

         p_writer->write_string( m_str_double_click_command, p_abort );
         p_writer->write( &m_guid_double_click_command, sizeof(m_guid_double_click_command), p_abort );
         p_writer->write( &m_guid_double_click_subcommand, sizeof(m_guid_double_click_subcommand), p_abort );

         p_writer->write_string( m_default_image_path, p_abort );
      }

      static void g_browse( const pfc::list_base_const_t<metadb_handle_ptr> & p_data );
};

static uie::window_factory<album_browser_window> g_album_browser;


void album_browser_window::g_browse( const pfc::list_base_const_t<metadb_handle_ptr> & p_data )
{
   for ( int n = 0; n < g_album_browser_windows.get_count(); n++ )
   {
      g_album_browser_windows[n]->browse( p_data );
   }
}

static const GUID context_guid_001 = { 0x75165fab, 0x10ce, 0x4416, { 0xae, 0xef, 0x3d, 0x9f, 0x8b, 0x78, 0x46, 0xfa } };

class albumart_browser_context : public contextmenu_item_simple
{
   enum { CMD_BROWSE_ALBUM_ART = 0, CMD_LAST };

   unsigned get_num_items()
   {
      return CMD_LAST;
   }

   GUID get_item_guid( unsigned int n )
   {
      switch ( n )
      {
      case CMD_BROWSE_ALBUM_ART:
         return context_guid_001;
      }
      return pfc::guid_null;
   }
   
   void get_item_name( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      { 
      case CMD_BROWSE_ALBUM_ART:
         out.set_string( "Browse album art" );
         return;
      }
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {  
      switch ( n )
      { 
      case CMD_BROWSE_ALBUM_ART:
         out.set_string( "" );
         break;
      }
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case CMD_BROWSE_ALBUM_ART:
         out.set_string( "Browse album art" );
         return true;
      }
      return false;
   }

   virtual void context_command(unsigned p_index, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      switch ( p_index )
      {
      case CMD_BROWSE_ALBUM_ART:
         album_browser_window::g_browse( p_data );
         return;
      }
   }
};

static service_factory_single_t<albumart_browser_context> g_context;

class my_on_initquit: public initquit
{
public:
   ULONG_PTR m_gdi_token;

   void on_init() 
   {
      Gdiplus::GdiplusStartupInput gdi_input;
      Gdiplus::GdiplusStartup( &m_gdi_token, &gdi_input, NULL );
   }

   void on_quit()
   {
      Gdiplus::GdiplusShutdown( m_gdi_token );
   }
};

static service_factory_single_t<my_on_initquit> g_my_on_initquit;

static const GUID guid_mainmenu_000 = { 0x74118fac, 0xbc0b, 0x4536, { 0xa0, 0x77, 0xdc, 0xe6, 0x64, 0x1e, 0x5, 0x6d } };

class album_browse_main : public mainmenu_commands
{
   t_uint get_command_count()
   {
      return 1;
   }
   virtual GUID get_command(t_uint32 p_index)
   {
      switch ( p_index )
      {
      case 0:
         return guid_mainmenu_000;
      }
      return pfc::guid_null;
   }
   virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case 0:
         p_out.set_string( "Browse Library Album Art" );
         break;
      }
   }
   virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
   {
      switch ( p_index )
      {
      case 0:
         p_out.set_string( "Show album art for entire media library in album art browsers" );
         return true;
      }
      return false;
   }

   virtual GUID get_parent()
   {
      return mainmenu_groups::view;
   }

   virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
   {
      switch ( p_index )
      {
      case 0:        
         {
            static_api_ptr_t<library_manager> lm;
            metadb_handle_list lib_handles;
            lm->get_all_items( lib_handles );

            DWORD dwStart = ::GetTickCount();
            album_browser_window::g_browse( lib_handles );
            DWORD dwStop  = ::GetTickCount();

            console::printf( "Elapsed time: %d", ( dwStop - dwStart ) / 1000 );
         }
         break;
      }
   }
};

static mainmenu_commands_factory_t< album_browse_main > g_mainmenu;


///// STARTUP 
#define STARTUP_VERSION ("0.0.2 [" __DATE__ " - " __TIME__ "]")

static componentversion_impl_simple_factory 
   g_componentversion_service_2( 
      "Startup Actions",
      STARTUP_VERSION, 
      "Allows you to select main menu actions to perform upon startup\n"
      "By Christopher Bowron <chris@bowron.us>\n"
      "http://wiki.bowron.us/index.php/Foobar2000"
      );


static const GUID var_guid_000 = { 0x29784dba, 0x7391, 0x489e, { 0x8d, 0xc1, 0x22, 0x5f, 0x45, 0x3b, 0x74, 0xbf } };

cfg_guidlist cfg_startup_commands( var_guid_000 );

class startup_on_initquit: public initquit, public main_thread_callback
{
public:
   void callback_run()
   {
      for ( int n = 0; n < cfg_startup_commands.get_count(); n++ )
      {
         mainmenu_commands::g_execute( cfg_startup_commands[n] );
      }
   }

   void on_init() 
   {
      static_api_ptr_t<main_thread_callback_manager>()->add_callback( this );
   }

   void on_quit()
   {
   }
};

static service_factory_single_t<startup_on_initquit> g_startup_on_initquit;

static const GUID guid_startup_preferences = { 0x8f23b7c, 0x5be4, 0x49f1, { 0xb8, 0x77, 0x17, 0xd0, 0xf9, 0xaf, 0x2f, 0x69 } };

pfc::list_t<GUID> g_mainmenu_commands;
GUID * g_pGUID = NULL;

class startup_preferences : public preferences_page, public cwb_menu_helpers
{  
   static bool find_menu_path( GUID & parent, pfc::string8 & out )
   {
      if ( parent == pfc::guid_null )
      {
         out.set_string( "" );
         return true;
      }
      else
      {
         service_enum_t<mainmenu_group> e;
	      service_ptr_t<mainmenu_group> ptr;

         while ( e.next( ptr ) )
         {
            if ( ptr->get_guid() == parent )
            {
               pfc::string8 tmp;

               service_ptr_t<mainmenu_group_popup> popup;

               if ( ptr->service_query_t( popup ) )
               {
                  popup->get_display_string( tmp );  
                  find_menu_path( ptr->get_parent(), out );
                  out.add_string( tmp );
                  out.add_string( "/" );
                  return true;
               }
               else
               {
                  find_menu_path( ptr->get_parent(), out );
                  return true;
               }
            }
         }
      }

      out.set_string( "" );
      return false;
   }

   static BOOL CALLBACK dialog_proc_add( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            HWND hwndActions = GetDlgItem( wnd, IDC_COMBO_ACTIONS );
            g_pGUID = (GUID*)lp;

            SendMessage( hwndActions, CB_RESETCONTENT, 0, 0 );

            g_mainmenu_commands.remove_all();

            service_enum_t<mainmenu_commands> e;
	         service_ptr_t<mainmenu_commands> ptr;

            while ( e.next( ptr ) )
            {               
               int m = ptr->get_command_count();

               for ( int n = 0; n < m; n++ )
               {
                  pfc::string8 str_name;
                  pfc::string8 str_path;

                  ptr->get_name( n, str_name );
                  find_menu_path( ptr->get_parent(), str_path );
                  str_path.add_string( str_name );

                  int list_index = uSendDlgItemMessageText( wnd, IDC_COMBO_ACTIONS, CB_ADDSTRING, n, str_path );
                  int cmd_index = g_mainmenu_commands.add_item( ptr->get_command( n ) );
                  SendDlgItemMessage( wnd, IDC_COMBO_ACTIONS, CB_SETITEMDATA, list_index, (LPARAM)cmd_index );
               }
            }
         }
         break;

      case WM_COMMAND:
         switch ( wp )
         {
         case IDOK:
            {
               int list_index = SendDlgItemMessage( wnd, IDC_COMBO_ACTIONS, CB_GETCURSEL, 0, 0 );
               int cmd_index = SendDlgItemMessage( wnd, IDC_COMBO_ACTIONS, CB_GETITEMDATA, list_index, 0 );
               *g_pGUID = g_mainmenu_commands[cmd_index];
            }
            // fall through
         case IDCANCEL:
            EndDialog( wnd, wp );
            break;
         }
         break;
      }
       
      return false;
   }

   static bool guid_to_name( GUID & p_guid, pfc::string_base & str_out )
   {
	   service_enum_t<mainmenu_commands> e;
	   service_ptr_t<mainmenu_commands> ptr;
	   while(e.next(ptr)) {
		   const t_uint32 count = ptr->get_command_count();
		   for(t_uint32 n=0;n<count;n++) {
			   if (ptr->get_command(n) == p_guid) {
               ptr->get_name( n, str_out );
				   return true;
			   }
		   }
	   }
	   return false;
   }

   static void display_startup_commands( HWND wnd )
   {
      HWND list = GetDlgItem( wnd, IDC_LIST_ACTIONS );

      ListView_DeleteAllItems( list );
      for ( int n = 0; n < cfg_startup_commands.get_count(); n++ )
      {
         pfc::string8 str;
                  
         if ( guid_to_name( cfg_startup_commands[n], str ) )
         {         
            listview_helper::insert_item( list, n, str, NULL );
         }
         else
         {
            listview_helper::insert_item( list, n, "<<Unknown command>>", NULL );
         }
      }
   }

   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            display_startup_commands( wnd );
         }
         break;

      case WM_COMMAND:
         switch ( wp )
         {
         case ( BN_CLICKED << 16 | IDC_BUTTON_ADD ):
            {            
               GUID guid;
               int nResult = uDialogBox( IDD_ADD_ACTION, wnd, dialog_proc_add, (LPARAM)&guid );
               if ( nResult == IDOK )
               {
                  cfg_startup_commands.add_item( guid );
                  display_startup_commands( wnd );
               }
            }
            break;

         case ( BN_CLICKED << 16 | IDC_BUTTON_REMOVE ):
            {
               HWND list = GetDlgItem( wnd, IDC_LIST_ACTIONS );
               int m = ListView_GetItemCount( list );
               for ( int n = m - 1; n >= 0; n-- )
               {
                  if ( ListView_GetItemState( list, n, LVIS_SELECTED ) )
                  {
                     cfg_startup_commands.remove_by_idx( n );
                  }
               }
               display_startup_commands( wnd );
            }
            break;
         }
         break;
      }
      return false;
   }

   virtual HWND create(HWND parent)
   {
      return uCreateDialog( IDD_STARTUP_PREFERENCES, parent, dialog_proc );
   }

   virtual const char * get_name()
   {
      return "Startup Actions";
   }

   virtual GUID get_guid()
   {
      return guid_startup_preferences;
   }

   virtual GUID get_parent_guid()
   {
      return guid_tools;
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

static service_factory_single_t<startup_preferences> g_startup_preferences;
