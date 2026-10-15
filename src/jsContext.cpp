#include "jsContext.hpp"

using namespace jsContext;

void JSContext::run(const std::string &code) {
  // returns non zero value on error and stores error message on top of stack.
  // We examin the stack top and report the JS error
  if (duk_peval_lstring(ctx.get(), code.data(), code.size()) != 0) {
    JSException("JS Error: " + std::string(duk_safe_to_string(ctx.get(), -1)));
  }
}
