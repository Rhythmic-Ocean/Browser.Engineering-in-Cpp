#include "Tasks.hpp"

void task::TaskRunner::run() {
  if (!m_tasks.empty()) {
    auto t = std::move(m_tasks.front());
    m_tasks.pop_front();
    t->run();
  }
}
