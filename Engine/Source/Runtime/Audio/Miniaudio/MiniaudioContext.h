#pragma once
#include "Audio/AudioContext.h"
#include "MiniAudio/miniaudio.h"

namespace Lumina
{
    class FMiniaudioContext : public IAudioContext
    {
    public:
        
        FMiniaudioContext();
        ~FMiniaudioContext() override;
      
        void* GetNative() const override { return (void*)&Engine; }
        
        void PlaySoundFromFile(FStringView File) override;
        
    private:
        
        ma_engine Engine;
    };
}
