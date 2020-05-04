#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

const unsigned int POLL_TIMEOUT = 500;
const std::string DEFAULT_GROUP("default");
const unsigned int MIN_DATA_LEN = sizeof(uint8_t);
const unsigned int DATA_BUFF_LEN = 1024;

using raw_data = std::vector<uint8_t>;

#endif // COMMON_H
