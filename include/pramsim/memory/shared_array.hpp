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
struct SharedArray : Memory {
    std::vector<T> _data;
    Model _model;
    Context* _context;

    std::vector<impl::ReadRequest<T>> _read_requests;
    std::vector<impl::WriteRequest<T>> _write_requests;

    size_t _n_reads = 0;
    size_t _n_writes = 0;

    SharedArray(size_t length, T value, Model model, Context* context)
        : _data(length, value), _model(model), _context(context) {}

    SharedArray(std::vector<T> data, Model model, Context* context)
        : _data(std::move(data)), _model(model), _context(context) {}

    size_t size() const { return _data.size(); }

    const std::vector<T>& debug_data() const { return _data; }

    T operator[](size_t index) {
        assert_or_throw(_context->current_pid.has_value(), "Read outside parallel region");
        _read_requests.push_back({.internal_ref = &_data[index], .pid = *_context->current_pid});
        return _data[index];
    }

    void write(size_t index, T value) {
        assert_or_throw(_context->current_pid.has_value(), "Write outside parallel region");
        _write_requests.push_back({.internal_ref = &_data[index], .value = value, .pid = *_context->current_pid});
    }

    void commit_round() override {
        std::ranges::sort(_read_requests, [](const impl::ReadRequest<T>& a, const impl::ReadRequest<T>& b) {
            return std::pair{a.internal_ref, a.pid} < std::pair{b.internal_ref, b.pid};
        });
        auto unique =
            std::ranges::unique(_read_requests, [](const impl::ReadRequest<T>& a, const impl::ReadRequest<T>& b) {
                return std::pair{a.internal_ref, a.pid} == std::pair{b.internal_ref, b.pid};
            });
        _read_requests.erase(unique.begin(), unique.end());
        std::ranges::sort(_write_requests, [](const impl::WriteRequest<T>& a, const impl::WriteRequest<T>& b) {
            return std::pair{a.internal_ref, a.pid} < std::pair{b.internal_ref, b.pid};
        });

        {
            for (size_t i = 0, j = 0; i < _read_requests.size() && j < _write_requests.size();) {
                if (_read_requests[i].internal_ref < _write_requests[j].internal_ref) {
                    i++;
                } else if (_read_requests[i].internal_ref > _write_requests[j].internal_ref) {
                    j++;
                } else {
                    assert_or_throw(false, "Read-write conflict: read and write to the same address");
                }
            }
        }

        switch (_model.read_policy) {
            case impl::ReadPolicy::Exclusive:  // 处理互斥读
                impl::check_exclusive_read(_read_requests);
                break;
            case impl::ReadPolicy::Concurrent:  // 处理并发读
                break;
        }

        switch (_model.write_policy) {
            case impl::WritePolicy::Exclusive:  // 处理互斥写
                impl::check_exclusive_write(_write_requests);
                impl::apply_write(_write_requests);
                break;
            case impl::WritePolicy::Common:  // 处理公共写
                impl::check_common_write(_write_requests);
                impl::apply_write(_write_requests);
                break;
            case impl::WritePolicy::Arbitrary:  // 处理任意写
                impl::apply_arbitrary_write(_write_requests, _context);
                break;
            case impl::WritePolicy::Priority:  // 处理优先级写
                impl::apply_priority_write(_write_requests);
                break;
            case impl::WritePolicy::Add:  // 处理合并写 加法
                impl::apply_combining_write(_write_requests, std::plus<T>{});
                break;
            case impl::WritePolicy::Max:  // 处理合并写 取最大值
                impl::apply_combining_write(_write_requests, [](const T& a, const T& b) { return std::max(a, b); });
                break;
            case impl::WritePolicy::Min:  // 处理合并写 取最小值
                impl::apply_combining_write(_write_requests, [](const T& a, const T& b) { return std::min(a, b); });
                break;
        }

        _n_reads += _read_requests.size();
        _n_writes += _write_requests.size();

        _read_requests.clear();
        _write_requests.clear();
    }

    size_t n_reads() const override { return _n_reads; }
    size_t n_writes() const override { return _n_writes; }
};
}  // namespace pram
