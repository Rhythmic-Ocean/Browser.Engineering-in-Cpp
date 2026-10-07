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
#include <cstring>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <span>
#include <unistd.h>
#define RIO_BUFSIZE 8192
#include "rio.hpp"

ssize_t rio::readn(SSL *ssl, std::span<char> usrbuf) {
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
  return static_cast<ssize_t>(n - nleft); // hw much read from the buffer
}
ssize_t rio::writen(SSL *ssl, std::span<const char> usrbuf) {
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

ssize_t rio::http_writen(int client_fd, std::span<const char> usrbuf) {
  ssize_t n{static_cast<ssize_t>(usrbuf.size())};
  ssize_t nleft{n};
  ssize_t nwrite{};
  const char *buf{usrbuf.data()};
  while (nleft > 0) {
    if ((nwrite = write(client_fd, buf, static_cast<size_t>(nleft))) < 0) {
      if (errno == EINTR) {
        nwrite = 0;
      } else {
        return -1;
      }
    } else if (nwrite == 0) {
      break;
    }
    nleft -= nwrite;
    buf += nwrite;
  }
  return n - nleft; // Number of bytes successfully encrypted and written
}

ssize_t rio::http_readn(int client_fd, std::span<char> usrbuf) {
  ssize_t n{static_cast<ssize_t>(usrbuf.size())};
  ssize_t nleft{n};
  ssize_t nread{};
  char *buf{usrbuf.data()};
  while (nleft > 0) {
    if ((nread = read(client_fd, buf, static_cast<size_t>(nleft))) < 0) {
      if (errno == EINTR) {
        nread = 0;
      } else {
        return -1;
      }
    } else if (nread == 0) {
      break;
    }
    nleft -= nread;
    buf += nread;
  }
  return n - nleft; /*returning however many of the requested bytes were read*/
}
