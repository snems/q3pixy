# What is it?

q3pixy is UDP proxy designed for use with quake3 game. Main feature - duplicate datagrams for poor internet connections, to prevent snapshots loss.

# How to build

Install requirements:

```
  sudo apt-get install cmake libjsoncpp-dev
```

build:

```
  cd q3pixy
  cmake -S . -B build && cmake --build build
```

and install it: 

```
  cmake --install build --prefix ~/.local
```

# How to use

Write your own config with json format: 

```
  {
    "Q3Pixy" : {
      "Servers" : [
        {
          "server_ip" : "92.38.139.65",
          "server_port" : 7700,
          "local_ip" : "0.0.0.0",
          "local_port" : 7700,
          "local_to_server_multiplier" : 1,
          "local_to_client_multiplier" : 4
        },
        {
          "server_ip" : "92.38.139.65",
          "server_port" : 27965,
          "local_to_server_multiplier" : 1,
          "local_to_client_multiplier" : 4
        },
        {
          "server_ip" : "92.38.139.65",
          "server_port" : 27977,
          "local_ip" : "0.0.0.0",
          "local_to_server_multiplier" : 1,
          "local_to_client_multiplier" : 4
        }
      ]
    }
  }
```

Main object is "Q3Pixy", array Servers described server list. All the keys are obvious:
  
- server_ip - address of server
- server_port - UDP port of server
- local_ip - listening address (all addresses if not specified) 
- local_port - listening port (equal server_port if not specified) 
- local_to_server_multiplier how many times proxy will send one datagram to server
- local_to_client_multiplier how many times proxy will send one datagram to client

After config was created you can run the proxy:

```
$q3pixy /path/to/config.json
```


