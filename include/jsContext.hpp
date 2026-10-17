#pragma once

#include "duktape.h"
#include "helpers.hpp"
#include <memory>
#include <unordered_map>

namespace browser {
class Tab;
}

namespace jsContext {

struct DukDeleter {
  void operator()(duk_context *ctx) const {
    if (ctx) {
      duk_destroy_heap(ctx);
      ctx = nullptr;
    }
  }
};

class JSException : public std::runtime_error {
public:
  explicit JSException(const std::string &message)
      : std::runtime_error(message) {}
};

class JSContext {

public:
  std::unordered_map<Item *, int> node_to_handler{};
  std::unordered_map<int, Item *> handler_to_node{};

  browser::Tab *tab{};
  std::unique_ptr<duk_context, DukDeleter> ctx;
  JSContext(browser::Tab *tab);

  void run(const std::string &code);
  bool dispatch_event(std::string type, Item *etl);
};
} // namespace jsContext
