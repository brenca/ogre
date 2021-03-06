/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "Ogre.h"
#include "RootWithoutRenderSystemFixture.h"
#include "OgreShaderGenerator.h"
#include "OgreShaderProgramManager.h"

#include "OgreShaderFFPTransform.h"
#include "OgreShaderFFPColour.h"

#include "OgreShaderFunctionAtom.h"

using namespace Ogre;

struct RTShaderSystem : public RootWithoutRenderSystemFixture
{
    std::unique_ptr<GpuProgramManager> gpuProgMgr;

    void SetUp()
    {
        RootWithoutRenderSystemFixture::SetUp();
        gpuProgMgr.reset(new GpuProgramManager());

        RTShader::ShaderGenerator::initialize();
        RTShader::ShaderGenerator::getSingleton().setTargetLanguage("glsl");
    }
    void TearDown()
    {
        RTShader::ShaderGenerator::destroy();
        gpuProgMgr.reset();
        RootWithoutRenderSystemFixture::TearDown();
    }
};

TEST_F(RTShaderSystem, createShaderBasedTechnique)
{
    auto& shaderGen = RTShader::ShaderGenerator::getSingleton();
    auto mat = MaterialManager::getSingleton().create("TestMat", RGN_DEFAULT);

    EXPECT_TRUE(shaderGen.createShaderBasedTechnique(mat->getTechniques()[0], "MyScheme"));

    EXPECT_EQ(mat->getTechniques().size(), size_t(1));
    shaderGen.validateMaterial("MyScheme", mat->getName(), mat->getGroup());
    EXPECT_EQ(mat->getTechniques().size(), size_t(2));

    auto newTech = mat->getTechniques()[1];

    EXPECT_EQ(newTech->getSchemeName(), "MyScheme");
    EXPECT_TRUE(newTech->getPasses()[0]->hasGpuProgram(GPT_VERTEX_PROGRAM));
    EXPECT_TRUE(newTech->getPasses()[0]->hasGpuProgram(GPT_FRAGMENT_PROGRAM));

    EXPECT_TRUE(shaderGen.removeShaderBasedTechnique(mat->getTechniques()[0], "MyScheme"));
}

TEST_F(RTShaderSystem, MaterialSerializer)
{
    auto& shaderGen = RTShader::ShaderGenerator::getSingleton();
    auto mat = MaterialManager::getSingleton().create("TestMat", RGN_DEFAULT);

    shaderGen.createShaderBasedTechnique(mat->getTechniques()[0], "MyScheme");

    auto rstate = shaderGen.getRenderState("MyScheme", "TestMat", RGN_DEFAULT, 0);
    rstate->addTemplateSubRenderState(shaderGen.createSubRenderState<RTShader::FFPColour>());

    shaderGen.validateMaterial("MyScheme", mat->getName(), mat->getGroup());

    MaterialSerializer ser;
    ser.addListener(shaderGen.getMaterialSerializerListener());
    ser.queueForExport(mat);
    EXPECT_TRUE(ser.getQueuedAsString().find("colour_stage") != String::npos);
}

TEST_F(RTShaderSystem, TargetRenderState)
{
    auto mat = MaterialManager::getSingleton().create("TestMat", RGN_DEFAULT);
    auto pass = mat->getTechniques()[0]->getPasses()[0];

    using namespace RTShader;
    auto& shaderGen = ShaderGenerator::getSingleton();

    TargetRenderState targetRenderState;
    targetRenderState.addSubRenderStateInstance(shaderGen.createSubRenderState<FFPTransform>());
    targetRenderState.addSubRenderStateInstance(shaderGen.createSubRenderState<FFPColour>());

    targetRenderState.acquirePrograms(pass);

    EXPECT_TRUE(pass->hasGpuProgram(GPT_VERTEX_PROGRAM));
    EXPECT_TRUE(pass->hasGpuProgram(GPT_FRAGMENT_PROGRAM));
}

TEST_F(RTShaderSystem, FunctionInvocationOrder)
{
    using namespace RTShader;

    FunctionInvocation a("name", 0);
    FunctionInvocation b("name", 0);
    FunctionInvocation c("name", 0);

    a.pushOperand(ParameterFactory::createConstParam(Vector3()), Operand::OPS_IN);
    b.pushOperand(ParameterFactory::createConstParam(Vector3()), Operand::OPS_IN, Operand::OPM_XY);
    c.pushOperand(ParameterFactory::createConstParam(Vector3()), Operand::OPS_IN, Operand::OPM_XYZ);

    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b < a);

    EXPECT_TRUE(c == a);
    EXPECT_FALSE(c < a);
}
