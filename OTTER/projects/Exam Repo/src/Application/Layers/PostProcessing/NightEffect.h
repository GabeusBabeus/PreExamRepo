#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Framebuffer.h"

class NightEffect : public PostProcessingLayer::Effect
{
public:
	MAKE_PTRS(NightEffect);

	Texture2D::Sptr _n;
	Texture2D::Sptr _m;

	NightEffect();
	NightEffect(bool x);
	virtual ~NightEffect();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	// Inherited from IResource

	NightEffect::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;

	float t = 1.0f;
	float l = 0.25f;
	float c = 4.5f;
};