#pragma once

#include <memory>
#include <netdb.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <string_view>

class Client {
  std::unique_ptr<SSL, decltype(&SSL_free)> ssl{nullptr, SSL_free};
  std::string m_hostname{};
  std::string m_port{};

  static SSL_CTX *get_ctx();
  int create_sock();
  void set_bio(int sock);
  void tls_handshake();

public:
  Client(std::string_view hostname = "www.google.com",
         std::string_view port = "https");
  Client(const Client &client) = delete;
  Client &operator=(const Client &client) = delete;
  Client(Client &&other) noexcept = default;
  Client &operator=(Client &&other) noexcept = default;
  ~Client() = default;
  SSL *get_ssl_client() const { return ssl.get(); }
};
