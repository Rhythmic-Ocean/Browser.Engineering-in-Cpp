#include "Random.hpp"
#include "riolib.hpp"
#include <cassert>
#include <climits>
#include <fcntl.h>
#include <filesystem>
#include <format>
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

typedef std::unordered_map<std::string, std::string> SessionData;
std::unordered_map<std::string, SessionData> SESSIONS{};
/*
 * token:{
 *      "user" = username
 * }
 *
 *
 * eg:
 * "60000"{
 *      "user" = "smone"
 * }
 * */
std::unordered_map<std::string, std::string> LOGINS{
    {"smthingsmthing", "smonesmone"},
    {"othersmthing", "othersmone"},
};

static std::vector<std::string> ENTRIES{"Ligma Balls by YoursTruely"};

void do_it(int fd);
std::unordered_map<std::string, std::string> read_headers(rio_t *rp);
std::pair<std::string, std::string>
do_request(SessionData &session, std::string_view method, std::string_view url,
           std::unordered_map<std::string, std::string> &headers,
           std::string_view body);
std::string show_comments(SessionData &session);
std::string login_form(SessionData &session);
std::pair<std::string, std::string>
do_login(SessionData &session,
         std::unordered_map<std::string, std::string> &params);

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

std::string html_escape(std::string str) {
  std::string response{};
  for (char c : str) {
    if (c == '<')
      response += "&lt";
    else if (c == '>')
      response += "&gt";
    response += c;
  }
  return response;
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
  std::string token{};
  if (headers.contains("cookie")) {
    token = headers["cookie"].substr(std::string("token=").size());
  } else {
    token = std::to_string(Random::get<int>(0, INT_MAX));
  }
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
  auto &session = SESSIONS[token];
  auto [status, response_body] =
      do_request(session, method, uri, headers, body);
  std::string response = "HTTP/1.0 " + status + "\r\n";
  if (!headers.contains("cookie")) {
    response += std::format("Set-Cookie: token={}; SameSite=Lax\r\n", token);
  }
  response += std::format(
      "Content-Security-Policy: default-src http://127.0.0.1:8000\r\n");
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

std::string show_comments(SessionData &session) {
  std::string out = "<!doctype html>";
  out += "<script src=comment.js></script>";
  if (session.contains("user")) {
    auto nonce = Random::get(0, 10000000);
    session["nonce"] = std::to_string(nonce);
    out += "<strong></strong>";
    out += "<h1>Hello, " + session["user"] + "</h1>";
    out += "<form action=add method=post>";
    out += "<p><input name=guest></p>";
    out += "<input name=nonce type=hidden value=" + std::to_string(nonce) + ">";
    out += "<p><button>Sign the book!</button></p>";
    out += "</form>";
  } else {
    out += "<a href=/login>Sign in to write in the guest book</a>";
  }
  for (auto &entry : ENTRIES) {
    out += "<p>" + html_escape(entry) + "</p>";
  }
  out += "</html>";
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

std::string add_entry(SessionData &session,
                      std::unordered_map<std::string, std::string> &params) {
  if (!session.contains("user"))
    return "no in sessioin";
  if (!session.contains("nonce") || !params.contains("nonce") ||
      session["nonce"] != params["nonce"])
    return "invalid nonce";
  if (params.contains("guest") && params["guest"].size() <= 100) {
    std::string str = std::format("{} by {}", params["guest"], session["user"]);
    std::cout << str << std::endl;
    ENTRIES.emplace_back(std::move(str));
  }
  return show_comments(session);
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
do_request(SessionData &session, std::string_view method, std::string_view url,
           std::unordered_map<std::string, std::string> &headers,
           std::string_view body) {
  if (method == "GET" && url == "/") {
    return {"200 OK", show_comments(session)};
  } else if (method == "POST" && url == "/") {
    auto params = form_decode(body);
    return do_login(session, params);
  } else if (method == "GET" && url == "/login") {
    return {"200 OK", login_form(session)};
  } else if (method == "POST" && url == "/add") {
    auto params = form_decode(body);
    return {"200 OK", add_entry(session, params)};
  } else if (method == "GET" && url == "/comment.js") {
    return {"200 OK", get_file(std::move("comment.js"))};
  } else {
    return {"404 Not Found", not_found(url, method)};
  }
}

std::string login_form(SessionData &session) {
  std::string body{};
  body = "<!doctype html>";
  body += "<form action=/ method=post>";
  body += "<p>Username: <input name=username></p>";
  body += "<p>Password: <input name=password type=password></p>";
  body += "<p><button>Log in</button></p>";
  body += "</form>";
  return body;
}

std::pair<std::string, std::string>
do_login(SessionData &session,
         std::unordered_map<std::string, std::string> &params) {
  std::string username = params["username"];
  std::string password = params["password"];
  if (LOGINS.contains(username) && LOGINS[username] == password) {
    session["user"] = username;
    return {"200 OK", show_comments(session)};
  } else {
    std::string out{};
    out = "<!doctype html>";
    out += std::format("<h1>Invalid password for {}</h1>", username);
    return {"401 Unauthorized", out};
  }
}
