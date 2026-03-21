#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "memory.hpp"
#include "pramsim/base/assert.hpp"
#include "pramsim/machine/context.hpp"
#include "pramsim/machine/model.hpp"
#include "resolver.hpp"

namespace pram {
template <typename T>
class SharedArray : public Memory {
   public:
    SharedArray(size_t length, T value, Model model, Context* context)
        : data_(length, value), model_(model), context_(context) {}

    SharedArray(std::vector<T> data, Model model, Context* context)
        : data_(std::move(data)), model_(model), context_(context) {}

    size_t size() const { return data_.size(); }

    const std::vector<T>& debug_data() const { return data_; }

    T operator[](size_t index) {
        assert_or_throw(context_->current_pid.has_value(), "Read outside parallel region");
        read_requests_.push_back({.internal_ref = &data_[index], .pid = *context_->current_pid});
        return data_[index];
    }

    void write(size_t index, T value) {
        assert_or_throw(context_->current_pid.has_value(), "Write outside parallel region");
        write_requests_.push_back({.internal_ref = &data_[index], .value = value, .pid = *context_->current_pid});
    }

    void commit_round() override {
        auto key = [](const auto& req) { return std::pair{req.internal_ref, req.pid}; };

        // 排序读请求和去重，一个处理器读同一地址只算一次读
        std::ranges::sort(read_requests_, {}, key);
        read_requests_.erase(std::ranges::unique(read_requests_, {}, key).begin(), read_requests_.end());

        // 排序写请求
        std::ranges::sort(write_requests_, {}, key);

        // 检查读写冲突
        check_read_write_conflict(read_requests_, write_requests_);

        switch (model_.read_policy) {
            case impl::ReadPolicy::Exclusive:  // 处理互斥读
                impl::check_exclusive_read(read_requests_);
                break;
            case impl::ReadPolicy::Concurrent:  // 处理并发读
                break;
        }

        switch (model_.write_policy) {
            case impl::WritePolicy::Exclusive:  // 处理互斥写
                impl::check_exclusive_write(write_requests_);
                impl::apply_write(write_requests_);
                break;
            case impl::WritePolicy::Common:  // 处理公共写
                impl::check_common_write(write_requests_);
                impl::apply_write(write_requests_);
                break;
            case impl::WritePolicy::Arbitrary:  // 处理任意写
                impl::apply_arbitrary_write(write_requests_, context_);
                break;
            case impl::WritePolicy::Priority:  // 处理优先级写
                impl::apply_priority_write(write_requests_);
                break;
            case impl::WritePolicy::Add:  // 处理合并写 加法
                impl::apply_combining_write(write_requests_, std::plus<T>{});
                break;
            case impl::WritePolicy::Max:  // 处理合并写 取最大值
                impl::apply_combining_write(write_requests_, [](const T& a, const T& b) { return std::max(a, b); });
                break;
            case impl::WritePolicy::Min:  // 处理合并写 取最小值
                impl::apply_combining_write(write_requests_, [](const T& a, const T& b) { return std::min(a, b); });
                break;
        }

        n_reads_ += read_requests_.size();
        n_writes_ += write_requests_.size();

        read_requests_.clear();
        write_requests_.clear();
    }

    size_t n_reads() const override { return n_reads_; }
    size_t n_writes() const override { return n_writes_; }

   private:
    std::vector<T> data_;
    Model model_;
    Context* context_;

    std::vector<impl::ReadRequest<T>> read_requests_;
    std::vector<impl::WriteRequest<T>> write_requests_;

    size_t n_reads_ = 0;
    size_t n_writes_ = 0;
};
}  // namespace pram
