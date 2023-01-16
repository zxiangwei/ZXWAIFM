#pragma once

template<typename T>
class BoyerMooreVote {
 public:
  BoyerMooreVote() = default;

  bool IsMode(T elem) {
    if (count_ == 0) {
      majority_ = elem;
      ++count_;
      return true;
    } else {
      if (majority_ == elem) {
        ++count_;
        return true;
      } else {
        --count_;
      }
    }
    return false;
  }

 private:
  T majority_;  // 众数
  int count_{0};
};
