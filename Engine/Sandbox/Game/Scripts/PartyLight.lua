-- New Lua Script


PartyLight = {

}


function PartyLight:Update(DeltaTime)
    local LightComponent = Context:TryGet(Entity, SPointLightComponent)
    if LightComponent then
        local time = Context:GetTime()
        LightComponent.Intensity = 100 + math.sin(time * 5) * 5
        LightComponent.LightColor = vec3(1, 0.5 + math.sin(time * 3) * 0.5, 0.5 + math.cos(time * 4) * 0.5)
    end
end


return PartyLight