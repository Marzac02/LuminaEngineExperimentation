-- New Lua Script


TopDownCameraScript = {
    PlayerEntity = 0,
}


function TopDownCameraScript:Update(DeltaTime)
    local PlayerTransform = Context:TryGet(self.PlayerEntity, STransformComponent)
    local CameraTransform = Context:TryGet(Entity, STransformComponent)
    
    if PlayerTransform and CameraTransform then
        CameraTransform:SetLocation(vec3(PlayerTransform:GetLocation().x, PlayerTransform:GetLocation().y + 50, PlayerTransform:GetLocation().z))
        CameraTransform:SetRotationFromEuler(vec3(90, 0, 0))
    end
    

    Context:DirtyTransform(Entity)
end



return TopDownCameraScript