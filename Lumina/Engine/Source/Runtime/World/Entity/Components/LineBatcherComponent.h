#pragma once
#include "RenderComponent.h"
#include "Renderer/Vertex.h"
#include <glm/gtx/quaternion.hpp>
#include <Renderer/ViewVolume.h>

namespace Lumina
{   
    struct LUMINA_API FLineBatcherComponent : SRenderComponent
    {
        struct FLineInstance
        {
            uint32 StartVertexIndex;
            float RemainingLifetime;
        };

        TVector<FSimpleElementVertex> Vertices;
        TVector<FLineInstance> Lines;

        void DrawLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness = 1.0f, float Duration = -1.0f)
        {
            if (Vertices.capacity() < Vertices.size() + 2)
            {
                Vertices.reserve(Vertices.capacity() * 2);
                Lines.reserve(Lines.capacity() * 2);
            }

            uint32 StartVertexIndex = static_cast<uint32>(Vertices.size());
            uint32 ColorPacked = PackColor(Color);

            Vertices.emplace_back(FSimpleElementVertex
            {
                .Position = Start,
                .Color    = ColorPacked,
            });

            Vertices.emplace_back(FSimpleElementVertex
            {
                .Position = End,
                .Color    = ColorPacked,
            });

            Lines.emplace_back(FLineInstance
            {
                .StartVertexIndex  = StartVertexIndex,
                .RemainingLifetime = Duration,
            });
        }
        
        void RemoveLine(int32 LineIndex)
        {
            const FLineInstance& Line = Lines[LineIndex];
            uint32 VertexIndex = Line.StartVertexIndex;

            Vertices.erase(Vertices.begin() + VertexIndex, Vertices.begin() + VertexIndex + 2);

            for (int32 i = LineIndex + 1; i < Lines.size(); ++i)
            {
                Lines[i].StartVertexIndex -= 2;
            }

            Lines.erase(Lines.begin() + LineIndex);
        }
    };
}
