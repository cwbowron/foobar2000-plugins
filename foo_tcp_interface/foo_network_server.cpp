#define VERSION         ("0.0.1 [" __DATE__ " - " __TIME__ "]")

#pragma warning(disable:4996)
#pragma warning(disable:4312)
#pragma warning(disable:4244)

#include <winsock.h>

#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"

#include "resource.h"
#include "../common/string_functions.h"
#include "../common/menu_stuff.h"

#define URL_HELP                 "http://wiki.bowron.us/index.php/Foobar2000"

#define COMMAND_SHUTDOWN                  "shutdown"
#define COMMAND_MAINMENU                  "mainmenu"
#define COMMAND_NOW_PLAYING               "now_playing"
#define COMMAND_FORMAT                    "format"

#define COMMAND_PLAYLIST_ACTIVE           "playlist_active"
#define COMMAND_PLAYLIST_PLAYING          "playlist_playing"
#define COMMAND_PLAYLIST_SET_ACTIVE       "playlist_set_active"
#define COMMAND_PLAYLIST_SET_PLAYING      "playlist_set_playing"
/*#define COMMAND_PLAYLIST_SET_FOCUS        "playlist_set_focus"*/
#define COMMAND_PLAYLIST_RENAME           "playlist_rename"
#define COMMAND_PLAYLIST_REMOVE           "playlist_remove"
#define COMMAND_PLAYLIST_FIND             "playlist_find"
#define COMMAND_PLAYLIST_FIND_OR_CREATE   "playlist_find_or_create"
#define COMMAND_PLAYLIST_CONTEXT          "playlist_context"

#define COMMAND_PLAYLIST_CLEAR            "playlist_clear"
#define COMMAND_PLAYLIST_ADD              "playlist_add"
#define COMMAND_PLAYLIST_CONTENTS         "playlist_contents"

#define COMMAND_HANDLE_LENGTH             "handle_length"
#define COMMAND_HANDLE_SIZE               "handle_size"
#define COMMAND_HANDLE_PATH               "handle_path"
#define COMMAND_HANDLE_INDEX              "handle_index"

#define COMMAND_RETRIEVE                  "retrieve"

#define RESPONSE_UNKNOWN         "unknown"
#define RESPONSE_SUCCESS         "ok"
#define RESPONSE_FAIL            "fail"
#define RESPONSE_INVALID_ARG     "bad"
#define RESPONSE_LIST_START      "start"
#define RESPONSE_LIST_END        "end"

static componentversion_impl_simple_factory 
   g_componentversion_service_2( 
      "tcp interface", 
      VERSION, 
      "tcp interface\n"
      "By Christopher Bowron <chris@bowron.us>\n"
      URL_HELP
      );


static const GUID cfg_guid_000 = { 0xfea0dbd4, 0x80ba, 0x41d8, { 0xb6, 0x5d, 0x5, 0x41, 0xa5, 0x9b, 0x88, 0xc0 } };

static cfg_int cfg_server_port( cfg_guid_000, 3079 );

#define PRINTERROR(s) console::printf( "error %s %d", s, WSAGetLastError() )

// #define net_send(socket,s) { console::printf( "SEND: %s", (const char *) s ); send(socket,s,strlen(s),0); }
#define net_send(socket,s) { send(socket,s,strlen(s),0); }


void net_send_list( SOCKET s, const metadb_handle_list & list )
{
   net_send( s, RESPONSE_LIST_START );
   net_send( s, "\n" );

   for ( int n = 0; n < list.get_count(); n++ )
   {
      net_send( s, pfc::string_printf( "%d", list[n].get_ptr() ) );
      net_send( s, "\n" );
   }

   net_send( s, RESPONSE_LIST_END );
}

class generic_command_callback : public main_thread_callback
{
   pfc::string8   m_command;
   SOCKET         m_response_socket;

public:
   generic_command_callback( const char * cmd, SOCKET response_socket )
   {
      m_command         = cmd;
      m_response_socket = response_socket;
   }

   void callback_run()
   {
      if ( 0 == m_command.find_first( COMMAND_FORMAT ) )
      {
         pfc::string8 args;

         string_substr( args, m_command, strlen(COMMAND_FORMAT) + 1 );

         int handle_len = args.find_first(" ");

         if ( handle_len != infinite )
         {
            pfc::string8 handle_str( args, handle_len );
            pfc::string8 format_spec;

            string_substr( format_spec, args, handle_len + 1 );
            metadb_handle * handle = (metadb_handle*) atoi( handle_str );
            string_format_title out( handle, format_spec );
            net_send( m_response_socket, out );
         }
         else
         {
            net_send( m_response_socket, RESPONSE_INVALID_ARG );
         }
      }
      else if ( 0 == m_command.find_first( COMMAND_HANDLE_PATH ) )
      {
         pfc::string8 args;
         string_substr( args, m_command, strlen(COMMAND_HANDLE_PATH) + 1 );
         metadb_handle * handle = (metadb_handle*) atoi( args );
         net_send( m_response_socket, handle->get_path() );
      }      
      else if ( 0 == m_command.find_first( COMMAND_HANDLE_INDEX ) )
      {
         pfc::string8 args;
         string_substr( args, m_command, strlen(COMMAND_HANDLE_INDEX ) + 1 );
         metadb_handle * handle = (metadb_handle*) atoi( args );
         net_send( m_response_socket, pfc::string_printf( "%d", handle->get_subsong_index() ) );
      }      
      else if ( 0 == m_command.find_first( COMMAND_HANDLE_LENGTH ) )
      {
         pfc::string8 args;
         string_substr( args, m_command, strlen(COMMAND_HANDLE_LENGTH ) + 1 );
         metadb_handle * handle = (metadb_handle*) atoi( args );
         net_send( m_response_socket, pfc::string_printf( "%d", (int)( handle->get_length() ) ) );
      }      
      else if ( 0 == m_command.find_first( COMMAND_HANDLE_SIZE ) )
      {
         pfc::string8 args;
         string_substr( args, m_command, strlen(COMMAND_HANDLE_SIZE ) + 1 );
         metadb_handle * handle = (metadb_handle*) atoi( args );
         net_send( m_response_socket, pfc::string_printf( "%d", handle->get_filesize() ) );
      }      
      else if ( 0 == strcmp( m_command, COMMAND_NOW_PLAYING ) )
      {
         metadb_handle_ptr handle;
         static_api_ptr_t<playback_control> srv;
      
         if ( srv->get_now_playing( handle ) )
         {
            pfc::string8 path( pfc::string_printf( "%d", handle.get_ptr() ) );
            net_send( m_response_socket, path );
         }   
         else
         {
            net_send( m_response_socket, "null" );
         }
      }   
      /*
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_SET_FOCUS  ) )
      {
         pfc::ptr_list_t<pfc::string8> strings;
         string_split( m_command, " ", strings );

         if ( strings.get_count() == 3 )
         {
            int playlist = atoi( strings[1]->get_ptr() );
            int select   = atoi( strings[2]->get_ptr() );

            static_api_ptr_t<playlist_manager>()->playlist_set_focus_item( playlist, select );
            net_send( m_response_socket, RESPONSE_SUCCESS );
         }
         else
         {
            net_send( m_response_socket, RESPONSE_INVALID_ARG );
         }
      } 
      */
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_ACTIVE ) )
      {           
         net_send( m_response_socket, pfc::string_printf( "%d", static_api_ptr_t<playlist_manager>()->get_active_playlist() ) );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_PLAYING ) )
      {           
         net_send( m_response_socket, pfc::string_printf( "%d", static_api_ptr_t<playlist_manager>()->get_playing_playlist() ) );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_SET_ACTIVE ) )
      {           
         int n = atoi( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_SET_ACTIVE ) + 1 );
         static_api_ptr_t<playlist_manager>()->set_active_playlist( n );
         net_send( m_response_socket, RESPONSE_SUCCESS );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_SET_PLAYING ) )
      {           
         int n = atoi( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_SET_PLAYING ) + 1 );
         static_api_ptr_t<playlist_manager>()->set_playing_playlist( n );
         net_send( m_response_socket, RESPONSE_SUCCESS );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_CLEAR ) )
      {           
         int n = atoi( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_CLEAR ) + 1 );
         static_api_ptr_t<playlist_manager>()->playlist_clear( n );
         net_send( m_response_socket, RESPONSE_SUCCESS );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_REMOVE ) )
      {           
         int n = atoi( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_CLEAR ) + 1 );
         static_api_ptr_t<playlist_manager>()->remove_playlist( n );
         net_send( m_response_socket, RESPONSE_SUCCESS );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_FIND_OR_CREATE ) )
      {           
         pfc::string8 name( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_FIND_OR_CREATE ) + 1 );
         t_size n = static_api_ptr_t<playlist_manager>()->find_or_create_playlist( name, name.get_length() );
         net_send( m_response_socket, pfc::string_printf( "%d", n ) );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_FIND ) )
      {           
         pfc::string8 name( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_FIND ) + 1 );
         
         t_size n = static_api_ptr_t<playlist_manager>()->find_playlist( name, name.get_length() );
         net_send( m_response_socket, pfc::string_printf( "%d", n ) );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_RENAME ) )
      {  
         pfc::string8 rest( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_RENAME ) + 1 );

         int space = rest.find_first( " " );
         pfc::string8 index_str( rest, space );
         pfc::string8 name( rest.get_ptr() + space + 1 );

         int n = atoi( index_str );
         
         if ( static_api_ptr_t<playlist_manager>()->playlist_rename( n, name, ~0 ) )
         {
            net_send( m_response_socket, RESPONSE_SUCCESS );
         }
         else
         {
            net_send( m_response_socket, RESPONSE_FAIL );
         } 
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_CONTENTS ) )
      {           
         int n = atoi( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_CONTENTS ) + 1 );
         metadb_handle_list list;
         static_api_ptr_t<playlist_manager>()->playlist_get_all_items( n, list );
         net_send_list( m_response_socket, list );
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_ADD ) )
      {           
         pfc::ptr_list_t<pfc::string8> strings;
         string_split( m_command, " ", strings );

         if ( strings.get_count() == 3 )
         {
            int n = atoi( strings[1]->get_ptr() );
            metadb_handle * handle = (metadb_handle*) atoi( strings[2]->get_ptr() );
            metadb_handle_list handle_list;
            handle_list.add_item( handle );
            if ( static_api_ptr_t<playlist_manager>()->playlist_add_items( n, handle_list, bit_array_false() ) )
            {
               net_send( m_response_socket, RESPONSE_SUCCESS );
            }
            else
            {
               net_send( m_response_socket, RESPONSE_FAIL );
            }
         }
         else
         {
            net_send( m_response_socket, RESPONSE_INVALID_ARG );
         }
      }
      else if ( 0 == m_command.find_first( COMMAND_RETRIEVE ) )
      {
         metadb_handle_list handles;
         static_api_ptr_t<library_manager>()->get_all_items( handles );
         
         t_size space = m_command.find_first( " " );

         if ( space != ~-0 )
         {
            search_tools::search_filter filter;
            pfc::string8 filter_string( m_command.get_ptr() + space + 1 );
            filter.init( filter_string );

            metadb_handle_list subset;

            for ( int n = 0; n < handles.get_count(); n++ )
            {
               if ( filter.test( handles[n] ) )
               {
                  subset.add_item( handles[n] );
               }
            }
            net_send_list( m_response_socket, subset );
         }
         else
         {
            net_send_list( m_response_socket, handles );
         }
      }
      else if ( 0 == m_command.find_first( COMMAND_PLAYLIST_CONTEXT ) )
      {  
         pfc::string8 rest( m_command.get_ptr() + strlen( COMMAND_PLAYLIST_CONTEXT ) + 1 );

         int space = rest.find_first( " " );
         pfc::string8 index_str( rest, space );
         pfc::string8 name( rest.get_ptr() + space + 1 );

         int n = atoi( index_str );

         GUID guid;

         if ( menu_helpers::guid_from_name( name, ~0, guid ) )
         {
            metadb_handle_list handles;

            static_api_ptr_t<playlist_manager>()->playlist_get_all_items( n, handles );

            if ( menu_helpers::run_command_context( guid, pfc::guid_null, handles ) )
            {
               net_send( m_response_socket, RESPONSE_SUCCESS );
            }
            else
            {
               net_send( m_response_socket, RESPONSE_FAIL );
            } 
         }
         else
         {
            net_send( m_response_socket, RESPONSE_INVALID_ARG );
         }         
      }
      else if ( 0 == m_command.find_first( COMMAND_MAINMENU ) )
      {
         GUID guid;
         pfc::string8 sub;    
         string_substr( sub, m_command, strlen( COMMAND_MAINMENU ) + 1 );
         
         if ( mainmenu_commands::g_find_by_name( sub, guid ) )
         {
            mainmenu_commands::g_execute( guid );
            net_send( m_response_socket, RESPONSE_SUCCESS );
         }
         else
         {
            net_send( m_response_socket, RESPONSE_INVALID_ARG );
         }
      }
      else
      {
         net_send( m_response_socket, RESPONSE_UNKNOWN );
      }

      closesocket( m_response_socket );
   }
};


int StreamServer(short nPort)
{
	SOCKET	listenSocket;

	listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if (listenSocket == INVALID_SOCKET)
	{
		PRINTERROR("socket()");
		return 0;
	}


	SOCKADDR_IN saServer;		

	saServer.sin_family        = AF_INET;
	saServer.sin_addr.s_addr   = INADDR_ANY;	
	saServer.sin_port          = htons(nPort);

	int nRet;

	nRet = bind( listenSocket, (LPSOCKADDR)&saServer, sizeof(struct sockaddr) );

	if (nRet == SOCKET_ERROR)
	{
		PRINTERROR("bind()");
		closesocket(listenSocket);
		return 0;
	}

   // console::printf( "Starting network server on port %d", nPort );

   nRet = listen( listenSocket, SOMAXCONN );

	if (nRet == SOCKET_ERROR)
	{
		PRINTERROR("listen()");
		closesocket(listenSocket);
		return 0;
	}

   for (;;)
   {
	   SOCKET remoteSocket = accept( listenSocket, NULL, NULL );

	   if (remoteSocket == INVALID_SOCKET)
	   {
		   PRINTERROR("accept()");
		   closesocket(listenSocket);
		   return 0;
	   }

	   //
	   // Receive data from the client
	   //
	   char szBuf[1024];
	   memset(szBuf, 0, sizeof(szBuf));
	   nRet = recv( remoteSocket, szBuf, sizeof(szBuf), 0 );

      if (nRet == INVALID_SOCKET)
	   {
		   PRINTERROR("recv()");
		   closesocket(listenSocket);
		   closesocket(remoteSocket);
		   return 0;
	   }

      pfc::string8 cmd_string( szBuf );
      {         
         static_api_ptr_t<main_thread_callback_manager>()->add_callback(
            new service_impl_t<generic_command_callback>( cmd_string, remoteSocket ) );
      }

      if ( 0 == strcmp( szBuf, COMMAND_SHUTDOWN ) )
      {
         console::printf( "Shutting down network server" );
         closesocket( listenSocket );
         return 0;
      } 
   }
}


DWORD WINAPI runServer( LPVOID value )
{
   while ( StreamServer( /*cfg_server_port*/ 63033 ) )
   {
   }	
   return 0;
}


class server_on_initquit: public initquit
{
   HANDLE   m_serverThreadHandle;
   DWORD    m_serverThreadId;
   int      m_threadExited;

public:
   void on_init() 
   {
	   WORD wVersionRequested = MAKEWORD(1,1);
	   WSADATA wsaData;
	   int nRet;

	   nRet = WSAStartup( wVersionRequested, &wsaData );
	   if ( wsaData.wVersion != wVersionRequested )
	   {	
         console::printf( "error initializing winsock" );
         m_serverThreadHandle = NULL;
	   }
      else
      {
         m_serverThreadHandle = ::CreateThread( NULL, 0, runServer, &m_threadExited, 0, &m_serverThreadId );
      }
   }

   void on_quit()
   {
      if ( m_serverThreadHandle != NULL )
      {
         ::TerminateThread( m_serverThreadHandle, 0 );
      }

      ::WSACleanup();
   }
};

static service_factory_single_t<server_on_initquit> g_server_on_initquit;
