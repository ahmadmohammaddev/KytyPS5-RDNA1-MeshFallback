#include "research/MeshIrComputeLowering.h"
#include "graphics/shader/recompiler/frontend/cfg/ShaderCFG.h"
#include "graphics/shader/recompiler/frontend/decode/ShaderDecoder.h"
#include "graphics/shader/recompiler/frontend/translate/Translate.h"
#include "graphics/shader/recompiler/ir/passes/SsaRewrite.h"
#include "graphics/shader/recompiler/ir/passes/ConstantPropagation.h"
#include "graphics/shader/recompiler/ir/passes/DeadCodeElimination.h"
#include "graphics/shader/recompiler/ir/passes/ReadLaneElimination.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

namespace S = Libs::Graphics::ShaderRecompiler;
namespace R = Kyty::MeshResearch;
namespace IR = S::IR;
int failures = 0;
void Check(bool value, const char* text) {
  if (!value) { ++failures; std::cerr << "FAIL: " << text << '\n'; }
}
// Public authored ISA, using the encoding fields already exercised by
// shaderCfgTests.cpp. No captured shader or game instructions are used.
constexpr uint32_t SMov(uint32_t dst, uint32_t src) {
  return 0xbe800300u | (dst << 16u) | src;
}
constexpr uint32_t VMov(uint32_t dst, uint32_t src) {
  return 0x7e000200u | (dst << 17u) | src;
}
constexpr uint32_t Exp(uint32_t target, uint32_t mask, bool done) {
  return 0xf8000000u | (target << 4u) | mask | (done ? 0x800u : 0u);
}
std::vector<uint32_t> Fixture(bool consumes_prologue) {
  std::vector<uint32_t> code = {SMov(124,255),0x1003,0xbf900009};
  if (consumes_prologue) code.push_back(VMov(0,261)); // v5 = prologue vertex ID
  else { code.push_back(VMov(0,255)); code.push_back(0xbf000000); }
  const uint32_t tail[] = {
    VMov(1,255),0xbf000000, VMov(2,128), VMov(3,255),0x3f800000,
    Exp(0x0c,15,false),0x03020100, // position v0/v1/v2/v3
    VMov(4,255),0x00200400, Exp(0x14,1,true),0x04040404,0xbf810000
  };
  code.insert(code.end(),std::begin(tail),std::end(tail));
  return code;
}
void Prepare(IR::Program& program) {
  // Exact production pre-resource normalization prefix in ShaderRecompiler.cpp.
  // No test-side rewriting, register substitution, block selection or flattening.
  IR::RewriteToSsa(program.blocks);
  IR::ConstantPropagationPass(program.blocks,program.wave_size);
  IR::ResolveControlFlowIdentities(program);
  IR::RemoveIdentities(program.blocks);
  IR::EliminateDeadCode(program.blocks);
  for (int round=0;round<4 && IR::FoldLaneMasks(program)!=0;++round) {
    IR::ConstantPropagationPass(program.blocks,program.wave_size);
    IR::ResolveControlFlowIdentities(program);
    IR::RemoveIdentities(program.blocks);
    IR::EliminateDeadCode(program.blocks);
  }
  if (IR::EliminateReadLane(program,program.wave_size).rewritten_reads) {
    IR::ConstantPropagationPass(program.blocks,program.wave_size);
    IR::ResolveControlFlowIdentities(program);
    IR::RemoveIdentities(program.blocks);
    IR::EliminateDeadCode(program.blocks);
  }
  IR::ValidateProgram(program,true);
}
void Test(bool consumes_prologue,const std::filesystem::path& out) {
  const auto name=consumes_prologue ? "vertex-id" : "constant";
  auto code=Fixture(consumes_prologue);
  S::Decoder::Program decoded;
  S::Decoder::DecodeProgram(code,decoded);
  for (const auto& inst:decoded.instructions)
    Check(inst.opcode!=S::Decoder::Opcode::UNSUPPORTED,"authored ISA fully decodes");
  auto cfg=S::CFG::BuildGraph(decoded);
  Libs::Graphics::ShaderVertexInputInfo input{};
  auto& mesh=input.mesh;
  mesh.threads_num[0]=32; mesh.threads_num[1]=mesh.threads_num[2]=1;
  mesh.wave_size=mesh.host_subgroup_size=32;
  mesh.input_primitive=static_cast<uint32_t>(Libs::Graphics::Prospero::PrimitiveType::kTriList);
  mesh.vertices_per_group=3;mesh.primitives_per_group=1;
  mesh.max_vertices=mesh.max_primitives=64;
  S::Frontend::TranslateOptions options{};
  options.stage=Libs::Graphics::ShaderType::Mesh;options.wave_size=32;
  options.user_data_count=0;options.input_info.vertex=&input;
  auto program=S::Frontend::TranslateProgram(decoded,cfg,options);
  const auto raw=IR::ProgramToString(program);
  Check(program.blocks.size()==2,"real frontend includes entry prologue and body");
  Check(raw.find("MeshDrawParameter")!=std::string::npos,"normal NGG prologue present before preparation");
  Prepare(program);
  std::map<std::string,unsigned> inventory;
  for(const auto* block:program.blocks)
    for(const auto& inst:*block) ++inventory[std::string(IR::ValueOpcodeName(inst.GetOpcode()))];
  Check(program.blocks.size()==2,"preparation retains complete block graph");
  Check(inventory["MeshAllocate"]==1 && inventory["SetAttribute"]==2,"allocation and exports survive normal preparation");
  if(consumes_prologue) Check(inventory["MeshDrawParameter"]>0,"used prologue values survive ordinary DCE");
  R::Input view{.stage=program.stage,.mesh=mesh,.blocks=program.blocks,.exports=program.export_info};
  const auto result=R::Lower(view);
  Check(!result && result.binary.empty() && result.assembly.empty(),"unsupported whole frontend program fails closed");
  Check(result.error=="exactly one complete IR block required","first compatibility gap is structural, not hidden by selecting body");
  std::cout << name << ": frontend PASS, complete blocks=" << program.blocks.size()
            << ", compute compatibility UNSUPPORTED: " << result.error << '\n';
  for(const auto& [op,count]:inventory) std::cout << "  " << op << '=' << count << '\n';
  if(!out.empty()) {
    std::filesystem::create_directories(out);
    std::ofstream(out/(std::string(name)+"-raw.ir")) << raw;
    std::ofstream(out/(std::string(name)+"-prepared.ir")) << IR::ProgramToString(program);
  }
}
int main(int argc,char** argv) {
  std::filesystem::path out;
  if(argc==3 && std::string(argv[1])=="--dump") out=argv[2];
  else if(argc!=1) return 1;
  Test(false,out);Test(true,out);
  std::cout << "frontend compatibility regression: " << (failures ? "FAIL" : "PASS") << " (GPU NOT RUN)\n";
  return failures ? 1 : 0;
}
