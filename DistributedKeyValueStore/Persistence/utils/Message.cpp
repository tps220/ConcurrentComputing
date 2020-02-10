#include "Message.hpp"

std::pair<int, int> getTwoNumbers(char* data) {
  int val_1 = 0, val_2 = 0, i = 0;
  while (data[i] >= '0' && data[i] <= '9') {
    val_1 = val_1 * 10 + data[i] - '0';
    i++;
  }
  i++;
  while (data[i] != '\0') {
    val_2 = val_2 * 10 + data[i] - '0';
    i++;
  }
  return { val_1, val_2 };
}

Message Message::deserialize(Buffer* protocol) {
  Message message;

  //replace the \n character at the end of the message with the null character
  protocol -> data[protocol -> currentSize - 1] = '\0';
  fprintf(stderr, "Received Protocol: %s\n", protocol -> data);

  //separate protocol (length of 3) and {key,val} into two separate strings
  protocol -> data[3] = '\0';

  message.protocol = strcmp(protocol -> data, "GET") == 0 ? GET : PUT;
  message.data = getTwoNumbers(protocol -> data + 4);

  return message;
}

bool Message::verify(Buffer* protocol) {
  //if there was an error reading from socket, return -1
  if (protocol == NULL) {
    return false;
  }
  //minimum size: GET/PUT + _ + {key},{val} + \n == 8 chars
  else if (protocol -> currentSize < 8) {
    delete protocol;
    return false;
  }
  //must have space separating request from filename
  else if (protocol -> data[3] != ' ') {
    delete protocol;
    return false;
  }
  return true;
}