Script = {

    PlayerSystem = {
        Type = "System",
        Stage = UpdateStage.PrePhysics

    },
    CameraSystem = {
        Type = "System",
        Stage = UpdateStage.PostPhysics
    },
    TestSystem = {
        Type = "System",
        Stage = UpdateStage.PostPhysics
    }
}

Script.TestSystem.Update = function(Ctx)

    Ctx:View(STransformComponent):Each(function(Entity)
        local Transform = Ctx:Get(Entity, STransformComponent)
        
    
    end)

end

Script.PlayerSystem.Update = function(Ctx)

    local View = Ctx:View(SCharacterControllerComponent, SInputComponent)
    View:Each(function(Entity)

        local Controller = Ctx:Get(Entity, SCharacterControllerComponent)
        local Input = Ctx:Get(Entity, SInputComponent)

        if Input:IsKeyDown(EKeyCode.W) then
            Controller:AddMovementInput(vec3(0.0, 0.0, 1.0))
        end

        if Input:IsKeyDown(EKeyCode.S) then
            Controller:AddMovementInput(vec3(0.0, 0.0, -1.0))
        end

        if Input:IsKeyDown(EKeyCode.A) then
            Controller:AddMovementInput(vec3(1.0, 0.0, 0.0))
        end

        if Input:IsKeyDown(EKeyCode.D) then
            Controller:AddMovementInput(vec3(-1.0, 0.0, 0.0))
        end

        if Input:IsKeyDown(EKeyCode.Space) then

            local Transform = Ctx:Get(Entity, STransformComponent)
            local End = Transform:GetLocation() + Transform:GetForward() * 100.0

            local Body = Ctx:Get(Entity, SCharacterPhysicsComponent):GetBodyID()
            Ctx:CastRay(Transform:GetLocation(), End, true, 0.0, 0, Body)
        end
    end)
end

Script.CameraSystem.Update = function(Ctx)

    local View = Ctx:View(SCameraComponent, "FollowCamera")
    View:Each(function(Entity)
        Ctx:View(SCharacterControllerComponent):Each(function(TargetEntity)

            local CameraTransform = Ctx:Get(Entity, STransformComponent)
            local TargetTransform = Ctx:Get(TargetEntity, STransformComponent)

            local TargetPosition = TargetTransform:GetPosition()
            local CameraOffset = vec3(0.0, 50.0, -0.5)
            
            CameraTransform:SetLocation(TargetPosition + CameraOffset)
            local Forward = TargetPosition - CameraTransform:GetPosition()
            Forward = Forward:Normalize()
            
            local LookAtRotation = glm.QuatLookAt(Forward, vec3(0.0, 1.0, 0.0))
            CameraTransform:SetRotation(LookAtRotation)
            
            Ctx:DirtyTransform(Entity)
            
        end)
    end)
end



return Script