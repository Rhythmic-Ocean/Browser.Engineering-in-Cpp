#include "duktape.h"
#include <memory>

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
  std::unique_ptr<duk_context, DukDeleter> ctx;
  JSContext() : ctx{duk_create_heap_default()} {
    if (!ctx) {
      throw JSException("Unable to initialize duktape context\n");
    }
  }

  void run(const std::string &code);
};
} // namespace jsContext
