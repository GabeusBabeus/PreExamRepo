#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/Textures/Texture2DArray.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"
#include "Gameplay/Components/ShipMoveBehaviour.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad|AppLayerFunctions::OnUpdate;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

double preFrame = glfwGetTime();
float moveTimer = 0.0f;
int randomDir = 0;

void DefaultSceneLayer::OnUpdate()
{
	Application& app = Application::Get();
	currScene = app.CurrentScene();

	double currFrame = glfwGetTime();
	float dt = static_cast<float>(currFrame - preFrame);

	Gameplay::GameObject::Sptr intro = currScene->FindObjectByName("Intro");

	Gameplay::GameObject::Sptr pc = currScene->FindObjectByName("PC");
	Gameplay::GameObject::Sptr enemy = currScene->FindObjectByName("Enemy");

	if (InputEngine::GetKeyState(GLFW_KEY_ENTER) == ButtonState::Pressed)
	{
		if (currScene->IsPlaying == false)
		{
			currScene->IsPlaying = true;
			intro->SetEnabled(false);
		}
	}

	if (InputEngine::GetKeyState(GLFW_KEY_A) == ButtonState::Down)
	{
		pc->Get<Gameplay::Physics::RigidBody>()->ApplyImpulse(glm::vec3(-0.05f, 0.0f, 0.0f));
	}
	if (InputEngine::GetKeyState(GLFW_KEY_D) == ButtonState::Down)
	{
		pc->Get<Gameplay::Physics::RigidBody>()->ApplyImpulse(glm::vec3(0.05f, 0.0f, 0.0f));
	}

	moveTimer += dt;
	if (moveTimer >= 3000.0f)
	{
		randomDir = rand() % 3 + 1;
		if (randomDir == 1)
		{
			enemy->Get<Gameplay::Physics::RigidBody>()->ApplyImpulse(glm::vec3(0.5f, 0.0f, 0.0f));
			moveTimer = 0.0f;
		}
		else if (randomDir == 2)
		{
			enemy->Get<Gameplay::Physics::RigidBody>()->ApplyImpulse(glm::vec3(-0.5f, 0.0f, 0.0f));
			moveTimer = 0.0f;
		}
		else if (randomDir == 3)
		{
			enemy->Get<Gameplay::Physics::RigidBody>()->ApplyImpulse(glm::vec3(0.0f, 0.0f, 0.5f));
			moveTimer = 0.0f;
		}
	}

}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 
		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");


		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr shipMesh   = ResourceManager::CreateAsset<MeshResource>("fenrir.obj");
		MeshResource::Sptr goblinMesh = ResourceManager::CreateAsset<MeshResource>("GoblinRun_000001.obj");
		MeshResource::Sptr zombieMesh = ResourceManager::CreateAsset<MeshResource>("ZombieRun_000001.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/DarkWall.jpg");
		Texture2D::Sptr    wallTexture = ResourceManager::CreateAsset<Texture2D>("textures/LightWall.jpg");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    goblinTex = ResourceManager::CreateAsset<Texture2D>("textures/GoblinUVComp.png");
		Texture2D::Sptr    zombieTex = ResourceManager::CreateAsset<Texture2D>("textures/ZombieUVblood.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);

		// Load some images for drag n' drop
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight.png");
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight-2.png");
		ResourceManager::CreateAsset<Texture2D>("textures/light_projection.png");

		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particles.png", 2, 2);

		//DebugWindow::Sptr debugWindow = app.GetLayer<ImGuiDebugLayer>()->GetWindow<DebugWindow>();

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		  
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr goblinMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			goblinMaterial->Name = "Goblin";
			goblinMaterial->Set("u_Material.AlbedoMap", goblinTex);
			goblinMaterial->Set("u_Material.Shininess", 0.1f);
			goblinMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr zombieMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			zombieMaterial->Name = "Zombie";
			zombieMaterial->Set("u_Material.AlbedoMap", zombieTex);
			zombieMaterial->Set("u_Material.Shininess", 0.1f);
			zombieMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr wallMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			wallMaterial->Name = "Wall";
			wallMaterial->Set("u_Material.AlbedoMap", wallTexture);
			wallMaterial->Set("u_Material.Shininess", 0.1f);
			wallMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.AlbedoMap", monkeyTex);
			monkeyMaterial->Set("u_Material.NormalMap", normalMapDefault);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward); 
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture); 
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Our foliage vertex shader material 
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.AlbedoMap", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.DiscardThreshold", 0.1f);
			foliageMaterial->Set("u_Material.NormalMap", normalMapDefault);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			toonMaterial->Name = "Toon"; 
			toonMaterial->Set("u_Material.AlbedoMap", boxTexture);
			toonMaterial->Set("u_Material.NormalMap", normalMapDefault);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f); 
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.AlbedoMap", diffuseMap);
			displacementTest->Set("u_Material.NormalMap", normalMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr grey = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grey->Name = "Grey";
			grey->Set("u_Material.AlbedoMap", solidGreyTex);
			grey->Set("u_Material.Specular", solidBlackTex);
			grey->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr polka = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			polka->Name = "Polka";
			polka->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
			polka->Set("u_Material.Specular", solidBlackTex);
			polka->Set("u_Material.NormalMap", normalMapDefault);
			polka->Set("u_Material.EmissiveMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
		}

		Material::Sptr whiteBrick = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			whiteBrick->Name = "White Bricks";
			whiteBrick->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png"));
			whiteBrick->Set("u_Material.Specular", solidGrey);
			whiteBrick->Set("u_Material.NormalMap", ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png"));
		}



		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.NormalMapA", normalMapDefault);
			multiTextureMat->Set("u_Material.NormalMapB", normalMapDefault);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f); 
		}

		//// Create some lights for our scene
		//GameObject::Sptr lightParent = scene->CreateGameObject("Lights");

		//for (int ix = 0; ix < 50; ix++) {
		//	GameObject::Sptr light = scene->CreateGameObject("Light");
		//	light->SetPostion(glm::vec3(glm::diskRand(25.0f), 1.0f));
		//	lightParent->AddChild(light);

		//	Light::Sptr lightComponent = light->Add<Light>();
		//	lightComponent->SetColor(glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)));
		//	lightComponent->SetRadius(glm::linearRand(0.1f, 10.0f));
		//	lightComponent->SetIntensity(glm::linearRand(1.0f, 2.0f));
		//}

		Gameplay::GameObject::Sptr shadow = scene->CreateGameObject("Shadowlight");
		{
			//Set Position
			shadow->SetPostion(glm::vec3(-0.370f, -1.0f, 18.0f));
			shadow->LookAt(glm::vec3(0.0f));

			ShadowCamera::Sptr shadowCam = shadow->Add<ShadowCamera>();
			shadowCam->Flags = ShadowFlags::PcfEnabled | ShadowFlags::AttenuationEnabled;
			shadowCam->Bias = 0.000001f;


			shadowCam->SetProjection(glm::perspective(glm::radians(120.0f), 1.0f, 0.4f, 100.0f));
			shadowCam->Intensity = 7.5f;

			
		}




		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion(glm::vec3( 0.0f, -6.0f, 7.0f));
			camera->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			//camera->LookAt(glm::vec3(0.0f));

			//camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}


		// Set up all our sample objects


		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);
			plane->SetRotation(glm::vec3(90.0f, 90.0f, 0.0f));
			plane->SetPostion(glm::vec3(0.0f, 0.5f, 0.0f));
		}
		
		// Add some walls :3
		{
			MeshResource::Sptr wall = ResourceManager::CreateAsset<MeshResource>();
			wall->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			wall->GenerateMesh();

			GameObject::Sptr wall1 = scene->CreateGameObject("Wall1");
			wall1->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(wallMaterial);
			wall1->SetScale(glm::vec3(20.0f, 1.0f, 3.0f));
			wall1->SetPostion(glm::vec3(0.0f, 0.0f, 14.0f));
			RigidBody::Sptr wall1Phy = wall1->Add<RigidBody>(RigidBodyType::Static);
			wall1Phy->AddCollider(BoxCollider::Create(glm::vec3(10.0f, 0.5f, 1.5f)));
			//plane->AddChild(wall1);

			GameObject::Sptr wall2 = scene->CreateGameObject("Wall2");
			wall2->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(wallMaterial);
			wall2->SetScale(glm::vec3(20.0f, 1.0f, 3.0f));
			wall2->SetPostion(glm::vec3(0.0f, 0.0f, 1.5f));
			RigidBody::Sptr wall2Phy = wall2->Add<RigidBody>(RigidBodyType::Static);
			wall2Phy->AddCollider(BoxCollider::Create(glm::vec3(10.0f, 0.5f, 1.5f)));
			//plane->AddChild(wall2);

			GameObject::Sptr wall3 = scene->CreateGameObject("Wall3");
			wall3->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(wallMaterial);
			wall3->SetScale(glm::vec3(3.0f, 1.0f, 15.5f));
			wall3->SetPostion(glm::vec3(11.5f, 0.0f, 7.77f));
			RigidBody::Sptr wall3Phy = wall3->Add<RigidBody>(RigidBodyType::Static);
			wall3Phy->AddCollider(BoxCollider::Create(glm::vec3(1.5f, 0.5f, 7.75f)));
			//plane->AddChild(wall3);

			GameObject::Sptr wall4 = scene->CreateGameObject("Wall4");
			wall4->Add<RenderComponent>()->SetMesh(wall)->SetMaterial(wallMaterial);
			wall4->SetScale(glm::vec3(3.0f, 1.0f, 15.5f));
			wall4->SetPostion(glm::vec3(-11.5f, 0.0f, 7.77f));
			RigidBody::Sptr wall4Phy = wall4->Add<RigidBody>(RigidBodyType::Static);
			wall4Phy->AddCollider(BoxCollider::Create(glm::vec3(1.5f, 0.5f, 7.75f)));
			//plane->AddChild(wall4);
		}

		GameObject::Sptr pc = scene->CreateGameObject("PC");
		{
			RenderComponent::Sptr renderer = pc->Add<RenderComponent>();
			renderer->SetMesh(goblinMesh);
			renderer->SetMaterial(goblinMaterial);

			pc->SetPostion(glm::vec3(-8.0f, 0.0f, 3.0f));
			pc->SetRotation(glm::vec3(90.0f, 0.0f, 90.0f));

			RigidBody::Sptr pcPhy = pc->Add<RigidBody>(RigidBodyType::Dynamic);
			pcPhy->AddCollider(BoxCollider::Create(glm::vec3(0.5f, 1.0f, 0.5f)))->SetPosition({0,1,0});
			pcPhy->SetMass(0.1f);
			pcPhy->SetAngularDamping(0.9f);
			pcPhy->SetLinearDamping(0.9f);
			pc->Add<JumpBehaviour>();
			pc->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr enemy = scene->CreateGameObject("Enemy");
		{
			RenderComponent::Sptr renderer = enemy->Add<RenderComponent>();
			renderer->SetMesh(zombieMesh);
			renderer->SetMaterial(zombieMaterial);

			enemy->SetPostion(glm::vec3(8.0f, 0.0f, 3.0f));
			enemy->SetRotation(glm::vec3(90.0f, 0.0f, -90.0f));

			RigidBody::Sptr enemyPhy = enemy->Add<RigidBody>(RigidBodyType::Dynamic);
			enemyPhy->AddCollider(BoxCollider::Create(glm::vec3(0.5f, 1.0f, 0.5f)))->SetPosition({ 0,1,0 });
			enemyPhy->SetMass(0.1f);
			enemyPhy->SetAngularDamping(0.9f);
			enemyPhy->SetLinearDamping(0.9f);
		}


		/*

		GameObject::Sptr ship = scene->CreateGameObject("Fenrir");
		{
			// Set position in the scene
			ship->SetPostion(glm::vec3(1.5f, 0.0f, 4.0f));
			ship->SetScale(glm::vec3(0.1f));

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = ship->Add<RenderComponent>();
			renderer->SetMesh(shipMesh);
			renderer->SetMaterial(grey);

			GameObject::Sptr particles = scene->CreateGameObject("Particles");
			ship->AddChild(particles);
			particles->SetPostion({ 0.0f, -7.0f, 0.0f});

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;

			particleManager->_gravity = glm::vec3(0.0f);

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			emitter.Lifetime = 1.0f / 50.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 3.0f };
			emitter.SphereEmitterData.Radius = 0.5f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 1.0f };

			ParticleSystem::ParticleData emitter2;
			emitter2.Type = ParticleType::SphereEmitter;
			emitter2.TexID = 2;
			emitter2.Position = glm::vec3(0.0f);
			emitter2.Color = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
			emitter2.Lifetime = 1.0f / 40.0f;
			emitter2.SphereEmitterData.Timer = 1.0f / 40.0f;
			emitter2.SphereEmitterData.Velocity = 0.1f;
			emitter2.SphereEmitterData.LifeRange = { 0.5f, 1.0f };
			emitter2.SphereEmitterData.Radius = 0.25f;
			emitter2.SphereEmitterData.SizeRange = { 0.25f, 0.5f };

			particleManager->AddEmitter(emitter);
			particleManager->AddEmitter(emitter2);

			ShipMoveBehaviour::Sptr move = ship->Add<ShipMoveBehaviour>();
			move->Center = glm::vec3(0.0f, 0.0f, 4.0f);
			move->Speed = 180.0f;
			move->Radius = 6.0f;
		}

		GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");

		GameObject::Sptr polkaBox = scene->CreateGameObject("Polka Box");
		{
			MeshResource::Sptr boxMesh = ResourceManager::CreateAsset<MeshResource>();
			boxMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			boxMesh->GenerateMesh();

			// Set and rotation position in the scene
			polkaBox->SetPostion(glm::vec3(0, 0.0f, 3.0f));

			// Add a render component
			RenderComponent::Sptr renderer = polkaBox->Add<RenderComponent>();
			renderer->SetMesh(boxMesh);
			renderer->SetMaterial(polka);

			demoBase->AddChild(polkaBox);
		}

		// Box to showcase the specular material
		GameObject::Sptr specBox = scene->CreateGameObject("Specular Object");
		{
			MeshResource::Sptr boxMesh = ResourceManager::CreateAsset<MeshResource>();
			boxMesh->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			boxMesh->GenerateMesh();

			// Set and rotation position in the scene
			specBox->SetPostion(glm::vec3(0, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = specBox->Add<RenderComponent>();
			renderer->SetMesh(boxMesh);
			renderer->SetMaterial(testMaterial); 

			demoBase->AddChild(specBox);
		}

		// sphere to showcase the foliage material
		GameObject::Sptr foliageBall = scene->CreateGameObject("Foliage Sphere");
		{
			// Set and rotation position in the scene
			foliageBall->SetPostion(glm::vec3(-4.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = foliageBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(foliageMaterial);

			demoBase->AddChild(foliageBall);
		}

		// Box to showcase the foliage material
		GameObject::Sptr foliageBox = scene->CreateGameObject("Foliage Box");
		{
			MeshResource::Sptr box = ResourceManager::CreateAsset<MeshResource>();
			box->AddParam(MeshBuilderParam::CreateCube(glm::vec3(0, 0, 0.5f), ONE));
			box->GenerateMesh();

			// Set and rotation position in the scene
			foliageBox->SetPostion(glm::vec3(-6.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = foliageBox->Add<RenderComponent>();
			renderer->SetMesh(box);
			renderer->SetMaterial(foliageMaterial);

			demoBase->AddChild(foliageBox);
		}

		// Box to showcase the specular material
		GameObject::Sptr toonBall = scene->CreateGameObject("Toon Object");
		{
			// Set and rotation position in the scene
			toonBall->SetPostion(glm::vec3(-2.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = toonBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(toonMaterial);

			demoBase->AddChild(toonBall);
		}

		GameObject::Sptr displacementBall = scene->CreateGameObject("Displacement Object");
		{
			// Set and rotation position in the scene
			displacementBall->SetPostion(glm::vec3(2.0f, -4.0f, 1.0f));

			// Add a render component
			RenderComponent::Sptr renderer = displacementBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(displacementTest);

			demoBase->AddChild(displacementBall);
		}

		GameObject::Sptr multiTextureBall = scene->CreateGameObject("Multitextured Object");
		{
			// Set and rotation position in the scene 
			multiTextureBall->SetPostion(glm::vec3(4.0f, -4.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = multiTextureBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(multiTextureMat);

			demoBase->AddChild(multiTextureBall);
		}

		GameObject::Sptr normalMapBall = scene->CreateGameObject("Normal Mapped Object");
		{
			// Set and rotation position in the scene 
			normalMapBall->SetPostion(glm::vec3(6.0f, -4.0f, 1.0f));

			// Add a render component 
			RenderComponent::Sptr renderer = normalMapBall->Add<RenderComponent>();
			renderer->SetMesh(sphere);
			renderer->SetMaterial(normalmapMat);

			demoBase->AddChild(normalMapBall);
		}
		*/
		// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr trigger = scene->CreateGameObject("Trigger");
		{
			TriggerVolume::Sptr volume = trigger->Add<TriggerVolume>();
			CylinderCollider::Sptr collider = CylinderCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
			collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
			volume->AddCollider(collider);

			trigger->Add<TriggerVolumeEnterBehaviour>();
		}

		

		/////////////////////////// UI //////////////////////////////
		
		
		GameObject::Sptr canvas = scene->CreateGameObject("Intro"); 
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 128, 128 });

			transform->SetPosition(glm::vec2(800.0f, 200.0f));

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();	
			canPanel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/StartScreen.png"));
		}
		
		
/*
		GameObject::Sptr particles = scene->CreateGameObject("Particles"); 
		{
			particles->SetPostion({ -2.0f, 0.0f, 2.0f });

			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();  
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			emitter.Lifetime = 0.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 50.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 1.0f, 4.0f };
			emitter.SphereEmitterData.Radius = 1.0f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 1.5f };

			particleManager->AddEmitter(emitter);
		}
		*/
		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
