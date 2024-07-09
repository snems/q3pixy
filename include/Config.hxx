#ifndef Q3PIXY_CONFIG_HXX
#define Q3PIXY_CONFIG_HXX
#include <chrono>
#include <arpa/inet.h>
#include <vector>

namespace Q3Pixy::Config
{
  struct Server
  {
    struct sockaddr_in server;
    struct sockaddr_in local;
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

  struct Config
  {
    std::vector<Server> servers;
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
      const Config& get_config() const;
      void dump_config() const;
    private:
      Config config;
  };
}
#endif
