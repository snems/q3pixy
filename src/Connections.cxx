#include "include/Connections.hxx"
#include "include/Config.hxx"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <sys/epoll.h>

namespace 
{
  struct SockKey
  {
    sockaddr_in sock{};

    explicit SockKey(struct sockaddr sa)
    {
      std::memcpy(&sock, &sa, sizeof(sock));
    }

    explicit SockKey(struct sockaddr_in si)
    {
      std::memcpy(&sock, &si, sizeof(sock));
    }

    SockKey(const SockKey& other)
    :sock(other.sock)
    {

    }

    SockKey(SockKey&& other)
    :sock(other.sock)
    {

    }
    
    SockKey& operator=(const SockKey& other)
    {
      std::memcpy(&sock, &other.sock, sizeof(sock));
      return *this;
    }
    
    SockKey& operator=(SockKey&& other)
    {
      std::memcpy(&sock, &other.sock, sizeof(sock));
      return *this;
    }


    virtual ~SockKey() = default;
  
    bool operator==(const SockKey &other) const
    { 
      return (sock.sin_port == other.sock.sin_port && sock.sin_addr.s_addr == other.sock.sin_addr.s_addr);
    }
  };
}

namespace std 
{
  template<> struct std::hash<SockKey>
  {
      std::size_t operator()(const SockKey& s) const noexcept
      {
          std::size_t h1 = std::hash<uint32_t>{}(s.sock.sin_addr.s_addr);
          std::size_t h2 = std::hash<uint16_t>{}(s.sock.sin_port);
          return h1 ^ (h2 << 1); 
      }
  };
}


namespace Q3Pixy::Connections
{
  struct Manager::Impl
  {
    struct ServerProxy;
    struct ClientConnecton
    {
      explicit ClientConnecton(std::shared_ptr<ServerProxy> server, const SockKey &sock)
      : server(server)
      , fd_to_server(0)
      , sock(sock)
      , last_event()
      {
        struct sockaddr_in bind_to{};
        bind_to.sin_family = AF_INET;

        fd_to_server = socket(AF_INET, SOCK_DGRAM, 0);

        if (fd_to_server < 0)
        {
          throw std::runtime_error("Couldn't create local to server socket");
        }

        if (bind(fd_to_server, (const struct sockaddr*)&bind_to, sizeof(bind_to)) < 0)
        {
          throw std::runtime_error("Couldn't bind locals to server socket");
        }
        last_event = std::chrono::system_clock::now();
      }

      ClientConnecton(const ClientConnecton& other) = delete;
      ClientConnecton(ClientConnecton&& other)
      : server(std::move(other.server))
      , fd_to_server(other.fd_to_server)
      , sock(std::move(other.sock))
      , last_event(std::move(other.last_event))
      {

      }
      ClientConnecton& operator=(const ClientConnecton& other) = delete;
      ClientConnecton& operator=(ClientConnecton&& other)
      {
        server = std::move(other.server);
        fd_to_server = other.fd_to_server;
        sock = std::move(other.sock);
        last_event = std::move(other.last_event);
        return *this;
      }
      virtual ~ClientConnecton()
      {
        close(fd_to_server);
      }

      std::shared_ptr<ServerProxy> server;
      int fd_to_server;
      SockKey sock;
      std::chrono::time_point<std::chrono::system_clock> last_event;
    };

    struct ServerProxy
    {
      explicit ServerProxy(const Config::Server &config)
      : server_config(config)
      {
        fd_to_client = socket(AF_INET, SOCK_DGRAM, 0);

        if (fd_to_client < 0)
        {
          throw std::runtime_error("Couldn't create local to client socket");
        }

        if (bind(fd_to_client, (const struct sockaddr*)&server_config.local, sizeof(server_config.server)) < 0)
        {
          close(fd_to_client);
          throw std::runtime_error("Couldn't bind local to client socket");
        }
      }
      ServerProxy(const ServerProxy&) = delete;
      ServerProxy(const ServerProxy&&) = delete;
      ServerProxy& operator=(const ServerProxy&) = delete;
      ServerProxy& operator=(const ServerProxy&&) = delete;
      virtual ~ServerProxy()
      {
        close(fd_to_client);
      }

      Config::Server server_config{};
      int fd_to_client{0};
      std::unordered_map<SockKey, std::shared_ptr<ClientConnecton>> clients{};
    };

    void teardown_client(std::shared_ptr<ClientConnecton> client)
    {
      // destroy locals_to_servers
      fd_to_servers.erase(client->fd_to_server);
      close(client->fd_to_server);

      // destroy locals_to_clients
      clients_socks.erase(client->sock);

      // destroy client from server
      // after that, client will exist only in this scope
      client->server->clients.erase(client->sock);

    }

    void teardown_server(std::shared_ptr<ServerProxy> server)
    {
      close(server->fd_to_client);

      while (!server->clients.empty())
      {
        teardown_client(server->clients.begin()->second);
      }

    }

    decltype(ServerProxy::clients)::iterator create_client(std::shared_ptr<ServerProxy> server, const SockKey& key)
    {
      auto client = std::make_shared<ClientConnecton>(server, key);

      //save as server child
      auto result_of_empl = server->clients.emplace(key, client);

      if (!std::get<1>(result_of_empl))
      {
        throw std::runtime_error("Double client createon");
      }

      // register for polling
      this->epoll_register(client->fd_to_server, client.get());

      // register for fast search
      clients_socks.emplace(client->sock, client);
      fd_to_servers.emplace(client->fd_to_server, client);

      return std::get<0>(result_of_empl);
    }

    void create_server(const Config::Server& server)
    {
      auto new_server = std::make_shared<Impl::ServerProxy>(server);

      epoll_register(new_server->fd_to_client, new_server.get());

      fd_to_clients.emplace(new_server->fd_to_client, new_server);
    }

    /* 1. Читаем датаграмму из сокета 
     * 2. Определеям контекст:
     *   - если fd есть в locals_to_clients, то ищем сокет в clietns.
     *     - если нет, создаем клиента
     *     - отплавляем в привязаный fd к серверу
     *   - если нет в locals_to_clients, то смотрим в locals_to_servers
     *     - если есть, то определяем fd сервера и сокет
     *     - если нет, то отбрасываем датаграмму
     *
     *     т.е. в итоге мы имеем родительскую структуру Server и вариативное количество клиентов
     *
     */
    void epoll_event(epoll_event &event)
    {
      struct sockaddr remote_addr;
      socklen_t remote_struct_length = sizeof(remote_addr);

      int recv_len = recvfrom(event.data.fd, datagram, sizeof(datagram), 0, (struct sockaddr*)&remote_addr, &remote_struct_length);
      if (recv_len < 0)
      {
        throw std::runtime_error("Couldn't receive datagram");
      }
      SockKey key(remote_addr);

      auto server = fd_to_clients.find(event.data.fd);
      decltype(fd_to_servers)::iterator client_from_fd{};

      if (server != fd_to_clients.end()) // is it from client
      {
        // find client and create if it does not exists
        auto client = clients_socks.find(key);

        if (client == clients_socks.end())
        {
          // if new client, create it
          client = create_client(server->second, key);
        }

        // send datagram via client socket to the server
        const auto &server_sock = client->second->server->server_config.server;
        auto send_len = sendto(client->second->fd_to_server, datagram, recv_len, 0, reinterpret_cast<const struct sockaddr*>(&server_sock), sizeof(server_sock));
        if (send_len < 0)
        {
          throw std::runtime_error("Couldn't send datagram to server");
        }

        client->second->last_event = std::chrono::system_clock::now();
      }
      else if ((client_from_fd = fd_to_servers.find(event.data.fd)) != fd_to_servers.end()) // is it from server
      {
        // find client and send data to it
        auto send_len = sendto(client_from_fd->second->server->fd_to_client, datagram, recv_len, 
            0, 
            reinterpret_cast<const struct sockaddr*>(&client_from_fd->second->sock.sock), 
            sizeof(client_from_fd->second->sock.sock));
        if (send_len < 0)
        {
          throw std::runtime_error("Couldn't send datagram to client");
        }
      }
      else 
      {
      } // if not from client and not form server - what is it?
    }

    void epoll_register(int fd, void *context = nullptr)
    {
      struct epoll_event ev{};

      ev.data.ptr = context;
      ev.data.fd = fd;
      ev.events = EPOLLIN;
      if ( epoll_ctl( pollfd, EPOLL_CTL_ADD, fd, &ev ) != 0 )
      {
        throw std::runtime_error("Couldn't install socket for polling");
      }
    }

    void epoll_unregister(int fd, void *context = nullptr)
    {
      struct epoll_event ev{};

      ev.data.ptr = context;
      ev.data.fd = fd;
      ev.events = EPOLLIN;
      if ( epoll_ctl( pollfd, EPOLL_CTL_DEL, fd, &ev ) != 0 )
      {
        throw std::runtime_error("Couldn't remove socket polling");
      }
    }

    int pollfd{};
    uint8_t datagram[65535];

    std::unordered_map<int, std::shared_ptr<ServerProxy>> fd_to_clients{};          // locals listened to clients by fd, main container!
    std::unordered_map<int, std::shared_ptr<ClientConnecton>> fd_to_servers{};      // locals listened to servers by fd
    std::unordered_map<SockKey, std::shared_ptr<ClientConnecton>> clients_socks{};  // clinets by socket <sock, index in ServerProxy::clients>
  };

  Manager::Manager()
  :pimpl(new Manager::Impl)
  {
    pimpl->pollfd = epoll_create(1);
    if (!pimpl->pollfd)
    {
      throw std::runtime_error("Couldn't create socket for polling");
    }

  }

  Manager::~Manager()
  {
    close(pimpl->pollfd);
  }

  void Manager::teardown()
  {
    for (const auto l: pimpl->fd_to_clients)
    {
      pimpl->teardown_server(l.second);
    }

    pimpl->fd_to_clients.clear();
  }

  void Manager::init(const Config::Config& config)
  {
    teardown();

    for (const auto &a: config.servers)
    {
      pimpl->create_server(a);
    }
  }

  bool Manager::routine()
  {
    struct epoll_event pevents[1];
    const uint32_t size = sizeof(pevents)/sizeof(struct epoll_event);
    int rc = epoll_wait( pimpl->pollfd, pevents, size, 1000 );
    if (rc == -1)
    {
      return false;
    }
    if (rc == 0)
    {
      return true;
    }

    for (int i = 0; i < size; ++i)
    {
      if (pevents[i].events & EPOLLIN)
      {
        pimpl->epoll_event(pevents[i]);
      }
    }

    return true;
  }

  void Manager::kill_zombie()
  {
    auto now = std::chrono::system_clock::now();

    for (auto s: pimpl->fd_to_clients)
    {
      for (auto it = s.second->clients.begin(); it != s.second->clients.end(); ++it)
      {
        if ((now - it->second->last_event)/std::chrono::seconds(1) >  dead_time_seconds)
        {
          pimpl->teardown_client(it->second);
          break; // break, as map changed
        }
      }
    }
  }
}

