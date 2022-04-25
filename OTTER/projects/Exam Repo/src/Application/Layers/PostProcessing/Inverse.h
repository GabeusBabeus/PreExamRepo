#pragma once

#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"

class Inverse : public PostProcessingLayer::Effect {
public:
	MAKE_PTRS(Inverse);

	Inverse();
	virtual ~Inverse();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	Inverse::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;


};
