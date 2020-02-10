#pragma once
#include <iostream>
#include "Connection.hpp"

enum MESSAGE_TYPE {
  GET,
  PUT,
  INVALID
};

class Message {
public:
  MESSAGE_TYPE protocol;
  std::pair<int, int> data;

  static char* serialize(Message message);
  static Message deserialize(Buffer* data);
  static bool verify(Buffer* data);
};