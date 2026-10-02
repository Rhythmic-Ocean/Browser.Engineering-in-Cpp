#include "client.hpp"
#include "helpers.hpp"
#include <memory>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/x509_vfy.h>
#include <sys/socket.h>
#include <unistd.h>

SSL_CTX *Client::get_ctx() {
  static struct ContextHolder {
    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> ctx{nullptr,
                                                          &SSL_CTX_free};
    ContextHolder() {
      SSL_CTX *raw_ctx = SSL_CTX_new(TLS_client_method());
      if (raw_ctx == NULL) {
        NetworkException("Failed to create SSL_CTX\n");
      }
      ctx.reset(raw_ctx);
      SSL_CTX_set_verify(ctx.get(), SSL_VERIFY_PEER, NULL);
      if (!SSL_CTX_set_default_verify_paths(ctx.get())) {
        throw NetworkException(
            "Failed to set the default trusted certificate store\n");
      }
      if (!SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION)) {
        throw NetworkException(
            "Failed to set the minimum TLS protocol version\n");
      }
    }
  } holder;
  return holder.ctx.get();
}

int Client::create_sock() {
  int sock{-1};
  BIO_ADDRINFO *raw_res{nullptr};
  if (!BIO_lookup_ex(m_hostname.c_str(), m_port.c_str(), BIO_LOOKUP_CLIENT,
                     AF_UNSPEC, SOCK_STREAM, 0, &raw_res)) {
    throw NetworkException("Could not resolve given hostname and ports\n");
  }
  std::unique_ptr<BIO_ADDRINFO, decltype(&BIO_ADDRINFO_free)> res{
      raw_res, &BIO_ADDRINFO_free};
  for (const BIO_ADDRINFO *temp = res.get(); temp != nullptr;
       temp = BIO_ADDRINFO_next(temp)) {
    sock = BIO_socket(BIO_ADDRINFO_family(temp), SOCK_STREAM, 0, 0);
    if (sock == -1)
      continue;
    if (!BIO_connect(sock, BIO_ADDRINFO_address(temp), BIO_SOCK_NODELAY)) {
      BIO_closesocket(sock);
      sock = -1;
      continue;
    }
    break;
  }
  if (sock == -1) {
    throw NetworkException(
        "Could not open a TCP socket for given hostname and port\n");
  }
  return sock;
}

void Client::set_bio(int sock) {
  std::unique_ptr<BIO, decltype(&BIO_free)> bio{BIO_new(BIO_s_socket()),
                                                &BIO_free};
  if (!bio.get()) {
    BIO_closesocket(sock);
    throw NetworkException("Unable to set up the SSL's bio on proved socket\n");
  }
  BIO_set_fd(bio.get(), sock, BIO_CLOSE);
  auto *raw_bio{bio.release()};
  SSL_set_bio(ssl.get(), raw_bio, raw_bio);
}

void Client::tls_handshake() {
  if (SSL_connect(ssl.get()) < 1) {
    if (SSL_get_verify_result(ssl.get()) != X509_V_OK) {
      throw NetworkException(
          "Certificate verification error\n" + std::string("Verify error: ") +
          X509_verify_cert_error_string(SSL_get_verify_result(ssl.get())) +
          "\n");
    }
    throw NetworkException("OpenSSL error while verifying certificate\n");
  }
}

Client::Client(std::string_view hostname, std::string_view port)
    : m_hostname{hostname}, m_port{port} {
  SSL *raw_ssl{SSL_new(get_ctx())};
  if (raw_ssl == NULL) {
    throw NetworkException("Failed to create SSL object\n");
  }
  ssl.reset(raw_ssl);
  int sock{create_sock()};
  set_bio(sock);
  if (!SSL_set_tlsext_host_name(ssl.get(), m_hostname.c_str())) {
    NetworkException("Failed to set the SNI hostname\n");
  }
  if (!SSL_set1_host(ssl.get(), m_hostname.c_str())) {
    NetworkException("Failed to set the certificate verification hostname\n");
  }
  tls_handshake();
}
