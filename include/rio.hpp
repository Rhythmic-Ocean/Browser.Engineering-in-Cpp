#pragma once

#include <openssl/ssl.h>
#include <span>
#include <sys/types.h>

#define RIO_BUFSIZE 8192

namespace rio {

typedef struct {
  int rio_fd;
  int rio_cnt;
  char *rio_bufptr;
  char rio_buf[RIO_BUFSIZE];
} rio_t;

void readinitb(rio_t &rp, int fd);

[[nodiscard]] ssize_t readn(SSL *ssl, std::span<char> usrbuf);
[[nodiscard]] ssize_t writen(SSL *ssl, std::span<const char> usrbuf);

[[nodiscard]] ssize_t readnb(rio_t &rp, std::span<char> usrbuf);

[[nodiscard]] ssize_t readlineb(rio_t &rp, std::span<char> usrbuf);

} // namespace rio
