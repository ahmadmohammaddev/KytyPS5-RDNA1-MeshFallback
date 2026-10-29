// Independent test-side SPIR-V inspection and finite scalar evaluation.
// Uses literal ABI expectations, never the lowerer's lane values or assembly IDs.
bool CheckUvBinary(const R::Result& result) {
  struct Def { spv::Op op; uint32_t type; std::vector<uint32_t> args; };
  struct Store { uint32_t pointer, value, block; size_t at; };
  std::map<uint32_t, Def> defs;
  std::map<uint32_t, uint32_t> constants, bindings, sets, strides, offsets, guards;
  std::set<uint32_t> blocks;
  std::vector<Store> stores;
  uint32_t local = 0, block = 0, uv = 0;
  size_t barrier = 0, barriers = 0;
  const auto& words = result.binary;
  if (words.size() < 5) return false;
  for (size_t at = 5; at < words.size();) {
    const auto n = words[at] >> 16;
    const auto op = static_cast<spv::Op>(words[at] & 65535);
    if (!n || at + n > words.size()) return false;
    const auto* w = words.data() + at;
    switch (op) {
    case spv::OpConstant:
      if (n != 4) return false;
      constants[w[2]] = w[3]; break;
    case spv::OpConstantTrue: case spv::OpConstantFalse:
      if (n != 3) return false;
      constants[w[2]] = op == spv::OpConstantTrue; break;
    case spv::OpDecorate:
      if (n == 3 && w[2] == spv::DecorationBlock) blocks.insert(w[1]);
      if (n == 4) {
        if (w[2] == spv::DecorationBinding) bindings[w[1]] = w[3];
        if (w[2] == spv::DecorationDescriptorSet) sets[w[1]] = w[3];
        if (w[2] == spv::DecorationArrayStride) strides[w[1]] = w[3];
        if (w[2] == spv::DecorationBuiltIn && w[3] == spv::BuiltInLocalInvocationIndex) local = w[1];
      }
      break;
    case spv::OpMemberDecorate:
      if (n == 5 && w[2] == 0 && w[3] == spv::DecorationOffset) offsets[w[1]] = w[4];
      break;
    case spv::OpTypeArray: case spv::OpTypeStruct: case spv::OpTypePointer:
      if (n < 3) return false;
      defs[w[1]] = {op, 0, {w + 2, w + n}}; break;
    case spv::OpVariable: case spv::OpLoad: case spv::OpIAdd: case spv::OpIMul:
    case spv::OpIEqual: case spv::OpULessThan: case spv::OpLogicalAnd:
    case spv::OpLogicalOr: case spv::OpSelect: case spv::OpCompositeConstruct:
    case spv::OpCompositeExtract: case spv::OpAccessChain:
      if (n < 4) return false;
      defs[w[2]] = {op, w[1], {w + 3, w + n}}; break;
    case spv::OpLabel:
      if (n != 2) return false;
      block = w[1]; break;
    case spv::OpBranchConditional:
      if (n != 4) return false;
      guards[w[2]] = w[1]; break;
    case spv::OpStore:
      if (n != 3) return false;
      stores.push_back({w[1], w[2], block, at}); break;
    case spv::OpControlBarrier: barrier = at; ++barriers; break;
    default: break;
    }
    at += n;
  }
  for (const auto& [id, binding] : bindings) {
    if (binding == 2) { if (uv) return false; uv = id; }
  }
  auto definition = [&](uint32_t id, spv::Op op) -> const Def* {
    auto it = defs.find(id);
    return it != defs.end() && it->second.op == op ? &it->second : nullptr;
  };
  if (!uv || !local || !sets.contains(uv) || sets.at(uv) != 0 || barriers != 1) return false;
  const auto* variable = definition(uv, spv::OpVariable);
  if (!variable || variable->args != std::vector<uint32_t>{spv::StorageClassStorageBuffer}) return false;
  const auto* ptr = definition(variable->type, spv::OpTypePointer);
  if (!ptr || ptr->args.size() != 2 || ptr->args[0] != spv::StorageClassStorageBuffer) return false;
  const auto* structure = definition(ptr->args[1], spv::OpTypeStruct);
  if (!structure || structure->args.size() != 1 || !blocks.contains(ptr->args[1]) ||
      !offsets.contains(ptr->args[1]) || offsets.at(ptr->args[1]) != 0) return false;
  const auto* array = definition(structure->args[0], spv::OpTypeArray);
  if (!array || array->args.size() != 2 || !constants.contains(array->args[1]) ||
      constants.at(array->args[1]) != 136 || !strides.contains(structure->args[0]) ||
      strides.at(structure->args[0]) != 4) return false;

  bool ok = true;
  std::function<uint32_t(uint32_t, uint32_t, unsigned)> eval;
  eval = [&](uint32_t id, uint32_t lane, unsigned depth) -> uint32_t {
    if (depth > 64) { ok = false; return 0; }
    if (constants.contains(id)) return constants.at(id);
    auto it = defs.find(id);
    if (it == defs.end()) { ok = false; return 0; }
    const auto& d = it->second; const auto& a = d.args;
    auto arg = [&](size_t i) { if (i >= a.size()) { ok = false; return 0u; } return eval(a[i], lane, depth + 1); };
    if (d.op == spv::OpLoad && a == std::vector<uint32_t>{local}) return lane;
    if (d.op == spv::OpCompositeExtract && a.size() == 2) {
      const auto* vector = definition(a[0], spv::OpCompositeConstruct);
      if (vector && a[1] < vector->args.size()) return eval(vector->args[a[1]], lane, depth + 1);
    }
    if (d.op == spv::OpSelect && a.size() == 3) return arg(0) ? arg(1) : arg(2);
    if (a.size() == 2) {
      const uint32_t x = arg(0), y = arg(1);
      if (d.op == spv::OpIAdd) return x + y;
      if (d.op == spv::OpIMul) return x * y;
      if (d.op == spv::OpIEqual) return x == y;
      if (d.op == spv::OpULessThan) return x < y;
      if (d.op == spv::OpLogicalAnd) return x && y;
      if (d.op == spv::OpLogicalOr) return x || y;
    }
    ok = false; return 0;
  };
  std::array<uint32_t, 136> actual, expected;
  actual.fill(0xA5A5A5A5u);
  for (auto i : {0, 1, 130, 131, 134, 135}) actual[i] = 0xC0DEFEEDu;
  actual[132] = 0; actual[133] = 0xFFFFFFFFu; expected = actual;
  const uint32_t bits[6] = {0x3E800000, 0x3E800000, 0x3FA00000, 0x3E800000, 0x3E800000, 0x3FA00000};
  if (result.vertices != 0 && result.vertices != 3) return false;
  for (size_t i = 0; i < result.vertices * 2; ++i) expected[2 + i] = bits[i];
  expected[132] = result.vertices; expected[133] = 0;
  std::array<unsigned, 136> writes{};
  unsigned uv_stores = 0;
  size_t count_write = 0, status_write = 0;
  for (const auto& store : stores) {
    const auto* p = definition(store.pointer, spv::OpAccessChain);
    if (!p || p->args.size() != 3) return false;
    if (p->args[0] != uv) continue;
    ++uv_stores;
    if (!constants.contains(p->args[1]) || constants.at(p->args[1]) != 0 || !guards.contains(store.block)) return false;
    const bool metadata = constants.contains(p->args[2]);
    if (metadata) {
      if (store.at < barrier || (constants.at(p->args[2]) != 132 && constants.at(p->args[2]) != 133)) return false;
      if (constants.at(p->args[2]) == 132) count_write = store.at;
      else status_write = store.at;
    } else {
      if (store.at > barrier) return false;
      const auto* active = definition(guards.at(store.block), spv::OpLogicalAnd);
      if (!active || active->args.size() != 2) return false;
      bool bounded = false;
      for (auto id : active->args) {
        const auto* bound = definition(id, spv::OpULessThan);
        if (!bound || bound->args.size() != 2) continue;
        const auto* load = definition(bound->args[0], spv::OpLoad);
        if (load && load->args == std::vector<uint32_t>{local} &&
            constants.contains(bound->args[1]) && constants.at(bound->args[1]) == result.vertices) bounded = true;
      }
      if (!bounded) return false;
    }
    for (uint32_t lane = 0; lane < 32; ++lane) {
      if (!eval(guards.at(store.block), lane, 0)) continue;
      if (metadata && lane != 0) return false;
      auto index = eval(p->args[2], lane, 0);
      if (index >= actual.size() || (!metadata && (index < 2 || index >= 130))) return false;
      actual[index] = eval(store.value, lane, 0); ++writes[index];
    }
  }
  if (!ok || uv_stores != 4 || actual != expected || count_write >= status_write) return false;
  for (size_t i = 0; i < writes.size(); ++i) {
    const unsigned wanted = (i >= 2 && i < 2 + result.vertices * 2) || i == 132 || i == 133;
    if (writes[i] != wanted) return false;
  }
  return true;
}
