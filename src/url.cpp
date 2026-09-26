
#include "url.hpp"
#include "helpers.hpp"
#include "rio.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <cassert>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <unistd.h>
#include <unordered_map>

void URL::parse() {
  size_t scheme_end{m_url.find("://")};
  if (scheme_end == std::string::npos) {
    throw NetworkException("Invalid URL format, missing \'://\' marker.");
  }
  scheme = std::string_view(m_url).substr(0, scheme_end);
  if (scheme != "http" && scheme != "https") {
    throw NetworkException("Unsupported network scheme: " +
                           std::string(scheme));
  }
  if (scheme == "http")
    m_port = "80";
  else if (scheme == "https")
    m_port = "443";

  size_t host_start{scheme_end + 3};
  size_t path_start{m_url.find('/', host_start)};
  if (path_start == std::string::npos) {
    host = std::string_view(m_url).substr(host_start);
    path = "/";
  } else {
    host = std::string_view(m_url).substr(host_start, path_start - host_start);
    path = std::string_view(m_url).substr(path_start);
  }

  if (auto portPos = host.find(':'); portPos != std::string_view::npos) {
    m_port = host.substr(portPos + 1);
    host = host.substr(0, portPos);
  }
}

URL::URL(const std::string &url) : m_url{url} {
  parse();
  m_client = Client(host, scheme);
}

std::string URL::request() {
  std::string request{"GET " + std::string(path) + " HTTP/1.1\r\n"};
  request += "Host: " + std::string(host) + "\r\n";
  request += "Connection: close\r\n";
  request += "User-Agent: IHateCpp!!\r\n";
  request += "\r\n";
  size_t num{};
  if ((num = rio::writen(m_client.get_ssl_client(), request)) !=
      request.size()) {
    throw NetworkException("Failed to send about " + std::to_string(num) +
                           " chars thru server");
  }
  std::string response{};
  get_response(response);
  int indx{};
  std::vector<std::string_view> data{hlp::split(response, "\r\n")};
  std::vector<std::string_view> statusline = hlp::split(data[indx], " ", 2);
  auto [version, status, explanation] =
      std::make_tuple(statusline[0], statusline[1], statusline[2]);
  ++indx;
  auto response_headers{parse_response(data, indx)};
  assert(response_headers.find("transfer-encoding") == response_headers.end());
  assert(response_headers.find("content-encoding") == response_headers.end());
  auto content_view{data[indx].data()};
  return response.substr(response.find(content_view));
}

std::unordered_map<std::string, std::string_view>
URL::parse_response(std::vector<std::string_view> response, int &indx) {
  std::unordered_map<std::string, std::string_view> response_headers{};
  while (true) {
    auto line = response[indx];
    if (line == "") {
      ++indx;
      break;
    }
    auto header_val{hlp::split(line, ":", 1)};
    auto [header, value] =
        std::make_tuple(std::string(header_val[0]), std::string(header_val[1]));
    hlp::casefold(header);
    response_headers[header] = hlp::strip(value);
    ++indx;
  }
  return response_headers;
}

void URL::get_response(std::string &response) {
  char chunk[4096];
  ssize_t n;
  while ((n = rio::readn(m_client.get_ssl_client(),
                         std::span(chunk, sizeof(chunk)))) > 0) {
    response.append(chunk, n);
  }
}
URL URL::resolve(std::string_view url) {
  std::string resolvedURL{};
  if (url.find("://") == std::string_view ::npos)
    return {std::move(std::string(url))};
  if (!url.starts_with('/')) {
    size_t pos1 = m_url.find_last_of('/');
    std::string_view dir{m_url.substr(0, pos1)};
    while (url.starts_with("../")) {
      size_t pos2 = url.find_first_of('/');
      url = url.substr(pos2 + 1);
      if (size_t pos3 = dir.find('/'); pos3 != std::string_view::npos) {
        dir = dir.substr(0, pos3);
      }
    }
    resolvedURL = std::string(dir) + '/' + std::string(url);
    return resolvedURL;
  }
  if (url.starts_with("//")) {
    return {std::string(scheme) + ':' + std::string(url)};
  } else {
    return {std::string(scheme) + "://" + std::string(host) + ":" +
            std::string(m_port) + std::string(url)};
  }
}

std::ostream &operator<<(std::ostream &out, const URL &url) {
  out << "Full URL: " << url.m_url << std::endl;
  out << "Scheme: " << url.scheme << std::endl;
  out << "Host: " << url.host << std::endl;
  out << "Path: " << url.path;
  return out;
}
