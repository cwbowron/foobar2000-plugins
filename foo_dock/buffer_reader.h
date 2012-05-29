#include "../foobar2000/SDK/foobar2000.h"

class buffer_reader : public stream_reader 
{
   char * m_data;
   t_size m_size;
   t_size m_index;

public:
   buffer_reader( stream_reader * in, t_size size,  foobar2000_io::abort_callback & abort ) : stream_reader()
   {
      m_index  = 0;
      m_size   = size;
      m_data   = new char[size];
      in->read( m_data, m_size, abort );
   }

   ~buffer_reader()
   {
      delete m_data;
   }

   virtual t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort)
   {
      if ( p_bytes + m_index <= m_size )
      {
         memcpy( p_buffer, m_data + m_index, p_bytes );
         m_index += p_bytes;
         return p_bytes;
      }
      else
      {
         t_size bytes = m_size - m_index;
         memcpy( p_buffer, m_data + m_index, bytes );
         m_index = m_size;
         return bytes;
      }
   }
};
