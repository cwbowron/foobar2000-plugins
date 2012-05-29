#pragma once

enum { MENU_CANCELLED, MENU_SAVE_TREE, MENU_LOAD_TREE, MENU_SAVE_NODE, 
MENU_LOAD_NODE, MENU_EDIT, MENU_NEW_FOLDER, MENU_NEW_QUERY, 
MENU_REMOVE, MENU_SORT_CHILDREN, MENU_SORT_RECURSIVE,
MENU_REFRESH_QUERIES,
MENU_LAST };

enum { HIDDEN_NOT, HIDDEN_GRAYED, HIDDEN_COMPLETELY, HIDDEN_LEAVES };

#include <stdio.h>

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"

#define STRING_INVALID_HANDLE "<Invalid Handle>"

// from foo_browser
static const GUID foo_browser_context_guid_001 = { 0x4fe8e38a, 0x7efb, 0x4ef5, { 0x95, 0xd4, 0xb5, 0xdc, 0xa5, 0x0, 0x8f, 0xa } };

bool is_dotted_directory( const char * dir )
{
   if ( strcmp( dir, "." ) == 0 )
      return true;
   if ( strcmp( dir, ".." ) == 0 )
      return true;
   return false;
}

// forward declarations
int node_compare( const node * a, const node * b );
class folder_node;
class leaf_node;
// 

IDataObject *g_fb2k_data_object = NULL;

//pfc::com_ptr_t<IDataObject> g_fb2k_data_object = NULL;
class callback_node
{
public:
   virtual bool   get_entries( pfc::list_base_t<metadb_handle_ptr> & list ) const = 0;
   virtual bool   is_leaf() const     = 0;
   virtual bool   is_folder() const   = 0;
   virtual bool   is_query() const    = 0;
   virtual int    get_entry_count() const = 0;
   virtual void   get_name( pfc::string_base & out ) const = 0; 
};

class NOVTABLE node_select_callback : public service_base 
{
public:
   virtual void on_node_select( const callback_node * node ) = 0;

   FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(node_select_callback);
};

const GUID node_select_callback::class_guid = 
{ 0x9c2ae3c3, 0xdc04, 0x4042, { 0xad, 0xf3, 0x88, 0x84, 0x11, 0x7b, 0x49, 0x55 } };

class notify_node_listeners : public main_thread_callback
{
   callback_node * m_callback_node;

public:
   notify_node_listeners( callback_node * n ) : main_thread_callback()
   {
      m_callback_node = n;
   }

   void callback_run()
   {
      service_enum_t<node_select_callback> e; 
      service_ptr_t<node_select_callback> ptr; 
      while( e.next( ptr ) ) 
      {
         ptr->on_node_select( m_callback_node );
      }
   }
};

void post_callback_notification( callback_node * n )
{
   if ( core_api::are_services_available() && !core_api::is_initializing() && !core_api::is_shutting_down() )
   {
      service_ptr_t<main_thread_callback> p_callback = new service_impl_t<notify_node_listeners>( n );
      static_api_ptr_t<main_thread_callback_manager> man;
      man->add_callback( p_callback );
   }
}

/*
class sample_callback : public node_select_callback
{
public:
   void on_node_select( const callback_node * n )
   {
      pfc::string8 str_name;
      n->get_name( str_name );
      console::printf( "node callback: %s", str_name.get_ptr() );
   }
};

service_factory_single_t<sample_callback> g_sample_callback;
*/

class node : public IDataObject, public callback_node
{
public:
   pfc::string8   m_label;
   pfc::string8   m_displayedname;

   HTREEITEM      m_tree_item;
   HWND           m_hwnd_tree;
   folder_node *  m_parent;

   long           m_refcount;
   int            m_custom_icon;
   unsigned       m_hidden;
   unsigned       m_custom_color;

   // for menus or what not
   int            m_id;

   bool           m_fakelevel;

   node( const char * label )
   { 
      m_label        = label;
      m_tree_item    = NULL;
      m_hwnd_tree    = NULL;
      m_parent       = NULL;
      m_refcount     = 1;
      m_hidden       = 0;
      m_custom_icon  = -1;
      m_id           = -1;
      m_custom_color = CLR_DEFAULT;

      m_fakelevel    = false;
   }

   virtual ~node();

   node * get_root();

   virtual bool get_entries( pfc::list_base_t<metadb_handle_ptr> & list ) const = 0;

   virtual node * add_delimited_child( const char *strings, metadb_handle_ptr handle );
   virtual node * add_child( node * n, unsigned pos = ~0 ) { return NULL; }
   
   virtual node * add_child_conditional( const char * str, bool process_tags = true  ) 
   {
      return NULL; 
   }      

   virtual node * find_by_id( int id )
   {
      if ( m_id == id ) return this;
      return NULL;
   }

   virtual node *add_child_query_conditional( const char *, const char *, const char *, const char *, const char * );

   virtual bool can_add_child() { return false; }
   virtual int get_child_index( node *child ) { return -1; }
   virtual bool has_subchild( node *test ) { return false; }
   virtual bool remove_child( node *child ) { return false; }
   virtual node *copy() = 0;

   virtual __int64 get_file_size() = 0;
   virtual double get_play_length() = 0;
   virtual node * get_parent( int n = 1 );
   
   virtual void get_name( pfc::string_base & out ) const
   {
      out.set_string( m_displayedname );
   }

   virtual void collapse( bool recurse )
   {
   }
   
   virtual void files_moved( const pfc::list_base_const_t<const char *> & p_from,const pfc::list_base_const_t<const char *> & p_to )
   {
   }

   virtual void files_removed( const pfc::list_base_const_t<const char *> & p_items )
   {
   }

   virtual bool find_node( node *needle, int *running_total )
   {
		if ( this == needle )
			return true;
      return false;
   }

	int get_folder_overall_index()
	{
		int ret = 0;
		get_root()->find_node( this, &ret );
		return ret;
	}

   virtual void add_to_hmenu( HMENU menu, int & baseID, pfc::ptr_list_t<pfc::string8> options )
   {      
   }

   virtual bool is_leaf() const { return false; };
   virtual bool is_folder() const { return false; }
   virtual bool is_query() const { return false; }   
   virtual node * find_by_name( const char * name ) { return NULL; }

   virtual int get_entry_count() const = 0;

   virtual bool edit() { return false; }
   virtual int get_folder_count( bool recurse = false ) { return 0; }

   virtual bool get_first_entry( metadb_handle_ptr & entry ) = 0;

   virtual bool rename_as_tag( const char * tag )
   {
      metadb_handle_ptr first;

      if ( get_first_entry( first ) )
      {
         service_ptr_t<titleformat_object> tf_ptr;
	
         pfc::string8 tf_string;
         static_api_ptr_t<titleformat_compiler> tc;
         titleformat_text_out_impl_string t_obj( tf_string );            
         tc->compile_safe( tf_ptr, tag );

         first->format_title( NULL, m_label, tf_ptr, NULL );
         refresh_tree_label();
         return true;
      }
      return false;
   }

	virtual int get_nest_level();
   virtual void sort( bool recurse ) { } 
   virtual void repopulate() { }
   virtual bool repopulate_auto( int reason, WPARAM dw = 0, LPARAM lp = 0 ) { return false; }

   virtual node *iterate_next();

   virtual int get_bitmap_index()
   {
      if ( m_custom_icon >= 0 )
         return m_custom_icon;
      else
         return get_default_bitmap_index();
   }

   /*
   virtual void set_display_redraw( bool redraw, bool force_redraw = false )
   {
      static int false_counts = 0;

      if ( force_redraw )
      {

      }
      else
      {
         if ( redraw )
         {
            false_counts--;
         }
         else
         {
            false_counts++;
         }
      }
      
   }
   */

   virtual int get_default_bitmap_index(){ return -1; }

   virtual void get_display_name( pfc::string_base & out ) 
   { 
      out.set_string( m_label ); 
   }

   virtual void populate_treeview_item( TVITEMEX *item )
   {
      static pfc::string8_fastalloc display_name;
      static pfc::stringcvt::string_wide_from_utf8 wide_title;

      get_display_name( display_name );
      wide_title.convert( display_name );

      item->pszText = (LPWSTR)(const wchar_t *) wide_title;
      item->lParam = (LPARAM)this;
      item->iImage = get_bitmap_index();
      item->iSelectedImage = get_bitmap_index();
      item->mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
      item->stateMask |= TVIS_CUT;

      if ( m_tree_item )
      {
         item->hItem = m_tree_item;
         item->mask |= TVIF_HANDLE;
      }

      if ( m_hidden == HIDDEN_GRAYED )
      {
         item->state |= TVIS_CUT;
      }
   }

   virtual bool get_formatting_handle( metadb_handle_ptr & handle ) = 0;
   
   virtual bool format_title( pfc::string_base & out, const char * in )
   {
      metadb_handle_ptr h;
      get_formatting_handle( h );

      if ( !h.is_empty() )
      {
         service_ptr_t<titleformat_object> script;
	      
         static_api_ptr_t<titleformat_compiler>()->compile_safe(script,in);

         g_format_hook->set_node( this );
         h->format_title( g_format_hook, out, script, NULL );
         return true;
      }
      return false;
   }

   virtual bool hide_leaves();
   /*
   {
      return cfg_hideleaves;
   }
   */

   virtual void setup_tree( HWND hwnd_tree, HTREEITEM parent_tree )
   {	      
      if ( hide_leaves() && is_leaf() ) return;
      if ( m_hidden == HIDDEN_COMPLETELY ) return;

      static TVINSERTSTRUCT is;
      memset( &is, 0, sizeof(is) );
      is.hParent = parent_tree;
      is.hInsertAfter = TVI_LAST;
      populate_treeview_item( &is.itemex );
      m_tree_item = TreeView_InsertItem( hwnd_tree, &is );
      m_hwnd_tree = hwnd_tree;
   }   

   virtual void remove_tree()
   {
      if ( !m_fakelevel )
      {
         if ( m_tree_item && m_hwnd_tree )
         {
            TreeView_DeleteItem( m_hwnd_tree, m_tree_item );
         }
      }
   }

   void ensure_in_tree();

   virtual void refresh_subtree( HWND hwnd_tree ) { }

   void refresh_tree_label()
   {
      TVITEMEX item;
      memset( &item, 0, sizeof(item) );
      populate_treeview_item( &item );
      SendMessage( m_hwnd_tree, TVM_SETITEM, 0, (WPARAM)&item );
   }

   virtual void force_tree_selection()
   {
      TreeView_EnsureVisible( m_hwnd_tree, m_tree_item );
      TreeView_SelectItem( m_hwnd_tree, m_tree_item );
   }

   virtual void select();
   /*
   {          
      post_callback_notification( this );
      static_api_ptr_t<metadb> db;
      db->database_lock();
      metadb_handle_list lst;
      get_entries( lst );
      db->database_unlock();

      if ( lst.get_count() > 0 )
      {
         bool overLimit = ( lst.get_count() > cfg_selection_action_limit );
         if ( cfg_selection_action_limit == 0 || !overLimit )
         {
            run_context_command( cfg_selection_action_1, &lst, this );
            run_context_command( cfg_selection_action_2, &lst, this );
         }
         else if ( overLimit )
         {
            console::printf( "%s exceeds selection action limit",
               (const char *) m_label );
         }
      }
   }
   */

   virtual HMENU generate_context_menu();
   /*
   {
      return NULL;
   }
   */
   virtual void handle_context_menu_command( int n );
   /*
   {
   }
   */
   virtual void do_node_context_menu( POINT * pt = NULL )
   {
      HMENU m = generate_context_menu();

      if ( m )
      {
         POINT p;

         if ( pt ) 
         { 
            p.x = pt->x;
            p.y = pt->y;
         }
         else
         {
            /*
            RECT rect;

            TreeView_GetItemRect( m_hwnd_tree, m_tree_item, &rect, TRUE );

            p.x = rect.right;
            p.y = rect.bottom;        
            ClientToScreen( m_hwnd_tree, &p );
            */
            GetCursorPos( &p );
         }

         int cmd = TrackPopupMenu( m, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, 
            p.x, p.y, 0, m_hwnd_tree, 0);

         if ( cmd ) 
         {
            handle_context_menu_command( cmd );
         }
      }

      DestroyMenu( m );
   }

   virtual void do_foobar2000_context_menu( POINT * pt = NULL )
   {
      POINT p;

      if ( pt )
      {
         p.x = pt->x;
         p.y = pt->y;
      }
      else
      {   
         /*
         RECT rect;

         TreeView_GetItemRect( m_hwnd_tree, m_tree_item, &rect, TRUE );

         p.x = rect.right;
         p.y = rect.bottom;        
         ClientToScreen( m_hwnd_tree, &p );
         */
         GetCursorPos( &p );
      }

      metadb_handle_list handles;
      get_entries( handles );
      static_api_ptr_t<contextmenu_manager> man;
      man->win32_run_menu_context( m_hwnd_tree, handles, &p );
   }

   virtual void do_context_menu( POINT *pt = NULL )
   {        
      if ( shift_pressed() )
      {
         do_node_context_menu( pt );
      }
      else
      {
         do_foobar2000_context_menu( pt );
      }
   }

   node * process_dropped_file( const char * file, bool do_not_nest = false, 
      int index = ~0, bool dynamic = false, metadb_handle_list * h = NULL );

   node * process_hdrop( HDROP hDrop, int index = ~0, bool dynamic = false, metadb_handle_list * h = NULL );

   bool do_drag_drop( playlist_tree_panel * panel, bool copy = false )
   {
      tree_drop_source drop_source( panel, this );

      metadb_handle_list list;
      get_entries( list );

      DWORD dropeffect_in;

      if ( copy )
      {
         dropeffect_in = DROPEFFECT_COPY;
      }
      else
      {                  
         dropeffect_in = DROPEFFECT_COPY | DROPEFFECT_MOVE;
      }

      DWORD dropeffect_out;
      
      // service_ptr_t<playlist_incoming_item_filter> filter;

      static_api_ptr_t<playlist_incoming_item_filter> filter;
      //if ( playlist_incoming_item_filter::g_get( filter ) )

      {
         g_fb2k_data_object = filter->create_dataobject( list );
      }      

      DoDragDrop( this, &drop_source, dropeffect_in, &dropeffect_out );         

      if ( g_fb2k_data_object )
      {
         g_fb2k_data_object->Release();
         g_fb2k_data_object = NULL;
      }

      return dropeffect_out != DROPEFFECT_NONE;
   }

   virtual void write_sexp( FILE *f, int offset = 0 ) = 0;
   void write( FILE *f, int level ) { write_sexp( f, 0 ); }

   virtual bool write( const char * file_name )
   {
      pfc::stringcvt::string_wide_from_utf8 wide_name( file_name );
      FILE *out = _wfopen( wide_name, _T("w+") );

      if ( out == NULL )
      {
         console::warning( pfc::string_printf("Error opening %s for writing", 
            (const char * ) file_name ) );
         return false;
      }

      write( out, 0 );
      fclose( out );
      return true;
   }

   virtual void write_text( FILE * f, bool only_visible = false, int indent = 0 )
   {
      for ( int i = 0; i < indent; i++ )
      {
         fprintf( f, "%s", (const char *) cfg_export_prefix );
      }
      
      pfc::string8 name;
      get_display_name( name );

      fprintf( f, " %s\n", (const char *) name );
   }


   bool save( pfc::string8 * out_name = NULL )
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

         write( out, 0 );
         fclose( out );

         if ( out_name )
         {
            out_name->set_string( file_name );
         }
         return true;
      } 
      return false;
   }


   // IDataObject
   virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid,void ** ppvObject )
   {
      if ( iid == IID_IUnknown ) 
      {
         AddRef();
         *ppvObject = (IUnknown*)this;
         return S_OK;
      } 
      else if ( iid == IID_IDataObject) 
      {
         AddRef();
         *ppvObject = (IDataObject*)this;
         return S_OK;
      }
      else if ( g_fb2k_data_object )
      {
         return g_fb2k_data_object->QueryInterface( iid, ppvObject );
      }
      else 
      {
         return E_NOINTERFACE;
      }
   }

   virtual ULONG STDMETHODCALLTYPE AddRef() 
   {
      return InterlockedIncrement(&m_refcount);
   }

   virtual ULONG STDMETHODCALLTYPE Release()
   {
      return InterlockedDecrement(&m_refcount);
   }

   HRESULT STDMETHODCALLTYPE GetData( FORMATETC * pFormatetc, STGMEDIUM * pmedium )
   {
      if ( pFormatetc->cfFormat == CF_NODE ) 
      {
         pmedium->tymed = TYMED_NULL;
         return S_OK;
      }
      else if ( g_fb2k_data_object )
      {
         // console::printf( "GetData: Format = %d", pFormatetc->cfFormat );
         HRESULT hrResult = g_fb2k_data_object->GetData( pFormatetc, pmedium );
         // console::printf( "Returning: %d", hrResult );
         return hrResult;
      }
      else
      {
         return DV_E_FORMATETC;
      }
   }

   HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC * pFormatetc, STGMEDIUM * pmedium)
   {
      return S_OK;
   }

   HRESULT STDMETHODCALLTYPE QueryGetData( FORMATETC * pFormatEtc )
   {
      if ( pFormatEtc->cfFormat == CF_NODE )
      {
         return S_OK;
      }
      if ( g_fb2k_data_object )
      {
         // console::printf( "GetQueryData: Format = %d", pFormatEtc->cfFormat );
         HRESULT hrResult = g_fb2k_data_object->QueryGetData( pFormatEtc );
         // console::printf( "Returning: %d", hrResult );
         return hrResult;
      }
      else
      {
         return DV_E_FORMATETC;
      }
   }

   HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC * pFormatetcIn, FORMATETC * pFormatetcOut)
   {
      return S_OK;
   }

   HRESULT STDMETHODCALLTYPE SetData(FORMATETC * pFormatetc, STGMEDIUM * pmedium,BOOL fRelease)
   {
      return S_OK;
   }

   HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatetc)
   {
      return S_OK;
   }

   HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC * pFormatetc, DWORD advf, IAdviseSink * pAdvSink,DWORD * pdwConnection)
   {
      return OLE_E_ADVISENOTSUPPORTED;
   }

   HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection)
   {
      return OLE_E_ADVISENOTSUPPORTED;
   }

   HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
   {		
      return OLE_E_ADVISENOTSUPPORTED;
   }
   // End IDataObject

};

#define for_each_child( n, fn ) { int n, m = get_num_children(); for(n=0;n<m;n++) { fn; } }
#define for_each_child_rev( n, fn ) { int n, m = get_num_children(); for(n=m-1;n>=0;n--) { fn; } }

class folder_node : public node
{
public:
   pfc::ptr_list_t<node>   m_children;
   bool                    m_expanded;
   unsigned                m_child_limit;

   folder_node( const char * label, bool expanded = false ) : node( label )
   {
      m_expanded     = expanded;
      m_child_limit  = 0;
      m_fakelevel    = false;
   }

   virtual ~folder_node()
   {
      free_children();
   }

   virtual void free_children()
   {
      for_each_child_rev( n, delete m_children[n] );
   }

   __int64 get_file_size() 
   {
      __int64 length = 0;
      unsigned i;

      for (i=0;i<get_num_children();i++)
      {
         length += m_children[i]->get_file_size();
      }
      return length;
   }

   double get_play_length()
   {
      double length = 0.0;
      for (unsigned i=0;i<get_num_children();i++)
      {
         length += m_children[i]->get_play_length();
      }
      return length;
   }

   virtual int get_entry_count() const
   {
      int c = 0;

      int n;
      for ( n = 0; n < m_children.get_count(); n++ )
      {
         c += m_children[n]->get_entry_count();         
      }   

      return c;
   }

   virtual int get_folder_count( bool recurse = false )
   {
      int total = 0;
      for ( unsigned i = 0; i < get_num_children(); i++ )
      {
         if ( recurse )
            total += m_children[i]->get_folder_count( recurse ) + 1;
         else
            total++;
      }
      return total;
   }

   virtual bool is_folder() const { return true; }

   virtual void files_moved( const pfc::list_base_const_t<const char *> & p_from,const pfc::list_base_const_t<const char *> & p_to )
   {
      for_each_child( n, m_children[n]->files_moved( p_from, p_to ) );
   }

   virtual void files_removed( const pfc::list_base_const_t<const char *> & p_items )
   {
      for_each_child_rev( n, m_children[n]->files_removed( p_items ) );
   }


   virtual node * find_by_id( int id )
   {
      if ( m_id == id ) return this;

      int n;
      for ( n = 0; n < get_num_children(); n++ )
      {
         node * r = m_children[n]->find_by_id( id );
         if ( r ) return r;
      }
      return NULL;
   }

   virtual bool can_add_child() 
   { 
      if ( m_child_limit )
      {
         if ( get_num_children() >= m_child_limit )
         {
            return false;
         }
      }
      return true; 
   }

   virtual void write_text( FILE * f, bool only_visible = false, int indent = 0 )
   {
      node::write_text( f, only_visible, indent );

      if ( m_expanded || ( only_visible == false ) )
      {
         for_each_child( n, m_children[n]->write_text( f, only_visible, indent+1 ) );
      }
   }

   virtual void add_to_hmenu( HMENU menu, int & baseID, pfc::ptr_list_t<pfc::string8> options )
   {      
      if ( get_num_children() > 1 )
      {
         uAppendMenu( menu, MF_POPUP, (UINT_PTR) CreatePopupMenu(), "All" );
         uAppendMenu( menu, MF_SEPARATOR, 0, 0 );
      }

      for ( int n = 0; n < get_num_children(); n++ )
      {
         pfc::string8 strChild;

         m_children[n]->get_display_name( strChild );
         HMENU child = CreatePopupMenu();
         m_children[n]->m_id = (int)child;
         uAppendMenu( menu, MF_POPUP, (UINT_PTR)child, strChild );
         //baseID++;
      }
   }


   bool apply_format_if_necessary( pfc::string_base & out, const char * in )
   {
      bool found = false;

      while ( strstr( in, TAG_FORMAT ) )
      {
         pfc::string8_fastalloc copy = in;
         pfc::string8_fastalloc param;
         pfc::string8_fastalloc processed;
         int start;
         int end;

         found = true;

         if ( string_get_function_param( param, copy, TAG_FORMAT, &start, &end ) == 0 )
            break;

         g_format_hook->set_node( this );


         service_ptr_t<titleformat_object> script;
	      static_api_ptr_t<titleformat_compiler>()->compile_safe(script,param);
         //g_silence_handle->format_title( g_format_hook, processed, param, NULL );
         g_silence_handle->format_title( g_format_hook, processed, script, NULL );

         string_replace( out, copy, start, end + 1, processed );

         in = out.get_ptr();
      }		

      return found;
   }

   virtual bool get_first_entry( metadb_handle_ptr & entry )
   {
      for_each_child( n, if ( m_children[n]->get_first_entry( entry ) ) return true; );
      return false;
   }

   virtual void reset_tag_variables()
   {
      m_custom_icon  = -1;
      m_hidden       = 0;
      m_child_limit  = 0;
      m_custom_color = CLR_DEFAULT;
      m_fakelevel    = false;
   }

   virtual bool process_quote( pfc::string_base & out, const char * in )
   {
      pfc::string8_fastalloc tmp;         
      int start, end;
      out.set_string( in );
      bool found_flag = false;
      while ( strstr( out, TAG_QUOTE ) )
      {
         if ( string_get_function_param( tmp, out, TAG_QUOTE, &start, &end ) )
         {
            pfc::string8_fastalloc t1;
            string_make_format_friendly( t1, tmp );
            string_replace( tmp, out, start, end + 1, t1 );
            out.set_string( tmp );
            found_flag = true;
         }
         else
         {
            break;
         }
      }
      return found_flag;
   }

   virtual bool process_local_tags( pfc::string_base & out, const char * in )
   {
      // always contains the currently processed version of in... 
      out.set_string( in );

      pfc::string8_fastalloc tmp;         
      int start, end;
      bool found_flag = false;

      if ( strstr( out, TAG_ICON ) )
      {
         if ( string_get_function_param( tmp, out, TAG_ICON, &start, &end ))
         {
            m_custom_icon = atoi( tmp );
            string_replace( tmp, out, start, end+1, "" );
            out.set_string( tmp );
            // console::printf( "after processing @icon: %s", (const char *)out );
            found_flag = true;
         } 
      }

      while ( strstr( out, TAG_QUOTE ) )
      {
         if ( string_get_function_param( tmp, out, TAG_QUOTE, &start, &end ) )
         {
            pfc::string8_fastalloc t1;
            string_make_format_friendly( t1, tmp );
            string_replace( tmp, out, start, end + 1, t1 );
            out.set_string( tmp );
            found_flag = true;
         }
         else
         {
            break;
         }
      }

      while ( strstr( out, TAG_LIMIT ) )
      {
         if ( string_get_function_param( tmp, out, TAG_LIMIT, &start, &end ) )
         {
            m_child_limit = atoi( tmp );
            // console::printf( "@limit: %d", m_child_limit );
            string_replace( tmp, out, start, end + 1, "");
            out.set_string( tmp );
            found_flag = true;
         }
         else
         {
            break;
         }
      }

      if ( apply_format_if_necessary( tmp, out ) )
      {
         out.set_string( tmp );
         found_flag = true;
      }

      while ( strstr( out, TAG_HIDDEN3 ) ) 
      {
         string_replace( tmp, out, TAG_HIDDEN3, "", 0 );
         out.set_string( tmp );
         m_hidden = HIDDEN_LEAVES;
         found_flag = true;
      }

      while ( strstr( out, TAG_HIDDEN2 ) )
      {
         string_replace( tmp, out, TAG_HIDDEN2, "", 0 );
         out.set_string( tmp );
         m_hidden = HIDDEN_COMPLETELY;
         found_flag = true;
      }

      while ( strstr( out, TAG_HIDDEN ) )
      {
         string_replace( tmp, out, TAG_HIDDEN, "", 0 );
         out.set_string( tmp );
         m_hidden = HIDDEN_GRAYED;
         found_flag = true;
      }

      while ( strstr( out, TAG_FAKELEVEL ) )
      {
         string_replace( tmp, out, TAG_FAKELEVEL, "", 0 );
         out.set_string( tmp );
         m_fakelevel = true;
         m_expanded  = true;
         found_flag  = true;
      }

      while ( strstr( out, TAG_RGB ) )
      {
         if ( string_get_function_param( tmp, out, TAG_RGB, &start, &end ) )
         {
            pfc::ptr_list_t<pfc::string8> rgb;
            string_split( tmp, ",", rgb );

            if ( rgb.get_count() == 3 )
            {
               int nR = atoi(*rgb[0]);
               int nG = atoi(*rgb[1]);
               int nB = atoi(*rgb[2]);

               m_custom_color = RGB(nR,nG,nB);
            }
            string_replace( tmp, out, start, end + 1, "");
            out.set_string( tmp );
            found_flag = true;
         }
         else
         {
            break;
         }
      }
      return found_flag;
   }

   virtual void get_display_name( pfc::string_base & out ) 
   { 
      pfc::string8_fastalloc format_applied;

      g_format_hook->set_node( this );

      service_ptr_t<titleformat_object> script;
      static_api_ptr_t<titleformat_compiler>()->compile_safe(script,cfg_folder_display);

      g_silence_handle->format_title( g_format_hook, out, 
         script, NULL );

      reset_tag_variables();

      if ( process_local_tags( format_applied, out ) )
      {
         out.set_string( format_applied );
      }

      m_displayedname.set_string( out );
   }

   virtual int get_child_index( node * child )
   {
      for_each_child( n, if ( m_children[n] == child ) { return n; } );
      return -1;
   }

   virtual bool has_subchild( node * test )
   {
      if (this == test)
         return true;

      for_each_child( n, if ( m_children[n]->has_subchild( test ) ) { return true; } );
      return false;
   }

   virtual void repopulate()
   {
      for_each_child( n, m_children[n]->repopulate() );
   }

   virtual bool repopulate_auto( int reason, WPARAM wp = 0, LPARAM lp = NULL )
   {
      bool result = false;      
      for_each_child( n, if ( m_children[n]->repopulate_auto( reason, wp, lp ) ) result = true; );
      return result;
   }

   virtual bool remove_child( node * child )
   {
      m_children.remove_item( child );
      return 1;
   }

   virtual node * copy()
   {
      node * cpy = new folder_node( m_label, m_expanded );

      for_each_child( n, cpy->add_child( m_children[n]->copy() ) );
      return cpy;
   }

   virtual bool find_node( node *needle, int *running_total )
   {
		if ( this == needle )
			return true;

		for ( unsigned i = 0; i < get_num_children(); i++ )
		{
			if ( !m_children[i]->is_leaf() )
         {
				*running_total += 1;
         }

			if ( m_children[i] == needle )
         {
				return true;
         }
		}

		for ( int i = 0; i < get_num_children(); i++ )
		{
			if ( m_children[i]->find_node( needle, running_total ) )
         {
				return true;
         }
		}

		return false;
	}


   int get_num_children() const { return m_children.get_count(); }

   bool has_child_folder() 
   {
      for_each_child( n, if ( !m_children[n]->is_leaf() ) return true );
      return false;
   }

   inline void add_fake_child( HWND list, HTREEITEM parent )
   {
      static int setFake = 1;
      static uTVINSERTSTRUCT fake;

      // only set this up on the first call...
      if ( setFake )
      {
         memset( &fake, 0, sizeof( fake ));
         fake.hInsertAfter = TVI_LAST;

         fake.item.pszText	   = "FAKE";
         fake.item.state		= 0;
         fake.item.mask		   = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
         fake.item.state		= 0;
         fake.item.stateMask  = TVIS_EXPANDED;
         setFake = 0;
      }

      if ( !cfg_hideleaves || has_child_folder() )
      {
         fake.hParent = parent;
         uTreeView_InsertItem( list, &fake );
      }
   }

   /*
   virtual void setup_children_trees_maybe( HWND tree )
   {
      if ( m_children.get_count() > 0 )
      {
         // theyve been drawn once already, continue drawing them
         if ( m_children[0]->m_tree_item )
         {
            setup_children_trees( m_hwnd_tree );
         }
         else
         {
            add_fake_child( m_hwnd_tree, m_tree_item );
         }
      }
   }
   */

   virtual bool should_add_fake_child()
   {
      pfc::string8 tmp;
      for ( int n = 0; n < get_num_children(); n++ )
      {
         // force processing of local tags
         m_children[n]->get_display_name( tmp );

         if ( m_hidden == HIDDEN_LEAVES )
         {
            if ( m_children[n]->is_leaf() == false )
            {
               if ( m_children[n]->m_hidden != HIDDEN_COMPLETELY )
                  return true;
            }
         }
         else
         {
            if ( m_children[n]->m_hidden != HIDDEN_COMPLETELY )
               return true;
         }
      }
      return false;
   }

   virtual void setup_children_trees( HWND tree )
   {   
      for_each_child( n, m_children[n]->setup_tree( tree, m_tree_item ) );   
   }

   virtual void setup_tree( HWND tree, HTREEITEM parent )
   {
      //SendMessage( m_hwnd_tree, WM_SETREDRAW, FALSE, 0 );

      if ( m_fakelevel == false )
      {
         node::setup_tree( tree, parent );
      }
      else
      {        
         m_expanded  = true;
         m_tree_item = parent;
         m_hwnd_tree = tree;
      }

      if ( !m_hidden || ( m_hidden == HIDDEN_LEAVES ) )
      {
         if ( m_expanded )
         {
            setup_children_trees( m_hwnd_tree );
         }
         else
         {
            if ( m_children.get_count() > 0 )
            {
               // theyve been drawn once already, continue drawing them
               if ( m_children[0]->m_tree_item )
               {
                  setup_children_trees( m_hwnd_tree );
               }
               else if ( should_add_fake_child() )
               {
                  add_fake_child( m_hwnd_tree, m_tree_item );
               }
            }
         }
      }

      //SendMessage( m_hwnd_tree, WM_SETREDRAW, TRUE, 0 );
      //InvalidateRect( m_hwnd_tree, NULL, TRUE );
   }

   virtual void clear_subtree() 
   {
      //for_each_child( n, m_children[n]->remove_tree() );
      HTREEITEM ch = TreeView_GetChild( m_hwnd_tree, m_tree_item );

      while ( ch )
      {
         HTREEITEM next = TreeView_GetNextSibling( m_hwnd_tree, ch );
         TreeView_DeleteItem( m_hwnd_tree, ch );                      
         ch = next;
      }
   }

   virtual void refresh_subtree( HWND tree = NULL )
   {
      if ( m_tree_item )
      {
         clear_subtree();

         if ( tree == NULL ) tree = m_hwnd_tree;

         setup_children_trees( tree );
      }
   }

   virtual void populate_treeview_item( TVITEMEX * item )
   {
      node::populate_treeview_item( item );

      item->mask |= TVIS_EXPANDED;

      if ( m_expanded ) item->state |= TVIS_EXPANDED;

      item->stateMask |= TVIS_EXPANDED;
   }

   virtual node * add_child( node * n, unsigned pos = ~0 )
   {
      if ( !can_add_child() )
      {
         // console::printf( "Over child limit" );
         return NULL;
      }

      if ( pos >= get_num_children() )
      {
         m_children.add_item( n );
      }
      else
      {
         m_children.insert_item( n, pos );
      }

      n->m_parent    = this;

      // remove the old one item the tree and invalidate it
      if ( n->m_hwnd_tree && n->m_tree_item )
      {
         TreeView_DeleteItem( n->m_hwnd_tree, n->m_tree_item );
      }
      n->m_hwnd_tree = NULL;
      n->m_tree_item = NULL;

      return n;
   }

   virtual node * add_child_conditional( const char * str, bool process_tags = true )
   {
      for ( unsigned i = 0; i < get_num_children(); i++)
      {
         if ( !m_children[i]->is_leaf() )
         {
            if ( stricmp_utf8_ex( m_children[i]->m_label, ~0, str, ~0 ) == 0 )
            //if ( strcmp( m_children[i]->m_label, str ) == 0 ) 
            {
               return m_children[i];
            }
         }
      }

      if ( can_add_child() )
      {
         folder_node * fn = new folder_node( str );
         
         // process tags to process @limit
         if ( process_tags )
         {
            pfc::string8_fastalloc tmp;
            fn->get_display_name( tmp );
         }

         add_child( fn );
         return fn;
      }
      else
      {
         return NULL;
      }
   }

   virtual bool get_formatting_handle( metadb_handle_ptr & handle ) 
   {
      handle = g_silence_handle;
      return true;
   }

   virtual bool get_entries( pfc::list_base_t<metadb_handle_ptr> & list ) const
   { 
      if ( m_hidden && ( m_hidden != HIDDEN_LEAVES ) ) return false;
      
      for ( int n = 0; n < m_children.get_count(); n++ )
      {
         m_children[n]->get_entries( list );
      }

      return true;
   }

   int get_default_bitmap_index() { return cfg_bitmap_folder; };

   virtual node * find_by_name( const char * name )
   {
      pfc::string8_fastalloc display;

      get_display_name( display );

      if ( strcmp( display, name ) == 0 )
         return this;

      int m = get_num_children();

      for ( int n = 0; n < m; n++ )
      {
         node * ch = m_children[n]->find_by_name( name );

         if ( ch )
         {
            return ch;
         }
      }

      return NULL;
   }

   virtual void write_sexp_children( FILE *f, int offset )
   {
      if ( get_num_children() > 0 )
      {
         fprintf( f, " :CONTENTS (\n" );

         for ( int i = 0; i < get_num_children(); i++ )
         {
            m_children[i]->write_sexp( f, offset + 1);
         }

         fprintf( f, ")" );
      }
   }

   virtual void write_sexp( FILE *f, int offset = 0 )
   {
      for ( int j = 0; j < offset; j++ )
      {
         fprintf( f, "\t" );
      }

      fprintf( f, "(FOLDER %s %d",      
         (const char *) quoted_string( m_label ),
         m_expanded );

      write_sexp_children( f, offset + 1 );

      fprintf( f, ")" );
   }

   static BOOL CALLBACK staticEditProc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
   {
      static folder_node *g_static_folder_edit = NULL;
      static HICON icon_normal = NULL;
      //static HICON icon_expanded = NULL;
      //static string_sub value_string;
      static pfc::string8 value_string;
      static HIMAGELIST imagelist;

      static struct { int item; char * tag; } label_map[] = 
      {
         {IDC_CHECK_NOBROWSE, TAG_NOBROWSE},
         {IDC_CHECK_NOSAVE, TAG_NOSAVE},
      };

      switch( msg )
      {
      case WM_INITDIALOG:
         {
            g_static_folder_edit = (folder_node *)lp;

            value_string.set_string( g_static_folder_edit->m_label );


            pfc::string8_fastalloc tmp;

            while ( string_remove_tag_complex( tmp, value_string, TAG_ICON ) )
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

            uSetDlgItemText(wnd, IDC_EDIT_VALUE, value_string);

            imagelist = TreeView_GetImageList( g_static_folder_edit->m_hwnd_tree, TVSIL_NORMAL );

            if ( imagelist )
            {
               icon_normal = ImageList_GetIcon( imagelist, g_static_folder_edit->get_bitmap_index(), ILD_TRANSPARENT );
               SendDlgItemMessage( wnd, IDC_ICON_NORMAL, STM_SETIMAGE, (WPARAM) IMAGE_ICON,(LPARAM) icon_normal );
            }      		

            /*
            HWND edit_list = GetDlgItem(wnd, IDC_CB_LEAF);

            SendMessage(edit_list, CB_RESETCONTENT, 0, 0);

            if (!g_static_folder_edit->m_leaf_format.is_empty())
            SendMessage(edit_list, CB_ADDSTRING, 0, (LPARAM)(const char *)g_static_folder_edit->m_leaf_format);

            SendMessage(edit_list, CB_ADDSTRING, 0, (LPARAM)LIST_DEFAULT);

            int max = sizeof(g_format_options)/sizeof(g_format_options[0]);
            for ( int i = 0; i < max; i++ )
            {
            SendMessage(edit_list, CB_ADDSTRING, 0, (LPARAM)g_format_options[i]);
            }
            SendMessage(edit_list, CB_SETCURSEL, 0, 0);

            edit_list = GetDlgItem(wnd, IDC_CB_SORT);

            SendMessage(edit_list, CB_RESETCONTENT, 0, 0);

            if (!g_static_folder_edit->m_sort_criteria.is_empty())
            SendMessage(edit_list, CB_ADDSTRING, 0, (LPARAM)(const char *)g_static_folder_edit->m_sort_criteria);

            SendMessage(edit_list, CB_ADDSTRING, 0, (LPARAM)(const char *)LIST_DEFAULT);

            max = sizeof(g_format_options)/sizeof(g_format_options[0]);
            for ( int i = 0; i < max; i++ )
            {
            SendMessage(edit_list, CB_ADDSTRING, 0, (LPARAM)g_format_options[i]);
            }
            SendMessage(edit_list, CB_SETCURSEL, 0, 0);
            */
            uSetWindowText( wnd, pfc::string_printf( "Folder::%s", (const char*)value_string ) );
            SetFocus( GetDlgItem( wnd, IDC_EDIT_VALUE ) );
         }
         break;

      case WM_COMMAND:
         switch (wp)
         {            
         case IDC_ICON_RESET:
            if ( imagelist ) 
            {
               g_static_folder_edit->m_custom_icon = -1;

               if ( icon_normal ) 
               {
                  DestroyIcon( icon_normal );
               }

               icon_normal = ImageList_GetIcon( imagelist, g_static_folder_edit->get_bitmap_index(), ILD_TRANSPARENT );
               SendDlgItemMessage( wnd, IDC_ICON_NORMAL,STM_SETIMAGE, (WPARAM) IMAGE_ICON,(LPARAM) icon_normal );
            }
            break;

         case IDC_SET_NORMAL:
            if ( imagelist )
            {
               int res = select_icon( wnd, imagelist );
               if ( res >= 0 )
               {
                  if (icon_normal) 
                  {
                     DestroyIcon(icon_normal);
                  }
                  g_static_folder_edit->m_custom_icon = res;
                  icon_normal = ImageList_GetIcon( imagelist, g_static_folder_edit->get_bitmap_index(), ILD_TRANSPARENT );
                  SendDlgItemMessage( wnd, IDC_ICON_NORMAL,STM_SETIMAGE, (WPARAM) IMAGE_ICON,(LPARAM) icon_normal );				
               }
            }
            break;
         case IDOK:
            {
               pfc::string8 temp;

               uGetDlgItemText(wnd, IDC_EDIT_VALUE, g_static_folder_edit->m_label);

               if ( g_static_folder_edit->m_custom_icon >= 0 )
               {
                  g_static_folder_edit->m_label += 
                     pfc::string_printf("%s<%d>", 
                        TAG_ICON, 
                        g_static_folder_edit->m_custom_icon );
               }

               /*
               int check_max = sizeof(label_map)/sizeof(label_map[0]);
               for (int i=0;i<check_max;i++)
               {
               if (IsDlgButtonChecked(wnd, label_map[i].item) == BST_CHECKED)
               {
               g_static_folder_edit->m_value += label_map[i].tag;
               }
               }

               uGetDlgItemText(wnd, IDC_CB_LEAF, temp);

               if (strlen(temp)==0 || strcmp(temp,LIST_DEFAULT) == 0)
               {
               g_static_folder_edit->m_leaf_format.reset();
               }
               else
               {
               g_static_folder_edit->m_leaf_format.set_string(temp);
               }

               temp.reset();
               uGetDlgItemText(wnd, IDC_CB_SORT, temp);
               if (strlen(temp)==0 || strcmp(temp,LIST_DEFAULT) == 0)
               {
               g_static_folder_edit->m_sort_criteria.reset();
               }
               else
               {
               g_static_folder_edit->m_sort_criteria.set_string(temp);
               }
               */
               //g_static_folder_edit->refresh_subtree( g_static_folder_edit->m_hwnd_tree );
               g_static_folder_edit->refresh_tree_label();
            }
            // fall through

         case IDCANCEL:

            //if (icon_normal) DestroyIcon(icon_normal);
            //if (icon_expanded) DestroyIcon(icon_expanded);

            EndDialog(wnd, wp); 
            break;
         }
      }
      return 0;
   }

   virtual bool edit()
   {
      int res = DialogBoxParam(
         core_api::get_my_instance(), 
         MAKEINTRESOURCE( IDD_STATIC_FOLDER_EDIT ), 
         m_hwnd_tree, 
         staticEditProc, 
         (LPARAM) this );

      return res == IDOK;
   }

   virtual void collapse( bool recurse )
   {
      m_expanded = false;

      if ( recurse )
      {
         for_each_child( n, m_children[n]->collapse( true ) );
      }
   }
   //virtual void handle_context_menu_command( int n );
   /*
   {
      switch ( n )
      {
      case MENU_REFRESH_QUERIES:
         repopulate();
         refresh_subtree();
         break;

      case MENU_SORT_CHILDREN:
         sort( false );
         refresh_subtree();
         break;

      case MENU_SORT_RECURSIVE:
         sort( true );
         refresh_subtree();
         break;

      case MENU_NEW_FOLDER:
         {
            folder_node * f = new folder_node( "New Folder" );
            add_child( f );

            f->ensure_in_tree();
            f->force_tree_selection();

            if ( !f->edit() )
            {
               delete f;
            }
         }
         break;

      case MENU_NEW_QUERY:
         lazy_add_user_query( this );
         break;

      case MENU_REMOVE:
         delete this;
         break;

      case MENU_EDIT:
         edit();
         break;

      case MENU_SAVE_TREE:
         get_root()->save();
         break;

      case MENU_LOAD_TREE:
         {
            node * n = load( NULL );
            if ( n )
            {
               lazy_set_root( m_hwnd_tree, n );
            }
         }
         break;

      case MENU_SAVE_NODE:
         save();
         break;

      case MENU_LOAD_NODE:
         {
            node * n = load( NULL );

            if ( n ) 
            {
               add_child( n );
            }
         }
         break;
      }
   }
   */
   //virtual HMENU generate_context_menu();

   virtual void sort( bool recurse )
   {
      m_children.sort_t( &node_compare );

      if ( recurse )
      {
         for_each_child( n, m_children[n]->sort( true ) );
      }
   }
};

class leaf_node : public node 
{
public:
   metadb_handle_ptr    m_handle;

   leaf_node( const char * label, metadb_handle_ptr handle ) : node( label )
   {
      m_handle = handle;
   }

   ~leaf_node()
   {        
      m_handle.release();        
   }

   node * copy() 
   {
      return new leaf_node( m_label, m_handle );
   }

   virtual bool get_formatting_handle( metadb_handle_ptr & handle )
   {
      handle = m_handle;
      return true;
   }

   virtual bool get_first_entry( metadb_handle_ptr & entry )
   {
      if ( m_handle.is_empty() )
         return false;

      entry = m_handle;         
      return true;
   }

   virtual void add_to_hmenu( HMENU menu, int & baseID, pfc::ptr_list_t<pfc::string8> options )
   {      
      /*
      for ( int n = 0; n < options.get_count(); n++ )
      {      
         uAppendMenu( menu, MF_STRING, baseID, options[n]->get_ptr() );
         baseID++;
      }
      */
      add_actions_to_hmenu( menu, baseID, options );
   }

   virtual void get_display_name( pfc::string_base & out ) 
   {
      if ( m_label.is_empty() )
      {
         if ( m_handle.is_empty() )
         {
            out.set_string( STRING_INVALID_HANDLE );
         }
         else
         {
            service_ptr_t<titleformat_object> script;
            static_api_ptr_t<titleformat_compiler>()->compile_safe(script,cfg_file_format);             
            m_handle->format_title( NULL, out, script, NULL );
         }
      }
      else
      {
         node::get_display_name( out );
      }

      m_displayedname.set_string( out );
   }

   virtual bool is_leaf() const { return true; }

   __int64 get_file_size() 
   {
      return m_handle->get_filesize();
   }

   double get_play_length()
   {
      return m_handle->get_length();
   }

   virtual int get_entry_count() const
   {
      if ( m_handle.is_empty() )
      {
         return 0;
      }
      return 1;
   }

   virtual bool get_entries( pfc::list_base_t<metadb_handle_ptr> & list ) const
   {
      if ( !m_handle.is_empty() )
         list.add_item( m_handle );
      return true;
   }

   virtual void files_moved( const pfc::list_base_const_t<const char *> & p_from,const pfc::list_base_const_t<const char *> & p_to )
   {
      for ( int n = 0; n < p_from.get_count(); n++ )
      {
         if ( 0 == stricmp_utf8_ex( p_from[n], ~0, m_handle->get_path(), ~0 ) )
         {
            console::printf( "Moving Node: %s", (const char *)m_label );
            static_api_ptr_t<metadb> db;
            db->handle_replace_path_canonical( m_handle, p_to[n] );
         }
      }
   }

   virtual void files_removed( const pfc::list_base_const_t<const char *> & p_items )
   {
      for ( int n = 0; n < p_items.get_count(); n++ )
      {
         if ( 0 == stricmp_utf8_ex( p_items[n], ~0, m_handle->get_path(), ~0 ) )
         {
            console::printf( "Removing Node: %s", (const char *)m_label );
            delete this;
            break;
         }
      }
   }


   int get_default_bitmap_index() { return cfg_bitmap_leaf; }

   virtual void write_sexp( FILE *f, int offset = 0 )
   {
      if ( m_handle.is_empty() ) return;

      for ( int j = 0; j < offset; j++ )
      {
         fprintf( f, "\t" );
      }
      
      if ( m_handle->get_subsong_index() != 0 )
      {
         fprintf( f, "(LEAF %s %s :SUBSONG %u)\n", 
            (const char *) quoted_string( m_label ), 
            (const char *) quoted_string( m_handle->get_path() ),
            m_handle->get_subsong_index()
            );
      }
      else
      {
         fprintf( f, "(LEAF %s %s)\n", 
            (const char *) quoted_string( m_label ), 
            (const char *) quoted_string( m_handle->get_path() ));
      }
   }


   /*
   virtual void handle_context_menu_command( int n )
   {
      switch ( n )
      {
      case MENU_REMOVE:
         delete this;
         break;
      }
   }
   */
   /*
   virtual HMENU generate_context_menu()    
   {
      HMENU m = CreatePopupMenu();

      uAppendMenu( m, MF_STRING, MENU_REMOVE, "Remove" );

      return m;
   }
*/
};

/*
node * node::process_dropped_file( const char * file, bool do_not_nest, int index, bool dynamic )
{
	node *ret_val = NULL;

   if ( dynamic )
   {
      return add_child_query_conditional( pfc::string_filename(file), 
               pfc::string_printf( "@drop<%s>", file ), 
               NULL,
               "@default", 
               NULL );
   }

   pfc::string_extension s_ext( file );

   if ( PathIsDirectory( pfc::stringcvt::string_wide_from_utf8( file )))
   {
      pfc::string8 stufferoo = file;
	   stufferoo += "\\*.*";

	   uFindFile *find_file = uFindFirstFile( stufferoo );

	   if ( do_not_nest )
	   {
		   ret_val = this;
	   }
	   else
	   {
         ret_val = add_child(
            new folder_node( pfc::string_filename_ext( file ) ), index );
	   }

      pfc::string8 as_utf8;

      pfc::ptr_list_t<const char> file_list;

	   do
	   {
		   const char *file_name = find_file->GetFileName();

		   as_utf8 = file;
		   as_utf8 += "\\";
		   as_utf8 += file_name;

		   if ( !is_dotted_directory( file_name ) ) 
		   {		
			   ret_val->process_dropped_file( as_utf8 );
		   } 

	   } while ( find_file->FindNext() );
   }
   else if ( strcmp( s_ext, PLAYLIST_TREE_EXTENSION ) == 0) 
   {
      ret_val = add_child( do_load_helper( file )  );
   }
   else
   {
      abort_callback_impl abort;
      playlist_loader_callback_impl loader( abort );
      //playlist_loader::g_process_path( file, loader );
      playlist_loader::g_process_path_ex( file, loader );      

      //console::printf( "Processing: %s [%d items]", (const char *) file, loader.get_count() );

      for ( int i = 0; i < loader.get_count(); i++ )
      {
         if ( useable_handle( loader.get_item( i ) ) )
         {
            ret_val = add_child( new leaf_node( NULL, loader.get_item( i ) ), index );
         }
      }
      
      return ret_val;
   }

   return ret_val;
}
*/



node * node::process_hdrop( HDROP hDrop, int index, bool dynamic, metadb_handle_list * handles ) 
{
	node *ret_val = NULL;

	unsigned count = uDragQueryFileCount( hDrop );

	unsigned n;
	for ( n = 0; n < count; n++ ) 
	{		
      pfc::string8 filename;
		if ( uDragQueryFile( hDrop, n, filename )) 
		{
			ret_val = process_dropped_file( filename, false, index, dynamic, handles );

			if ( index >= 0 ) 
				index++;
		}
	}

   refresh_subtree( m_hwnd_tree );
	return ret_val;
}

node * node::get_root()
{
   if ( m_parent ) 
   {
      return m_parent->get_root();
   }
   return this;
}

int node_compare( const node * a, const node * b )
{  
   bool a_leaf = a->is_leaf();
   bool b_leaf = b->is_leaf();

   if ( a_leaf == b_leaf )
   {
      pfc::string8_fastalloc a_display;   
      pfc::string8_fastalloc b_display;

      ((node *)a)->get_display_name( a_display );
      ((node *)b)->get_display_name( b_display );
     
      return stricmp_utf8_ex( a_display, ~0, b_display, ~0 );
   }
   else if ( a_leaf )
   {
      return 1;
   }
   else if ( b_leaf )
   {
      return -1;
   }
   // huh?
   return 0;
}


void node::ensure_in_tree()
{
   if ( m_hwnd_tree && m_tree_item )
   {
      return;
   }
      
   if ( m_parent )
   {
      if ( !m_parent->m_hwnd_tree || !m_parent->m_tree_item )
      {
         m_parent->ensure_in_tree();
      }
      
      HTREEITEM firstChild = TreeView_GetChild( m_parent->m_hwnd_tree, m_parent->m_tree_item );

      if ( firstChild )
      {
		   // if there are 2 or more children it must have already been setup...
         // and we just need to add ourselves...
		   if ( TreeView_GetNextSibling( m_parent->m_hwnd_tree, firstChild ) )
         {
            setup_tree( m_parent->m_hwnd_tree, m_parent->m_tree_item );             
         }
         else
         {
            TreeView_DeleteItem( m_parent->m_hwnd_tree, firstChild );
            m_parent->setup_children_trees( m_parent->m_hwnd_tree );
         }
      }
      else
      {
         m_parent->setup_children_trees( m_parent->m_hwnd_tree );
      }
   }
}

int node::get_nest_level()
{
	if ( m_parent )
		return 1 + m_parent->get_nest_level();
	else
		return 0;
}

node *node::iterate_next()
{
	if ( !is_leaf() )     
	{
      folder_node * fn = (folder_node*) this;

     	if ( fn->get_num_children() > 0 )
      {
		   return fn->m_children[0];
      }
	}
	
   if ( m_parent ) 
	{
		node * ptr = this;
		folder_node * parent = m_parent;

		while ( parent )
		{
			int n = parent->get_num_children();

			// if we are not the last child return the next one...
			for ( unsigned i=0; i < n-1; i++) 
			{
				if ( parent->m_children[i] == ptr )  
				{
					return parent->m_children[i+1];						
				}
			}		

			// we are the last child...
			ptr = parent;
			parent = ptr->m_parent;
		}
	}

	return NULL;
}

node * node::get_parent( int n )
{
   if ( n == 0 ) return this;
   if ( m_parent ) return m_parent->get_parent( n - 1 );
   return NULL;
}

bool node::hide_leaves()
{
   if ( cfg_hideleaves )
      return true;
   if ( m_parent && m_parent->m_hidden == HIDDEN_LEAVES )
      return true;
   return false;
}
