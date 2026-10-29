#include "research/MeshIrComputeLowering.h"
#include "research/MeshIrFrontendSpecialization.h"
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
#include <spirv-tools/libspirv.hpp>

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
constexpr uint32_t Compare(uint32_t value) {
  return (0x3eu<<25u) | (0xc2u<<17u) | (5u<<9u) | (128u+value);
}
constexpr uint32_t Select(uint32_t dst,uint32_t src0,uint32_t src1) {
  return (1u<<25u) | (dst<<17u) | (src1<<9u) | src0;
}
std::vector<uint32_t> Fixture(bool consumes_prologue) {
  std::vector<uint32_t> code = {SMov(124,255),0x1003,0xbf900009};
  if (consumes_prologue) {
    // v5 is the actual NGG vertex ID. Select raw float bits for a triangle.
    const uint32_t positions[]={VMov(6,255),0xbf000000,VMov(7,255),0x3f000000,
      Compare(1),Select(0,262,7),VMov(8,255),0x3ef80000,Compare(2),Select(1,262,8)};
    code.insert(code.end(),std::begin(positions),std::end(positions));
  }
  else { code.insert(code.end(),{VMov(0,255),0xbf000000,VMov(1,255),0xbf000000}); }
  const uint32_t tail[] = {
    VMov(2,128), VMov(3,255),0x3f800000,
    Exp(0x0c,15,false),0x03020100, // position v0/v1/v2/v3
    VMov(4,255),0x00200400, Exp(0x14,1,true),0x04040404,0xbf810000
  };
  code.insert(code.end(),std::begin(tail),std::end(tail));
  return code;
}
void Simplify(IR::Program& program) {
  // Exact production pre-resource normalization prefix in ShaderRecompiler.cpp.
  // No test-side rewriting, register substitution, block selection or flattening.
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
  IR::RewriteToSsa(program.blocks);
  Simplify(program);
  std::map<std::string,unsigned> inventory;
  for(const auto* block:program.blocks)
    for(const auto& inst:*block) ++inventory[std::string(IR::ValueOpcodeName(inst.GetOpcode()))];
  Check(program.blocks.size()==2,"preparation retains complete block graph");
  Check(inventory["MeshAllocate"]==1 && inventory["SetAttribute"]==2,"allocation and exports survive normal preparation");
  if(consumes_prologue) Check(inventory["MeshDrawParameter"]>0,"used prologue values survive ordinary DCE");
  R::Input view{.stage=program.stage,.mesh=mesh,.blocks=program.blocks,.exports=program.export_info,.block_info=program.block_info};
  const auto prepared=IR::ProgramToString(program);
  const auto unbounded=R::Lower(view);
  if(consumes_prologue) {
    Check(!unbounded && unbounded.binary.empty(), "unspecialized frontend fails closed");
    const auto before=IR::ProgramToString(program);
    std::string error;
    for(int which=0;which<9;++which) {
      R::NonIndexedDraw bad;
      switch(which) {
      case 0:bad.count=4;break;case 1:bad.index_bytes=4;break;
      case 2:bad.index_low=1;break;case 3:bad.index_high=1;break;
      case 4:bad.group_count_x=2;break;case 5:bad.group_count_y=2;break;
      case 6:bad.group_count_z=2;break;case 7:bad.first_group=1;break;
      case 8:bad.group_count_x=0;break;
      }
      Check(!R::SpecializeNonIndexedDraw(program,bad,error) && !error.empty(), "unsupported draw context rejected");
      Check(IR::ProgramToString(program)==before, "rejection leaves full input unchanged");
    }
    auto& invalid=program.blocks.back()->AppendNewInst(IR::ValueOpcode::MeshDrawParameter,{IR::Value(7u)});
    const auto malformed=IR::ProgramToString(program);
    Check(!R::SpecializeNonIndexedDraw(program,{},error), "unknown draw parameter rejects");
    Check(IR::ProgramToString(program)==malformed, "late malformed parameter causes no partial specialization");
    invalid.Invalidate();
    Simplify(program);
    Check(R::SpecializeNonIndexedDraw(program,{},error), "explicit bounded draw specializes");
    Simplify(program);
    for(auto* block:program.blocks) for(const auto& inst:*block)
      Check(inst.GetOpcode()!=IR::ValueOpcode::LoadAddressU32 &&
            inst.GetOpcode()!=IR::ValueOpcode::GetAddressResource &&
            inst.GetOpcode()!=IR::ValueOpcode::MeshDrawParameter,
            "ordinary propagation/DCE removed inactive indexed path");
  }
  const auto result=R::Lower(view);
  {
    Check(static_cast<bool>(result), "complete frontend program lowers");
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
    Check(tools.Validate(result.binary), "frontend SPIR-V validates Vulkan 1.3");
    Check(result.vertices==3 && result.primitives==1, "frontend allocation preserved");
  }
  std::cout << name << ": frontend PASS, complete blocks=" << program.blocks.size()
            << ", compute compatibility " << (result ? "PASS" : "UNSUPPORTED: "+result.error) << '\n';
  std::cout << "  pre-specialization inventory:\n";
  for(const auto& [op,count]:inventory) std::cout << "  " << op << '=' << count << '\n';
  if(!out.empty()) {
    std::filesystem::create_directories(out);
    std::ofstream(out/(std::string(name)+"-raw.ir")) << raw;
    std::ofstream(out/(std::string(name)+"-prepared.ir")) << prepared;
    std::ofstream(out/(std::string(name)+"-specialized.ir")) << IR::ProgramToString(program);
    if(result) {
      std::ofstream binary(out/(std::string(name)+".spv"),std::ios::binary);
      binary.write(reinterpret_cast<const char*>(result.binary.data()),result.binary.size()*4);
      binary.close();Check(!binary.fail(), "frontend SPIR-V artifact saved");
      std::ofstream(out/(std::string(name)+".spvasm")) << result.assembly;
    }
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
