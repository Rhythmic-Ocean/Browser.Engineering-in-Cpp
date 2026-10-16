#include "jsContext.hpp"
#include "Browser.hpp"
#include "Parser.hpp"
#include "duktape.h"
#include "helpers.hpp"
#include <duk_config.h>
#include <exception>
#include <print>

using namespace jsContext;

class JSBindings {
public:
  static duk_ret_t querySelectorAll(duk_context *ctx) {
    std::string selector_text = duk_safe_to_string(ctx, -1);
    auto selector = Parser::CSSParser(selector_text).selector();
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "js_context");
    JSContext *js = static_cast<JSContext *>(duk_get_pointer(ctx, -1));
    duk_pop_2(ctx);
    browser::Tab *tab = js->tab;
    auto nodes = hlp::tree_to_list(tab->m_rootNode.get());
    duk_idx_t arr_indx = duk_push_array(ctx);
    duk_uarridx_t i = 0;
    for (auto node : nodes) {
      if (selector->matches(node)) {
        int handle = get_handler(js, node);
        duk_push_int(ctx, handle);
        duk_put_prop_index(ctx, arr_indx, i++);
      }
    }
    return 1;
  }

  static duk_ret_t native_print(duk_context *ctx) {
    duk_push_string(ctx, " ");
    duk_insert(ctx, 0);
    duk_join(ctx, duk_get_top(ctx) - 1);
    std::println("{}", duk_safe_to_string(ctx, -1));
    return 0;
  }

  static int get_handler(JSContext *js, Item *item) {
    auto &nth_map = js->node_to_handler;
    auto &htn_map = js->handler_to_node;
    if (!nth_map.contains(item)) {
      int handle = static_cast<int>(nth_map.size());
      nth_map[item] = handle;
      htn_map[handle] = item;
    }
    return nth_map[item];
  }
};

JSContext::JSContext(browser::Tab *l_tab) : ctx{duk_create_heap_default()} {
  if (!ctx) {
    throw JSException("Unable to initialize duktape context\n");
  }
  tab = l_tab;
  duk_push_heap_stash(ctx.get());
  duk_push_pointer(ctx.get(), this);
  duk_put_prop_string(ctx.get(), -2, "js_context");
  duk_pop(ctx.get());

  duk_push_c_function(ctx.get(), JSBindings::native_print, DUK_VARARGS);
  duk_put_global_string(ctx.get(), "log");

  duk_push_c_function(ctx.get(), JSBindings::querySelectorAll, 1);
  duk_put_global_string(ctx.get(), "querySelectorAll");
}

void JSContext::run(const std::string &code) {
  try {
    // returns non zero value on error and stores error message on top of stack.
    // We examin the stack top and report the JS error
    if (duk_peval_lstring(ctx.get(), code.data(), code.size()) != 0) {
      JSException("JS Error: " +
                  std::string(duk_safe_to_string(ctx.get(), -1)));
    }
  } catch (std::exception &except) {
    std::print(stderr, "Unable to evaluate the following script:\n {}", code);
  }
}
