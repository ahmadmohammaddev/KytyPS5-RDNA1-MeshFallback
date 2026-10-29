// SPDX-License-Identifier: GPL-2.0-only
#pragma once

#include "graphics/shader/recompiler/ir/ShaderIR.h"
#include "graphics/shader/recompiler/ir/passes/ReadLaneElimination.h"

namespace Libs::Graphics::ShaderRecompiler::IR {

// The shared normalization prefix every in-tree consumer runs between frontend
// translation and resource tracking: SSA rewrite plus ordinary constant
// propagation, control-flow identity removal, dead-code elimination, optional
// lane-mask folding and read-lane elimination with its follow-up passes.
// Compute-adapter consumers must call this function instead of keeping a
// drifting hand-copied sequence of the same passes. Returns the read-lane
// elimination statistics so the caller can issue its own logging.
ReadLaneStats PreparePreResource(Program& program);
// SSA-preserving follow-up after specializing already prepared IR.
ReadLaneStats NormalizePreResource(Program& program);

} // namespace Libs::Graphics::ShaderRecompiler::IR
