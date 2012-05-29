#define TAGGER_VERSION  ("1.0.6 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4996)
#pragma warning(disable:4312)
#pragma warning(disable:4244)
#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "resource.h"
#include "../common/string_functions.h"

#define TAG_RATING   "RATING"

static componentversion_impl_simple_factory 
   g_componentversion_service_2( 
      "Tagger Panel Window", 
      TAGGER_VERSION, 
      "Columns UI Panel for tagging music\n"
      "By Christopher Bowron <chris@bowron.us>\n"
      "http://wiki.bowron.us/index.php/Foobar2000"
      );


static const GUID guid_var_008 = { 0x78267f3d, 0xc5a1, 0x4662, { 0xa1, 0xca, 0x15, 0x48, 0xc0, 0x8f, 0x76, 0xe0 } };
static const GUID guid_var_009 = { 0xfb47e33e, 0x5ac4, 0x4a27, { 0x8d, 0x39, 0x47, 0x8c, 0x4a, 0xef, 0x2a, 0xa2 } };
static const GUID guid_var_010 = { 0xcdc46702, 0x78aa, 0x4cc5, { 0x95, 0xcd, 0x1d, 0x57, 0x6f, 0x12, 0xfc, 0x1a } };

static cfg_string cfg_tagger_tags( guid_var_008, "Happy;Sad;Instrumental;Live;NSFW;Acoustic" );
static cfg_string cfg_tagger_meta( guid_var_009, "TAG" );
static cfg_int    cfg_tagger_show_ratings( guid_var_010, 1 );

#define TAG_DELIM ";"

static const GUID g_tagger_window_guid = { 0xa08a260c, 0xfd81, 0x4990, { 0x9a, 0x4d, 0xcf, 0x59, 0xf4, 0xbc, 0x5, 0x93 } };

class tagger_window;
// tagger_window * g_tagger = NULL;

pfc::ptr_list_t<tagger_window> g_available_tagger_windows;
     
UINT g_arRatingButtonIDs[] =
{  
   IDC_RATING_NONE,
   IDC_RATING_1,      
   IDC_RATING_2,
   IDC_RATING_3,
   IDC_RATING_4,
   IDC_RATING_5
};


class tagger_window_base;

class tagger_window_destroyer : public completion_notify
{
   tagger_window_base * m_tagger;

public:
   tagger_window_destroyer( tagger_window_base * p )
   {
      m_tagger = p;
   }

   virtual void on_completion( unsigned p_code );
};

class tagger_window_base
{
public:
   HWND                            m_wnd;
   
   ui_extension::window_host_ptr   m_host;
   metadb_handle_list              m_handles;
   pfc::string8                    m_tag_meta;
   pfc::string8                    m_tag_options;
   bool                            m_show_ratings;
   HIMAGELIST                      m_imagelist;
   bool                            m_ignore_changes;
   bool                            m_show_files;
   bool                            m_scan_library;
   WNDPROC                         m_orig_edit_wndproc;

   static int CALLBACK sort_proc( LPARAM lp1, LPARAM lp2, LPARAM lpSort )
   {
      int nIndex1    = (int) lp1;
      int nIndex2    = (int) lp2;
      HWND hwndList  = (HWND) lpSort;

      pfc::string8 str_one;
      pfc::string8 str_two;
      listview_helper::get_item_text( hwndList, nIndex1, 0, str_one );
      listview_helper::get_item_text( hwndList, nIndex2, 0, str_two );

      int nResult = comparator_stricmp_utf8::compare( str_one, str_two );
      return nResult;
   }

   static BOOL CALLBACK options_dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:
         {
            tagger_window_base * tagger = (tagger_window_base*)lp;
            SetWindowLong( wnd, GWL_USERDATA, lp );
            uSetDlgItemText( wnd, IDC_EDIT_TAGGER_TAGS, tagger->m_tag_options );
            uSetDlgItemText( wnd, IDC_EDIT_TAGGER_META, tagger->m_tag_meta );
            CheckDlgButton( wnd, IDC_CHECK_USE_RATINGS, tagger->m_show_ratings );
            CheckDlgButton( wnd, IDC_CHECK_SHOW_FILES, tagger->m_show_files );
            CheckDlgButton( wnd, IDC_CHECK_SCAN_LIBRARY, tagger->m_scan_library );
         }
         break;
         
      case WM_COMMAND:
         switch ( wp )
         {          
         case IDOK:
            {
               tagger_window_base * tagger = get_tagger_window_base( wnd );
               uGetDlgItemText( wnd, IDC_EDIT_TAGGER_META, tagger->m_tag_meta );
               uGetDlgItemText( wnd, IDC_EDIT_TAGGER_TAGS, tagger->m_tag_options );
               tagger->m_show_ratings  = IsDlgButtonChecked( wnd, IDC_CHECK_USE_RATINGS );           
               tagger->m_show_files    = IsDlgButtonChecked( wnd, IDC_CHECK_SHOW_FILES );
               tagger->m_scan_library  = IsDlgButtonChecked( wnd, IDC_CHECK_SCAN_LIBRARY );

               cfg_tagger_tags         = tagger->m_tag_options;
               cfg_tagger_meta         = tagger->m_tag_meta;
               cfg_tagger_show_ratings = tagger->m_show_ratings;

               // console::printf( "m_tag_meta: %s", tagger->m_tag_meta.get_ptr() );
               // console::printf( "m_tag_options: %s", tagger->m_tag_options.get_ptr() );
               // console::printf( "m_show_ratings: %s", tagger->m_show_ratings ? "TRUE" : "FALSE" );
            }
            /* FALL THROUGH */
         case IDCANCEL:
            EndDialog( wnd, wp );
            break;
         }
         break;
      }
      return FALSE;
   }

public:
   static LRESULT CALLBACK edit_wndproc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_KEYDOWN:
         if ( wp == VK_RETURN )
         {
            get_tagger_window_base( GetParent( wnd ) )->on_apply();
         }
         break;        
      }
      return get_tagger_window_base( GetParent( wnd ) )->m_orig_edit_wndproc( wnd, msg, wp, lp );
   }

   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_INITDIALOG:   
         {
            tagger_window_base * p = (tagger_window_base*)lp;
            set_tagger_window_base( wnd, p );
            p->on_initdialog( wnd );
         }
         break;

      case WM_SIZE:
         if ( get_tagger_window_base( wnd ) )
         {
            get_tagger_window_base( wnd )->recalc_layout();
         }
         break;

      case WM_NOTIFY:
         {
            NMHDR * pHeader = (NMHDR*) lp;

            switch ( pHeader->code )
            {
            case LVN_ITEMCHANGED:
               {  
                  NMLISTVIEW * pNM = (NMLISTVIEW*)lp;
                  if ( ( pNM->uNewState & LVIS_STATEIMAGEMASK ) != ( pNM->uOldState & LVIS_STATEIMAGEMASK ) )
                  {
                     tagger_window_base * tagger = (tagger_window_base*)GetWindowLong( wnd, GWL_USERDATA );
                     tagger->on_list_change( pNM );
                  }
               }
               break;

            case NM_DBLCLK:
               {
                  LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lp;
                  LVHITTESTINFO info;
                  info.pt = lpnmitem->ptAction;

                  ListView_HitTest( pHeader->hwndFrom, &info );
                  if ( info.flags & LVHT_ONITEMLABEL )
                  {
                     get_tagger_window_base( wnd )->on_tag_doubleclick( lpnmitem->iItem );
                  }                 
               }
               break;
            }
         }
         break;

      case WM_CONTEXTMENU:
         {
            HMENU menu = CreatePopupMenu();               
            uAppendMenu( menu, MF_STRING, 1, "Options..." );
            POINT pt;
            GetCursorPos( &pt );
            int nCmd = TrackPopupMenu( menu, TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, NULL, wnd, NULL );
            if ( nCmd == 1 )
            {
               get_tagger_window_base( wnd )->show_config( wnd );
            }
            DestroyMenu( menu );
            return TRUE;
         }
         break;

      case WM_CLOSE:   
         {
            tagger_window_base * tagger = get_tagger_window_base( wnd );
            tagger->on_close();
            // DestroyWindow( wnd );
            // delete tagger;
         }
         break;

      case WM_COMMAND:
         switch ( wp )
         {
         case BN_CLICKED<<16|IDC_RATING_NONE:
         case BN_CLICKED<<16|IDC_RATING_1:
         case BN_CLICKED<<16|IDC_RATING_2:
         case BN_CLICKED<<16|IDC_RATING_3:
         case BN_CLICKED<<16|IDC_RATING_4:
         case BN_CLICKED<<16|IDC_RATING_5:
         case EN_CHANGE<<16|IDC_EDIT_MORE_TAGS:
            {
               tagger_window_base * tagger = get_tagger_window_base( wnd );
               tagger->on_change();
            }
            break;

         case IDOK:
            {
               tagger_window_base * tagger = get_tagger_window_base( wnd );
               tagger->on_apply();
            }
            break;

         case IDCANCEL:
            {
               tagger_window_base * tagger = get_tagger_window_base( wnd );
               tagger->on_revert();
            }
            break;
         }
         break;

      }
            
      return FALSE;
   }

public:
   tagger_window_base()
   {
      m_wnd             = NULL;
      m_host            = NULL;
      m_tag_meta        = cfg_tagger_meta;
      m_tag_options     = cfg_tagger_tags;
      m_show_ratings    = cfg_tagger_show_ratings;
      m_imagelist       = NULL;
      m_ignore_changes  = false;
      m_show_files      = true;
      m_scan_library    = false;
   }

   virtual ~tagger_window_base()
   {
      clear_cached_handles();

      if ( m_imagelist )
      {
         ImageList_Destroy( m_imagelist );
         m_imagelist = NULL;
      }
   }

   virtual void clear_cached_handles()
   {
      m_handles.remove_all();
   }

   static tagger_window_base * get_tagger_window_base( HWND wnd )
   {
      return (tagger_window_base*)GetWindowLong( wnd, GWL_USERDATA );
   }

   static void set_tagger_window_base( HWND wnd, tagger_window_base * p )
   {
      SetWindowLong( wnd, GWL_USERDATA, (LPARAM)p );
   }

	virtual void on_quit()
   {
      clear_cached_handles();
   }    

   virtual bool show_config( HWND wnd_parent )
   {
      if ( IDOK == uDialogBox( IDD_TAGGER_OPTIONS, wnd_parent, options_dialog_proc, (LPARAM) this ) )
      {
         recalc_layout();
         refresh_tags();
         on_revert();
         return true;
      }
      return false;
   }

   virtual void on_initdialog( HWND wnd )
   {
      m_wnd = wnd;

      HWND hwndList = get_list();

      ListView_SetExtendedListViewStyle( hwndList,
         ListView_GetExtendedListViewStyle( hwndList ) | LVS_EX_CHECKBOXES );            

      HBITMAP bitmap = (HBITMAP)LoadImage( core_api::get_my_instance(), MAKEINTRESOURCE(IDB_BITMAP1),
         IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT );

      m_imagelist = ::ImageList_Create(16,16, ILC_MASK|ILC_COLOR8, 10, 10);
      ::ImageList_AddMasked( m_imagelist, bitmap, CLR_DEFAULT );
      DeleteObject( bitmap );
      ListView_SetImageList( hwndList, m_imagelist, LVSIL_STATE );

      uSetWindowText( GetDlgItem( wnd, IDC_STATIC_FILENAME ), "" );

      m_orig_edit_wndproc = uHookWindowProc( GetDlgItem( wnd, IDC_EDIT_MORE_TAGS ), edit_wndproc );
      refresh_tags();
      recalc_layout();
      enable_buttons( false );
   }

   virtual void on_close()
   {
   }

   virtual void refresh_tags()
   {
      if ( m_scan_library )
      {
         console::printf( "Scanning Library for additional tags" );
         in_metadb_sync sync;
         metadb_handle_list lib_handles;
         static_api_ptr_t<library_manager>()->get_all_items( lib_handles );

         for ( int n = 0; n < lib_handles.get_count(); n++ )
         {
            const file_info * fip;
            lib_handles[n]->get_info_locked( fip );
            int tag_count = fip->meta_get_count_by_name( m_tag_meta );
            for ( int m = 0; m < tag_count; m++ )
            {
               pfc::string8 str_tag( fip->meta_get( m_tag_meta, m ) );

               if ( m_tag_options.find_first( str_tag ) == infinite )
               {
                  m_tag_options.add_string( ";" );
                  m_tag_options.add_string( str_tag );
                  // console::printf( "Adding %s from %s", str_tag.get_ptr(), lib_handles[n]->get_path() );
               }
            }
         }
      }

      HWND hwndList = get_list();
      ListView_DeleteAllItems( hwndList );

      pfc::ptr_list_t<pfc::string8> tags;
      string_split( m_tag_options, TAG_DELIM, tags );

      for ( int n = 0; n < tags.get_count(); n++ )
      {
         if ( !tags[n]->is_empty() )
         {
            listview_helper::insert_item( hwndList, n, tags.get_item(n)->get_ptr(), NULL );
         }
      }

      sort_tags();
   }

   virtual void recalc_layout()
   {
      HWND wnd = m_wnd;
      HWND hwndList     = GetDlgItem( wnd, IDC_LIST_TAGS );
      HWND hwndApply    = GetDlgItem( wnd, IDOK );
      HWND hwndRevert   = GetDlgItem( wnd, IDCANCEL );
      HWND hwndMore     = GetDlgItem( wnd, IDC_EDIT_MORE_TAGS );
      HWND hwndFilename = GetDlgItem( wnd, IDC_STATIC_FILENAME );
      RECT rc;
      RECT rcApply;
      RECT rcFilename;
      ::GetClientRect( wnd, &rc );
      ::GetWindowRect( hwndApply, &rcApply );
      ::GetWindowRect( hwndFilename, &rcFilename );
      int nButtonHeight = rcApply.bottom - rcApply.top;
      int nButtonWidth  = rcApply.right - rcApply.left;

      HWND hwndRatingNone  = GetDlgItem( wnd, IDC_RATING_NONE );
      HWND hwndRatingOne   = GetDlgItem( wnd, IDC_RATING_1 );
      HWND hwndRatingTwo   = GetDlgItem( wnd, IDC_RATING_2 );
      HWND hwndRatingThree = GetDlgItem( wnd, IDC_RATING_3 );
      HWND hwndRatingFour  = GetDlgItem( wnd, IDC_RATING_4 );
      HWND hwndRatingFive  = GetDlgItem( wnd, IDC_RATING_5 );

      RECT rcRatingNone;
      ::GetWindowRect( hwndRatingNone, &rcRatingNone );

      int nRatingWidth     = rcRatingNone.right - rcRatingNone.left;
      int nRatingHeight    = rcRatingNone.bottom - rcRatingNone.top;
      int nFilenameHeight  = rcFilename.bottom - rcFilename.top;

      int nOffsetY = nFilenameHeight + 8;

      if ( m_show_files )
      {
         MoveWindow( hwndFilename, 6, 4, rc.right - 8, nFilenameHeight, FALSE );              
         ShowWindow( hwndFilename, SW_SHOW );
      }
      else
      {
         nOffsetY = 4;
         ShowWindow( hwndFilename, SW_HIDE );
      }


      MoveWindow( hwndRevert, rc.right - nButtonWidth - 4, rc.bottom - nButtonHeight - 4, nButtonWidth, nButtonHeight, FALSE );
      MoveWindow( hwndApply, rc.right - 2 * (nButtonWidth + 4), rc.bottom - nButtonHeight - 4, nButtonWidth, nButtonHeight, FALSE );
      MoveWindow( hwndMore, 4, rc.bottom - nButtonHeight - 4, rc.right - 2 * (nButtonWidth + 8) - 4, nButtonHeight, FALSE );

      MoveWindow( hwndRatingFive, rc.right - 4 - nRatingWidth, nOffsetY + nRatingHeight * 0, nRatingWidth, nRatingHeight, FALSE );
      MoveWindow( hwndRatingFour, rc.right - 4 - nRatingWidth, nOffsetY + nRatingHeight * 1, nRatingWidth, nRatingHeight, FALSE );
      MoveWindow( hwndRatingThree, rc.right - 4 - nRatingWidth, nOffsetY + nRatingHeight * 2, nRatingWidth, nRatingHeight, FALSE );
      MoveWindow( hwndRatingTwo, rc.right - 4 - nRatingWidth, nOffsetY + nRatingHeight * 3, nRatingWidth, nRatingHeight, FALSE );
      MoveWindow( hwndRatingOne, rc.right - 4 - nRatingWidth, nOffsetY + nRatingHeight * 4, nRatingWidth, nRatingHeight, FALSE );
      MoveWindow( hwndRatingNone, rc.right - 4 - nRatingWidth, nOffsetY + nRatingHeight * 5, nRatingWidth, nRatingHeight, FALSE );

      int nCmdShow = SW_SHOW;

      if ( m_show_ratings )
      {
         MoveWindow( hwndList, 4, nOffsetY, rc.right - 8 - nRatingWidth - 4, rc.bottom - nButtonHeight - 8 - nOffsetY, FALSE );
         nCmdShow = SW_SHOW;
      }
      else
      {
         MoveWindow( hwndList, 4, nOffsetY, rc.right - 8, rc.bottom - nButtonHeight - 8 - nOffsetY, FALSE );
         nCmdShow = SW_HIDE;
      }

      ShowWindow( hwndRatingFive, nCmdShow );
      ShowWindow( hwndRatingFour, nCmdShow );
      ShowWindow( hwndRatingThree, nCmdShow );
      ShowWindow( hwndRatingTwo, nCmdShow );
      ShowWindow( hwndRatingOne, nCmdShow );
      ShowWindow( hwndRatingNone, nCmdShow );

      InvalidateRect( wnd, NULL, TRUE );
      UpdateWindow( wnd );
   }

   virtual void on_tag_doubleclick( int n )
   {
      pfc::string8 str_tag;
      listview_helper::get_item_text( get_list(), n, 0, str_tag );
      pfc::string_printf str_filter( "%s HAS %s", (const char *)m_tag_meta, (const char *)str_tag );

      search_tools::search_filter filter;
      if ( filter.init( str_filter ) )
      {
         metadb_handle_list handles;
         static_api_ptr_t<library_manager> db;
         db->get_all_items( handles );

         int count = handles.get_count();

         bit_array_bittable mask( count );
         for( unsigned n = 0; n < count; n++ )
         {
            mask.set( n, filter.test( handles[n] ) );
         }

         handles.filter_mask( mask );

         static_api_ptr_t<playlist_manager> pm;
         bit_array_false baf;
         t_size index = pm->find_or_create_playlist( str_tag, ~0 );
         pm->playlist_add_items( index, handles, baf );
         pm->set_active_playlist( index );
      }
   }

   virtual void enable_buttons( bool bEnable = true )
   {
      EnableWindow( GetDlgItem( m_wnd, IDOK ), bEnable );
      EnableWindow( GetDlgItem( m_wnd, IDCANCEL ), bEnable );
   }

   virtual int get_tag_index( const char * string )
   {
      for ( int n = 0; n < ListView_GetItemCount( get_list() ); n++ )
      {
         pfc::string8 str_list_label;
         listview_helper::get_item_text( get_list(), n, 0, str_list_label );                 
         if ( stricmp_utf8_ex( str_list_label, ~0, string, ~0 ) == 0 )
         {
            return n;
         }
      }
      return -1;
   }

   virtual int add_tag_option( const char * string, bool bChecked = FALSE, LPARAM nCount = 0 )
   {
      int z = listview_helper::insert_item( get_list(), ListView_GetItemCount( get_list() ), string, nCount );

      m_tag_options.add_string( TAG_DELIM );
      m_tag_options.add_string( string );

      if ( bChecked )
      {
         ListView_SetCheckState( get_list(), z, BST_CHECKED );
      }

      return z;
   }

   virtual void check_item_or_add( const char * str_tag )
   {
      bool bFound = false;
      for ( int n = 0; n < ListView_GetItemCount( get_list() ); n++ )
      {
         pfc::string8 str_list_label;
         listview_helper::get_item_text( get_list(), n, 0, str_list_label );                 
         if ( stricmp_utf8_ex( str_list_label, ~0, str_tag, ~0 ) == 0 )
         {
            ListView_SetCheckState( get_list(), n, BST_CHECKED );
            bFound = true;
            break;
         }
      }
      if ( !bFound )
      {
         add_tag_option( str_tag, true );
      }
   }

   virtual bool remove_file_tag_if_necessary( file_info * info, const char * str_tag )
   {
      int nCount = info->meta_get_count_by_name( m_tag_meta );

      int nTagIndex = info->meta_find( m_tag_meta );

      bool bResult = false;
      for ( int n = nCount - 1; n >= 0; n-- )
      {
         pfc::string8 str_this_tag;
         str_this_tag = info->meta_get( m_tag_meta, n );
         if ( 0 == stricmp_utf8_ex( str_tag, ~0, str_this_tag, ~0 ) )
         {
            info->meta_remove_value( nTagIndex, n );
            bResult = true;
         }         
      }
      return bResult;
   }

   virtual bool add_file_tag_if_necessary( file_info * info, const char * str_tag )
   {
      for ( int n = 0; n < info->meta_get_count_by_name( m_tag_meta ); n++ )
      {
         pfc::string8 str_this_tag;
         str_this_tag = info->meta_get( m_tag_meta, n );
         if ( 0 == stricmp_utf8_ex( str_tag, ~0, str_this_tag, ~0 ) )
         {
            return false;
         }
      }
         
      info->meta_add( m_tag_meta, str_tag );
      return true;
   }

   virtual void on_apply( completion_notify_ptr p_notify = NULL  )
   {
      if ( m_handles.get_count() == 0 )
         return;

      assert( core_api::is_main_thread() );

      pfc::string8 str_more_tags;
      uGetDlgItemText( m_wnd, IDC_EDIT_MORE_TAGS, str_more_tags );
      uSetDlgItemText( m_wnd, IDC_EDIT_MORE_TAGS, "" );

      if ( str_more_tags.is_empty() == false )
      {
         pfc::ptr_list_t<pfc::string8> more_tags;
         string_split( str_more_tags, TAG_DELIM, more_tags );            
         for ( int n = 0; n < more_tags.get_count(); n++ )
         {
            check_item_or_add( more_tags[n]->get_ptr() );
         }
      }

      in_metadb_sync db_lock;

      static_api_ptr_t<metadb_io_v2> io;
      
      int nHandles = m_handles.get_count();

      pfc::list_t<const file_info * > info_list;

      for ( int n = 0; n < m_handles.get_count(); n++ )
      {
         file_info_impl * info = new file_info_impl;
         
         bool b = m_handles[n]->get_info( *info );
         assert( b );

         // by default, we will not change the rating... 
         int nNewRating = -1;
         int nCount = 0;
         for ( int n = 0; n < 6; n++ )
         {
            if ( BST_CHECKED == IsDlgButtonChecked( m_wnd, g_arRatingButtonIDs[n] ) )
            {
               nCount++;
               nNewRating = n;
            }
         }

         if ( nCount == 1 )
         {
            // Remove ratings
            if ( nNewRating == 0 )
            {
               info->meta_remove_field( TAG_RATING );
            }
            // set to a specific rating
            else if ( nNewRating > 0 )
            {
               info->meta_set( TAG_RATING, pfc::string_printf( "%d", nNewRating ) );
            }
         }

         HWND hwndList = get_list();
         for ( int m = 0; m < ListView_GetItemCount( hwndList ); m++ )
         {
            pfc::string8 str_tag;
            listview_helper::get_item_text( hwndList, m, 0, str_tag );

            int nState = ListView_GetCheckState( hwndList, m );

            // remove it if necessary
            if ( nState ==  0 )
            {
               remove_file_tag_if_necessary( info, str_tag );
            }
            // add it if necessary
            else if ( nState == 1 )
            {
               add_file_tag_if_necessary( info, str_tag );
            }
         }

         info_list.add_item( (const file_info*)info );
      }

      io->update_info_async_simple( m_handles, info_list, m_wnd, 
         /*metadb_io_v2::op_flag_delay_ui | metadb_io_v2::op_flag_background*/ 0, p_notify );

      sort_tags();
      enable_buttons( false );
   }

   virtual void on_revert()
   {
      metadb_handle_list copy = m_handles;
      uSetDlgItemText( m_wnd, IDC_EDIT_MORE_TAGS, "" );
      show_handles( copy );
   }

   virtual void on_list_change( NMLISTVIEW * pmv )
   {
      if ( !m_ignore_changes )
      {
         int nStateImage = (pmv->uNewState & LVIS_STATEIMAGEMASK) >> 12;
         // console::printf( "on_changing: nStateImage = %d", nStateImage );
         if ( nStateImage > 2 )
         {
            DWORD dwNewState = INDEXTOSTATEIMAGEMASK(1);
            ListView_SetItemState( get_list(), pmv->iItem, dwNewState, LVIS_STATEIMAGEMASK );
         }        
      }

      on_change();
   }

   virtual void on_change()
   {
      if ( m_handles.get_count() > 0 )
      {
         enable_buttons( true );
      }
   }

   virtual void sort_tags()
   {
      ListView_SortItemsEx( get_list(), sort_proc, get_list() );
   }

   virtual void show_handles_files( const pfc::list_base_t<metadb_handle_ptr> & handles )
   {
      int nHandles = handles.get_count();

      pfc::string8 filenames;
      if ( nHandles > 4 )
      {
         filenames = pfc::string_printf( "<%d files>", nHandles );
      }
      else if ( nHandles > 1 )
      {
         for ( int n = 0; n < handles.get_count(); n++ )
         {
            pfc::string_filename_ext fn( handles[n]->get_path() );
            if ( n > 0 )
               filenames.add_string( "; " );
            filenames.add_string( fn );
         }
      }
      else if ( nHandles == 1 )
      {
         filenames = handles[0]->get_path();
      }
      else
      {
         filenames = "<none>";
      }

      uSetDlgItemText( m_wnd, IDC_STATIC_FILENAME, filenames );
   }

   virtual int get_tag_state( int n )
   {            
      return ListView_GetCheckState( get_list(), n);
   }

   virtual void set_tag_state( int n, int nState )
   {
      ListView_SetItemState( get_list(), n, INDEXTOSTATEIMAGEMASK( nState + 1 ), LVIS_STATEIMAGEMASK );
   }

   virtual bool file_has_tag( const char * str_tag, const file_info & info )
   {
      int tag_count = info.meta_get_count_by_name( m_tag_meta );
      
      if ( tag_count == infinite )
         return false;

      for ( int m = 0; m < tag_count; m++ )
      {
         pfc::string8 str_this_tag = info.meta_get( m_tag_meta, m );
         if ( 0 == stricmp_utf8_ex( str_tag, ~0, str_this_tag, ~0 ) )
         {
            return true;
         }
      }
      return false;
   }

   virtual void add_tags_from_file_if_necessary( const file_info & info, bool bCheck = false )
   {
      int tag_count = info.meta_get_count_by_name( m_tag_meta );
      
      if ( tag_count != infinite )
      {
         for ( int m = 0; m < tag_count; m++ )
         {
            pfc::string8 str_this_tag = info.meta_get( m_tag_meta, m );

            if ( get_tag_index( str_this_tag ) < 0 )
            {
               add_tag_option( str_this_tag, bCheck );
            }
         }
      }
   }

   virtual void show_handles( const pfc::list_base_const_t<metadb_handle_ptr> & handles )
   {
      m_handles = handles;
      
      m_ignore_changes = true;

      show_handles_files( m_handles );

      int nHandles = m_handles.get_count();
      HWND hwndList = get_list();

      for ( int n = 0; n < ListView_GetItemCount( hwndList ); n++ )
      {
         ListView_SetCheckState( hwndList, n, FALSE );
      }

      for ( int n = 0; n < tabsize( g_arRatingButtonIDs ); n++ )
      {
         CheckDlgButton( m_wnd, g_arRatingButtonIDs[n], BST_UNCHECKED );
      }

      for ( int n = 0; n < m_handles.get_count(); n++ )
      {
         file_info_impl info;
         m_handles[n]->get_info( info );

         add_tags_from_file_if_necessary( info, n == 0 );

         for ( int m = 0; m < ListView_GetItemCount( hwndList ); m++ )
         {
            pfc::string8 str_tag;
            listview_helper::get_item_text( hwndList, m, 0, str_tag );
            
            if ( file_has_tag( str_tag, info ) )
            {
               // INDETERMINATE => INDETERMINATE
               // CHECKED       => CHECKED
               // UNCHECKED     => CHECKED (iff n = 0), INDETERMINATE (otherwise) 
               if ( BST_UNCHECKED == get_tag_state( m ) )
               {
                  set_tag_state( m, n == 0 ? BST_CHECKED : BST_INDETERMINATE );
               }
            }
            else
            {
               // CHECKED        => INDETERMINATE
               // UNCHECKED      => UNCHECKED
               // INDETERMINTE   => INDETERMINATE
               if ( BST_CHECKED == get_tag_state( m ) )
               {
                  set_tag_state( m, BST_INDETERMINATE );
               }
            }
         }

         const char * str_rating = info.meta_get( TAG_RATING, 0 );
         int nRatingIndex = 0;         
         if ( str_rating )
         {
            int nRating = atoi( str_rating );
            if ( nRating > 0 && nRating <= 5 )
            {
               nRatingIndex = nRating;
            }
         }

         CheckDlgButton( m_wnd, g_arRatingButtonIDs[nRatingIndex], BST_CHECKED );
      }

      sort_tags();

      m_ignore_changes = false;
      enable_buttons( false );
   }

   virtual void on_selection_change()
   {
      static_api_ptr_t<playlist_manager> pm;
      metadb_handle_list handles;
      pm->activeplaylist_get_selected_items( handles );
      if ( handles.get_count() > 0 )
      {
         show_handles( handles );
      }
   }

   virtual HWND get_list()
   {
      return GetDlgItem( m_wnd, IDC_LIST_TAGS );
   }

   virtual void show_handle_tags( metadb_handle_ptr handle )
   {  
      metadb_handle_list list;
      list.add_item( handle );
      show_handles( list );
   }
};


void tagger_window_destroyer::on_completion( unsigned p_code )
{
   if ( p_code == metadb_io::update_info_success )
   {
      DestroyWindow( m_tagger->m_wnd );
      delete m_tagger;
   }
   else
   {
   }
}

class tagger_window_standalone : public tagger_window_base
{
   virtual void on_apply( completion_notify_ptr p_notify = NULL )
   {          
      service_ptr_t<tagger_window_destroyer> p = new service_impl_t<tagger_window_destroyer>( this );
      __super::on_apply( p );
   }

   virtual void on_close()
   {
      __super::on_close();

      DestroyWindow( m_wnd );
      delete this;
   }
};

class tagger_window : public ui_extension::window, public play_callback, public playlist_callback, public tagger_window_base
{
public:
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
      out.set_string( "Tagger");
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
      out.set_string( "Window for tagging songs" );
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
         m_wnd = uCreateDialog( IDD_TAGGER, wnd_parent, tagger_window_base::dialog_proc, (LPARAM) dynamic_cast<tagger_window_base*>( this ) );

         static_api_ptr_t<play_callback_manager>()->register_callback( this, play_callback::flag_on_playback_new_track, true );
         static_api_ptr_t<playlist_manager>()->register_callback( this, playlist_callback::flag_on_items_selection_change );

         g_available_tagger_windows.add_item( this );
         return m_wnd;
      }
   }

	/**
	* \brief Destroys the extension window.
	*/
	virtual void destroy_window()
   {
      // console::printf( "tagger_window::destroy_window" );
      static_api_ptr_t<play_callback_manager>()->unregister_callback( this );
      static_api_ptr_t<playlist_manager>()->unregister_callback( this );

      DestroyWindow( m_wnd );
      m_wnd    = NULL;
      // g_tagger = NULL;
      g_available_tagger_windows.remove_item( this );
      clear_cached_handles();
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
      return g_tagger_window_guid;
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
      out.set_string( "Tagger Window" );
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
      int nVersion;
      abort_callback_impl abort;
      if ( p_reader->read( &nVersion, sizeof( nVersion ), abort ) )
      {
         p_reader->read_string( m_tag_meta, abort );
         p_reader->read_string( m_tag_options, abort );
         p_reader->read( &m_show_ratings, sizeof(m_show_ratings), abort );

         if ( nVersion > 1 )
         {
            p_reader->read( &m_show_files, sizeof(m_show_files), abort );

            if ( nVersion > 2 )
            {
               p_reader->read( &m_scan_library, sizeof(m_scan_library), abort );
            }
         }
      }
      else
      {
         m_tag_meta     = cfg_tagger_meta;
         m_tag_options  = cfg_tagger_tags;
         m_show_ratings = cfg_tagger_show_ratings;
         m_show_files   = true;
      }

      if ( infinite == m_tag_options.find_first( TAG_DELIM ) )
      {
         while ( 1 )
         {
            pfc::string8 tmp( m_tag_options );
            if ( string_replace( m_tag_options, tmp, " ", ";" ) < 0 )
               break;
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
      int nVersion = 3;
      abort_callback_impl abort;
      p_writer->write( &nVersion, sizeof(nVersion), abort );
      p_writer->write_string( m_tag_meta, abort );
      p_writer->write_string( m_tag_options, abort );
      p_writer->write( &m_show_ratings, sizeof(m_show_ratings), abort );
      p_writer->write( &m_show_files, sizeof(m_show_files), abort );
      p_writer->write( &m_scan_library, sizeof(m_scan_library), abort );
   }



   //// play_callback
   virtual unsigned get_flags()
   {
      return flag_on_playback_new_track;
   }

 	//! Playback advanced to new track.
	virtual void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track)
   {
      show_handle_tags( p_track );
   }

   virtual void FB2KAPI on_playback_starting(play_control::t_track_command p_command,bool p_paused){}
   virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason){}
	virtual void FB2KAPI on_playback_seek(double p_time){}
	virtual void FB2KAPI on_playback_pause(bool p_state){}
	virtual void FB2KAPI on_playback_edited(metadb_handle_ptr p_track) {}
	virtual void FB2KAPI on_playback_dynamic_info(const file_info & p_info) {}
	virtual void FB2KAPI on_playback_dynamic_info_track(const file_info & p_info) {}
	virtual void FB2KAPI on_playback_time(double p_time) {}
	virtual void FB2KAPI on_volume_change(float p_new_val) {}

   // playlist_callback
	virtual void FB2KAPI on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state)
   {
      on_selection_change();
   }

   virtual void FB2KAPI on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection){}
	virtual void FB2KAPI on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count){}
   virtual void FB2KAPI on_items_removing(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	virtual void FB2KAPI on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	virtual void FB2KAPI on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to){}
	virtual void FB2KAPI on_items_modified(t_size p_playlist,const bit_array & p_mask){}
	virtual void FB2KAPI on_items_modified_fromplayback(t_size p_playlist,const bit_array & p_mask,play_control::t_display_level p_level){}
	virtual void FB2KAPI on_items_replaced(t_size p_playlist,const bit_array & p_mask,const pfc::list_base_const_t<t_on_items_replaced_entry> & p_data){}
	virtual void FB2KAPI on_item_ensure_visible(t_size p_playlist,t_size p_idx){}
	virtual void FB2KAPI on_playlist_activate(t_size p_old,t_size p_new){}
	virtual void FB2KAPI on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len){}
	virtual void FB2KAPI on_playlists_reorder(const t_size * p_order,t_size p_count){}
	virtual void FB2KAPI on_playlists_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	virtual void FB2KAPI on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	virtual void FB2KAPI on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len){}
	virtual void FB2KAPI on_default_format_changed(){}
	virtual void FB2KAPI on_playback_order_changed(t_size p_new_index){}
	virtual void FB2KAPI on_playlist_locked(t_size p_playlist,bool p_locked){}
};

static uie::window_factory<tagger_window> g_tagger_window;

static const GUID guid_tagger_context_000 = { 0xba1c2b8b, 0x8ad3, 0x4479, { 0x89, 0x7c, 0xab, 0x26, 0x66, 0x91, 0xa9, 0x1d } };

class tagger_contextmenu_items : public contextmenu_item_simple
{
   unsigned get_num_items()
   {
      return 1;
   }

   GUID get_item_guid( unsigned int n )
   {
      switch ( n )
      {
      case 0:
         return guid_tagger_context_000;
      }
      return pfc::guid_null;
   }
   
   void get_item_name( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case 0:
         out.set_string( "Tag in Tagger Window" );
         break;
      }
   }

   void get_item_default_path( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case 0:
         out.set_string( "" );
         break;
      }
   }

   bool get_item_description( unsigned n, pfc::string_base & out )
   {
      switch ( n )
      {
      case 0:
         out.set_string( "Populate Tagger Window with Files" );
         return true;
      }

      return false;
   }

   virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
   {
      switch ( p_index )
      {
      case 0:
         {
            if ( g_available_tagger_windows.get_count() == 0 )
            {
               tagger_window_standalone * p = new tagger_window_standalone();
               HWND hwnd = uCreateDialog( IDD_TAGGER_WINDOW_STANDALONE, core_api::get_main_window(), tagger_window_standalone::dialog_proc, (LPARAM)p );
               ::ShowWindow( hwnd, SW_SHOWNORMAL );
               p->show_handles( p_data );
            }
            else
            {
               for ( int n = 0; n < g_available_tagger_windows.get_count(); n++ )
               {
                  g_available_tagger_windows[n]->show_handles( p_data );
               }
            }
         }
         break;
      }
   }
};

static service_factory_single_t<tagger_contextmenu_items> g_contextmenu_items;
