class NOVTABLE titleformatting_variable_callback : public service_base 
{
public:
   virtual void on_var_change( const char * var ) = 0;

   FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(titleformatting_variable_callback);
};

const GUID titleformatting_variable_callback::class_guid = 
{ 0xfcb81645, 0xe7b2, 0x4bc1, { 0x92, 0xb0, 0xf6, 0xfb, 0x4b, 0x43, 0x70, 0xe3 } };

class var_notify_callback : public main_thread_callback
{
   pfc::string8_fastalloc m_var;

public:
   var_notify_callback( const char * var ) : main_thread_callback()
   {
      m_var = var;
   }

   void callback_run()
   {
      service_enum_t<titleformatting_variable_callback> e; 
      service_ptr_t<titleformatting_variable_callback> ptr; 
      while( e.next( ptr ) ) 
      {
         ptr->on_var_change( m_var );
      }
   }
};

void post_variable_callback_notification( const char * str )
{
   if ( core_api::are_services_available() && !core_api::is_initializing() && !core_api::is_shutting_down() )
   // if ( false )
   {
      service_ptr_t<main_thread_callback> p_callback = new service_impl_t<var_notify_callback>( str );
      static_api_ptr_t<main_thread_callback_manager> man;
      man->add_callback( p_callback );
   }
}
