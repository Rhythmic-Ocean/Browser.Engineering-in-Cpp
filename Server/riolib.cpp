#include "riolib.hpp"
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINES 150
#define LISTENQ 1024

void riolib::rio_readinitb(rio_t *rp, int fd) {
  rp->rio_fd = fd;
  rp->rio_cnt = 0;
  rp->rio_bufptr = 0;
}

/* buffered version of unix's default read() function*/
/* the buffer is stored in rp->rio_buffer and read into userbuf in bits of n
 * as*/
/* requested when there's no more stuff to read in rp's internal buffer it reads
 more chunk from the file*/
/*
 * @param: ssize_t n -> How many bytes the calling functioin wants us to put in
 * it's userbuf from out internal buffer*/
static ssize_t rio_read(rio_t *rp, std::span<char> userbuf, ssize_t n) {

  ssize_t cnt = n;
  // when here's no byte that's left to be read in buffer
  while (rp->rio_cnt <= 0) {
    // We try reading as much as our buffer allows at once
    rp->rio_cnt =
        read(rp->rio_fd, rp->rio_buffer.data(), rp->rio_buffer.size());
    if (rp->rio_cnt < 0) {
      if (errno != EINTR) {
        return -1;
      }
      // reached EOF
    } else if (rp->rio_cnt == 0) {
      return 0;
      // We got in the loop cuz all bytes got read so we start from start again
    } else {
      rp->rio_bufptr = 0;
    }
  }

  // if amount of bytes read is less than what the calling function requested we
  // update cnt
  if (rp->rio_cnt < n) {
    cnt = rp->rio_cnt;
  }
  // We copy cnt amount of bytes from our rio buffer to userbuf
  std::copy_n(rp->rio_buffer.data() + rp->rio_bufptr, cnt, userbuf.data());
  // update our buffer index to point to next unread index
  rp->rio_bufptr += cnt;
  // update internal buffer cnt index to reflect hw many bytes left to be read
  rp->rio_cnt -= cnt;
  return cnt;
}

/*reading one line at a time!! uses rio_read to accomplish this as reading each
 * character at a time just to search for \n would be quite foolish*/
ssize_t riolib::rio_readlineb(rio_t *rp, std::string &usrbuf) {
  ssize_t n, rc;
  std::array<char, 1> c;
  while (true) {
    if ((rc = rio_read(rp, c, 1)) == 1) {
      usrbuf += *c.data();
      n++;
      if (*c.data() == '\n') {
        break;
      }
    } else if (rc == 0) {
      if (n == 1) { // nothing was read
        return 0;
      } else { // got eof before \n
        break;
      }
    } else {
      return -1; // got some kind of error
    }
  }
  return n - 1;
}

int riolib::open_listenfd(std::span<char> port_str) {
  char *port = port_str.data();
  int error_msg, listenfd, optval = 1;
  struct addrinfo hints, *listp, *p;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  hints.ai_flags |= AI_NUMERICSERV;

  if ((error_msg = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
    fprintf(stderr, "sm error: %s\n", gai_strerror(error_msg));
    exit(1);
  } else {
    fprintf(stdout,
            "Success: Port info successfully extracted for port %s. Success "
            "Code: %d\n",
            port, error_msg);
  }
  for (p = listp; p; p = p->ai_next) {
    if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
      fprintf(stdout, "Failed opening lister socket, trying again...\n");
      continue;
    }
    fprintf(stdout, "Success opening listener socket\n");
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
               sizeof(int));
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
      break;
    }
    fprintf(stdout,
            "Failed binding listerner socket, trying to open next one...\n");
    close(listenfd);
  }
  freeaddrinfo(listp);
  if (!p)
    return -1;
  // converts from active to listening socket
  if (listen(listenfd, LISTENQ) < 0) {
    fprintf(stdout, "Failed to get open a valid listener socket\n");
    close(listenfd);
    return -1;
  }
  return listenfd;
}

// Unbuffered rio read/write imlementation

ssize_t riolib::rio_readn(int fd, std::span<char> usrbuf) {
  ssize_t n{static_cast<ssize_t>(usrbuf.size())};
  ssize_t nleft{n};
  ssize_t nread{};
  char *buf{usrbuf.data()};
  while (nleft > 0) {
    if ((nread = read(fd, buf, static_cast<size_t>(nleft))) < 0) {
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

ssize_t riolib::rio_readnp(rio_t *rp, std::span<char> usrbuf) {
  ssize_t n{static_cast<ssize_t>(usrbuf.size())};
  ssize_t nleft{n};
  ssize_t nread{};
  char *buf{usrbuf.data()};
  while (nleft > 0) {
    if ((nread = rio_read(rp, usrbuf, static_cast<size_t>(nleft))) < 0) {
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

ssize_t riolib::rio_writen(int fd, std::span<char> usrbuf) {
  ssize_t n{static_cast<ssize_t>(usrbuf.size())};
  ssize_t nleft{n};
  ssize_t nwrite{};
  const char *buf{usrbuf.data()};
  while (nleft > 0) {
    if ((nwrite = write(fd, buf, static_cast<size_t>(nleft))) < 0) {
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
