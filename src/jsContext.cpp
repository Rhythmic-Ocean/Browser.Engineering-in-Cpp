#include "jsContext.hpp"
#include "Browser.hpp"
#include "Parser.hpp"
#include "duktape.h"
#include "helpers.hpp"
#include <duk_config.h>
#include <print>

using namespace jsContext;

class JSBindings {
public:
  static duk_ret_t innerHTML_set(duk_context *ctx) {
    JSContext *js = getJSContext(ctx);
    std::string str{
        std::format("<html><body>{}</body></html>", duk_get_string(ctx, -1))};
    int handle = duk_get_int(ctx, -2);

    auto doc = Parser::HTMLParser(str).parse();
    auto &new_nodes = doc->m_children[0]->m_children;
    auto elt = js->handler_to_node[handle];
    for (auto &node : new_nodes) {
      elt->m_children.push_back(std::move(node));
    }
    for (auto &child : elt->m_children) {
      child->m_parent = elt;
    }
    js->tab->render();
    return 0;
  }

  static duk_ret_t getAttribute(duk_context *ctx) {
    JSContext *js = getJSContext(ctx);
    std::string attr = duk_get_string(ctx, -1);
    int handle = duk_get_int(ctx, -2);
    Tag *tag = static_cast<Tag *>(js->handler_to_node[handle]);
    if (tag->m_attributes.contains(attr)) {
      duk_push_string(ctx, tag->m_attributes[attr].c_str());
    } else {

      duk_push_undefined(ctx);
    }
    return 1;
  }

  static duk_ret_t querySelectorAll(duk_context *ctx) {
    JSContext *js = getJSContext(ctx);
    std::string selector_text = duk_safe_to_string(ctx, -1);
    auto selector = Parser::CSSParser(selector_text).selector();
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

  static JSContext *getJSContext(duk_context *ctx) {
    duk_push_heap_stash(ctx);
    duk_get_prop_string(ctx, -1, "js_context");
    JSContext *js = static_cast<JSContext *>(duk_get_pointer(ctx, -1));
    duk_pop_2(ctx);
    return js;
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

  duk_push_c_function(ctx.get(), JSBindings::getAttribute, 2);
  duk_put_global_string(ctx.get(), "getAttribute");
}

void JSContext::run(const std::string &code) {
  // returns non zero value on error and stores error message on top of stack.
  // We examin the stack top and report the JS error
  if (duk_peval_lstring(ctx.get(), code.data(), code.size()) != 0) {
    JSException("JS Error: " + std::string(duk_safe_to_string(ctx.get(), -1)));
  }
}

void JSContext::dispatch_event(std::string type, Item *etl) {
  int handle = -1;
  if (node_to_handler.contains(etl)) {
    handle = node_to_handler[etl];
  }
  std::string eval_string =
      std::format("new Node({}).dispatchEvent(\"{}\")", handle, type);
  if (duk_peval_lstring(ctx.get(), eval_string.c_str(), eval_string.size()) !=
      0) {
    std::println(stderr, "Failed dispatching event:{} for the Tag: {}", type,
                 etl->m_text);
    std::println(stderr, "Error: {}", duk_safe_to_string(ctx.get(), -1));
  }
  duk_pop(ctx.get());
}
