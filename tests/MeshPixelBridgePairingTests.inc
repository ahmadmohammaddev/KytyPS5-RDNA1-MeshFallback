// SPDX-License-Identifier: GPL-2.0-only
// Included inside shaderCfgTests.cpp's existing anonymous namespace so these
// public ISA fixtures reuse the actual top-level production compiler helpers.
void TestMeshPixelBridgePairing() {
  namespace M=ShaderRecompiler::MeshCompute;
  auto compile=[&](uint32_t channel,uint32_t setting,bool linear,bool parameter=true) {
    std::vector<uint32_t> code;
    if(parameter) {
      code.push_back(EncodeVintrp(0,10,0,channel,4));
      code.push_back(EncodeVintrp(1,10,0,channel,4));
      code.push_back(EncodeVintrp(0,11,0,1,4));
      code.push_back(EncodeVintrp(1,11,0,1,4));
    }
    code.insert(code.end(),{EncodeExp0(0,15),EncodeExp1(10,11,10,11),0xbf810000u});
    ShaderPixelInputInfo info{};info.input_num=parameter?1:0;
    SetIdentityInterpolatorSettings(&info);info.interpolator_settings[0]=setting;
    info.ps_no_perspective=linear;info.target_output_mode[0]=4;
    auto options=MakeCompileOptions(ShaderType::Pixel);options.input_info.pixel=&info;
    auto result=RecompileForTest(code,options);
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
    Check(tools.Validate(result.spirv),"actual emitted pixel validates strict Vulkan1.3");
    Check(!result.spirv.empty(),"production compiler emitted pixel");
    return result;
  };
  for(bool parameter:{false,true}) {
    auto pixel=compile(0,0,false,parameter);
    auto bridge=M::BuildVertexBridge(pixel.spirv,parameter);
    if(!bridge)std::fprintf(stderr,"actual pixel bridge diagnostic: %s\n",bridge.error.c_str());
    Check(bool(bridge),"actual production pixel pairs with bridge");
    Check(bridge.interface_identity==std::vector<uint8_t>({1,0,0,0,uint8_t(parameter),0,0,0}),"canonical bridge interface matches actual pixel contract");
    Check(ProgramInputCount(pixel.program,ShaderRecompiler::IR::StageInputKind::Parameter)==unsigned(parameter),"actual live pixel parameter count");
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
    Check(tools.Validate(bridge.binary),"paired bridge validates strict Vulkan1.3");
    const auto ids=SpirvDecorationValueTargets(pixel.spirv,30,0);
    Check(!ids.empty(),"actual pixel exposes location0");
    if(parameter)Check(!M::BuildVertexBridge(pixel.spirv,false),"actual parameter requires UV export");
    if(const char* prefix=std::getenv("KYTY_MESH_PIXEL_BRIDGE_DUMP")) {
      for(const auto& pair:{std::pair{"pixel",&pixel.spirv},std::pair{"bridge",&bridge.binary}}) {
        const auto path=std::string(prefix)+(parameter?"-uv-":"-plain-")+pair.first+".spv";
        auto* file=std::fopen(path.c_str(),"wb");Check(file!=nullptr,"paired artifact opens");
        Check(std::fwrite(pair.second->data(),sizeof(uint32_t),pair.second->size(),file)==pair.second->size(),"paired artifact saved");std::fclose(file);
      }
    }
  }
  for(auto channel:{2u,3u}) {
    auto p=compile(channel,0,false);auto b=M::BuildVertexBridge(p.spirv,true);
    Check(!b && !b.error.empty() && b.binary.empty() && b.assembly.empty() && b.interface_identity.empty(),"actual z/w reader rejects empty output");
  }
  for(auto setting:{1u,0x400u}) {
    auto p=compile(0,setting,false);Check(!M::BuildVertexBridge(p.spirv,true),"actual remapped/flat input rejects");
  }
  auto p=compile(0,0,true);Check(!M::BuildVertexBridge(p.spirv,true),"actual no-perspective input rejects");
  std::printf("actual pixel compiler + vertex bridge: PASS (offline only)\n");
}
