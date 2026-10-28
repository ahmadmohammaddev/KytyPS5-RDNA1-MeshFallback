#include "MeshOutputContract.h"
#include "graphics/shader/recompiler/ir/IREmitter.h"
#include "research/MeshIrComputeLowering.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.hpp>
#include <stdexcept>

namespace R = Kyty::MeshResearch;
namespace IR = Libs::Graphics::ShaderRecompiler::IR;
using Op = IR::ValueOpcode;
int failures = 0;
void Check(bool ok, const char *why) {
  if (!ok) {
    ++failures;
    std::cerr << "FAIL: " << why << "\n";
  }
}
struct Fixture {
  IR::Block block;
  IR::IREmitter ir{&block};
  std::array<IR::Block *, 1> blocks{&block};
  std::array<IR::ExportInfo, 2> exports{};
  Libs::Graphics::ShaderMeshInputInfo mesh;
  IR::Inst *allocation = nullptr, *position = nullptr, *primitive = nullptr,
           *mask = nullptr, *lane = nullptr;
  Fixture(bool use_add = false, uint32_t addend = 0, uint32_t second_addend = 0, uint32_t index_addend = 0) {
    mesh.threads_num[0] = 32;
    mesh.threads_num[1] = mesh.threads_num[2] = 1;
    mesh.wave_size = mesh.host_subgroup_size = 32;
    mesh.input_primitive = static_cast<uint32_t>(
        Libs::Graphics::Prospero::PrimitiveType::kTriList);
    mesh.vertices_per_group = 3;
    mesh.primitives_per_group = 1;
    mesh.max_vertices = 64;
    mesh.max_primitives = 64;
    exports[0] = {
        .kind = IR::ExportTargetKind::Position, .target = 0x0c, .en = 15};
    exports[1] = {.kind = IR::ExportTargetKind::Primitive,
                  .target = 0x14,
                  .en = 1,
                  .done = true};
    allocation =
        &block.AppendNewInst(Op::MeshAllocate, {IR::Value(3u | (1u << 12))});
    auto local = ir.Emit(Op::GetAttribute,
                         {IR::Value(static_cast<uint32_t>(
                              IR::StageInputKind::LocalInvocationIndex)),
                          IR::Value(0u)});
    lane = local.TryInstruction();
    auto is0 = ir.Emit(Op::IEqual32, {local, IR::Value(0u)});
    auto is1 = ir.Emit(Op::IEqual32, {local, IR::Value(1u)});
    auto choose = [&](float a, float b, float c) {
      auto second =
          ir.Emit(Op::SelectU32, {is1, IR::Value(std::bit_cast<uint32_t>(b)),
                                  IR::Value(std::bit_cast<uint32_t>(c))});
      return ir.Emit(Op::SelectU32,
                     {is0, IR::Value(std::bit_cast<uint32_t>(a)), second});
    };
    auto x = choose(-.5f, .5f, -.5f), y = choose(-.5f, -.5f, .484375f);
    auto data = ir.Emit(Op::CompositeConstructU32x4,
                        {x, y, IR::Value(0u), IR::Value(0x3f800000u)});
    auto mask_lane = use_add ? ir.Emit(Op::IAdd32, {local, IR::Value(addend)}) : local;
    if (use_add && second_addend)
      mask_lane = ir.Emit(Op::IAdd32, {mask_lane, IR::Value(second_addend)});
    auto active = ir.Emit(Op::ULessThan32, {mask_lane, IR::Value(3u)});
    mask = active.TryInstruction();
    position = &block.AppendNewInst(
        Op::SetAttribute, {data, active},
        std::bit_cast<uint64_t>(IR::ExportFlags{.index = 0}));
    auto packed_value = IR::Value((1u << 10) | (2u << 20));
    if (index_addend)
      packed_value = ir.Emit(Op::IAdd32, {packed_value, IR::Value(index_addend)});
    auto packed = ir.Emit(Op::CompositeConstructU32x4,
                          {packed_value, IR::Value(0u),
                           IR::Value(0u), IR::Value(0u)});
    primitive = &block.AppendNewInst(
        Op::SetAttribute, {packed, is0},
        std::bit_cast<uint64_t>(IR::ExportFlags{.index = 1}));
  }
  R::Input Input() const {
    return {.mesh = mesh, .blocks = blocks, .exports = exports};
  }
};
// Independent binary structural checker: no reliance on compiler assembly IDs
// or its lane-coverage evaluator. Does not claim to execute the GPU module.
bool CheckAbi(const R::Result &r) {
  struct Def {
    spv::Op op;
    std::vector<uint32_t> args;
  };
  struct Store {
    uint32_t pointer, value, block;
    size_t offset;
  };
  std::map<uint32_t, uint32_t> constants;
  std::map<uint32_t, Def> defs;
  std::vector<Store> stores;
  uint32_t output = 0, local = 0, current_block = 0;
  std::map<uint32_t, uint32_t> guards;
  size_t barrier = 0, barrier_count = 0;
  std::vector<uint32_t> barrier_args;
  bool set_zero = false, array_stride = false, block_offset = false,
       array_length = false;
  if (r.binary.size() < 5)
    return false;
  for (size_t at = 5; at < r.binary.size();) {
    auto count = r.binary[at] >> 16;
    auto op = static_cast<spv::Op>(r.binary[at] & 0xffff);
    if (!count || at + count > r.binary.size())
      return false;
    const auto *w = r.binary.data() + at;
    switch (op) {
    case spv::OpConstant:
      if (count != 4)
        return false;
      constants[w[2]] = w[3];
      break;
    case spv::OpDecorate:
      if (count == 4 && w[2] == spv::DecorationBinding && w[3] == 1)
        output = w[1];
      if (count == 4 && w[2] == spv::DecorationBuiltIn &&
          w[3] == spv::BuiltInLocalInvocationIndex)
        local = w[1];
      if (count == 4 && w[2] == spv::DecorationArrayStride && w[3] == 4)
        array_stride = true;
      break;
    case spv::OpMemberDecorate:
      if (count == 5 && w[2] == 0 && w[3] == spv::DecorationOffset && w[4] == 0)
        block_offset = true;
      break;
    case spv::OpTypeArray:
      if (count == 4 && constants.contains(w[3]) && constants.at(w[3]) == 460)
        array_length = true;
      break;
    case spv::OpLabel:
      if (count != 2)
        return false;
      current_block = w[1];
      break;
    case spv::OpBranchConditional:
      if (count != 4)
        return false;
      guards[w[2]] = w[1];
      break;
    case spv::OpIEqual:
    case spv::OpULessThan:
    case spv::OpLogicalAnd:
    case spv::OpLoad:
    case spv::OpAccessChain:
    case spv::OpIMul:
    case spv::OpIAdd:
    case spv::OpCompositeExtract:
    case spv::OpBitFieldUExtract:
      if (count < 4)
        return false;
      defs[w[2]] = {op, std::vector<uint32_t>(w + 3, w + count)};
      break;
    case spv::OpStore:
      if (count != 3)
        return false;
      stores.push_back({w[1], w[2], current_block, at});
      break;
    case spv::OpControlBarrier:
      if (count != 4)
        return false;
      barrier = at;
      ++barrier_count;
      barrier_args.assign(w + 1, w + 4);
      break;
    default:
      break;
    }
    at += count;
  }
  auto c = [&](uint32_t id, uint32_t v) {
    return constants.contains(id) && constants.at(id) == v;
  };
  // The actual output, not an unrelated descriptor, must have set 0.
  for (size_t at = 5; at < r.binary.size();) {
    auto n = r.binary[at] >> 16;
    auto op = static_cast<spv::Op>(r.binary[at] & 0xffff);
    const auto *w = r.binary.data() + at;
    if (op == spv::OpDecorate && n == 4 && w[1] == output &&
        w[2] == spv::DecorationDescriptorSet && w[3] == 0)
      set_zero = true;
    at += n;
  }
  if (!output || !local || !set_zero || !array_stride || !array_length ||
      !block_offset || stores.size() != 11 || barrier_count != 1)
    return false;
  if (!c(barrier_args[0], 2) || !c(barrier_args[1], 2) ||
      !c(barrier_args[2], 72))
    return false;
  auto lane = [&](uint32_t id) {
    auto it = defs.find(id);
    return it != defs.end() && it->second.op == spv::OpLoad &&
           it->second.args == std::vector<uint32_t>{local};
  };
  auto guard = [&](const Store &store, bool commit, uint32_t count) {
    if (!guards.contains(store.block))
      return false;
    auto condition = defs.find(guards.at(store.block));
    if (condition == defs.end())
      return false;
    const auto &def = condition->second;
    if (commit)
      return def.op == spv::OpIEqual && def.args.size() == 2 &&
             lane(def.args[0]) && c(def.args[1], 0);
    if (def.op != spv::OpLogicalAnd || def.args.size() != 2)
      return false;
    for (auto id : def.args) {
      auto bound = defs.find(id);
      if (bound != defs.end() && bound->second.op == spv::OpULessThan &&
          bound->second.args.size() == 2 && lane(bound->second.args[0]) &&
          c(bound->second.args[1], count))
        return true;
    }
    return false;
  };
  std::map<uint32_t, uint32_t> metadata;
  std::set<uint32_t> position_components, primitive_components;
  for (const auto &store : stores) {
    auto ptr = defs.find(store.pointer);
    if (ptr == defs.end() || ptr->second.op != spv::OpAccessChain)
      return false;
    const auto &args = ptr->second.args;
    if (args.size() != 3 || args[0] != output || !c(args[1], 0))
      return false;
    if (constants.contains(args[2])) {
      if (!guard(store, true, 0) || store.offset < barrier ||
          !constants.contains(store.value) ||
          metadata.contains(constants.at(args[2])))
        return false;
      metadata[constants.at(args[2])] = constants.at(store.value);
      continue;
    }
    if (store.offset > barrier)
      return false;
    auto idx = defs.find(args[2]);
    if (idx == defs.end() || idx->second.op != spv::OpIAdd ||
        idx->second.args.size() != 2)
      return false;
    auto component_id = idx->second.args[1];
    if (!constants.contains(component_id))
      return false;
    auto component = constants.at(component_id);
    auto base = defs.find(idx->second.args[0]);
    if (base == defs.end())
      return false;
    auto val = defs.find(store.value);
    if (val == defs.end())
      return false;
    if (base->second.op == spv::OpIMul) {
      if (!guard(store, false, r.vertices))
        return false;
      const auto &a = base->second.args;
      if (a.size() != 2 || !lane(a[0]) || !c(a[1], 4) || component > 3 ||
          !position_components.insert(component).second)
        return false;
      if (val->second.op != spv::OpCompositeExtract ||
          val->second.args.size() != 2 || val->second.args[1] != component)
        return false;
    } else {
      if (!guard(store, false, r.primitives))
        return false;
      const auto &a = base->second.args;
      if (base->second.op != spv::OpIAdd || a.size() != 2 || !c(a[1], 258) ||
          component > 2 || !primitive_components.insert(component).second)
        return false;
      auto mul = defs.find(a[0]);
      if (mul == defs.end() || mul->second.op != spv::OpIMul ||
          mul->second.args.size() != 2 || !lane(mul->second.args[0]) ||
          !c(mul->second.args[1], 3))
        return false;
      if (val->second.op != spv::OpBitFieldUExtract ||
          val->second.args.size() != 3 ||
          !c(val->second.args[1], component * 10) ||
          !c(val->second.args[2], 10))
        return false;
      auto packed = defs.find(val->second.args[0]);
      if (packed == defs.end() ||
          packed->second.op != spv::OpCompositeExtract ||
          packed->second.args.size() != 2 || packed->second.args[1] != 0)
        return false;
    }
  }
  const std::map<uint32_t, uint32_t> expected = {
      {452, r.vertices}, {453, r.primitives}, {454, 0}, {457, 0}};
  if (metadata != expected || position_components.size() != 4 ||
      primitive_components.size() != 3)
    return false;
  const auto &last = defs.at(stores.back().pointer).args;
  return c(last[2], 457) && stores.back().offset > barrier;
}
void StructuralMutations(const R::Result &canonical,
                         const std::filesystem::path &dir) {
  if (!canonical)
    return;
  unsigned mutation_index = 0;
  auto mutate = [&](const char *label, std::string text) {
    R::Result changed;
    changed.vertices = canonical.vertices;
    changed.primitives = canonical.primitives;
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
    Check(tools.Assemble(text, &changed.binary) &&
              tools.Validate(changed.binary),
          "structural mutant remains valid SPIR-V");
    Check(!CheckAbi(changed), label);
    if (!dir.empty()) {
      std::filesystem::create_directories(dir);
      auto name = std::string("abi_mutant_") + std::to_string(++mutation_index);
      std::ofstream file(dir / (name + ".spv"), std::ios::binary);
      file.write(reinterpret_cast<const char *>(changed.binary.data()),
                 changed.binary.size() * 4);
      file.close();
      Check(!file.fail(), "mutant binary saved");
      std::ofstream assembly(dir / (name + ".spvasm"));
      assembly << text;
      assembly.close();
      Check(!assembly.fail(), "mutant assembly saved");
    }
  };
  auto text = canonical.assembly;
  auto at = text.rfind("OpStore ");
  text.erase(at, text.find('\n', at) - at + 1);
  mutate("missing final success store rejected", text);
  for (auto [from, to] :
       {std::pair{"OpConstant %uint 457\n", "OpConstant %uint 458\n"},
        std::pair{"OpConstant %uint 20\n", "OpConstant %uint 21\n"},
        std::pair{"OpConstant %uint 4\n", "OpConstant %uint 5\n"},
        std::pair{"OpConstant %uint 72\n", "OpConstant %uint 0\n"}}) {
    text = canonical.assembly;
    at = text.find(from);
    Check(at != std::string::npos, "mutation anchor found");
    if (at != std::string::npos)
      text.replace(at, std::strlen(from), to);
    mutate("ABI/barrier mutation rejected", text);
  }
  text = canonical.assembly;
  at = text.find(" = OpLogicalAnd %bool ");
  Check(at != std::string::npos, "bound conjunction mutation anchor");
  if (at != std::string::npos)
    text.replace(at, strlen(" = OpLogicalAnd %bool "), " = OpLogicalOr %bool ");
  mutate("lost bounds conjunction rejected", text);
  text = canonical.assembly;
  at = text.rfind(" = OpIEqual %bool %lane ");
  Check(at != std::string::npos, "single writer mutation anchor");
  if (at != std::string::npos)
    text.replace(at, strlen(" = OpIEqual %bool %lane "),
                 " = OpINotEqual %bool %lane ");
  mutate("nonzero-lane publication rejected", text);
}
void Valid(const R::Result &r) {
  Check(static_cast<bool>(r), r.error.c_str());
  if (!r)
    return;
  spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
  Check(tools.Validate(r.binary), "independent Vulkan 1.3 validator");
  Check(CheckAbi(r), "independent binary ABI/stores/barrier checker");
  Check(r.binary.size() > 5 && r.binary[1] == 0x00010600u, "SPIR-V 1.6 target");
  std::string text;
  Check(tools.Disassemble(r.binary, &text), "disassemble generated binary");
  for (auto token : {"OpEntryPoint GLCompute", "LocalSize 32 1 1",
                     "BuiltIn LocalInvocationIndex", "Binding 1", "OpSelect",
                     "OpBitFieldUExtract", "OpControlBarrier"})
    Check(text.find(token) != std::string::npos, token);
  for (auto forbidden : {"MeshShading", "mesh_shader", "OpSetMeshOutputsEXT",
                         "OpEntryPoint Mesh", "OpExtInstImport", "Subgroup"})
    Check(text.find(forbidden) == std::string::npos,
          "no mesh extension/subgroup/GLSL translator dependency");
  Check(r.assembly.find("OpControlBarrier") < r.assembly.rfind("OpStore"),
        "publication stores follow uniform barrier");
}
void Emit(const R::Result &r, const std::filesystem::path &dir,
          const std::string &name) {
  if (dir.empty() || !r)
    return;
  std::filesystem::create_directories(dir);
  std::ofstream file(dir / (name + ".spv"), std::ios::binary);
  file.write(reinterpret_cast<const char *>(r.binary.data()),
             r.binary.size() * 4);
  file.close();
  Check(!file.fail(), "SPIR-V artifact write");
  // Explicit auxiliary Vulkan 1.2 variant assembled from the same IR-derived
  // assembly; never substitute it for strict 1.3.
  spvtools::SpirvTools auxiliary(SPV_ENV_VULKAN_1_2);
  std::vector<uint32_t> binary12;
  const bool valid12 = auxiliary.Assemble(r.assembly, &binary12) && auxiliary.Validate(binary12);
  Check(valid12, "auxiliary IR-derived module validates Vulkan 1.2");
  if (valid12) {
    auto name12 = name;
    const auto suffix = name12.rfind("_vk13");
    Check(suffix != std::string::npos, "IR module name contains API suffix");
    if (suffix != std::string::npos) {
      name12.replace(suffix, 5, "_vk12");
      std::ofstream f12(dir / (name12 + ".spv"), std::ios::binary);
      f12.write(reinterpret_cast<const char*>(binary12.data()), binary12.size()*4);
      f12.close(); Check(!f12.fail(), "auxiliary IR module write");
    }
  }
  std::ofstream text(dir / (name + ".spvasm"));
  text << r.assembly;
  text.close();
  Check(!text.fail(), "assembly artifact write");
}
void Positives(const std::filesystem::path &dir) {
  Fixture f;
  auto input = f.Input();
  const auto canonical = R::Lower(input);
  Valid(canonical);
  Check(canonical.vertices == 3 && canonical.primitives == 1,
        "allocation fields decoded from actual IR");
  auto again = R::Lower(input);
  Check(canonical.binary == again.binary &&
            canonical.assembly == again.assembly,
        "deterministic lowering");
  Check(f.block.Instructions().size() == 13,
        "fixture instruction count unchanged by lowering");
  StructuralMutations(canonical, dir);
  Emit(canonical, dir, "mesh_ir_triangle_vk13");
  auto data = f.position->Arg(0).TryInstruction();
  const auto old = data->Arg(0);
  data->SetArg(0, IR::Value(0x3e800000u));
  const auto changed = R::Lower(input);
  Valid(changed);
  Check(changed.binary != canonical.binary,
        "emitted module depends on IR position data, not a fixed triangle "
        "template");
  data->SetArg(0, old);
  auto primdata = f.primitive->Arg(0).TryInstruction();
  primdata->SetArg(0, IR::Value(2u | (1u << 20)));
  const auto permuted = R::Lower(input);
  Valid(permuted);
  Check(permuted.binary != canonical.binary,
        "IR packed indices affect generated module");
  Emit(permuted, dir, "mesh_ir_permuted_vk13");
  f.allocation->SetArg(0, IR::Value(0u));
  const auto zero = R::Lower(input);
  Valid(zero);
  Check(zero.vertices == 0 && zero.primitives == 0 &&
            zero.binary != canonical.binary,
        "zero allocation emits different guarded module");
  Emit(zero, dir, "mesh_ir_zero_vk13");
  Fixture added(true, 0);
  auto add_result = R::Lower(added.Input());
  Valid(add_result);
  Check(add_result.binary != canonical.binary, "lane addition affects module");
  Fixture unsafe_add(true, 1);
  auto unsafe_result = R::Lower(unsafe_add.Input());
  Check(!unsafe_result && unsafe_result.binary.empty() && unsafe_result.error.find("undefined") != std::string::npos, "addition cannot bypass lane coverage proof");
  // Independent arithmetic expectations: unsigned 32-bit addition wraps.
  for (uint32_t lane = 0; lane < 32; ++lane) {
    const uint32_t sum = static_cast<uint32_t>(uint64_t(lane) + 0xffffffffull + 1ull);
    Check(sum == lane, "host wraparound oracle");
  }
  Fixture wrapping(true, 0xffffffffu, 1u);
  auto wrapping_result = R::Lower(wrapping.Input());
  Valid(wrapping_result);
  Check(wrapping_result.binary != canonical.binary, "wrap addition emits distinct SPIR-V");
  Fixture invalid_index(false, 0, 0, 3);
  auto bad_index = R::Lower(invalid_index.Input());
  Check(!bad_index && bad_index.binary.empty() && bad_index.error.find("index outside") != std::string::npos,
        "addition producing invalid packed index rejected");
  Fixture valid_index(false, 0, 0, 1);
  auto added_index = R::Lower(valid_index.Input());
  Valid(added_index);
  Emit(added_index, dir, "mesh_ir_add_vk13");
  Emit(wrapping_result, dir, "mesh_ir_wrap_vk13");
  Fixture unmasked;
  unmasked.position->SetArg(1, IR::Value(true));
  unmasked.primitive->SetArg(1, IR::Value(true));
  Valid(R::Lower(unmasked.Input()));
  // Expected HOST oracle for the later GPU experiment, never injected as GPU
  // output.
  if (!dir.empty()) {
    Kyty::MeshOutput::MeshOutputBlob expected;
    expected.Init();
    const float positions[12] = {-.5f, -.5f, 0,    1,        .5f, -.5f,
                                 0,    1,    -.5f, .484375f, 0,   1};
    std::array<uint32_t, 12> bits;
    std::memcpy(bits.data(), positions, sizeof(positions));
    const uint32_t indices[3] = {0, 1, 2};
    Kyty::MeshOutput::WriteOutput(expected, {3, bits.data(), 1, indices});
    std::ofstream file(dir / "mesh_ir_triangle_expected_host.bin",
                       std::ios::binary);
    file.write(reinterpret_cast<const char *>(expected.words.data()),
               Kyty::MeshOutput::kTotalBytes);
    file.close();
    Check(!file.fail(), "host oracle artifact write");
  }
}
using Mutate = std::function<void(Fixture &, R::Input &)>;
void Reject(const char *name, const char *diagnostic, const Mutate &mutation) {
  Fixture f;
  auto input = f.Input();
  mutation(f, input);
  auto result = R::Lower(input);
  Check(!result && result.binary.empty() && result.assembly.empty(), name);
  Check(result.error.find(diagnostic) != std::string::npos, name);
  std::cout << "reject " << name << ": " << result.error << "\n";
}
void Rejections() {
  Reject("wrong stage", "stage", [](auto &, auto &i) {
    i.stage = Libs::Graphics::ShaderType::Compute;
  });
  Reject("wave64", "wave32", [](auto &, auto &i) { i.mesh.wave_size = 64; });
  Reject("host subgroup mismatch", "wave32",
         [](auto &, auto &i) { i.mesh.host_subgroup_size = 64; });
  Reject("multiple waves", "wave32",
         [](auto &, auto &i) { i.mesh.threads_num[0] = 64; });
  Reject("LDS", "LDS", [](auto &, auto &i) { i.mesh.lds_size_dwords = 1; });
  Reject("scratch", "scratch",
         [](auto &, auto &i) { i.mesh.scratch_size_dwords = 1; });
  Reject("split group", "split",
         [](auto &, auto &i) { i.mesh.split_groups = 1; });
  Reject("wrong topology", "triangle-list",
         [](auto &, auto &i) { i.mesh.input_primitive = 0; });
  Reject("ABI capacity", "capacities",
         [](auto &, auto &i) { i.mesh.max_vertices = 65; });
  Reject("declared allocation overflow", "capacity",
         [](auto &, auto &i) { i.mesh.max_vertices = 2; });
  Reject("dynamic allocation", "immediate",
         [](auto &f, auto &) { f.allocation->SetArg(0, IR::Value(f.lane)); });
  Reject("reserved allocation bits", "reserved", [](auto &f, auto &) {
    f.allocation->SetArg(0, IR::Value(3u | (1u << 12) | (1u << 31)));
  });
  Reject("unsupported allocation size", "only zero", [](auto &f, auto &) {
    f.allocation->SetArg(0, IR::Value(4u | (1u << 12)));
  });
  Reject("missing block", "one complete",
         [](auto &, auto &i) { i.blocks = {}; });
  Reject("partial export", "full position",
         [](auto &f, auto &) { f.exports[0].en = 7; });
  Reject("compressed export", "compressed",
         [](auto &f, auto &) { f.exports[0].compr = true; });
  Reject("metadata missing", "metadata",
         [](auto &, auto &i) { i.exports = {}; });
  Reject("varying export", "packed triangle", [](auto &f, auto &) {
    f.exports[1].kind = IR::ExportTargetKind::Parameter;
  });
  Reject("masked-off allocated vertex", "undefined",
         [](auto &f, auto &) { f.mask->SetArg(1, IR::Value(2u)); });
  Reject("masked-off primitive", "undefined",
         [](auto &f, auto &) { f.primitive->SetArg(1, IR::Value(false)); });
  Reject("primitive cull flag", "cull", [](auto &f, auto &) {
    f.primitive->Arg(0).TryInstruction()->SetArg(0, IR::Value(0x80200400u));
  });
  Reject("invalid index", "index outside", [](auto &f, auto &) {
    f.primitive->Arg(0).TryInstruction()->SetArg(
        0, IR::Value(3u | (1u << 10) | (2u << 20)));
  });
  Reject("NGG draw parameter", "unsupported IR opcode", [](auto &f, auto &) {
    f.block.AppendNewInst(Op::MeshDrawParameter, {IR::Value(0u)});
  });
  Reject("unresolved EXEC", "unsupported IR opcode",
         [](auto &f, auto &) { f.block.AppendNewInst(Op::GetExec); });
  Reject("addition wrong type", "type mismatch",
         [](auto &f, auto &) {
           f.block.AppendNewInst(Op::IAdd32, {IR::Value(true), IR::Value(2u)});
         });
  Reject("input other than local lane", "only local", [](auto &f, auto &) {
    f.lane->SetArg(
        0, IR::Value(static_cast<uint32_t>(IR::StageInputKind::WorkgroupId)));
  });
  Reject("wrong scalar type", "type mismatch",
         [](auto &f, auto &) { f.mask->SetArg(1, IR::Value(true)); });
  Reject("unknown instruction flags", "instruction flags",
         [](auto &f, auto &) { f.mask->SetFlags(uint64_t{1}); });
  {
    Fixture f;
    IR::Block other;
    f.block.AddBranch(&other);
    auto r = R::Lower(f.Input());
    Check(!r && r.error.find("CFG") != std::string::npos, "CFG edge rejected");
  }
  {
    Fixture f;
    auto *data = f.position->Arg(0).TryInstruction();
    data->SetArg(0, IR::Value(data));
    auto r = R::Lower(f.Input());
    Check(!r && r.error.find("cyclic") != std::string::npos,
          "SSA cycle rejected without recursion");
  }
  {
    Fixture f;
    f.block.AppendNewInst(Op::MeshAllocate, {IR::Value(3u | (1u << 12))});
    auto r = R::Lower(f.Input());
    Check(!r && r.error.find("one immediate") != std::string::npos,
          "duplicate allocation rejected");
  }
}
int main(int argc, char **argv) {
  const std::string mode = argc > 1 ? argv[1] : "";
  if (mode == "--positive")
    Positives({});
  else if (mode == "--reject")
    Rejections();
  else if (mode == "--emit" && argc == 3)
    Positives(argv[2]);
  else {
    std::cerr << "usage: --positive | --reject | --emit DIRECTORY\n";
    return 1;
  }
  std::cout << "mesh IR compute offline: " << (failures ? "FAIL" : "PASS")
            << " (GPU NOT RUN)\n";
  return failures ? 1 : 0;
}
