// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/shader/recompiler/MeshVertexBridge.h"
#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.hpp>
#include <iostream>
#include <map>
#include <fstream>
namespace M=Libs::Graphics::ShaderRecompiler::MeshCompute;
int failures=0;
void Check(bool b,const char* why){if(!b){++failures;std::cerr<<"FAIL: "<<why<<"\n";}}
std::vector<uint32_t> Fragment(std::string decorate="",unsigned component=0,bool whole=false,bool scalar=false,bool input=true,unsigned width=4) {
 std::string s="OpCapability Shader\nOpMemoryModel Logical GLSL450\nOpEntryPoint Fragment %main \"main\" %color";
 if(input)s+=" %parameter";
 s+="\nOpExecutionMode %main OriginUpperLeft\nOpDecorate %color Location 0\n";
 if(input)s+="OpDecorate %parameter Location 0\n"+decorate;
 s+="%void = OpTypeVoid\n%fn = OpTypeFunction %void\n%float = OpTypeFloat 32\n%v4 = OpTypeVector %float 4\n%out4 = OpTypePointer Output %v4\n%color = OpVariable %out4 Output\n%zero = OpConstant %float 0\n";
 if(input){if(width!=4)s+="%iv = OpTypeVector %float "+std::to_string(width)+"\n";s+="%in4 = OpTypePointer Input "+std::string(width==4?"%v4":"%iv")+"\n%parameter = OpVariable %in4 Input\n";}
 if(scalar)s+="%uint = OpTypeInt 32 0\n%index = OpConstant %uint "+std::to_string(component)+"\n%ptr = OpTypePointer Input %float\n";
 s+="%main = OpFunction %void None %fn\n%entry = OpLabel\n";
 if(input) {
  if(scalar)s+="%p = OpAccessChain %ptr %parameter %index\n%x = OpLoad %float %p\n";
  else{s+="%v = OpLoad "+std::string(width==4?"%v4":"%iv")+" %parameter\n";if(!whole)s+="%x = OpCompositeExtract %float %v "+std::to_string(component)+"\n";}
 }
 if(whole)s+="OpStore %color %v\n";
 else {s+="%c = OpCompositeConstruct %v4 ";s+=input?"%x %zero %zero %zero\n":"%zero %zero %zero %zero\n";s+="OpStore %color %c\n";}
 s+="OpReturn\nOpFunctionEnd\n";
 spvtools::SpirvTools t(SPV_ENV_VULKAN_1_3);t.SetMessageConsumer([](spv_message_level_t,const char*,const spv_position_t&,const char* m){std::cerr<<m<<"\n";});std::vector<uint32_t> words;
 Check(t.Assemble(s,&words) && t.Validate(words),"test fragment itself validates");return words;
}
// Independent binary dataflow check: position store is its input load, UV
// output is x/y from the UV input load plus two literal float zero components.
bool Oracle(const M::BridgeResult& r,bool uv) {
 struct D{spv::Op op;std::vector<uint32_t>w;};std::map<uint32_t,D>d;std::map<uint32_t,uint32_t>loc,builtin;std::map<uint32_t,uint32_t>stores;
 for(size_t at=5;at<r.binary.size();at+=r.binary[at]>>16){auto n=r.binary[at]>>16;auto op=spv::Op(r.binary[at]&65535);std::vector<uint32_t>w(r.binary.begin()+at,r.binary.begin()+at+n);
  if(op==spv::OpDecorate){if(w[2]==spv::DecorationLocation)loc[w[1]]=w[3];if(w[2]==spv::DecorationBuiltIn)builtin[w[1]]=w[3];}
  if(op==spv::OpVariable || op==spv::OpLoad || op==spv::OpConstant || op==spv::OpCompositeExtract || op==spv::OpCompositeConstruct)d[w[2]]={op,w};
  if(op==spv::OpStore)stores[w[1]]=w[2];
 }
 unsigned pos=0,param=0;
 for(auto [target,value]:stores){auto it=d.find(value);if(it==d.end())return false;auto def=it->second;
  if(builtin.contains(target) && builtin[target]==spv::BuiltInPosition){if(def.op!=spv::OpLoad || !loc.contains(def.w[3]) || loc[def.w[3]]!=0)return false;++pos;}
  else{
   if(!uv || !loc.contains(target) || loc[target]!=0 || def.op!=spv::OpCompositeConstruct || def.w.size()!=7)return false;
   for(unsigned i=0;i<4;++i){auto p=d.find(def.w[3+i]);if(p==d.end())return false;const auto& v=p->second;
    if(i<2){if(v.op!=spv::OpCompositeExtract || v.w[4]!=i)return false;auto load=d.find(v.w[3]);if(load==d.end() || load->second.op!=spv::OpLoad || !loc.contains(load->second.w[3]) || loc[load->second.w[3]]!=1)return false;}
    else if(v.op!=spv::OpConstant || v.w[3]!=0)return false;
   }++param;
  }
 }
 return pos==1 && param==unsigned(uv) && stores.size()==(uv?2u:1u);
}
int main(int argc,char** argv){
 for(bool uv:{false,true}){auto f=Fragment("",0,false,false,uv);auto r=M::BuildVertexBridge(f,uv);Check(bool(r),"bridge builds");Check(Oracle(r,uv),"independent bridge dataflow");Check(r.binary==M::BuildVertexBridge(f,uv).binary,"deterministic bridge");
  if(uv){auto bad=r;for(size_t at=5;at<bad.binary.size();at+=bad.binary[at]>>16)if((bad.binary[at]&65535)==spv::OpConstant)bad.binary[at+3]=0x3f800000;spvtools::SpirvTools t(SPV_ENV_VULKAN_1_3);Check(t.Validate(bad.binary) && !Oracle(bad,true),"valid wrong padding mutant detected");}
  if(argc==2){std::ofstream out(std::string(argv[1])+(uv?"-uv.spv":"-plain.spv"),std::ios::binary);out.write(reinterpret_cast<const char*>(r.binary.data()),r.binary.size()*4);Check(bool(out),"bridge artifact saved");}
 }
 for(bool scalar:{false,true})for(unsigned c=0;c<4;++c){auto r=M::BuildVertexBridge(Fragment("",c,false,scalar),true);Check(bool(r)==(c<2),"only xy reads admitted");}
 for(auto dec:{"OpDecorate %parameter Flat\n","OpDecorate %parameter NoPerspective\n","OpDecorate %parameter Centroid\n","OpDecorate %parameter Component 0\n"}){auto r=M::BuildVertexBridge(Fragment(dec),true);Check(!r && !r.error.empty() && r.binary.empty() && r.assembly.empty() && r.interface_identity.empty(),"unsupported decoration rejects empty");}
 Check(!M::BuildVertexBridge(Fragment(),false),"missing UV export rejects");
 Check(!M::BuildVertexBridge(Fragment("",0,true),true),"whole parameter use rejects");
 Check(!M::BuildVertexBridge(Fragment("",0,false,false,true,2),true),"float2 test interface is not native float4");
 Check(!M::BuildVertexBridge({},true),"malformed binary rejects");
 std::cout<<"vertex bridge: "<<(failures?"FAIL":"PASS")<<" (offline only)\n";return failures?1:0;
}
