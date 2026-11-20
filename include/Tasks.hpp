#pragma once

#include <deque>
#include <memory>
#include <tuple>
#include <utility>

namespace browser {
class Tab;
}

namespace task {

class TaskWrapper {
public:
  virtual void run() = 0;
  virtual ~TaskWrapper() = default;
};

template <typename T> class TaskWrapper_ : public TaskWrapper {
  T m_task;

public:
  TaskWrapper_(T task) : m_task{std::move(task)} {}

  void run() { m_task.run(); }
};

template <typename F, typename... Args> class Task {
  F m_fPtr;
  std::tuple<Args...> m_args;

public:
  template <typename Fn, typename... A>
  Task(Fn &&fPtr, A &&...args)
      : m_fPtr{std::forward<Fn>(fPtr)}, m_args(std::forward<A>(args)...) {}

  void run() { std::apply(std::move(m_fPtr), std::move(m_args)); }

  Task &operator=(Task const &) = delete;
  Task(Task const &) = delete;

  Task(Task &&) = default;
  Task &operator=(Task &&) = default;
  ~Task() = default;
};

class TaskRunner {
  const browser::Tab &m_tab;
  std::deque<std::unique_ptr<TaskWrapper>> m_tasks{};

public:
  TaskRunner(const browser::Tab &tab) : m_tab{tab} {}
  void schedule_task(std::unique_ptr<TaskWrapper> task) {
    m_tasks.push_back(std::move(task));
  }

  void run();

  TaskRunner &operator=(TaskRunner const &) = delete;
  TaskRunner(TaskRunner const &) = delete;

  TaskRunner(TaskRunner &&) = delete;
  TaskRunner &operator=(TaskRunner &&) = delete;
  ~TaskRunner() = default;
};
} // namespace task
