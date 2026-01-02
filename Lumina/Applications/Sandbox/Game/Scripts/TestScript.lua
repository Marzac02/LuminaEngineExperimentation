

local MyScript =
{
    CoolSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 10,
        Query = { "SCameraComponent", "SInputComponent" },

        Init = function(Context)


        end,

        Shutdown = function(Context)

        end,

        Execute = function(Context, Entities, DeltaTime)

            for _, Entity in ipairs(Entities) do
                
                local InputComp, Transform = Context:Get(Entity, SInputComponent, STransformComponent)

                if InputComp:IsKeyDown(Input.EKeyCode.R) then
                
                    local To = Transform:GetLocation() + (Transform:GetForward() * 50)

                    local Result = Context:CastRay(Transform:GetLocation(), To, true, DeltaTime)
                    if Result then

                        local RaySettings           = FSphereCastSettings()
                        RaySettings.Start           = Result.Location
                        RaySettings.End             = Result.Location
                        RaySettings.Radius          = 10.0
                        RaySettings.bDrawDebug      = false
                        RaySettings.DebugDuration   = 0.1
                        
                        local SphereResult = Context:CastSphere(RaySettings)

                        for i = 1, #SphereResult do
                            local Hit = SphereResult[i]

                            local Direction = glm.Normalize(Hit.Location - Hit.End)

                            local Event = SImpulseEvent()
                            Event.BodyID = Hit.BodyID
                            Event.Impulse = Direction * 10000.0
                            Context:DispatchEvent(Event)
                            
                        end
                    end
                end
            end
        end
    },

}

return MyScript