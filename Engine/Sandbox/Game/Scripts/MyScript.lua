LightFlicker = {
    OtherEntity = 0,
    BaseIntensity = 12,      -- Base brightness
    FlickerSpeed = 5.0,      -- How fast it flickers
    FlickerAmount = 0.3,     -- How much it varies (0.0 to 1.0)
}

function LightFlicker:Update(DeltaTime)
    
    local PointLight = Context:TryGet(self.OtherEntity, SPointLightComponent)
    
    if PointLight then
        local flicker = math.sin(Context:GetTime() * self.FlickerSpeed) * 0.5 + 0.5
        
        local randomFlicker = math.random() * 0.5 - 0.1
        
        local variation = (flicker + randomFlicker) * self.FlickerAmount
        
        PointLight.Intensity = self.BaseIntensity * (1.0 - self.FlickerAmount + variation)
    end
end

return LightFlicker