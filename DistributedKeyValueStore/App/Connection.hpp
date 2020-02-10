#include <iostream>
#include <vector>

#define BUFFER_SIZE 8196

class Buffer {
public:
	char* data;
	unsigned long long currentSize;
	unsigned long long maxSize;
	
	Buffer(unsigned long long size);
	~Buffer();

	//driver functions
	void add(const char ch);
};

class Connection {
public:
  static void die(std::string msg1, std::string msg2);
  static void shutdown(const int fd);
  static Buffer* socket_get_line(const int fd);
  static int stream_data(const int socketid, const void* data, const long long size);

  class Client {
  public:
    static int connect_to_server(char* server, const int port);
  };
  class Server {
  public:
    static int open_server_socket(const int port);
  };
};