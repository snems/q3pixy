#ifndef Q3PIXY_CONNECTIONS_HXX
#define Q3PIXY_CONNECTIONS_HXX
#include "include/Config.hxx"

namespace Q3Pixy::Connections 
{
  class Manager
  {
  public:
    void add_server(const Config::Server& server);
    void add_client(const Config::Client& server);
    bool is_connitions_list_changed();
  };
}

#endif
