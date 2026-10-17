#include "riolib.hpp"
#include <cassert>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <ranges>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define MAXLEN 4096
#define MAXBUF 4096
#define MY_DIR "/home/rhythmic_ocean/Desktop/Browser/Server"

static std::vector<std::string> ENTRIES{};

void do_it(int fd);
std::unordered_map<std::string, std::string> read_headers(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
std::pair<std::string, std::string>
do_request(std::string_view method, std::string_view url,
           std::unordered_map<std::string, std::string> &headers,
           std::string_view body);
std::string show_comments();

extern char **environ;

int main(int argc, char **argv) {
  typedef struct sockaddr SA;
  char hostname[MAXLEN], port[MAXLEN];
  int listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage sockadr;
  if (argc != 2) {
    fprintf(stderr, "Invalid usage. Usage: %s <port>\n", argv[0]);
    exit(1);
  }
  std::string port_str = argv[1];
  if ((listenfd = riolib::open_listenfd(port_str)) == -1) {
    fprintf(stderr, "Error opening a listening socket for port %s\n", argv[1]);
  }
  fprintf(stdout, "Listening for connections...\n");
  while (1) {
    clientlen = sizeof(sockadr);
    if ((connfd = accept(listenfd, (SA *)&sockadr, &clientlen)) < 0) {
      fprintf(stderr, "Failed connection with client!\n");
      exit(1);
    }
    if ((getnameinfo((SA *)&sockadr, clientlen, hostname, sizeof(hostname),
                     port, sizeof(port), 0)) != 0) {
      fprintf(stderr,
              "Error occured grabbing client name and port (service name)\n");
      exit(1);
    }
    fprintf(stdout, "Successfully established connection with %s:%s\n",
            hostname, port);
    do_it(connfd);
    close(connfd);
  }
}

void casefold(std::string &stri) {
  for (char &c : stri) {
    c = static_cast<char>(std::tolower(c));
  }
}

void do_it(int fd) {
  int is_static;
  rio_t rio;
  std::string usrbuf;
  std::string method, uri, ver;
  std::string filename, cgiargs;
  struct stat statbuf;
  riolib::rio_readinitb(&rio, fd);
  if ((riolib::rio_readlineb(&rio, usrbuf)) == -1) {
    fprintf(stdout, "Error reading connection file descriptor\n");
    exit(1);
  }
  std::stringstream stream{usrbuf};
  stream >> method >> uri >> ver;
  assert((!method.empty() && !uri.empty() && !ver.empty()) &&
         "Method, URI or VER corrupted");
  assert((method == "GET" || method == "POST") && "Method not GET or POST");
  std::unordered_map<std::string, std::string> headers = read_headers(&rio);
  std::string body{};
  if (headers.contains("content-length")) {
    int length = std::stoi(headers["content-length"]);
    std::string buf(length, '\0');
    if (riolib::rio_readnp(&rio, buf) != length) {
      std::cerr << "Unable to read all of html content. Read content: "
                << usrbuf << std::endl;
      exit(1);
    }
    body = riolib::PercentageDecoding::decode(std::move(buf));
  }
  auto [status, response_body] = do_request(method, uri, headers, body);
  std::string response = "HTTP/1.0 " + status + "\r\n";
  response +=
      "Content-Length: " + std::to_string(response_body.size()) + "\r\n";
  response += "\r\n" + response_body;
  if (riolib::rio_writen(fd, response) != response.size()) {
    std::cerr << "Unable to send a proper response!";
    exit(1);
  }
  return;
}

std::string_view strip(std::string_view str) {
  auto start = str.find_first_not_of(" \t\n\r\f\v");
  if (start == std::string_view::npos)
    return "";
  auto end = str.find_last_not_of(" \t\n\r\f\v");
  return str.substr(start, end - start + 1);
}

std::unordered_map<std ::string, std::string> read_headers(rio_t *rp) {
  std::unordered_map<std::string, std::string> headers{};
  std::string line{};
  while (true) {
    line.clear();
    riolib::rio_readlineb(rp, line);
    if (line == "\r\n")
      break;
    auto indx = line.find_first_of(':');
    std::string ke = line.substr(0, indx);
    std::string va = std::string(strip(line.substr(indx + 1)));
    casefold(ke);
    headers[ke] = va;
  }
  return headers;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  if (!strstr(uri, "cgi-bin")) { // means they are static content
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  } else { // means dynamic content
    ptr = strchr(uri, '?');
    if (*ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } else {
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

std::string show_comments() {
  std::string out = "<!doctype html>";
  out += "<script src=/comment.js > < / script > ";
  out += "<strong></strong>";
  out += "<form action=add method=post>";
  out += "<p><input name=guest></p>";
  out += "<p><button>Sign the book!</button></p>";
  out += "</form>";
  for (auto &entry : ENTRIES) {
    out += "<p>" + entry + "</p>";
  }
  return out;
}

/* NOTE: std::view::split returns std::ranges::split_view which is basically
       a lazy range of std::ranges::subranges which references the string like
        std::string_view (in our case) with const char* and size inside.
*       So if we were to just pass the field.data() into std::string, it will
        just be const char* and std::string wouldn't know where to put the null
        terminator
*       So we pass both the field.data() and the size
  */
std::unordered_map<std::string, std::string>
form_decode(std::string_view body) {
  std::unordered_map<std::string, std::string> params{};
  for (auto field : std::views::split(body, '&')) {
    std::string f = std::string(field.data(), field.size());
    size_t indx = f.find('=');
    std::string ke = f.substr(0, indx);
    params[ke] = f.substr(indx + 1);
  }
  return params;
}

std::string add_entry(std::unordered_map<std::string, std::string> &params) {
  if (params.contains("guest") && params["guest"].size() <= 100) {
    ENTRIES.emplace_back(params["guest"] + " was here!!");
  }
  return show_comments();
}

std::string not_found(std::string_view url, std::string_view method) {
  std::string out = "<!doctype html>";
  out += "<h1>" + std::string(method) + std::string(url) + " not found!</h1>";
  return out;
}

std::string get_file(std::string str) {
  std::filesystem::path jsPath = std::filesystem::path(MY_DIR) / str;
  std::ifstream file(jsPath);
  if (!file.is_open()) {
    std::cerr << ("Unable to open " + std::string(jsPath) + "\n");
    return "";
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

std::pair<std::string, std::string>
do_request(std::string_view method, std::string_view url,
           std::unordered_map<std::string, std::string> &headers,
           std::string_view body) {
  if (method == "GET" && url == "/") {
    return {"200 OK", show_comments()};
  } else if (method == "POST" && url == "/add") {
    auto params = form_decode(body);
    return {"200 OK", add_entry(params)};
  } else if (method == "GET" && url == "/comment.js") {
    return {"200 OK", get_file(std::move("comment.js"))};
  } else {
    return {"404 Not Found", not_found(url, method)};
  }
}
