// SPDX-License-Identifier: GPL-2.0-only
// Independent integer pixel oracle for native public EXP(x,y,x,y) output.
std::array<uint8_t,4> PairedPixel(uint32_t x,uint32_t y,bool draw) {
  if(!draw || !TrianglePixel(x,y))return {0,0,0,255};
  // From screen-space triangle (16,16),(48,16),(16,47.5), UV .25..1.25.
  // Quantization has no half ties: odd numerator/64 and (8*y-61)/252.
  auto red=std::min(255u,((2*x-15)*255+32)/64);
  auto green=std::min(255u,((8*y-61)*255+126)/252);
  return {uint8_t(red),uint8_t(green),uint8_t(red),uint8_t(green)};
}
size_t PairedMismatches(const uint8_t* pixels,bool draw) {
  size_t bad=0;
  for(uint32_t y=0;y<64;++y) {
    for(uint32_t x=0;x<64;++x) {
      const auto expected=PairedPixel(x,y,draw);
      if(std::memcmp(pixels+(y*64+x)*4,expected.data(),4))++bad;
    }
  }
  return bad;
}
// Diagnostic only: unclamped mathematical interpolation before UNORM conversion.
// No tolerance here can close the strict paired RGBA8 acceptance gate.
std::array<double,4> PairedFloatIdeal(uint32_t x,uint32_t y,bool draw) {
  if(!draw || !TrianglePixel(x,y))return {0,0,0,1};
  const double u=(2.0*x-15)/64, v=(8.0*y-61)/252;
  return {u,v,u,v};
}
struct PairedFloatStats {size_t covered=0,structural_errors=0;double max_error=0;};
PairedFloatStats InspectPairedFloat(const float* values,bool draw) {
  PairedFloatStats stats;
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    const bool covered=draw&&TrianglePixel(x,y);stats.covered+=covered;
    const auto ideal=PairedFloatIdeal(x,y,draw);const auto* p=values+(y*64+x)*4;
    bool bad=p[0]!=p[2] || (covered && p[1]!=p[3]);
    for(unsigned c=0;c<4;++c) {
      bad|=!std::isfinite(p[c]) || (!covered && p[c]!=ideal[c]);
      if(std::isfinite(p[c]))stats.max_error=std::max(stats.max_error,std::abs(double(p[c])-ideal[c]));
    }
    stats.structural_errors+=bad;
  }
  return stats;
}
int CheckPairedOracle() {
  std::vector<uint8_t> pixels(kDrawBytes);
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    auto c=PairedPixel(x,y,true);std::memcpy(pixels.data()+(y*64+x)*4,c.data(),4);
  }
  Expect(PairedPixel(16,16,true)==std::array<uint8_t,4>{68,68,68,68},"independent native pixel corner");
  Expect(PairedPixel(40,16,true)==std::array<uint8_t,4>{255,68,255,68},"native pixel saturation");
  Expect(PairedMismatches(pixels.data(),true)==0,"paired full reference accepted");
  Expect(PairedMismatches(pixels.data(),false)==496,"missing native draw rejected");
  pixels[(20*64+20)*4+3]^=1;
  Expect(PairedMismatches(pixels.data(),true)==1,"native alpha corruption rejected");
  std::vector<float> diagnostic(64*64*4);
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    const auto ideal=PairedFloatIdeal(x,y,true);
    for(unsigned c=0;c<4;++c)diagnostic[(y*64+x)*4+c]=float(ideal[c]);
  }
  auto stats=InspectPairedFloat(diagnostic.data(),true);
  Expect(stats.covered==496 && stats.structural_errors==0 && stats.max_error<1e-7,"diagnostic ideal reference statistics");
  Expect(PairedFloatIdeal(40,16,true)[0]>1,"diagnostic keeps unclamped values");
  diagnostic[0]=1;Expect(InspectPairedFloat(diagnostic.data(),true).structural_errors==1,"diagnostic clear corruption detected");diagnostic[0]=0;
  diagnostic[(20*64+20)*4]=NAN;Expect(InspectPairedFloat(diagnostic.data(),true).structural_errors==1,"diagnostic NaN detected");
  diagnostic[(20*64+20)*4]=0;Expect(InspectPairedFloat(diagnostic.data(),true).structural_errors==1,"diagnostic channel corruption detected");
  PrintLine1("paired oracle: %s (integer RGBA, 496 covered pixels)",g_failures?"FAIL":"PASS");
  return g_failures?1:0;
}
std::filesystem::path PairedModulePath(bool api13,const char* stage) {
  return g_shader_directory/(std::string("mesh_paired_uv_")+stage+(api13?"_vk13.spv":"_vk12.spv"));
}
bool ValidatePairedShaders(bool api13) {
  for(auto stage:{"vert","frag"}) {
    auto words=LoadSpirvFile(PairedModulePath(api13,stage),"paired-generated");
    if(!ValidateSpirv(words,api13?SPV_ENV_VULKAN_1_3:SPV_ENV_VULKAN_1_2,"paired-generated"))return false;
    spvtools::SpirvTools tools(api13?SPV_ENV_VULKAN_1_3:SPV_ENV_VULKAN_1_2);std::string text;
    if(!tools.Disassemble(words,&text) || text.find(std::string("OpEntryPoint ")+(std::strcmp(stage,"vert")==0?"Vertex":"Fragment"))==std::string::npos){Expect(false,"paired stage model");return false;}
  }return true;
}
