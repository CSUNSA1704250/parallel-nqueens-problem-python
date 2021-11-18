//#include <fmt/core.h>
//#include <fmt/ranges.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>
using namespace std;

class Log {
 private:
  mutex m_lock;
  ofstream file;
  string file_path;
  vector<vector<int>> solutions;

 public:
  Log(const string& file_path) : file_path(file_path) {
    file.open(file_path.c_str());
    if (!file.is_open() || !file.good()) {
      // throw relevant exception.
    }
  }
  Log() {}
  void open(const string& filepath) {
    file_path = filepath;
    file.open(file_path.c_str());
  }
  void write(const string& log) {
    lock_guard<mutex> lock(m_lock);
    file << log;
  }
  void addSol(vector<int>& sol) {
    lock_guard<mutex> lock(m_lock);
    solutions.push_back(sol);
  }
  void writeVec(vector<int>& n) {
    lock_guard<mutex> lock(m_lock);
    for (auto i = 0u; i < n.size() - 1; i++) {
      file << n[i] << " ";
    }
    file << n.back() << "\n";
  }
  void writeAll() {
    // lock_guard<mutex> lock(m_lock);
    for (auto& s : solutions) {
      writeVec(s);
    }
  }

  void close() { file.close(); }

  ~Log() { close(); }
};

Log w;
Log dot;
mutex m_lock;

class ThreadPool {
 public:
  using Task = function<void()>;
  explicit ThreadPool(size_t numThreads) { start(numThreads); }
  ~ThreadPool() { stop(); }
  template <class T>
  auto enqueue(T task) -> future<decltype(task())> {
    auto wrapper = make_shared<packaged_task<decltype(task())()>>(move(task));
    {
      unique_lock<mutex> lock{mEventMutex};
      mTasks.emplace([=] { (*wrapper)(); });
    }
    mEventVar.notify_one();
    return wrapper->get_future();
  }
  size_t size() {
    unique_lock<mutex> lock{mEventMutex};
    return mTasks.size();
  }

 private:
  vector<thread> mThreads;
  condition_variable mEventVar;
  mutex mEventMutex;
  bool mStopping = false;
  queue<Task> mTasks;

  void start(size_t numThreads) {
    for (auto i = 0u; i < numThreads; ++i) {
      mThreads.emplace_back([=] {
        while (true) {
          Task task;

          {
            unique_lock<mutex> lock{mEventMutex};
            mEventVar.wait(lock, [=] { return mStopping || !mTasks.empty(); });
            if (mStopping && mTasks.empty())
              break;

            task = move(mTasks.front());
            mTasks.pop();
          }

          task();
        }
      });
    }
  }

  void stop() noexcept {
    {
      unique_lock<mutex> lock{mEventMutex};
      mStopping = true;
    }
    mEventVar.notify_all();
    for (auto& thread : mThreads)
      thread.join();
  }
};

atomic<bool> found(false);

struct FNqueens {
  unsigned long long ans = 0;
  vector<int> board;
  int n;
  bitset<64> _bits;
  array<uint64_t, 64> tracker{0};
  ThreadPool* pool = nullptr;
  int threshold = 1000000;
  bool child = false;

  FNqueens(int n) : n(n) {
    for (auto i = 0u; i < n; i++) {
      board.push_back(0);
    }
  }

  FNqueens(int n, vector<int>& board) : n(n), board(board) {}

  void all(int rowMask, int ld, int rd, int done) {
    if (rowMask == done) {
      if (n % 2 && board[0] == n / 2) {
        ans += 1;
        w.addSol(board);
      } else {
        ans += 2;
        w.addSol(board);
        auto v = vector<int>(board.begin(), board.end());
        for_each(v.begin(), v.end(), [&](auto& i) { i = abs(n - i - 1); });
        w.addSol(v);
      }
      return;
    }
    int safe = done & (~(rowMask | ld | rd));
    while (safe) {
      int i = __builtin_ctz(safe);
      int p = safe & (-safe);
      safe -= p;
      _bits = rowMask;
      board[_bits.count()] = i;
      all(rowMask | p, (ld | p) << 1, (rd | p) >> 1, done);
    }
  }

  bool firstblock(uint64_t rowMask, uint64_t ld, uint64_t rd, uint64_t done) {
    if (found) {
      return true;
    }
    if (rowMask == done && !found) {
      ++ans;
      // fmt::print("{}\n", vector<int>(tracker.begin(), tracker.begin() + n));
      found = true;
      genDot();
      return true;
    }
    uint64_t safe = done & (~(rowMask | ld | rd));
    while (safe) {
      uint64_t i = __builtin_ctzll(safe);  // counting trailing zeros
      uint64_t p = safe & (-safe);
      safe -= p;
      _bits = rowMask;
      auto count = _bits.count();
      board[count] = i;
      if (found ||
          firstblock(rowMask | p, (ld | p) << 1ull, (rd | p) >> 1ull, done)) {
        return true;
      }
    }
    return false;
  }

  bool first(uint64_t rowMask, uint64_t ld, uint64_t rd, uint64_t done) {
    if (found) {
      return true;
    }
    if (rowMask == done && !found && heavycheker(rowMask, ld, rd, done)) {
      ++ans;
      // fmt::print("{}\n", vector<int>(tracker.begin(), tracker.begin() + n));
      found = true;
      genDot();
      return true;
    }
    uint64_t safe = done & (~(rowMask | ld | rd));
    while (safe) {
      uint64_t i = __builtin_ctzll(safe);  // counting trailing zeros
      uint64_t p = safe & (-safe);
      safe -= p;
      _bits = rowMask;
      auto count = _bits.count();
      board[count] = i;
      if (!child && tracker[count] > threshold && pool->size() < 50) {
        pool->enqueue([&] {
          FNqueens q(n, board);
          q.child = true;
          q.first(rowMask | p, (ld | p) << 1ull, (rd | p) >> 1ull, done);
        });
        //} else if (child || tracker[r] < 90000) {
      } else if (child || tracker[count] < threshold * 0.1) {
        // return first(r + 1);
        if (first(rowMask | p, (ld | p) << 1ull, (rd | p) >> 1ull, done)) {
          return true;
        }
      }

      tracker[count]++;
    }
    return false;
  }

  bool heavycheker(uint64_t rowMask, uint64_t ld, uint64_t rd, uint64_t done) {
    set rows(board.begin(), board.end());
    return n == rows.size();
  }

  void genDot() {
    lock_guard<mutex> lock(m_lock);
    auto dotFile = vector<vector<string>>(n);
    // fmt::print("{}\n", board);
    fill(dotFile.begin(), dotFile.end(), vector<string>(n));
    for (auto i = 0u; i < n; i++) {
      fill(dotFile[i].begin(), dotFile[i].end(), "</td><td>    ");
      dotFile[i].front() = "<tr><td> ";
      dotFile[i].back() = "</td><td> </td></tr>\n";
      if (board[i] == n - 1)
        dotFile[i][board[i]] = "</td><td>&#9813; </td></tr>\n";
      else if (board[i] && board[i] < n - 1)
        dotFile[i][board[i]] = "</td><td>&#9813; ";
      else
        dotFile[i][board[i]] = "<tr><td>&#9813; ";
    }
    dot.write(
        "digraph D {\nnode [shape=plaintext]\nsome_node [\nlabel=<\n<table "
        "border=\"1\" cellspacing=\"0\">\n");
    for (auto i = 0u; i < n; i++) {
      for (auto j = 0u; j < n; j++) {
        dot.write(dotFile[i][j]);
      }
    }
    dot.write("</table>>\n];\n}\n");
  }
};

struct NQueens {
  int n;  // OPtimizable
  uint64_t ans = 0;
  bitset<64> pDiag, nDiag, column;
  array<int, 48> board{0};
  array<unsigned long long, 48> tracker{0};
  bool child = false;
  uint64_t threshold;
  uint64_t max = std::numeric_limits<unsigned long long>::max();
  ThreadPool* pool = nullptr;
  NQueens(int n) : n(n) { threshold = 1000000; }

  bool all(int r = 0) {
    if (r == n) {
      // state;
      auto v = vector<int>(board.begin(), board.begin() + n);
      if (n % 2 && board[0] == n / 2) {
        ans += 1;
        w.addSol(v);
      } else {
        ans += 2;
        w.addSol(v);
        for_each(v.begin(), v.end(), [&](auto& i) { i = abs(n - i - 1); });
        w.addSol(v);
      }
      return true;
    }
    for (auto c = 0u; c < n; c++) {
      if (!column[c] && !nDiag[r - c + n - 1] && !pDiag[r + c]) {
        column[c] = nDiag[r - c + n - 1] = pDiag[r + c] = 1;
        board[r] = c;
        all(r + 1);
        // backtrack
        column[c] = nDiag[r - c + n - 1] = pDiag[r + c] = 0;
      }
    }
    return false;
  }

  bool first(int r = 0) {
    if (found) {
      // fmt::print("found in another thread");
      return true;
    }

    if (!found && r == n && checker()) {
      ans += 1;
      found = true;
      genDot();
      return true;
    }

    if (tracker[r] == max) {
      return false;
    }

    for (auto c = 0u; c < n; c++) {
      if (!column[c] && !nDiag[r - c + n - 1] && !pDiag[r + c]) {
        // col[c] = row[r] = 1;
        column[c] = nDiag[r - c + n - 1] = pDiag[r + c] = 1;
        board[r] = c;

        if (!child && tracker[r] > threshold && pool->size() < 50) {
          pool->enqueue([&, c, r] {
            NQueens q(n);
            q.board = board;
            q.column = column;
            q.nDiag = nDiag;
            q.pDiag = pDiag;
            q.tracker = tracker;
            q.child = true;
            q.first(r + 1);
          });
        } else if (child || tracker[r] < threshold * 0.1) {
          if (first(r + 1)) {
            return true;
          }
        }
      }
    }
    return false;
  }

  void genDot() {
    lock_guard<mutex> lock(m_lock);
    // fmt::print("{}\n", vector<int>(tracker.begin(), tracker.begin() + n));
    auto dotFile = std::vector<std::vector<string>>(n);
    std::fill(dotFile.begin(), dotFile.end(), vector<std::string>(n));
    for (auto i = 0u; i < n; i++) {
      std::fill(dotFile[i].begin(), dotFile[i].end(), "</td><td>    ");
      dotFile[i].front() = "<tr><td> ";
      dotFile[i].back() = "</td><td> </td></tr>\n";
      if (board[i] == n - 1)
        dotFile[i][board[i]] = "</td><td>&#9813; </td></tr>\n";
      else if (board[i] && board[i] < n - 1)
        dotFile[i][board[i]] = "</td><td>&#9813; ";
      else
        dotFile[i][board[i]] = "<tr><td>&#9813; ";
    }
    dot.write(
        "digraph D {\nnode [shape=plaintext]\nsome_node [\nlabel=<\n<table "
        "border=\"1\" cellspacing=\"0\">\n");
    for (auto i = 0u; i < n; i++) {
      for (auto j = 0u; j < n; j++) {
        dot.write(dotFile[i][j]);
      }
    }
    dot.write("</table>>\n];\n}\n");
  }
  bool checker() {
    return column.count() == nDiag.count() && pDiag.count() == n;
  }
};

auto get_ans(int i, int rowMask, int ld, int rd, int done, int n) {
  FNqueens q(n);
  q.board[0] = i;
  q.all(rowMask, ld, rd, done);
  return q.ans;
}

void solve_all(int n) {
  w.write("#Solutions for " + to_string(n) + " queens\n");
  atomic<unsigned long long> ans(0);
  int done = (1 << n) - 1;
  int rowMask = 0, ld = 0, rd = 0;
  int safe = done & (~(rowMask | ld | rd)), i, p;
  {
    ThreadPool poolAll(thread::hardware_concurrency() * 2);
    for (auto c = 0u; c < (n + 1) / 2; ++c) {
      i = __builtin_ctz(safe);
      p = safe & (-safe);
      safe -= p;
      poolAll.enqueue([&ans, done, n, i, p, rowMask, ld, rd] {
        ans.fetch_add(
            get_ans(i, rowMask | p, (ld | p) << 1, (rd | p) >> 1, done, n));
      });
    }
  }
  w.write(std::to_string(ans) + "\n");
  w.writeAll();
}

void solve_all_n_threads(int n) {
  w.write("#Solutions for " + to_string(n) + " queens\n");
  vector<thread> workers;

  int done = (1 << n) - 1;
  int rowMask = 0, ld = 0, rd = 0;
  int safe = done & (~(rowMask | ld | rd)), i, p;

  atomic<unsigned long long> ans(0);
  for (auto c = 0u; c < (n + 1) / 2; ++c) {
    i = __builtin_ctz(safe);
    p = safe & (-safe);
    safe -= p;
    workers.push_back(thread([&ans, done, n, i, p, rowMask, ld, rd] {
      ans.fetch_add(
          get_ans(i, rowMask | p, (ld | p) << 1, (rd | p) >> 1, done, n));
    }));
  }
  for (auto& w : workers) {
    w.join();
  }
  w.write(std::to_string(ans) + "\n");
  w.writeAll();
}

void solve_first(int n) {
  w.close();
  NQueens q(n);
  ThreadPool pool(thread::hardware_concurrency() * 3);
  q.pool = &pool;
  q.first();
}

void solve_firstv2(uint64_t n) {
  w.close();
  uint64_t done = (1ull << n) - 1ull;
  uint64_t rowMask = 0ull, ld = 0ull, rd = 0ull;
  uint64_t safe = done & (~(rowMask | ld | rd)), i, p;
  {
    ThreadPool pool(thread::hardware_concurrency() * 2);
    for (auto c = 0u; c < (n + 1) / 2; ++c) {
      i = __builtin_ctzll(safe);
      p = safe & (-safe);
      safe -= p;
      pool.enqueue([&] {
        FNqueens q(n);
        q.board[0] = i;
        cout << q.firstblock(rowMask | p, (ld | p) << 1ull, (rd | p) >> 1ull,
                             done)
             << "\n";
      });
    }
  }
}

void solve_firstv3(uint64_t n) {
  w.close();
  {
    uint64_t done = (1ull << n) - 1ull;
    ThreadPool pool(thread::hardware_concurrency() * 2);
    FNqueens q(n);
    q.pool = &pool;
    q.first(0ull, 0ull, 0ull, done);
  }
}

int main(int argc, char** argv) {
  {
    auto mode = std::string(argv[2]);
    auto n = std::atoi(argv[4]);
    string extra;
    if (argc == 6)
      extra = std::string(argv[5]);
    // fmt::print("{} {}\n", mode, n);
    if (mode == "all") {
      w.open("solutions.txt");
      if (n > 16) {
        cout << "solve all threadpool\n";
        solve_all(n);
      } else {
        cout << "solve all n threads\n";
        solve_all_n_threads(n);
      }
    } else {
      dot.open("solution.dot");
      solve_firstv3(n);
    }
  }

  return 0;
}
