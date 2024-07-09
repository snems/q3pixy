#ifndef Q3PIXY_CONNECTIONS_HXX
#define Q3PIXY_CONNECTIONS_HXX
#include "include/Config.hxx"
#include <memory>
#include <netinet/in.h>

namespace Q3Pixy::Connections 
{
  class Manager
  {
  private:
    struct Impl;
  public:
    Manager();
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager();

    void init(const Config::Config& config);
    void teardown();
    bool routine();
  private:
    std::unique_ptr<Impl> pimpl;

  };
}

#endif
