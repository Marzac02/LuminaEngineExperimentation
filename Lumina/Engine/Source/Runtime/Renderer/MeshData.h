#pragma once

#include "RenderResource.h"
#include "Containers/Array.h"
#include "Core/Serialization/Archiver.h"
#include "Core/Utils/NonCopyable.h"
#include "Renderer/Vertex.h"

namespace Lumina
{
    struct FGeometrySurface final
    {
        FName   ID;
        uint32  IndexCount = 0;
        uint32  StartIndex = 0;
        int16   MaterialIndex = -1;

        friend FArchive& operator << (FArchive& Ar, FGeometrySurface& Data)
        {
            Ar << Data.ID;
            Ar << Data.StartIndex;
            Ar << Data.IndexCount;
            Ar << Data.MaterialIndex;

            return Ar;
        }
        
    };

    struct LUMINA_API FMeshResource : INonCopyable
    {
        struct FMeshBuffers
        {
            FRHIBufferRef VertexBuffer;
            FRHIBufferRef IndexBuffer;
            FRHIBufferRef ShadowIndexBuffer;
        };
        
        FName                       Name;
        TVector<FVertex>            Vertices;
        TVector<uint32>             Indices;
        TVector<uint32>             ShadowIndices;
        TVector<FGeometrySurface>   GeometrySurfaces;
        FMeshBuffers                MeshBuffers;
        
        FORCEINLINE SIZE_T GetNumSurfaces() const { return GeometrySurfaces.size(); }
        
        FORCEINLINE bool IsSurfaceIndexValid(SIZE_T Slot) const
        {
            return Slot < GetNumSurfaces();
        }
        
        FORCEINLINE const FGeometrySurface& GetSurface(SIZE_T Slot) const
        {
            return GeometrySurfaces[Slot];
        }
        
        friend FArchive& operator << (FArchive& Ar, FMeshResource& Data)
        {
            Ar << Data.Name;
            Ar << Data.Vertices;
            Ar << Data.Indices;
            Ar << Data.ShadowIndices;
            Ar << Data.GeometrySurfaces;

            return Ar;
        }
    };
}
