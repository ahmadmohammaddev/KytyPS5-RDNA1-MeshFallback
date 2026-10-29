// SPDX-License-Identifier: GPL-2.0-only
#pragma once
namespace Libs::Graphics::Detail {
// Shared by the legacy fatal constructor and recoverable creation. No global
// test hooks: callers supply per-operation callbacks. Trim once, then allow an
// over-budget attempt; preserve the original retry order and final error.
template <class Create, class Release, class Relax, class Report, class Result>
Result RetryBufferAllocation(Create create, Release release, Relax relax, Report report,
                             Result success) {
	auto result = create();
	if (result == success) return result;
	release();
	result           = create();
	bool over_budget = false;
	if (result != success) {
		relax();
		result      = create();
		over_budget = result == success;
	}
	report(result, over_budget);
	return result;
}
} // namespace Libs::Graphics::Detail
