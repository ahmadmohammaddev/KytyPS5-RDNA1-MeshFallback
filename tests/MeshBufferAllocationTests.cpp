// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/host_gpu/renderer/cache/bufferAllocationRetry.h"
#include "graphics/host_gpu/renderer/meshFallbackBufferPreparation.h"
#include "graphics/host_gpu/renderer/meshFallbackRetention.h"
#include <iostream>
#include <vector>
using namespace Libs::Graphics;
namespace B = MeshFallback;
int failures = 0;
void Check(bool b, const char *s) {
  if (!b) {
    ++failures;
    std::cerr << s << '\n';
  }
}
struct Resource {
  int *destroyed;
  ~Resource() { ++*destroyed; }
};
int main() {
  {
    // Controlled completion adapter tests the actual retention helper. This is
    // NOT execution of CommandScheduler or proof about an arbitrary GPU fence.
    struct Pending {
      uint64_t tick;
      Common::UniqueFunction<void> cleanup;
    };
    std::vector<Pending> pending;
    bool open = true;
    uint64_t recording = 7;
    int destroyed = 0;
    auto admit = [&](uint64_t tick, Common::UniqueFunction<void> &cleanup) {
      if (!open || tick != recording)
        return false;
      pending.push_back({tick, std::move(cleanup)});
      return true;
    };
    auto poll = [&](uint64_t completed) {
      for (auto i = pending.begin(); i != pending.end();) {
        if (i->tick > completed) {
          ++i;
          continue;
        }
        i->cleanup();
        i = pending.erase(i);
      }
    };
    auto first = std::make_unique<Resource>(&destroyed);
    const auto *first_address = first.get();
    Check(B::RetainBeforeUse(7, std::move(first), admit) && !first &&
              destroyed == 0 && pending.size() == 1,
          "accepted recording holds resource after producer scope");
    poll(6);
    Check(destroyed == 0, "incomplete work cannot release owner");
    recording = 8;
    auto second = std::make_unique<Resource>(&destroyed);
    Check(second.get() != first_address, "in-flight storage not reused");
    Check(B::RetainBeforeUse(8, std::move(second), admit),
          "next recording separate owner");
    auto stale = std::make_unique<Resource>(&destroyed);
    Check(!B::RetainBeforeUse(7, std::move(stale), admit) && destroyed == 1 &&
              pending.size() == 2,
          "stale registration cleans unsubmitted owner, keeps earlier pending");
    poll(7);
    Check(destroyed == 2 && pending.size() == 1,
          "only first completed tick retires");
    poll(7);
    Check(destroyed == 2, "repeated completion cannot double free");
    open = false;
    auto closed = std::make_unique<Resource>(&destroyed);
    Check(!B::RetainBeforeUse(8, std::move(closed), admit) && destroyed == 3 &&
              pending.size() == 1,
          "closed admission cannot publish or free in-flight owner");
    poll(8);
    Check(destroyed == 4 && pending.empty(), "drain releases last owner once");
    std::unique_ptr<Resource> invalid;
    Check(!B::RetainBeforeUse(8, std::move(invalid), admit) && destroyed == 4,
          "empty owner rejects");
    open = true;
    recording = 9;
    auto abandoned = std::make_unique<Resource>(&destroyed);
    Check(B::RetainBeforeUse(9, std::move(abandoned), admit),
          "accepted without recorded use");
    Check(destroyed == 4, "abandoned recording retained until drain");
    poll(9);
    Check(destroyed == 5 && pending.empty(),
          "completed empty recording cleanup");
    auto no_queue_memory = std::make_unique<Resource>(&destroyed);
    Check(!B::RetainBeforeUse(9, std::move(no_queue_memory),
                              [](uint64_t, Common::UniqueFunction<void> &)
                                  -> bool { throw std::bad_alloc(); }) &&
              destroyed == 6,
          "admission allocation failure cleans owner before any GPU use");
  }
  for (int succeeds = 1; succeeds <= 4; ++succeeds) {
    int calls = 0, trim = 0, relax = 0, reports = 0;
    std::vector<int> order;
    auto result = Detail::RetryBufferAllocation(
        [&] {
          order.push_back(1);
          return ++calls == succeeds ? VK_SUCCESS
                                     : VK_ERROR_OUT_OF_DEVICE_MEMORY;
        },
        [&] {
          ++trim;
          order.push_back(2);
        },
        [&] {
          ++relax;
          order.push_back(3);
        },
        [&](VkResult r, bool over) {
          ++reports;
          Check(over == (succeeds == 3), "over-budget report");
          Check(
              r == (succeeds <= 3 ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY),
              "final report error");
        },
        VK_SUCCESS);
    Check(calls == (succeeds < 4 ? succeeds : 3) &&
              trim == (succeeds == 1 ? 0 : 1) &&
              relax == (succeeds >= 3 ? 1 : 0) &&
              reports == (succeeds == 1 ? 0 : 1),
          "bounded retry counts");
    Check(order == (succeeds == 1   ? std::vector<int>{1}
                    : succeeds == 2 ? std::vector<int>{1, 2, 1}
                                    : std::vector<int>{1, 2, 1, 3, 1}),
          "trim/relax ordering");
    Check(result ==
              (succeeds <= 3 ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY),
          "retry final result");
  }
  for (bool zero : {false, true})
    for (int fail_at : {0, 1, 2}) {
      int calls = 0, destroyed = 0, attempts = 0;
      const auto plan = *B::CheckedExecutionLayout(zero ? 0 : 3, zero ? 0 : 1,
                                                   64, 64, true, 64);
      {
        auto prepared = B::PrepareBufferOwnership<Resource>(
            plan, [&](uint64_t bytes, bool uv) {
              Check(bytes == (uv ? 544 : 1840), "independent allocation sizes");
              const int allocation = ++calls;
              auto result = Detail::RetryBufferAllocation(
                  [&] {
                    ++attempts;
                    return allocation == fail_at ? VK_ERROR_OUT_OF_DEVICE_MEMORY
                                                 : VK_SUCCESS;
                  },
                  [] {}, [] {}, [](VkResult, bool) {}, VK_SUCCESS);
              return B::BufferAttempt<Resource>{
                  result == VK_SUCCESS
                      ? std::unique_ptr<Resource>(new Resource{&destroyed})
                      : nullptr,
                  result};
            });
        if (fail_at) {
          Check(!prepared && !prepared.geometry && !prepared.uv &&
                    prepared.error == VK_ERROR_OUT_OF_DEVICE_MEMORY,
                "failed transaction empty/precise error");
          Check(destroyed == (fail_at == 2 ? 1 : 0),
                "partial success released once before return");
        } else
          Check(bool(prepared) && prepared.geometry && prepared.uv &&
                    destroyed == 0,
                "success publishes complete ownership");
      }
      Check(destroyed == (fail_at == 0   ? 2
                          : fail_at == 2 ? 1
                                         : 0),
            "no double release");
      Check(attempts == (fail_at == 0   ? 2
                         : fail_at == 1 ? 3
                                        : 4),
            "factory uses shared retry path");
    }
  {
    int destroyed = 0, calls = 0;
    const auto plain = *B::CheckedExecutionLayout(0, 0, 64, 64, false, 64);
    {
      auto owned = B::PrepareBufferOwnership<Resource>(
          plain, [&](uint64_t bytes, bool uv) {
            ++calls;
            Check(bytes == 1840 && !uv,
                  "zero still allocates guarded geometry only");
            return B::BufferAttempt<Resource>{
                std::unique_ptr<Resource>(new Resource{&destroyed}),
                VK_SUCCESS};
          });
      Check(bool(owned) && !owned.uv && calls == 1, "UV-disabled success");
    }
    Check(destroyed == 1, "UV-disabled release once");
    auto unexpected =
        B::PrepareBufferOwnership<Resource>(plain, [&](uint64_t, bool) {
          return B::BufferAttempt<Resource>{
              std::unique_ptr<Resource>(new Resource{&destroyed}),
              VK_ERROR_OUT_OF_HOST_MEMORY};
        });
    Check(!unexpected && !unexpected.geometry &&
              unexpected.error == VK_ERROR_OUT_OF_HOST_MEMORY && destroyed == 2,
          "factory error with partial handle rolls back");
  }
  {
    int attempt = 0;
    const VkResult errors[] = {VK_ERROR_OUT_OF_DEVICE_MEMORY,
                               VK_ERROR_OUT_OF_HOST_MEMORY,
                               VK_ERROR_DEVICE_LOST};
    auto final =
        Detail::RetryBufferAllocation([&] { return errors[attempt++]; }, [] {},
                                      [] {}, [](VkResult, bool) {}, VK_SUCCESS);
    Check(final == VK_ERROR_DEVICE_LOST && attempt == 3,
          "precise final allocation error retained");
  }
  int calls = 0;
  auto bad = *B::CheckedExecutionLayout(3, 1, 64, 64, false, 64);
  bad.index_offset = UINT64_MAX;
  auto rejected = B::PrepareBufferOwnership<Resource>(bad, [&](uint64_t, bool) {
    ++calls;
    return B::BufferAttempt<Resource>{};
  });
  Check(!rejected && calls == 0 &&
            rejected.failure == B::BufferFailure::InvalidPlan,
        "invalid plan rejected before allocations");
  auto good = *B::CheckedExecutionLayout(3, 1, 64, 64, false, 64);
  auto null = B::PrepareBufferOwnership<Resource>(good, [](uint64_t, bool) {
    return B::BufferAttempt<Resource>{nullptr, VK_SUCCESS};
  });
  Check(!null && null.error == VK_ERROR_INITIALIZATION_FAILED,
        "success with null resource rejects");
  std::cout
      << (failures ? "FAIL" : "PASS")
      << ": shared retries, rollback and controlled recording retention (CPU "
         "injected allocation results; no VMA/GPU execution)\n";
  return failures ? 1 : 0;
}
