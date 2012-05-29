
class generic_group : public mainmenu_group_popup
{
   public:
      GUID           m_guid;
      GUID           m_parent;
      const char *   m_name;
      unsigned       m_priority;

      generic_group( const char * name, GUID guid, GUID parent, unsigned priority = mainmenu_commands::sort_priority_dontcare )
      {
         m_guid      = guid;
         m_parent    = parent;
         m_name      = name;
         m_priority  = priority;
      }

   virtual GUID get_guid() { return m_guid; }
   virtual GUID get_parent() { return m_parent; }
   virtual t_uint32 get_sort_priority() { return m_priority; }   
   virtual void get_display_string(pfc::string_base & p_out) { p_out.set_string( m_name ); }
};
