LightFlicker = {
    OtherEntity = 0,
}

function LightFlicker:Update(DeltaTime)


    local PointLight = Context:TryGet(OtherEntity, SPointLightComponent)
    
    PointLight.Attenuation = math.sin(Context:GetTime()) * self.MyValue
    

end

return LightFlicker