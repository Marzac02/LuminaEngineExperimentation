#pragma once

#include "Core/Application/Application.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Object.h"


class FSandboxEngine : public Lumina::FEngine
{
public:
	
	#if WITH_EDITOR
	Lumina::IDevelopmentToolUI* CreateDevelopmentTools() override { return nullptr; }
	#endif
	
	void OnUpdateStage(const Lumina::FUpdateContext& Context) override;

	bool Init() override;
	bool Shutdown() override;
};


class FSandbox : public Lumina::FApplication
{
public:

	FSandbox() :FApplication("Sandbox") {}

	Lumina::FEngine* CreateEngine() override;
	bool Initialize(int argc, char** argv) override;
	void Shutdown() override;
	Lumina::FWindowSpecs GetWindowSpecs() const override;

private:
	bool ShouldExit() const override;
};
