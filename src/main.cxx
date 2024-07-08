#include "include/Config.hxx"
#include <cstdlib>
#include <iostream>
#include <tuple>

int
main(void)
{
  bool load_config_success;
  std::string load_config_message;
  Q3Pixy::Config::Manager config_manager;
  // load config
  std::tie(load_config_success, load_config_message) = config_manager.load_config("./q3pixy.json");

  if (!load_config_success)
  {
    std::cerr << "Couldn't load config with message: " << load_config_message << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Config loaded" << std::endl;
  config_manager.dump_config();


  // start routines

  return EXIT_SUCCESS;
}
