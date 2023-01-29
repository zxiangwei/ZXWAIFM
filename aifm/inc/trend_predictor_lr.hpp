#pragma once

template<typename T, uint64_t kMaxWinSize>
class TrendPredictor {
 public:
  TrendPredictor() = default;

  void AddHistory(T history) {
    if (filled_elem_num_ <= kMaxWinSize) {
      filled_elem_num_++;
    }
    histories_[next_history_index_] = history;
    if (trend_len_ == 0) {  // 初始化 Trend
      // 预测成未来一直保持相同的 history
      trend_begin_index_ = next_history_index_;
      trend_len_ = 1;
      pos_in_trend_ = 0;
      success_ = true;
    } else {  // trend_len_ > 0
      uint64_t test_index = AddIndex(trend_begin_index_, pos_in_trend_);
      if (history == histories_[test_index]) {  // 符合 Trend
        // 按照 Trend 进行预测
        UpdatePosInTrend();
        success_ = true;
      } else {  // 不符合 Trend
        // 首先尝试往前扩大 Trend
        assert(filled_elem_num_ >= trend_len_);
        uint64_t extend_num = filled_elem_num_ - trend_len_ - 1;
        uint64_t search_index = DelIndex(trend_begin_index_, extend_num);
        // 从范围大往范围小搜索
        bool success = false;
        for (uint64_t i = 0; i < extend_num; ++i) {
          if (history == histories_[search_index]) {  // 搜索成功
            success = true;
            break;
          }
          search_index = AddIndex(search_index, 1);
        }
        if (success) {  // 扩大成功
          trend_begin_index_ = search_index;
          trend_len_ = (next_history_index_ + kHistorySize - trend_begin_index_) % kHistorySize;
          pos_in_trend_ = 1;
          success_ = true;
        } else {  // 扩大范围失败
          // 尝试缩小范围
          uint64_t narrow_num = trend_len_ - 1;
          search_index = AddIndex(trend_begin_index_, 1);
          for (uint64_t i = 0; i < narrow_num; ++i) {
            if (history == histories_[search_index]) {  // 搜索成功
              success = true;
              break;
            }
            search_index = AddIndex(search_index, 1);
          }
          if (success) { // 缩小成功
            trend_begin_index_ = search_index;
            trend_len_ = (next_history_index_ + kHistorySize - trend_begin_index_) % kHistorySize;
            pos_in_trend_ = 1;
            success_ = true;
          } else {  // 缩小失败
            // 预测失败
            trend_begin_index_ = next_history_index_;
            trend_len_ = 1;
            pos_in_trend_ = 0;
            success_ = false;
          }
        }
      }
    }
    next_history_index_ = AddIndex(next_history_index_, 1);
  }

  // MatchTrend must be true
  T GetTrend(uint64_t distance = 0) {
    distance = distance % trend_len_;
    return histories_[AddIndex(trend_begin_index_, pos_in_trend_ + distance)];
  }

  bool MatchTrend() {
    return success_;
  }

  std::optional<T> Predict(T history) {
    AddHistory(history);
    if (MatchTrend()) {
      return GetTrend(0);
    }
    return std::nullopt;
  }

 private:
  uint64_t AddIndex(uint64_t index, uint64_t diff) {
    return (index + diff) % kHistorySize;
  }
  uint64_t DelIndex(uint64_t index, uint64_t diff) {
    return (index + kHistorySize - diff) % kHistorySize;
  }
  void UpdatePosInTrend() {
    pos_in_trend_++;
    if (pos_in_trend_ == trend_len_) {
      trend_begin_index_ = AddIndex(trend_begin_index_, trend_len_);
      pos_in_trend_ = 0;
    }
  }

  static constexpr uint64_t kHistorySize = kMaxWinSize + 1;

  T histories_[kHistorySize];
  uint64_t trend_begin_index_{0};
  uint64_t trend_len_{0};
  uint64_t pos_in_trend_{0};
  uint64_t next_history_index_{0};
  uint64_t filled_elem_num_{0};
  bool success_{false};
};
