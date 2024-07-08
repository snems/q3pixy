#ifndef Q3PIXY_CONFIG_HXX
#define Q3PIXY_CONFIG_HXX
#include <chrono>
#include <arpa/inet.h>
#include <vector>

namespace Q3Pixy::Config
{
  struct Server
  {
    struct sockaddr_in server_ip;
    uint16_t server_port;
    struct sockaddr_in local_ip;
    uint16_t local_port;
    uint32_t local_to_server_multiplier;
    uint32_t local_to_client_multiplier;
  };

  struct Client
  {
    Server server;
    struct sockaddr_in client;
    uint16_t client_port;
    const std::chrono::time_point<std::chrono::system_clock> last_event;
  };

  class Manager
  {
    public:
      Manager() = default;
      Manager(const Manager&) = delete;
      Manager(Manager&&) = delete;
      Manager& operator=(const Manager&) = delete;
      Manager& operator=(const Manager&&) = delete;
      virtual ~Manager() = default;
      std::pair<bool, std::string> load_config(const std::string& path);
      std::vector<Server> get_config();
      void dump_config();
    private:
      std::vector<Server> config;
  };
}
#endif
