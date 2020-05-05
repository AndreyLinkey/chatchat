#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

const unsigned short DEFAULT_PORT = 8888;
const unsigned int POLL_TIMEOUT = 500;
const std::string DEFAULT_GROUP("default");
const unsigned int MIN_DATA_LEN = sizeof(uint8_t);
const unsigned int DATA_BUFF_LEN = 1024;

#endif // COMMON_H
