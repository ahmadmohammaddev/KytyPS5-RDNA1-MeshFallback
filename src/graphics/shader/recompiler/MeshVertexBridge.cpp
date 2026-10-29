// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/shader/recompiler/MeshVertexBridge.h"
#include <map>
#include <set>
#include <sstream>
#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.hpp>
namespace Libs::Graphics::ShaderRecompiler::MeshCompute {
BridgeResult BuildVertexBridge(std::span<const uint32_t> fragment,bool uv_available) {
  auto reject=[](const char* why){BridgeResult r;r.error=why;return r;};
  spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
  if(!tools.Validate(fragment.data(),fragment.size())) return reject("invalid fragment SPIR-V");
  struct Def {spv::Op op;std::vector<uint32_t> w;};
  std::map<uint32_t,Def> defs;std::map<uint32_t,std::vector<std::vector<uint32_t>>> decorations;
  std::vector<std::vector<uint32_t>> body;std::set<uint32_t> interfaces;
  unsigned entries=0;bool function=false;
  for(size_t at=5;at<fragment.size();at+=fragment[at]>>16) {
    const auto n=fragment[at]>>16;const auto op=spv::Op(fragment[at]&65535);
    std::vector<uint32_t> w(fragment.begin()+at,fragment.begin()+at+n);
    if(op==spv::OpEntryPoint) {
      if(++entries!=1 || w[1]!=spv::ExecutionModelFragment || w[3]!=0x6e69616d || w[4]!=0)
        return reject("require single Fragment main entry");
      interfaces.insert(w.begin()+5,w.end());
    }
    if(op==spv::OpDecorate)decorations[w[1]].push_back({w.begin()+2,w.end()});
    if(op==spv::OpTypeFloat || op==spv::OpTypeVector || op==spv::OpTypePointer)defs[w[1]]={op,w};
    if(op==spv::OpConstant || op==spv::OpVariable || op==spv::OpLoad || op==spv::OpAccessChain)defs[w[2]]={op,w};
    if(op==spv::OpFunction)function=true;
    if(function)body.push_back(w);
  }
  if(entries!=1)return reject("missing fragment entry");
  uint32_t parameter=0;
  for(auto id:interfaces) {
    if(!defs.contains(id))return reject("unsupported fragment interface");
    const auto& v=defs.at(id);
    if(v.op!=spv::OpVariable)return reject("unsupported interface declaration");
    if(v.w[3]!=spv::StorageClassInput)continue;
    if(parameter)return reject("multiple fragment parameters unsupported");
    if(decorations[id]!=std::vector<std::vector<uint32_t>>{{spv::DecorationLocation,0}})
      return reject("require smooth undecorated parameter0");
    if(!defs.contains(v.w[1]))return reject("missing input pointer type");
    const auto& p=defs.at(v.w[1]);
    if(p.op!=spv::OpTypePointer || !defs.contains(p.w[3]))return reject("unsupported input pointer type");
    const auto& vec=defs.at(p.w[3]);
    if(vec.op!=spv::OpTypeVector || vec.w[3]!=4 || !defs.contains(vec.w[2]))return reject("require float4 guest parameter");
    const auto& scalar=defs.at(vec.w[2]);
    if(scalar.op!=spv::OpTypeFloat || scalar.w[2]!=32)return reject("require float32 guest parameter");
    parameter=id;
  }
  if(parameter && !uv_available)return reject("fragment parameter requires UV export");
  // Track every direct interface use and every loaded float4 use. Reject rather
  // than infer that a whole vector consumer ignores undefined z/w components.
  std::set<uint32_t> vectors,scalar_ptrs;
  if(parameter)for(const auto& w:body) {
    const auto op=spv::Op(w[0]&65535);
    if(op==spv::OpLoad && w[3]==parameter) {vectors.insert(w[2]);continue;}
    if(op==spv::OpAccessChain && w[3]==parameter) {
      if(w.size()!=5 || !defs.contains(w[4]))return reject("dynamic parameter component unsupported");
      const auto& c=defs.at(w[4]);
      if(c.op!=spv::OpConstant || c.w.size()!=4 || c.w[3]>1)return reject("only parameter xy reads supported");
      scalar_ptrs.insert(w[2]);continue;
    }
    for(size_t i=1;i<w.size();++i)if(w[i]==parameter)return reject("unsupported parameter pointer use");
  }
  for(const auto& w:body) {
    const auto op=spv::Op(w[0]&65535);
    for(auto id:vectors)for(size_t i=1;i<w.size();++i)if(w[i]==id) {
      if(op==spv::OpLoad && i==2)continue;
      if(op!=spv::OpCompositeExtract || i!=3 || w.size()!=5 || w[4]>1)return reject("only parameter xy scalar extraction supported");
    }
    for(auto id:scalar_ptrs)for(size_t i=1;i<w.size();++i)if(w[i]==id) {
      if(op==spv::OpAccessChain && i==2)continue;
      if(op!=spv::OpLoad || i!=3)return reject("unsupported parameter scalar pointer use");
    }
  }
  const bool uv=parameter!=0;
  BridgeResult r;std::ostringstream a;
  a<<"OpCapability Shader\nOpMemoryModel Logical GLSL450\nOpEntryPoint Vertex %main \"main\" %position %out_position";
  if(uv)a<<" %uv %parameter";
  a<<"\nOpDecorate %position Location 0\nOpDecorate %out_position BuiltIn Position\n";
  if(uv)a<<"OpDecorate %uv Location 1\nOpDecorate %parameter Location 0\n";
  a<<"%void = OpTypeVoid\n%fn = OpTypeFunction %void\n%float = OpTypeFloat 32\n%v4 = OpTypeVector %float 4\n%in4 = OpTypePointer Input %v4\n%out4 = OpTypePointer Output %v4\n%position = OpVariable %in4 Input\n%out_position = OpVariable %out4 Output\n";
  if(uv)a<<"%v2 = OpTypeVector %float 2\n%in2 = OpTypePointer Input %v2\n%uv = OpVariable %in2 Input\n%parameter = OpVariable %out4 Output\n%zero = OpConstant %float 0\n";
  a<<"%main = OpFunction %void None %fn\n%entry = OpLabel\n%p = OpLoad %v4 %position\nOpStore %out_position %p\n";
  if(uv)a<<"%t = OpLoad %v2 %uv\n%x = OpCompositeExtract %float %t 0\n%y = OpCompositeExtract %float %t 1\n%out = OpCompositeConstruct %v4 %x %y %zero %zero\nOpStore %parameter %out\n";
  a<<"OpReturn\nOpFunctionEnd\n";r.assembly=a.str();
  if(!tools.Assemble(r.assembly,&r.binary) || !tools.Validate(r.binary))return reject("generated vertex bridge validation failed");
  r.interface_identity={1,0,0,0,uint8_t(uv),0,0,0};
  return r;
}
}
