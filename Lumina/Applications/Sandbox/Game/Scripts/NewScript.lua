

local MyScript =
{
    CoolSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 100,
        Query = { "STransformComponent", "SStaticMeshComponent", "SRigidBodyComponent" },
        
        Execute = function(Context, Entities, DeltaTime)

            for _, Entity in ipairs(Entities) do
                local Transform, StaticMesh, RigidBody = Context:Get(Entity, "STransformComponent", "SStaticMeshComponent", "SRigidBodyComponent")

                local NumEntities = Context:GetNumEntities()
                local ColorAspect = Entity / NumEntities
                local Color = vec4(0.0, 0.0, 1.0, 1.0)


                if Entity % 3 == 0 then
                    Color.x = ColorAspect
                elseif Entity % 2 == 0 then
                    Color.y = ColorAspect
                end

                --Context:DrawDebugBox(Transform:GetLocation(), (StaticMesh:GetAABB():GetSize() * Transform:GetScale())* 0.5, Transform:GetRotation(), Color, 1.0, 1.0)
            end
        end,
    },

    OtherSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Query = { "SPointLightComponent" },

        Execute = function(Context, Entities, DeltaTime)

            for _, Entity in ipairs(Entities) do

                local Light = Context:Get(Entity, "SPointLightComponent")

                Light.LightColor = vec3.Lerp(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), math.sin(Context:GetTime()))

            end

        end
    }
}

return MyScript