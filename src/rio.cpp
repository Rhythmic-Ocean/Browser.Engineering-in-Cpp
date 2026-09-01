/* Robust I/O package to handle the short count problem caused by normal
 read()/write()/send() operations*/

/*Problem Description:
 * When we do write(fd, buf, n) we are requesting the kernel to write n bytes
  from buffer to fd. However say we wanna send 1MB data thru network, so when
  we try writing that to network buffer we are not sending bytes directly thru
  n/w wire.
 * Instead we are copying from userspace buffer into kernel space
  socket-send buffer where the actual transmissing onto n/w happens
  asynchronously drivel by the kernel.
 * BUT! The kernel send buffer is of fixed size, but the buffer/ (data we wanna
  send) might be of variable size, so say if it's 64KB then of the 1MB data we
  wanna send we wll lose most of it.
 * Furthermore, if some signal inturrupts write then the kernel function call
  can return early with errno = EINTR, failing to do complete write
 * Thus we make this write() more "robust" by looping it until all the bytes we
  wanna send actually gets sent!!
 * The same problem in reverse exists for read()
 **/

/*
 *We have two type of read (buffered/ unbuffered) but only one kind of write
  (unbuffered).
 * Unbuffered read means we read from the socket in a stream
  however much we can and don't put it in a internal buffer. Instead we just
  push everything we read directly into the user buffer the function's been
  provided with. Most useful for reading binary streams
 * Buffered read is usually for non-binary textual streams where we might wanna
   read the input stream line by line or word by word. The function has a
 internal buffer that reads however much it can into it's input buffer and dumps
 into the user buffer line by line
 * */
// Unbuffered Robust Input/Output:

#include "helpers.hpp"
#include <cerrno>
#include <cstring>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <span>
#include <unistd.h>
#define RIO_BUFSIZE 8192

namespace rio {

typedef struct {
  int rio_fd;
  int rio_cnt;
  char *rio_bufptr;
  char rio_buf[RIO_BUFSIZE];
} rio_t;

void readinitb(rio_t &rp, int fd) {
  rp.rio_fd = fd;
  rp.rio_cnt = 0;
  rp.rio_bufptr = rp.rio_buf;
}
[[nodiscard]] ssize_t readn(SSL *ssl, std::span<char> usrbuf) {
  size_t n{usrbuf.size()};
  size_t nleft{n};
  size_t nread{};
  char *buf{usrbuf.data()};
  while (nleft > 0) {
    if (SSL_read_ex(ssl, buf, nleft, &nread)) {
      nleft -= nread;
      buf += nread;
    } else {
      int err{SSL_get_error(ssl, 0)};
      if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_WANT_READ ||
          err == SSL_ERROR_WANT_WRITE) {
        break;
      } else {
        throw NetworkException("Fatal SSL read error occured\n");
      }
    }
  }
  return (n - nleft); // hw much read from the buffer
}
[[nodiscard]] ssize_t writen(SSL *ssl, std::span<const char> usrbuf) {
  size_t n{usrbuf.size()};
  size_t nleft{n};
  size_t nwritten{};
  const char *buf{usrbuf.data()};

  while (nleft > 0) {
    if (SSL_write_ex(ssl, buf, nleft, &nwritten)) {
      nleft -= nwritten;
      buf += nwritten;
    } else {
      int err{SSL_get_error(ssl, 0)};
      if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        break;
      } else if (err == SSL_ERROR_ZERO_RETURN) {
        break;
      } else {
        throw NetworkException("Fatal SSL write error occurred\n");
      }
    }
  }

  return static_cast<ssize_t>(
      n - nleft); // Number of bytes successfully encrypted and written
}

// Buffered Read, useful when you wanna read line by line (/r/n seperators),
// usually for non binary data strems

// Basic rio read functon that first refills the internal buffer from the file
// descriptor if empty and then fills the usr buffer from the said internal
// buffer as needed.

// It's an alt buffered version for POSIX read function
namespace {
[[nodiscard]] ssize_t read(rio_t &rp, std::span<char> usrbuf) {
  size_t n{usrbuf.size()};
  int cnt;

  while (rp.rio_cnt <= 0) {
    rp.rio_cnt = ::read(rp.rio_fd, rp.rio_buf, sizeof(rp.rio_buf));
    if (rp.rio_cnt < 0) {
      if (errno != EINTR)
        throw NetworkException("Error at rio::read: ::read() failed writing "
                               "into rio buffer from fd" +
                               std::to_string(rp.rio_fd));
    } else if (rp.rio_cnt == 0)
      return 0;
    else
      rp.rio_bufptr = rp.rio_buf;
  }

  cnt = n;
  if (rp.rio_cnt < n)
    cnt = rp.rio_cnt;
  std::memcpy(usrbuf.data(), rp.rio_bufptr, cnt);
  rp.rio_bufptr += cnt;
  rp.rio_cnt -= cnt;
  return cnt;
}

} // namespace

// The rio_readnb function is basically rio_readn but instead uses buffered
// rio_read instead of POSIX read
[[nodiscard]] ssize_t readnb(rio_t &rp, std::span<char> usrbuf) {
  size_t n{usrbuf.size()};
  size_t nleft{n};
  ssize_t nread{};
  char *buf{usrbuf.data()};

  while (nleft > 0) {
    if ((nread = read(rp, std::span(buf, nleft))) == 0)
      break;
    nleft -= nread;
    buf += nread;
  }
  return (n - nleft);
}

//
[[nodiscard]] ssize_t readlineb(rio_t &rp, std::span<char> usrbuf) {
  size_t maxlen{usrbuf.size()};
  int n, rc;
  char c, *bufp{usrbuf.data()};

  for (n = 1; n < maxlen; ++n) {
    if ((rc = read(rp, std::span(&c, 1))) == 1) {
      *bufp++ = c;
      if (c == '\n') {
        n++;
        break;
      }
    } else if (rc == 0) {
      if (n == 1)
        return 0;
      else
        break;
    }
  }
  *bufp = 0;
  return n - 1;
}

} // namespace rio
