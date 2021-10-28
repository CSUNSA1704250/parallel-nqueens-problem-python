#include <fmt/core.h>
#include <fmt/ranges.h>
#include <algorithm>
#include <atomic>
#include <bitset>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

struct Wrapper {
  unsigned int n;
  bitset<64> column, diag1, diag2;
  array<int, 64> board;

  Wrapper(const unsigned int n) : n(n) {}

  bool solveNQueen(const unsigned int r, unsigned int& ans) {
    // void solveNQueen(const unsigned int r, unsigned int& ans) {
    if (r == n) {
      ans++;
      return true;
      // return;
    }
    for (int c = 0; c < n; c++) {
      if (!column[c] && !diag1[r - c + n - 1] && !diag2[r + c]) {
        column[c] = diag1[r - c + n - 1] = diag2[r + c] = 1;
        solveNQueen(r + 1, ans);
        // backtrack
        column[c] = diag1[r - c + n - 1] = diag2[r + c] = 0;
      }
    }
    return false;
  }
  bool solveFirst(const unsigned int r, unsigned int& ans) {
    // void solveNQueen(const unsigned int r, unsigned int& ans) {
    if (r == n) {
      ++ans;
      costlyPrint();
      return true;
    }
    for (auto c = 0u; c < n; c++) {
      // if (!column[c] && !diag1[r - c + n - 1] && !diag2[r + c]) {
      if (!column[c] && !diag1[r - c + n - 1] && !diag2[r + c] && !ans) {
        column[c] = diag1[r - c + n - 1] = diag2[r + c] = 1;
        board[r] = c;
        if (solveFirst(r + 1, ans)) {
          return true;
        }
        // backtrack
        column[c] = diag1[r - c + n - 1] = diag2[r + c] = 0;
      }
    }
    return false;
  }
  void costlyPrint() {
    auto vec = vector<int>(board.begin(), board.begin() + n);
    for_each(vec.begin(), vec.end(), [](auto& d) { d += 1; });
    fmt::print("({}) {}\n", vec.size(), vec);
    auto b = vector<string>(n);
    // bitset<n> b;

    for (auto i = 0u; i < n; i++) {
      fill(b.begin(), b.end(), " ");
      b[board[i]] = "█";
      for_each(b.begin(), b.end(), [](auto& e) { fmt::print("[{}]", e); });
      cout << "\n";
    }
  }
};

void accumulateAnswers(atomic<unsigned int>& ans,
                       const unsigned int col,
                       const unsigned int n) {
  // unsigned int r = 0;
  unsigned int tmp = 0;
  Wrapper queens(n);
  queens.column[col] = 1;
  queens.diag1[0 - col + n - 1] = 1;
  queens.diag2[0 + col] = 1;
  queens.solveNQueen(0 + 1, tmp);
  ans.fetch_add(tmp);
}

void queensThreads(const unsigned int n, atomic<unsigned int>& ans) {
  vector<thread> workers(n);
  for (unsigned int c = 0; c < n; ++c) {
    workers[c] = thread(accumulateAnswers, std::ref(ans), c, n);
    // workers[c] = thread([&ans, c, n] { accumulateAnswers(ans, c, n); });
  }
  for (auto& w : workers) {
    w.join();
  }
}

unsigned int queensFirst(const unsigned int n) {
  Wrapper queens(n);
  auto ans = 0u;
  queens.solveFirst(0, ans);
  return ans;
}

int main(int argc, char* argv[]) {
  const unsigned int n = atoi(argv[1]);
  // atomic<unsigned int> ans(0);
  // solveNQueen(0, n, ans);
  // queensThreads(n, ans);
  // cout << ans << endl;
  cout << queensFirst(n) << '\n';

  return 0;
}
