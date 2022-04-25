#pragma once
#include "Application/Layers/PostProcessingLayer.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Framebuffer.h"

class FilmGrainEffect : public PostProcessingLayer::Effect
{
public:
	MAKE_PTRS(FilmGrainEffect); 

	FilmGrainEffect();
	FilmGrainEffect(bool x);
	virtual ~FilmGrainEffect();

	virtual void Apply(const Framebuffer::Sptr& gBuffer) override;
	virtual void RenderImGui() override;

	// Inherited from IResource
	FilmGrainEffect::Sptr FromJson(const nlohmann::json& data);
	virtual nlohmann::json ToJson() const override;

protected:
	ShaderProgram::Sptr _shader;

	float A = 0.01f;
	glm::vec2 x = glm::vec2(0);
};