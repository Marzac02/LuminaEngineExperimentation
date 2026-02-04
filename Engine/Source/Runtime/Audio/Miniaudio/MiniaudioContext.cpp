#include "pch.h"
#include "MiniaudioContext.h"
#include "MiniAudio/miniaudio.h"

namespace Lumina
{
    FMiniaudioContext::FMiniaudioContext()
    {
        ma_engine_config Config = ma_engine_config_init();
        
        ma_result Result = ma_engine_init(&Config, &Engine);
        if (Result != MA_SUCCESS) 
        {
            return;
        }
    }

    FMiniaudioContext::~FMiniaudioContext()
    {
        ma_engine_uninit(&Engine);
    }

    void FMiniaudioContext::PlaySoundFromFile(FStringView File)
    {
        ma_engine_play_sound(&Engine, File.data(), nullptr);
    }
}
