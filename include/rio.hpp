//--WARNING: Buffered RIO functions are deprecated, they were for pre-SSL config

#pragma once

#include <openssl/ssl.h>
#include <span>
#include <sys/types.h>

#define RIO_BUFSIZE 8192

namespace rio {

[[nodiscard]] ssize_t readn(SSL *ssl, std::span<char> usrbuf);
[[nodiscard]] ssize_t writen(SSL *ssl, std::span<const char> usrbuf);
[[nodiscard]] ssize_t http_readn(int client_fd, std::span<char> usrbuf);
[[nodiscard]] ssize_t http_writen(int client_fd, std::span<const char> usrbuf);
} // namespace rio
