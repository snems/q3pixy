#include "include/Config.hxx"
#include "include/Connections.hxx"
#include <cstdlib>
#include <iostream>
#include <tuple>

int main(int argc, char* argv[])
{
  bool load_config_success;
  std::string load_config_message;
  Q3Pixy::Config::Manager config_manager;
  Q3Pixy::Connections::Manager conn_manager;

  if (argc != 2)
  {
    std::cout << "Usage:" << std::endl;
    std::cout << "\tq3pixy /path/to/config.json" << std::endl;
    return EXIT_FAILURE;
  }

  // load config
  std::tie(load_config_success, load_config_message) = config_manager.load_config(argv[1]);

  if (!load_config_success)
  {
    std::cerr << "Couldn't load config with message: " << load_config_message << std::endl;
    return EXIT_FAILURE;
  }



  std::cout << "Config " << argv[1] << " loaded" << std::endl << std::endl;

  config_manager.dump_config();

  conn_manager.init(config_manager.get_config());


  // start routines

  while(conn_manager.routine())
  {
    conn_manager.kill_zombie();
  }

  return EXIT_SUCCESS;
}
