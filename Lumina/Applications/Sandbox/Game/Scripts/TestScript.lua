

local MyScript =
{

    CameraSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 5,
        Query = { "SCameraComponent", "STransformComponent", "SCharacterControllerComponent" },

        Init = function(Context)
        end,

        Shutdown = function(Context)
        end,

        Execute = function(Context, Entities, DeltaTime)
            for _, Entity in ipairs(Entities) do
                
                local CameraTransform = Context:Get(Entity, STransformComponent)

                local View = Context:View(SInputComponent, SCharacterControllerComponent, STransformComponent)
                View:Each(function(Player)
                
                    local PlayerTransform = Context:Get(Player, STransformComponent)

                    local PlayerLocation = PlayerTransform:GetLocation()

                    local CameraLocation = CameraTransform:SetLocation(PlayerTransform:GetLocation() + vec3(0, 25, -0.5))

                    local ForwardDirection = (PlayerLocation - CameraLocation):Normalize()
                    local LookAt = glm.QuatLookAt(ForwardDirection, vec3(0, 1, 0))
                    
                    CameraTransform:SetRotation(LookAt)

                    Context:DirtyTransform(Entity)

                end)

            end
        end
    },

    EnemySystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 15,
        Query = { "Enemy" },

        Init = function(Context)
        end,

        Shutdown = function(Context)
        end,

        Execute = function(Context, Entities, DeltaTime)
            for _, Enemy in ipairs(Entities) do    
                local View = Context:View(SInputComponent, STransformComponent, SCharacterControllerComponent)

                View:Each(function(Entity)

                    local EnemyTransform = Context:Get(Enemy, STransformComponent)
                    local PlayerTransform = Context:Get(Entity, STransformComponent)
                    local Controller = Context:Get(Enemy, SCharacterControllerComponent)

                    local Direction = (PlayerTransform:GetLocation() - EnemyTransform:GetLocation()):Normalize()
                    
                    Controller:AddMovementInput(vec2(Direction.x, Direction.z))

                    Context:DirtyTransform(Enemy, EMoveMode.MoveKinematic)

                end)
            end
        end
    },

    CoolSystem = System
    {
        Stage = UpdateStage.PrePhysics,
        Priority = 10,
        Query = { "SInputComponent", "SCharacterControllerComponent", "STransformComponent" },

        Init = function(Context)
        end,

        Shutdown = function(Context)
        end,

        Execute = function(Context, Entities, DeltaTime)            
            for _, Player in ipairs(Entities) do
                
                local PlayerTransform = Context:Get(Player, STransformComponent)
                local PlayerController = Context:Get(Player, SCharacterControllerComponent)
                local InputComponent = Context:Get(Player, SInputComponent)
                local PlayerLocation = PlayerTransform:GetLocation()

                if InputComponent:IsKeyDown(Input.EKeyCode.W) then
                    PlayerController:AddMovementInput(vec2(0, 1))
                end

                if InputComponent:IsKeyDown(Input.EKeyCode.S) then
                    PlayerController:AddMovementInput(vec2(0, -1))
                end

                if InputComponent:IsKeyDown(Input.EKeyCode.A) then
                    PlayerController:AddMovementInput(vec2(1, 0))
                end

                if InputComponent:IsKeyDown(Input.EKeyCode.D) then
                    PlayerController:AddMovementInput(vec2(-1, 0))
                end
                    
                if(InputComponent:IsKeyDown(Input.EKeyCode.Space)) then
                    PlayerController:Jump()
                end
                    

                if InputComponent:IsKeyDown(Input.EKeyCode.F) then
        
                    local PlayerForward = PlayerTransform:GetForward()
                    local RayDirection = PlayerForward

                    local RayStart = PlayerLocation + PlayerForward * 1.2
                    local RayEnd = RayStart + RayDirection * 15

                    local Result = Context:CastRay(RayStart, RayEnd, true, DeltaTime)
                    if Result then
                        if Player ~= Result.Entity then
                            if Context:IsValidEntity(Result.Entity) then

                                local Health = Context:Get(Result.Entity, SHealthComponent)
                                Health:ApplyDamage(0.25)

                                if Health.Health <= 0 then
                                    Context:Destroy(Result.Entity)
                                end
                            end
                        end
                    end
                end
            end
        end
    },
}

return MyScript