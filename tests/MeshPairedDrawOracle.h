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
// https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedconv
// Vulkan fixedconv permits floor/ceil, with exact integral results. Integer
// rational arithmetic avoids a blanket +/-1 allowance or sampled RX reference.
std::array<uint8_t,2> PairedUnormRange(uint32_t numerator,uint32_t denominator) {
  const auto scaled=std::min(numerator,denominator)*255;
  return {uint8_t(scaled/denominator),uint8_t((scaled+denominator-1)/denominator)};
}
size_t PairedMismatches(const uint8_t* pixels,bool draw,bool nearest_only=false) {
  size_t bad=0;
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    const auto* p=pixels+(y*64+x)*4;
    const auto expected=PairedPixel(x,y,draw);
    if(nearest_only || !draw || !TrianglePixel(x,y)) {
      bad+=std::memcmp(p,expected.data(),4)!=0;continue;
    }
    const auto red=PairedUnormRange(2*x-15,64),green=PairedUnormRange(8*y-61,252);
    bool wrong=false;
    for(unsigned c=0;c<4;++c) {
      const auto range=(c%2==0)?red:green;
      wrong|=p[c]<range[0] || p[c]>range[1];
    }
    bad+=wrong;
  }
  return bad;
}
// Explicit bounded-fixture admission budget, not a universal Vulkan precision
// guarantee: 2^-20 absolute (<1/4096 of a UNORM step, and <1/48 of
// this fixture's nearest-rounding margin). Reject devices exceeding it.
constexpr double kPairedFloatBudget=1.0/1048576.0;
// Unclamped mathematical interpolation, independently checked in all channels.
std::array<double,4> PairedFloatIdeal(uint32_t x,uint32_t y,bool draw) {
  if(!draw || !TrianglePixel(x,y))return {0,0,0,1};
  const double u=(2.0*x-15)/64, v=(8.0*y-61)/252;
  return {u,v,u,v};
}
struct PairedFloatStats {size_t covered=0,structural_errors=0,numeric_errors=0;double max_error=0;};
PairedFloatStats InspectPairedFloat(const float* values,bool draw) {
  PairedFloatStats stats;
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    const bool covered=draw&&TrianglePixel(x,y);stats.covered+=covered;
    const auto ideal=PairedFloatIdeal(x,y,draw);const auto* p=values+(y*64+x)*4;
    bool numeric_bad=false;
    bool bad=p[0]!=p[2] || (covered && p[1]!=p[3]);
    for(unsigned c=0;c<4;++c) {
      numeric_bad|=!std::isfinite(p[c]) || std::abs(double(p[c])-ideal[c])>kPairedFloatBudget;
      bad|=!std::isfinite(p[c]) || (!covered && p[c]!=ideal[c]);
      if(std::isfinite(p[c]))stats.max_error=std::max(stats.max_error,std::abs(double(p[c])-ideal[c]));
    }
    stats.structural_errors+=bad;stats.numeric_errors+=numeric_bad;
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
  pixels[(20*64+20)*4+3]=0;
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
  Expect(PairedUnormRange(0,64)==std::array<uint8_t,2>{0,0},"zero exact");
  Expect(PairedUnormRange(64,64)==std::array<uint8_t,2>{255,255},"one exact");
  Expect(PairedUnormRange(65,64)==std::array<uint8_t,2>{255,255},"saturation exact");
  Expect(PairedUnormRange(1,3)==std::array<uint8_t,2>{85,85},"interior integer exact");
  Expect(PairedUnormRange(1,2)==std::array<uint8_t,2>{127,128},"half bracket");
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    auto c=PairedPixel(x,y,true);
    if(TrianglePixel(x,y)) {
      const auto r=PairedUnormRange(2*x-15,64),g=PairedUnormRange(8*y-61,252);
      c={r[0],g[0],r[1],g[1]};
    }
    std::memcpy(pixels.data()+(y*64+x)*4,c.data(),4);
  }
  Expect(PairedMismatches(pixels.data(),true)==0,"both adjacent choices accepted all channels");
  pixels[(16*64+16)*4]=66;
  Expect(PairedMismatches(pixels.data(),true)==1,"below permitted bracket rejected");
  pixels[(16*64+16)*4]=70;
  Expect(PairedMismatches(pixels.data(),true)==1,"above permitted bracket rejected");
  pixels[3]=254;Expect(PairedMismatches(pixels.data(),true)==2,"clear alpha exact");
  for(uint32_t y=0;y<64;++y)for(uint32_t x=0;x<64;++x) {
    auto v=PairedFloatIdeal(x,y,true);
    for(unsigned c=0;c<4;++c)diagnostic[(y*64+x)*4+c]=float(v[c]);
  }
  const auto at=(20*64+21)*4;
  auto saved=diagnostic;
  std::swap(diagnostic[at],diagnostic[at+1]);std::swap(diagnostic[at+2],diagnostic[at+3]);
  Expect(InspectPairedFloat(diagnostic.data(),true).numeric_errors==1,"paired channel swap rejected numerically");
  diagnostic=saved;diagnostic[at]+=float(kPairedFloatBudget*2);diagnostic[at+2]=diagnostic[at];
  Expect(InspectPairedFloat(diagnostic.data(),true).numeric_errors==1,"coherent small perturbation rejected");
  diagnostic=saved;diagnostic[at]=diagnostic[at+2]=0;
  Expect(InspectPairedFloat(diagnostic.data(),true).numeric_errors==1,"missing pair rejected");
  diagnostic=saved;
  for(unsigned c=0;c<4;++c)diagnostic[at+c]=saved[at+4+c];
  Expect(InspectPairedFloat(diagnostic.data(),true).numeric_errors==1,"one-pixel spatial shift rejected");
  diagnostic=saved;diagnostic[at]=INFINITY;
  Expect(InspectPairedFloat(diagnostic.data(),true).numeric_errors==1,"infinity rejected numerically");
  diagnostic=saved;diagnostic[at]+=float(kPairedFloatBudget/4);diagnostic[at+2]=diagnostic[at];
  Expect(InspectPairedFloat(diagnostic.data(),true).numeric_errors==0,"sub-budget perturbation admitted");
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
