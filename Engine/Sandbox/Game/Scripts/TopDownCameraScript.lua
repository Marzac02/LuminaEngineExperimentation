-- New Lua Script


TopDownCameraScript = {
}


function TopDownCameraScript:Update(DeltaTime)
    local PlayerEntity      = Context:GetByTag("Player")
    local PlayerTransform   = Context:Get(PlayerEntity, STransformComponent)
    local CameraTransform   = Context:Get(Entity, STransformComponent)
    
    if PlayerTransform and CameraTransform then
        CameraTransform:SetLocation(vec3(PlayerTransform:GetLocation().x, PlayerTransform:GetLocation().y + 50, PlayerTransform:GetLocation().z))
        CameraTransform:SetRotationFromEuler(vec3(90, 0, 0))
    end
    

    Context:DirtyTransform(Entity)
end



return TopDownCameraScript