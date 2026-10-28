#include "MeshIrComputeLowering.h"
#include "../MeshOutputContract.h"
#include <array>
#include <map>
#include <spirv-tools/libspirv.hpp>
#include <sstream>
#include <unordered_map>

namespace Kyty::MeshResearch {
namespace {
using Op = IR::ValueOpcode;
using Type = IR::Type;
struct Value {
  Type type = Type::Void;
  std::string id;
  std::array<std::array<uint32_t, 4>, 32> lanes{}; // coverage/safety proof only
};
struct Compiler {
  const Input &input;
  Result result;
  std::ostringstream declarations, body;
  uint32_t next_id = 0;
  std::map<std::pair<Type, uint32_t>, Value> constants;
  std::unordered_map<const IR::Inst *, Value> values;
  bool allocation = false, position = false, primitive = false;
  bool Fail(const std::string &why) {
    if (result.error.empty())
      result.error = why;
    return false;
  }
  std::string Id() { return "%r" + std::to_string(++next_id); }
  Value Constant(Type type, uint32_t bits) {
    auto key = std::pair(type, bits);
    if (auto it = constants.find(key); it != constants.end())
      return it->second;
    Value v;
    v.type = type;
    v.id = Id();
    for (auto &lane : v.lanes)
      lane[0] = bits;
    if (type == Type::U1)
      declarations << v.id << " = "
                   << (bits ? "OpConstantTrue" : "OpConstantFalse")
                   << " %bool\n";
    else
      declarations << v.id << " = OpConstant %uint " << bits << "\n";
    constants.emplace(key, v);
    return v;
  }
  std::string U(uint32_t n) { return Constant(Type::U32, n).id; }
  bool Argument(const IR::Inst &inst, size_t index, Type type, Value &v) {
    const auto arg = inst.Arg(index);
    if (!arg.IsImmediate()) {
      auto it = values.find(arg.TryInstruction());
      if (it == values.end())
        return Fail("forward, foreign, cyclic or unsupported SSA operand");
      v = it->second;
    } else {
      if (arg.GetType() == Type::U32)
        v = Constant(Type::U32, arg.U32());
      else if (arg.GetType() == Type::U1)
        v = Constant(Type::U1, arg.U1());
      else
        return Fail("only immediate U32/U1 operands supported");
    }
    return v.type == type || Fail("IR operand type mismatch");
  }
  std::string Emit(const std::string &op, const std::string &type,
                   const std::string &args) {
    auto id = Id();
    body << id << " = " << op << " " << type << " " << args << "\n";
    return id;
  }
  void Store(const std::string &index, const std::string &value) {
    auto ptr =
        Emit("OpAccessChain", "%ptr_uint", "%output " + U(0) + " " + index);
    body << "OpStore " << ptr << " " << value << "\n";
  }
  bool Export(const IR::Inst &inst) {
    if (!allocation)
      return Fail("export before uniform MeshAllocate");
    const auto flags = inst.Flags<IR::ExportFlags>();
    if (flags.index >= input.exports.size())
      return Fail("export metadata index out of range");
    const auto &exp = input.exports[flags.index];
    Value data, mask;
    if (!Argument(inst, 0, Type::U32x4, data) ||
        !Argument(inst, 1, Type::U1, mask))
      return false;
    if (exp.compr || exp.vm || exp.index != 0)
      return Fail("compressed/valid-mask/auxiliary export unsupported");
    bool pos = exp.kind == IR::ExportTargetKind::Position;
    if (pos) {
      if (position || exp.target != 0x0cu || exp.en != 15u)
        return Fail("require one full position-0 export");
      position = true;
    } else {
      if (primitive || exp.kind != IR::ExportTargetKind::Primitive ||
          exp.target != 0x14u || exp.en != 1u)
        return Fail(
            "require one packed triangle export; other outputs unsupported");
      primitive = true;
    }
    const auto count = pos ? result.vertices : result.primitives;
    // We cannot invent values for masked-off allocated outputs. Prove the
    // predicate covers every allocated lane; predicates outside it stay masked.
    for (uint32_t lane = 0; lane < count; ++lane) {
      if (!mask.lanes[lane][0])
        return Fail("masked export leaves an allocated output undefined");
      if (!pos) {
        auto packed = data.lanes[lane][0];
        if (packed & 0xc0000000u)
          return Fail("cull/null/reserved primitive bits unsupported");
        for (unsigned c = 0; c < 3; ++c)
          if (((packed >> (10 * c)) & 1023u) >= result.vertices)
            return Fail("primitive index outside allocated vertices");
      }
    }
    auto inside = Emit("OpULessThan", "%bool", "%lane " + U(count));
    auto active = Emit("OpLogicalAnd", "%bool", mask.id + " " + inside);
    auto then_label = Id(), merge = Id();
    body << "OpSelectionMerge " << merge << " None\nOpBranchConditional "
         << active << " " << then_label << " " << merge << "\n"
         << then_label << " = OpLabel\n";
    auto base = Emit("OpIMul", "%uint", "%lane " + U(pos ? 4 : 3));
    if (!pos)
      base = Emit("OpIAdd", "%uint",
                  base + " " + U(MeshOutput::kIndicesStartWord));
    auto packed = pos ? std::string()
                      : Emit("OpCompositeExtract", "%uint", data.id + " 0");
    for (unsigned c = 0; c < (pos ? 4u : 3u); ++c) {
      auto value = pos ? Emit("OpCompositeExtract", "%uint",
                              data.id + " " + std::to_string(c))
                       : Emit("OpBitFieldUExtract", "%uint",
                              packed + " " + U(10 * c) + " " + U(10));
      Store(Emit("OpIAdd", "%uint", base + " " + U(c)), value);
    }
    body << "OpBranch " << merge << "\n" << merge << " = OpLabel\n";
    return true;
  }
  bool Instruction(const IR::Inst &inst) {
    const auto op = inst.GetOpcode();
    if (op < Op{} || op >= Op::Count)
      return Fail("invalid IR opcode");
    if (inst.NumArgs() != IR::NumArgsOf(op))
      return Fail("IR arity mismatch");
    if (op != Op::SetAttribute && inst.Flags<uint64_t>() != 0)
      return Fail("nonzero instruction flags unsupported");
    if (op == Op::MeshAllocate) {
      const auto arg = inst.Arg(0);
      if (allocation || !arg.IsImmediate() || arg.GetType() != Type::U32)
        return Fail("require one immediate uniform MeshAllocate");
      auto packed = arg.U32();
      if (packed & ~(0x3ffu | (0x7ffu << 12)))
        return Fail("reserved allocation bits unsupported");
      result.vertices = packed & 1023u;
      result.primitives = (packed >> 12) & 2047u;
      if (!((result.vertices == 3 && result.primitives == 1) ||
            (result.vertices == 0 && result.primitives == 0)))
        return Fail(
            "only zero output or three vertices/one triangle supported");
      if (result.vertices > input.mesh.max_vertices ||
          result.primitives > input.mesh.max_primitives)
        return Fail("allocation exceeds declared output capacity");
      allocation = true;
      return true;
    }
    if (op == Op::SetAttribute)
      return Export(inst);
    Value v;
    v.id = Id();
    if (op == Op::GetAttribute) {
      const auto a = inst.Arg(0), b = inst.Arg(1);
      if (!a.IsImmediate() || !b.IsImmediate() || a.GetType() != Type::U32 ||
          b.GetType() != Type::U32 ||
          a.U32() !=
              static_cast<uint32_t>(IR::StageInputKind::LocalInvocationIndex) ||
          b.U32() != 0)
        return Fail("only local invocation index input supported; NGG draw "
                    "inputs need lowering");
      v.type = Type::U32;
      v.id = "%lane";
      for (uint32_t i = 0; i < 32; ++i)
        v.lanes[i][0] = i;
    } else if (op == Op::IAdd32) {
      Value a, b;
      if (!Argument(inst, 0, Type::U32, a) || !Argument(inst, 1, Type::U32, b))
        return false;
      v.type = Type::U32;
      body << v.id << " = OpIAdd %uint " << a.id << " " << b.id << "\n";
      for (size_t i = 0; i < 32; ++i)
        v.lanes[i][0] = a.lanes[i][0] + b.lanes[i][0];
    } else if (op == Op::CompositeConstructU32x4) {
      v.type = Type::U32x4;
      std::string args;
      for (size_t c = 0; c < 4; ++c) {
        Value arg;
        if (!Argument(inst, c, Type::U32, arg))
          return false;
        args += " " + arg.id;
        for (size_t i = 0; i < 32; ++i)
          v.lanes[i][c] = arg.lanes[i][0];
      }
      body << v.id << " = OpCompositeConstruct %uint4" << args << "\n";
    } else if (op == Op::SelectU32) {
      Value a, b, c;
      if (!Argument(inst, 0, Type::U1, a) || !Argument(inst, 1, Type::U32, b) ||
          !Argument(inst, 2, Type::U32, c))
        return false;
      v.type = Type::U32;
      body << v.id << " = OpSelect %uint " << a.id << " " << b.id << " " << c.id
           << "\n";
      for (size_t i = 0; i < 32; ++i)
        v.lanes[i][0] = a.lanes[i][0] ? b.lanes[i][0] : c.lanes[i][0];
    } else if (op == Op::IEqual32 || op == Op::ULessThan32 ||
               op == Op::LogicalAnd || op == Op::LogicalOr) {
      Value a, b;
      auto type =
          (op == Op::IEqual32 || op == Op::ULessThan32) ? Type::U32 : Type::U1;
      if (!Argument(inst, 0, type, a) || !Argument(inst, 1, type, b))
        return false;
      const char *name = op == Op::IEqual32      ? "OpIEqual"
                         : op == Op::ULessThan32 ? "OpULessThan"
                         : op == Op::LogicalAnd  ? "OpLogicalAnd"
                                                 : "OpLogicalOr";
      v.type = Type::U1;
      body << v.id << " = " << name << " %bool " << a.id << " " << b.id << "\n";
      for (size_t i = 0; i < 32; ++i) {
        auto x = a.lanes[i][0], y = b.lanes[i][0];
        v.lanes[i][0] = op == Op::IEqual32      ? x == y
                        : op == Op::ULessThan32 ? x < y
                        : op == Op::LogicalAnd  ? (x && y)
                                                : (x || y);
      }
    } else
      return Fail("unsupported IR opcode: " +
                  std::string(IR::ValueOpcodeName(op)));
    values.emplace(&inst, std::move(v));
    return true;
  }
  Result Run() {
    const auto &m = input.mesh;
    if (input.stage != Libs::Graphics::ShaderType::Mesh)
      Fail("input stage must be Mesh");
    else if (m.wave_size != 32 || m.host_subgroup_size != 32 ||
             m.threads_num[0] != 32 || m.threads_num[1] != 1 ||
             m.threads_num[2] != 1)
      Fail("only one wave32 workgroup supported");
    else if (m.lds_size_dwords || m.scratch_size_dwords || m.split_groups ||
             m.provoking_vertex)
      Fail("LDS/scratch/split groups/provoking-vertex state unsupported");
    else if (m.input_primitive !=
                 static_cast<uint32_t>(
                     Libs::Graphics::Prospero::PrimitiveType::kTriList) ||
             m.vertices_per_group != 3 || m.primitives_per_group != 1)
      Fail("only one triangle-list input group supported");
    else if (m.max_vertices == 0 || m.max_vertices > MeshOutput::kMaxVertices ||
             m.max_primitives == 0 ||
             m.max_primitives > MeshOutput::kMaxPrimitives)
      Fail("declared capacities outside guarded ABI");
    else if (input.blocks.size() != 1 || input.blocks[0] == nullptr)
      Fail("exactly one complete IR block required");
    if (!result.error.empty())
      return std::move(result);
    const auto &block = *input.blocks[0];
    if (!block.ImmPredecessors().empty() || !block.ImmSuccessors().empty())
      Fail("CFG edges/loops unsupported");
    if (block.Instructions().size() > 256)
      Fail("research IR instruction limit exceeded");
    body << "%main = OpFunction %void None %func\n%entry = OpLabel\n%lane = "
            "OpLoad %uint %local\n";
    if (result.error.empty())
      for (const auto &inst : block) {
        if (inst.Parent() != &block) {
          Fail("IR parent mismatch");
          break;
        }
        if (!Instruction(inst))
          break;
      }
    if (result.error.empty() && (!allocation || !position || !primitive))
      Fail("allocation/position/primitive export missing");
    if (!result.error.empty())
      return std::move(result);
    // Every invocation reaches this barrier: accepted IR has no CFG or early
    // return. UniformMemory applies to StorageBuffer. Scope=Workgroup,
    // AcquireRelease.
    body << "OpControlBarrier " << U(2) << " " << U(2) << " " << U(72) << "\n";
    auto first = Emit("OpIEqual", "%bool", "%lane " + U(0));
    auto commit = Id(), end = Id();
    body << "OpSelectionMerge " << end << " None\nOpBranchConditional " << first
         << " " << commit << " " << end << "\n"
         << commit << " = OpLabel\n";
    Store(U(MeshOutput::kCountsStartWord), U(result.vertices));
    Store(U(MeshOutput::kCountsStartWord + 1), U(result.primitives));
    Store(U(MeshOutput::kCountsStartWord + 2), U(0));
    Store(U(MeshOutput::kStatusWord), U(MeshOutput::kStatusOk));
    body << "OpBranch " << end << "\n"
         << end << " = OpLabel\nOpReturn\nOpFunctionEnd\n";
    const auto length = U(MeshOutput::kTotalWords);
    std::ostringstream module;
    module
        << "OpCapability Shader\nOpMemoryModel Logical GLSL450\n"
        << "OpEntryPoint GLCompute %main \"main\" %local "
           "%output\nOpExecutionMode %main LocalSize 32 1 1\n"
        << "OpDecorate %local BuiltIn LocalInvocationIndex\nOpDecorate %array "
           "ArrayStride 4\n"
        << "OpMemberDecorate %block 0 Offset 0\nOpDecorate %block Block\n"
        << "OpDecorate %output DescriptorSet 0\nOpDecorate %output Binding 1\n"
        << "%void = OpTypeVoid\n%bool = OpTypeBool\n%uint = OpTypeInt 32 "
           "0\n%uint4 = OpTypeVector %uint 4\n"
        << "%func = OpTypeFunction %void\n%ptr_in = OpTypePointer Input %uint\n"
        << "%ptr_uint = OpTypePointer StorageBuffer %uint\n"
        << declarations.str() << "%array = OpTypeArray %uint " << length
        << "\n%block = OpTypeStruct %array\n"
        << "%ptr_block = OpTypePointer StorageBuffer %block\n%local = "
           "OpVariable %ptr_in Input\n"
        << "%output = OpVariable %ptr_block StorageBuffer\n"
        << body.str();
    result.assembly = module.str();
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
    std::string message;
    tools.SetMessageConsumer([&](spv_message_level_t, const char *,
                                 const spv_position_t &,
                                 const char *text) { message += text; });
    if (!tools.Assemble(result.assembly, &result.binary) ||
        !tools.Validate(result.binary))
      Fail("generated SPIR-V rejected: " + message);
    if (!result.error.empty()) {
      result.binary.clear();
      result.assembly.clear();
    }
    return std::move(result);
  }
};
} // namespace
Result Lower(const Input &input) { return Compiler{input}.Run(); }
} // namespace Kyty::MeshResearch
