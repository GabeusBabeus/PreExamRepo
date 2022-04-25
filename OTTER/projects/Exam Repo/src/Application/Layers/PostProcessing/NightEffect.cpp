#include "NightEffect.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "../RenderLayer.h"
#include "Application/Application.h"

NightEffect::NightEffect() :
	NightEffect(true) {}

NightEffect::NightEffect(bool x) :
	PostProcessingLayer::Effect(),
	_shader(nullptr),
	_n(nullptr),
	_m(nullptr)
{
	Name = "Night Effect";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/Night_Vision.glsl" }
	});
	if (x) {
		_n = ResourceManager::CreateAsset<Texture2D>("textures/Noise.png");
		_m = ResourceManager::CreateAsset<Texture2D>("textures/Mask.png");
	}
}

NightEffect::~NightEffect() = default;

void NightEffect::Apply(const Framebuffer::Sptr & gBuffer)
{
	t += 0.01f;
	_shader->Bind();
	_n->Bind(1);
	_m->Bind(2);
	_shader->SetUniform("t", t);
	_shader->SetUniform("L", l);
	_shader->SetUniform("A", c);
}

void NightEffect::RenderImGui()
{
	ImGui::SliderFloat("SS", &t, 0.0f, 20.0f);
	ImGui::SliderFloat("L", &l, 0.0f, 50.0f);
	ImGui::SliderFloat("C", &c, 0.0f, 50.0f);
}

NightEffect::Sptr NightEffect::FromJson(const nlohmann::json & data)
{
	NightEffect::Sptr final = std::make_shared<NightEffect>();
	final->Enabled = JsonGet(data, "enabled", true);
	return final;
}

nlohmann::json NightEffect::ToJson() const
{
	return {
		{ "enabled", Enabled }
	};
}
