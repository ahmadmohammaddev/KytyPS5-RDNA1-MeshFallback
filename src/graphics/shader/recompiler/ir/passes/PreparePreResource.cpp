// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/shader/recompiler/ir/passes/PreparePreResource.h"

#include "graphics/shader/recompiler/ir/passes/ConstantPropagation.h"
#include "graphics/shader/recompiler/ir/passes/DeadCodeElimination.h"
#include "graphics/shader/recompiler/ir/passes/SsaRewrite.h"

namespace Libs::Graphics::ShaderRecompiler::IR {

ReadLaneStats NormalizePreResource(Program& program) {
	ConstantPropagationPass(program.blocks, program.wave_size);
	ResolveControlFlowIdentities(program);
	RemoveIdentities(program.blocks);
	EliminateDeadCode(program.blocks);
	// KYTY_FOLD_LANE_MASKS (default off): mask reads that fold can make select conditions and phis
	// identical, so fold until stable; before read-lane elimination, which then sees the folded
	// EXEC of a lane reduction (x || !x is every lane).
	for (int round = 0; round < 4 && FoldLaneMasks(program) != 0; round++) {
		ConstantPropagationPass(program.blocks, program.wave_size);
		ResolveControlFlowIdentities(program);
		RemoveIdentities(program.blocks);
		EliminateDeadCode(program.blocks);
	}
	const auto read_lane_stats = EliminateReadLane(program, program.wave_size);
	if (read_lane_stats.rewritten_reads != 0) {
		ConstantPropagationPass(program.blocks, program.wave_size);
		ResolveControlFlowIdentities(program);
		RemoveIdentities(program.blocks);
		EliminateDeadCode(program.blocks);
	}
	return read_lane_stats;
}

ReadLaneStats PreparePreResource(Program& program) {
	RewriteToSsa(program.blocks);
	return NormalizePreResource(program);
}

} // namespace Libs::Graphics::ShaderRecompiler::IR
