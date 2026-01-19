#include "pch.h"
#include "RenderContext.h"
#include "RenderResource.h"
#include "RHIGlobals.h"
#include "Shader.h"
#include "ShaderCompiler.h"
#include "Paths/Paths.h"

namespace Lumina
{
    void FShaderLibrary::CreateAndAddShader(const FName& Path, const FShaderHeader& Header, bool bReloadPipelines)
    {
        FRHIShaderRef Shader;
        
        switch (Header.Reflection.ShaderType)
        {
        case ERHIShaderType::None: break;
        case ERHIShaderType::Vertex:
            {
                Shader = GRenderContext->CreateVertexShader(Header);
            }
            break;
        case ERHIShaderType::Fragment:
            {
                Shader = GRenderContext->CreatePixelShader(Header);
            }
            break;
        case ERHIShaderType::Compute:
            {
                Shader = GRenderContext->CreateComputeShader(Header);
            }
            break;
        case ERHIShaderType::Geometry:
            {
                Shader = GRenderContext->CreateGeometryShader(Header);
            }
            break;
        }

        AddShader(Path, Shader);
        GRenderContext->OnShaderCompiled(Shader, false, bReloadPipelines);
    }

    void FShaderLibrary::AddShader(const FName& Path, FRHIShader* Shader)
    {
        FScopeLock Lock(Mutex);
        
        uint64 Hash = Path.GetID();
        for (const FString& Define : Shader->GetShaderHeader().Defines)
        {
            Hash::HashCombine(Hash, Define);
        }
        
        Shaders.insert_or_assign(Hash, Shader);
    }
    
    FRHIVertexShaderRef FShaderLibrary::GetVertexShader(const FName& Path, TSpan<FString> Macros)
    {
        return GRenderContext->GetShaderLibrary()->GetShader<FRHIVertexShader>(Path, Macros);
    }

    FRHIPixelShaderRef FShaderLibrary::GetPixelShader(const FName& Path, TSpan<FString> Macros)
    {
        return GRenderContext->GetShaderLibrary()->GetShader<FRHIPixelShader>(Path, Macros);
    }

    FRHIComputeShaderRef FShaderLibrary::GetComputeShader(const FName& Path, TSpan<FString> Macros)
    {
        return GRenderContext->GetShaderLibrary()->GetShader<FRHIComputeShader>(Path, Macros);
    }

    FRHIGeometryShaderRef FShaderLibrary::GetGeometryShader(const FName& Path, TSpan<FString> Macros)
    {
        return GRenderContext->GetShaderLibrary()->GetShader<FRHIGeometryShader>(Path, Macros);
    }

    FRHIShaderRef FShaderLibrary::GetShader(const FName& Path, TSpan<FString> Macros)
    {
        uint64 Hash = Path.GetID();
        for (const FString& Define : Macros)
        {
            Hash::HashCombine(Hash, Define);
        }
        
        auto It = Shaders.find(Hash);
        if (It == Shaders.end())
        {
            FString ShaderPath = Paths::GetEngineShadersDirectory() + "/" + Path.c_str();
            FShaderCompileOptions Options;
            Options.bGenerateReflectionData = true;
            Options.MacroDefinitions.assign(Macros.begin(), Macros.end());
            GRenderContext->GetShaderCompiler()->CompileShaderPath(ShaderPath, Options, [&](const FShaderHeader& Header)
            {
                CreateAndAddShader(Path, Header, true);
            });
            
            GRenderContext->GetShaderCompiler()->Flush();
            It = Shaders.find(Hash);
        }
        
        return It->second;
    }
}
