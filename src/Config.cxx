#include "include/Config.hxx"
#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <sstream>
#include <variant>

namespace Q3Pixy::Config
{
   std::pair<bool, std::string> Manager::load_config(const std::string& path)
   {
      Json::Value root;
      std::ifstream ifs(path.c_str(), std::ios::in);
      Json::CharReaderBuilder builder;
      JSONCPP_STRING errs;

      config.clear();

      if (!ifs.is_open())
      {
        return std::make_pair(false, "Couldn't open file");
      }

      if (!parseFromStream(builder, ifs, &root, &errs)) 
      {
        return std::make_pair(false, errs);
      }

      // Check base objects
      if (!root.isObject())
      {
        return std::make_pair(false, "\"root\" is not object");
      }

      if (!root.isMember("Q3Pixy"))
      {
        return std::make_pair(false, "\"Q3Pixy\" section is lost");
      }

      if (!root["Q3Pixy"].isMember("Servers"))
      {
        return std::make_pair(false, "\"Servers\" section is lost");
      }

      if (root["Q3Pixy"]["Servers"].empty())
      {
        return std::make_pair(false, "no servers configured");
      }

      // Load servers 
      for (auto it = root["Q3Pixy"]["Servers"].begin(); it != root["Q3Pixy"]["Servers"].end(); ++it)
      {
        auto &val = *it;
        // Check objects
        if (false && !val.isObject())
        {
          return std::make_pair(false, "server is not object");
        }
        if (!val.isMember("server_ip"))
        {
          return std::make_pair(false, "server IP is not specified");
        }
        if (!val.isMember("server_port"))
        {
          return std::make_pair(false, "server port is not specified");
        }
        // Read server config
        Q3Pixy::Config::Server server{};
        if (inet_pton(AF_INET, val["server_ip"].asString().c_str(), &(server.server_ip.sin_addr)) != 1)
        {
          return std::make_pair(false, "couldn't parse server ip address");
        }
        server.server_port = val["server_port"].asInt();

        // Read local ip or set it to zero
        if (val.isMember("local_ip"))
        {
          if (inet_pton(AF_INET, val["local_ip"].asString().c_str(), &(server.local_ip.sin_addr)) != 1)
          {
            return std::make_pair(false, "couldn't parse local ip address");
          }
        }
        else
        {
          std::memset(&server.local_ip, 0, sizeof(server.local_ip));
        }

        // Read local port or set it to server value
        if (val.isMember("local_port"))
        {
          server.local_port = val["local_port"].asInt();
        }
        else
        {
          server.local_port = server.server_port;
        }

        // Read multipliers or set it to 1
        if (val.isMember("local_to_server_multiplier"))
        {
          server.local_to_server_multiplier = val["local_to_server_multiplier"].asInt();
        }
        else
        {
          server.local_to_server_multiplier = 1;
        }
        if (val.isMember("local_to_client_multiplier"))
        {
          server.local_to_client_multiplier = val["local_to_client_multiplier"].asInt();
        }
        else
        {
          server.local_to_client_multiplier = 1;
        }
        config.push_back(server);
      }

      return std::make_pair(true, "Okey");
    }

    std::vector<Server> Manager::get_config(){ return config; }

    void Manager::dump_config()
    {
      char str[INET_ADDRSTRLEN];
      std::stringstream out;
      int cnt = 0;
      for (const auto &a: config)
      {
        out << "Entry " << ++cnt << ":" << std::endl;
        inet_ntop(AF_INET, &(a.server_ip.sin_addr), str, INET_ADDRSTRLEN);
        out << "Server: " << std::string(str) << ":" << a.server_port << std::endl;
        inet_ntop(AF_INET, &(a.local_ip.sin_addr), str, INET_ADDRSTRLEN);
        out << "Local: " << std::string(str) << ":" << a.local_port << std::endl;
        out << "Local -> Server multiplier: " << a.local_to_server_multiplier << std::endl;
        out << "Local -> Client multiplier: " << a.local_to_client_multiplier << std::endl;
        out << std::endl;
      }
      if (cnt == 0)
      {
        out << "Config is empty" << std::endl;
      }
      std::cout << out.str();
    }
}
