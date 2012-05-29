#pragma once

static const GUID guid_tree_searcher = { 0x9fdc37c6, 0x22af, 0x4461, { 0x92, 0xb7, 0x27, 0xac, 0x4c, 0xde, 0xfd, 0xf } };

#define WM_SETCOLOR  (WM_APP+1)

#define MY_TEXT_COLOR   (cfg_text_color)
#define COLOR_MISSING   (RGB(255,0,0))

static WNDPROC EditProc;

class tree_searcher : public ui_extension::window
{  
   static LRESULT CALLBACK EditBoxHook( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      switch ( msg )
      {
      case WM_CHAR:
         switch ( wp )
         {
         case VK_RETURN:
            {
               pfc::string8 string;
               uGetWindowText( wnd, string );

               if ( g_last_active_tree )
               {
                  if ( g_last_active_tree->search( string ) )
                  {
                     PostMessage( GetParent(wnd), WM_SETCOLOR, MY_TEXT_COLOR, 0 );      
                  }
                  else
                  {
                     PostMessage( GetParent(wnd), WM_SETCOLOR, COLOR_MISSING, 0 );      
                  }
               }
            }
            break;

         case VK_ESCAPE:
            g_last_active_tree->search( NULL );
            uSetWindowText( wnd, "" );
            PostMessage( GetParent(wnd), WM_SETCOLOR, MY_TEXT_COLOR, 0 );      
            break;

         default:
            PostMessage( GetParent(wnd), WM_SETCOLOR, MY_TEXT_COLOR, 0 );      
            break;
         }
            
         break;
      }
      return uCallWindowProc( EditProc, wnd, msg, wp, lp );
   }

   static BOOL CALLBACK dialog_proc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      static HBRUSH brush = NULL;
      static int edit_color = MY_TEXT_COLOR;

      switch ( msg )
      {
      case WM_DESTROY:
         if ( brush )
         {
            DeleteObject( brush );
         }
         return false;

      case WM_SETCOLOR:
         edit_color = wp;
         InvalidateRect( wnd, NULL, TRUE );
         break;

      case WM_CTLCOLOREDIT:                          
         {
            SetTextColor((HDC)wp, edit_color );
            SetBkColor( (HDC)wp, cfg_back_color );

            if ( brush )
            {
               DeleteObject( brush );
            }

            brush = CreateSolidBrush( cfg_back_color );
            SelectObject( (HDC) wp, brush );
            return (BOOL)brush;
         }
         break;

      case WM_INITDIALOG:
         {
		      EditProc = uHookWindowProc( GetDlgItem( wnd, IDC_SEARCH_STRING ), EditBoxHook );
         }
         break;      

	   case WM_GETMINMAXINFO:
		   {
			   LPMINMAXINFO info = (LPMINMAXINFO) lp;
			   info->ptMinTrackSize.y = 22;
			   return 0;
		   }

      case WM_SIZE:
         {
            int height = 20;
            int offset = 1;

            RECT r;
            GetWindowRect( wnd, &r );	          
            SetWindowPos( GetDlgItem( wnd, IDC_SEARCH_STRING ), 0, 
               offset, offset, r.right-r.left - 2 * offset, height, 
               SWP_NOZORDER );
         }
         break;

      }
      return FALSE;
   }

public:
   HWND  m_wnd;

   ui_extension::window_host_ptr m_host;

   tree_searcher()
   {
      m_wnd = NULL;
   }

   unsigned get_type() const 
   {
      return ui_extension::type_toolbar;
   }

   void destroy_window()
   {
      dialog_proc( m_wnd, WM_DESTROY, 0, 0 );
      m_wnd = NULL;      
   }

   void get_category( pfc::string_base & out ) const
   {
      out.set_string( "Toolbars" );
   }

   virtual bool get_description( pfc::string_base & out ) const
   {
      out.set_string( "Playlist Tree Search Tool" );
      return true;
   }

   virtual bool get_short_name( pfc::string_base & out ) const 
   {
      out.set_string( "Playlist Tree Search" );
      return true;
   }

   virtual void get_name( pfc::string_base & out ) const
   {
      out.set_string( "Playlist Tree Search" );
   }

   const GUID & get_extension_guid() const
   {
      return  guid_tree_searcher;
   }

   void get_size_limits( ui_extension::size_limit_t & p_out ) const
   {
      p_out.min_height = 20;
      p_out.min_width = 30;
   }
   
   virtual HWND get_wnd() const
   {
      return m_wnd;
   }

   virtual bool is_available( const ui_extension::window_host_ptr & p_host) const 
   {
      return ( m_wnd == NULL );
   }

   virtual HWND create_or_transfer_window( HWND wnd_parent, const ui_extension::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position ) 
   {
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
         m_wnd = uCreateDialog( IDD_SEARCH, wnd_parent, dialog_proc, (LPARAM)this );


         switch ( cfg_edge_style )
         {
         case EE_NONE:
            SetWindowLong( GetDlgItem( m_wnd, IDC_SEARCH_STRING ), GWL_EXSTYLE, MY_EDGE_NONE );
            break;
         case EE_GREY:
            SetWindowLong( GetDlgItem( m_wnd, IDC_SEARCH_STRING ), GWL_EXSTYLE, MY_EDGE_GREY );
            break;
         case EE_SUNKEN:
            SetWindowLong( GetDlgItem( m_wnd, IDC_SEARCH_STRING ), GWL_EXSTYLE, MY_EDGE_SUNKEN );
            break;
         }

         return m_wnd;
      }
   }

   const bool get_is_single_instance() const 
   {
      return true;
   }
};

//static ui_extension::window_factory_single<tree_searcher> g_tree_searcher();
static service_factory_single_t<tree_searcher> g_tree_searcher;