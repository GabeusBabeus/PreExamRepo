#include "Inverse.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"

Inverse::Inverse() :
	PostProcessingLayer::Effect(),
	_shader(nullptr)
{
	Name = "Inverse Light";
	_format = RenderTargetType::ColorRgb8;

	_shader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
		{ ShaderPartType::Vertex, "shaders/vertex_shaders/fullscreen_quad.glsl" },
		{ ShaderPartType::Fragment, "shaders/fragment_shaders/post_effects/invert.glsl" }
	});

}

Inverse::~Inverse() = default;

void Inverse::Apply(const Framebuffer::Sptr & gBuffer)
{
	_shader->Bind();

	gBuffer->BindAttachment(RenderTargetAttachment::Depth, 1);
	gBuffer->BindAttachment(RenderTargetAttachment::Color1, 2); // The normal buffer
}

void Inverse::RenderImGui()
{
	//LABEL_LEFT(ImGui::ColorEdit4, "Color", &_outlineColor.x);

}

Inverse::Sptr Inverse::FromJson(const nlohmann::json & data)
{
	Inverse::Sptr result = std::make_shared<Inverse>();
	result->Enabled = JsonGet(data, "enabled", true);

	return result;
}

nlohmann::json Inverse::ToJson() const
{
	return {
		{ "enabled", Enabled }
	};
}