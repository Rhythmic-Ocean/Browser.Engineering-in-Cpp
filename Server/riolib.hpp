#include <asm-generic/errno-base.h>
#include <span>
#include <stddef.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#define RIO_BUFSIZE 8192

typedef struct rio_t {
  int rio_fd;         // file descriptor of the file we are doing reading for
  ssize_t rio_cnt;    // number of bytes that's left to be read from buffer
  ssize_t rio_bufptr; // index for next unread byte
  std::array<char, RIO_BUFSIZE> rio_buffer; // buffer that stores whatever we
                                            // get from the file from rio_read
} rio_t;
namespace riolib {

ssize_t rio_readlineb(rio_t *rp, std::string &usrbuf);
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnp(rio_t *rp, std::span<char> usrbuf);
ssize_t rio_readn(int fd, std::span<char> usrbuf);
ssize_t rio_writen(int fd, std::span<char> usrbuf);
int open_listenfd(std::span<char> port_str);
} // namespace riolib
