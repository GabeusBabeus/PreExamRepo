#include "FilmGrainEffect.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "../RenderLayer.h"
#include "Application/Application.h"

#include "Application/Timing.h"

FilmGrainEffect::FilmGrainEffect() :
	FilmGrainEffect(true) {}

FilmGrainEffect::FilmGrainEffect(bool x) :
	PostProcessingLayer::Effect(),
	_shader(nullptr)
{
	Name = "Film Grain Effect";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/FilmGrain.glsl" }
	});
	if (x) {
		 
	}
}

FilmGrainEffect::~FilmGrainEffect() = default;

void FilmGrainEffect::Apply(const Framebuffer::Sptr & gBuffer)
{	
	Timing& t = Timing::Current();

	_shader->Bind();
	_shader->SetUniform("iAmount", A);
	_shader->SetUniform("iTime", t.TimeSinceAppLoad());
	_shader->SetUniform("pi", x);
}

void FilmGrainEffect::RenderImGui()
{	
	ImGui::SliderFloat("Amount", &A, 0.0f, 10.0f);
	ImGui::InputFloat2("Pi", &x.x);
}

FilmGrainEffect::Sptr FilmGrainEffect::FromJson(const nlohmann::json & data)
{
	FilmGrainEffect::Sptr final = std::make_shared<FilmGrainEffect>();
	final->Enabled = JsonGet(data, "enabled", true);
	return final;
}

nlohmann::json FilmGrainEffect::ToJson() const
{
	return {
		{ "enabled", Enabled }
	};
}
