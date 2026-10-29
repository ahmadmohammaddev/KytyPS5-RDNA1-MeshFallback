// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "common/uniqueFunction.h"
#include <cstdint>

#include <new>
#include <utility>

namespace Libs::Graphics::MeshFallback {
// Admission MUST be before any resource-dependent commands. Rejected work is
// still unsubmitted and is cleaned immediately. Accepted owner is held by the
// completion callback, never by the short-lived view or producer stack.
// Admit(tick, callback&) must move only on acceptance and must never run inline.
// A throwing admission must preserve caller ownership (strong guarantee).
// Completion must cover ALL last uses, on ONE recording/queue. This API cannot
// extend a lease to another recording/queue or make an arbitrary fence safe.
template <class Owner, class Admit>
bool RetainBeforeUse(uint64_t tick, Owner&& resources, Admit admit) {
	auto local = std::move(resources);
	if (!local) return false;
	try {
		Common::UniqueFunction<void> cleanup([owned = std::move(local)] {});
		return admit(tick, cleanup);
	} catch (const std::bad_alloc&) {
		// No GPU references exist yet, so cleanup on allocation failure is safe.
		return false;
	}
}
} // namespace Libs::Graphics::MeshFallback
