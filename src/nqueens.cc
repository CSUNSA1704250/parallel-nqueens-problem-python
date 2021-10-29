//#include <fmt/core.h>
//#include <fmt/ranges.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <condition_variable>
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
  string m_file_path;
  vector<vector<int>> solutions;

 public:
  Log(const string& file_path) : m_file_path(file_path) {
    file.open(m_file_path.c_str());
    if (!file.is_open() || !file.good()) {
      // throw relevant exception.
    }
  }
  Log() {}
  void open(const string& filepath) {
    m_file_path = filepath;
    file.open(m_file_path.c_str());
  }
  void write(const string& log) {
    lock_guard<mutex> lock(m_lock);
    file << log;
  }
  void addSol(vector<int>&& sol) {
    lock_guard<mutex> lock(m_lock);
    solutions.push_back(sol);
  }
  void writeArr(array<int, 48>& n, int sz) {
    lock_guard<mutex> lock(m_lock);
    for (auto i = 0u; i < sz - 1; i++) {
      file << n[i] << " ";
    }
    file << n[sz - 1] << "\n";
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
// Log w("solutions.txt");
// Log dot("solution.dot");

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

ThreadPool pool{thread::hardware_concurrency() * 2};
atomic<bool> found(false);

struct NQueens {
  int n;  // OPtimizable
  // NQueensState state;
  unsigned long long ans = 0;
  // int row = 0, col = 0;
  bitset<64> pDiag, nDiag, column;
  array<int, 48> board{0};
  array<unsigned long long, 48> tracker{0};
  bool child = false;
  unsigned long long threshold;
  unsigned long long max = std::numeric_limits<unsigned long long>::max();
  // vector<vector<int>> solutions;
  NQueens(int n) : n(n) { threshold = 10000000; }

  bool all(int r = 0) {
    if (r == n) {
      // state;
      ans += 1;
      w.addSol(vector<int>(board.begin(), board.begin() + n));
      // w.writeArr(board, n);
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
      costly_state();
      genDot();
      return true;
    }

    for (auto c = 0u; c < n; c++) {
      if (!column[c] && !nDiag[r - c + n - 1] && !pDiag[r + c]) {
        column[c] = nDiag[r - c + n - 1] = pDiag[r + c] = 1;
        board[r] = c;

        if (!child && tracker[r] > threshold && pool.size() < 1000) {
          pool.enqueue([&, c, r] {
            NQueens q(n);
            q.board = board;
            q.column = column;
            q.nDiag = nDiag;
            q.pDiag = pDiag;
            q.child = true;
            q.first(r + 1);
          });
          //} else if (child || tracker[r] < lthreshold) {
        } else if (child || tracker[r] < 900000) {
          if (first(r + 1)) {
            return true;
          }
        }
        // backtrack
        if (tracker[r] < max)
          tracker[r]++;
        column[c] = nDiag[r - c + n - 1] = pDiag[r + c] = 0;
      }
    }
    return false;
  }

  void print_state() { cout << ans << "\n"; }

  void costly_state() {
    // unique_lock<mutex> lk(mtx);
    // fmt::print("{}\n", vector<int>(board.begin(), board.begin() + n));
    // fmt::print("checker: {}\n", checker());
  }
  void costly_print() {
    auto vec = vector<int>(board.begin(), board.begin() + n);
    for_each(vec.begin(), vec.end(), [](auto& d) { d += 1; });
    // fmt::print("({}) {}\n", vec.size(), vec);
    auto b = vector<string>(n);
    // bitset<n> b;

    for (auto i = 0u; i < n; i++) {
      fill(b.begin(), b.end(), " ");
      b[board[i]] = "█";
      // for_each(b.begin(), b.end(), [](auto& e) { fmt::print("[{}]", e); });
      cout << "\n";
    }
  }
  void genDot() {
    lock_guard<mutex> lock(m_lock);
    auto dotFile = std::vector<std::vector<string>>(n);
    std::fill(dotFile.begin(), dotFile.end(), vector<std::string>(n));
    for (auto i = 0u; i < n; i++) {
      // dotFile[i] = std::vector<std::string>(n);
      // std::fill()
      // for (auto j = 0u; j < n; j++) {
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
    vector<vector<int>> v(n);
    std::fill(v.begin(), v.end(), vector<int>(n));
    for (auto i = 0u; i < n; i++) {
      std::fill(v[i].begin(), v[i].end(), 0);
    }
    for (auto i = 0u; i < n; i++) {
      v[i][board[i]] = 1;
      // fmt::print("{}\n", v[i]);
    }
    // auto rows = <int>();
    set<int> _pDiag, _nDiag, _column, _rows;
    // set<int> _pDiag, _nDiag, _column, _rows;
    for (auto i = 0u; i < n; i++) {
      for (auto j = 0u; j < n; j++) {
        if (v[i][j]) {
          _rows.insert(i);
          _column.insert(j);
          _nDiag.insert(i - j);
          _pDiag.insert(i + j);
        }
      }
    }
    // fmt::print("{} {} {} {}\n",_);
    return _rows.size() == _column.size() && _nDiag.size() == _pDiag.size() &&
           _pDiag.size() == n;
  }
};

void accumulateAll(atomic<unsigned int>& ans,
                   const unsigned int col,
                   const unsigned int n) {
  // unsigned int r = 0;
  // unsigned int tmp = 0;
  auto r = 0u;
  NQueens q(n);
  q.board[r] = col;
  q.column[col] = 1;
  q.nDiag[r - col + n - 1] = 1;
  q.pDiag[r + col] = 1;
  q.all(r + 1);
  ans.fetch_add(q.ans);
}

void solve_all(int n) {
  w.write("#Solutions for " + to_string(n) + " queens\n");
  atomic<unsigned int> ans(0);
  {
    ThreadPool poolAll(12);
    for (auto c = 0u; c < n; ++c) {
      poolAll.enqueue([&ans, c, n] { accumulateAll(ans, c, n); });
    }
  }
  w.write(std::to_string(ans) + "\n");
  w.writeAll();
  // fmt::print("{}\n", ans);
}
void solve_all_n_threads(int n) {
  w.write("#Solutions for " + to_string(n) + " queens\n");
  vector<thread> workers(n);
  atomic<unsigned int> ans(0);
  for (auto c = 0u; c < n; ++c) {
    // workers[c] = thread(accumulateAll, ref(ans), c, n,
    // ref(w));
    workers[c] = thread([&ans, c, n] { accumulateAll(ans, c, n); });
  }
  for (auto& w : workers) {
    w.join();
  }
  w.write(std::to_string(ans) + "\n");
  w.writeAll();
  // fmt::print("{}\n", ans);
}
void solve_first(int n) {
  w.close();
  NQueens q(n);
  q.first();
}

int main(int argc, char** argv) {
  {
    auto mode = std::string(argv[2]);
    auto n = std::atoi(argv[4]);
    // fmt::print("{} {}\n", mode, n);
    if (mode == "all") {
      w.open("solutions.txt");
      if (n > 16) {
        solve_all(n);
      } else {
        solve_all_n_threads(n);
      }
    } else {
      dot.open("solution.dot");
      solve_first(n);
    }
  }

  return 0;
}
