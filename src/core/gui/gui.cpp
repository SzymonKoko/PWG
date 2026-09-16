#include <glad/glad.h>              
#include "core/window/window.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "gui.h"
#include "core/logger/logger.h"
#include "scene/cameras/cameraManager.h"
#include "platform/nativeDialogs.h"
#include "project/projectSerializer.h"
#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cstring>

pwg::Gui::Gui(Window& window, std::shared_ptr<SceneManager> scene)
	:m_window(window), m_scene(scene)
{

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls 
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ApplyDarkGraphiteStyle();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(m_window.GetWindow(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	const char* glsl_version = "#version 330";
	ImGui_ImplOpenGL3_Init(glsl_version);
	LoadRecentProjects();
	m_welcomeTexture = std::make_shared<Texture>("../assets/images/welcome1.png", TextureType::ALBEDO);

	PWG_INFO("ImGui initialized");
}

pwg::Gui::~Gui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void pwg::Gui::Update(const float& dt)
{
	m_frameDeltaTime = dt;
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	HandleWindowCloseRequest();

	if (m_showWelcomeScreen)
	{
		DrawWelcomeScreen();
		DrawAboutWindow();
		DrawUnsavedChangesPopup();
		m_suppressDirtyDetection = false;
		return;
	}

	EnableDockSpace();
	DrawStatusBar();
	//ImGui::ShowDemoWindow();

	m_scene->Update(dt);
	m_scene->Draw();
	if (!m_suppressDirtyDetection && m_scene->GetEditorScene() && m_scene->GetEditorScene()->ConsumeDirtyFlag())
	{
		MarkProjectDirty();
	}
	DrawAboutWindow();
	DrawUnsavedChangesPopup();

	m_suppressDirtyDetection = false;
}

void pwg::Gui::Render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void pwg::Gui::EnableDockSpace()
{
	static bool opt_fullscreen = true;
	static bool opt_padding = false;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// Fullscreen window
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// DockSpace window
	ImGui::Begin("PWG Editor", nullptr, window_flags);
	DrawMainMenuBar();
	DrawTopToolbar();

	// Set up the dockspace
	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	if (!m_dockLayoutInitialized)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		ImGuiID dockMain = dockspace_id;
		ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.18f, nullptr, &dockMain);
		ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.28f, nullptr, &dockMain);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.22f, nullptr, &dockMain);

		ImGui::DockBuilderDockWindow("Main View", dockMain);
		ImGui::DockBuilderDockWindow("Outliner", dockLeft);
		ImGui::DockBuilderDockWindow("Terrain Graph", dockBottom);
		ImGui::DockBuilderDockWindow("Material Library", dockRight);
		ImGui::DockBuilderDockWindow("Vegetation Library", dockRight);
		ImGui::DockBuilderDockWindow("World", dockRight);
		ImGui::DockBuilderDockWindow("Debug", dockBottom);
		ImGui::DockBuilderFinish(dockspace_id);
		m_dockLayoutInitialized = true;
	}

	ImGui::End();
}

void pwg::Gui::ApplyDarkGraphiteStyle()
{
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 2.0f;
	style.ChildRounding = 2.0f;
	style.FrameRounding = 2.0f;
	style.PopupRounding = 2.0f;
	style.ScrollbarRounding = 2.0f;
	style.GrabRounding = 2.0f;
	style.TabRounding = 2.0f;
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowPadding = ImVec2(8.0f, 6.0f);
	style.FramePadding = ImVec2(6.0f, 3.0f);
	style.ItemSpacing = ImVec2(6.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 3.0f);

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.42f, 0.62f, 0.55f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.50f, 0.72f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.50f, 0.72f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.50f, 0.72f, 0.85f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.42f, 0.62f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.50f, 0.72f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.38f, 0.55f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.72f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.60f, 0.84f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.76f, 1.00f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.50f, 0.72f, 0.50f);
}

void pwg::Gui::DrawMainMenuBar()
{
	if (!ImGui::BeginMenuBar())
	{
		return;
	}

	EditorScene* editor = m_scene->GetEditorScene();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Project"))
		{
			RequestProjectAction(PendingProjectAction::NEW_PROJECT);
		}
		if (ImGui::MenuItem("Open Project..."))
		{
			RequestProjectAction(PendingProjectAction::OPEN_PROJECT);
		}
		if (ImGui::BeginMenu("Recent Projects", !m_recentProjects.empty()))
		{
			for (const auto& path : m_recentProjects)
			{
				std::string label = std::filesystem::path(path).filename().string();
				if (ImGui::MenuItem(label.c_str()))
				{
					RequestProjectAction(PendingProjectAction::OPEN_RECENT, path);
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", path.c_str());
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Save", "Ctrl+S", false, editor != nullptr))
		{
			SaveProject();
		}
		if (ImGui::MenuItem("Save As...", nullptr, false, editor != nullptr))
		{
			SaveProjectAs();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Exit"))
		{
			RequestProjectAction(PendingProjectAction::EXIT_APP);
		}
		ImGui::EndMenu();
	}

	if (editor && ImGui::BeginMenu("View"))
	{
		bool mainViewAlwaysOn = true;
		ImGui::MenuItem("Main View", nullptr, &mainViewAlwaysOn, false);
		ImGui::MenuItem("Outliner", nullptr, &editor->ShowOutliner());
		ImGui::MenuItem("Terrain Graph", nullptr, &editor->ShowTerrainGraph());
		ImGui::MenuItem("Material Library", nullptr, &editor->ShowMaterialLibrary());
		ImGui::MenuItem("Vegetation Library", nullptr, &editor->ShowVegetationLibrary());
		ImGui::MenuItem("Camera Settings", nullptr, &editor->ShowCameraSettings());
		ImGui::MenuItem("World", nullptr, &editor->ShowWorld());
		ImGui::MenuItem("Debug", nullptr, &editor->ShowDebugWindow());
		ImGui::EndMenu();
	}

	if (editor && ImGui::BeginMenu("Simulation"))
	{
		auto chunkManager = editor->GetTerrain()->GetChunkManager();
		if (ImGui::MenuItem("Generate Hydrology"))
		{
			chunkManager->GenerateHydrology();
			MarkProjectDirty();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Reset Hydrology"))
		{
			chunkManager->ResetHydrology();
			MarkProjectDirty();
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("About"))
		{
			m_showAbout = true;
		}
		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
}

void pwg::Gui::DrawTopToolbar()
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
	ImGui::BeginChild("TopToolbar", ImVec2(0.0f, 34.0f), false, ImGuiWindowFlags_NoScrollbar);

	if (ImGui::Button("New"))
	{
		RequestProjectAction(PendingProjectAction::NEW_PROJECT);
	}
	ImGui::SameLine();
	if (ImGui::Button("Open"))
	{
		RequestProjectAction(PendingProjectAction::OPEN_PROJECT);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		SaveProject();
	}

	ImGui::SameLine();
	ImGui::TextDisabled("|");
	ImGui::SameLine();

	auto chunkManager = editor->GetTerrain()->GetChunkManager();
	if (ImGui::Button("Generate"))
	{
		chunkManager->Regenerate();
		MarkProjectDirty();
	}
	ImGui::SameLine();
	if (ImGui::Button("Generate Hydrology"))
	{
		chunkManager->GenerateHydrology();
		MarkProjectDirty();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset Hydrology"))
	{
		chunkManager->ResetHydrology();
		MarkProjectDirty();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void pwg::Gui::DrawStatusBar()
{
	EditorScene* editor = m_scene->GetEditorScene();
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const float barHeight = 24.0f;

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - barHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, barHeight));
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
	ImGui::Begin("StatusBar", nullptr, flags);

	float fps = m_frameDeltaTime > 0.0001f ? 1.0f / m_frameDeltaTime : 0.0f;
	ImGui::Text("FPS: %.0f", fps);

	if (editor)
	{
		auto chunkManager = editor->GetTerrain()->GetChunkManager();
		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
		ImGui::Text(
			"World: %dx%d  Chunks: %d  Size: %d",
			chunkManager->GetWorldSize(),
			chunkManager->GetWorldSize(),
			chunkManager->GetChunkCount(),
			chunkManager->GetChunkSize());

		const char* cameraName = "Editor";
		switch (editor->GetCameraManager()->GetActiveCameraType())
		{
		case CameraType::FLYOVER: cameraName = "Flyover"; break;
		case CameraType::PLAYER: cameraName = "Player"; break;
		default: break;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
		ImGui::Text("Camera: %s", cameraName);

	}

	if (!m_projectStatus.empty())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
		if (m_isProjectDirty)
		{
			ImGui::Text("* %s", m_projectStatus.c_str());
		}
		else
		{
			ImGui::Text("%s", m_projectStatus.c_str());
		}
	}

	ImGui::End();
	ImGui::PopStyleColor();
}

void pwg::Gui::DrawWelcomeScreen()
{
	if (!m_showWelcomeScreen)
	{
		return;
	}

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.025f, 0.03f, 1.0f));
	ImGui::Begin("Welcome", nullptr, flags);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();
	if (m_welcomeTexture)
	{
		float imageAspect = static_cast<float>(m_welcomeTexture->GetWidth()) / static_cast<float>(m_welcomeTexture->GetHeight());
		float viewportAspect = size.x / size.y;
		ImVec2 imageSize = size;
		if (viewportAspect > imageAspect)
		{
			imageSize.y = size.x / imageAspect;
		}
		else
		{
			imageSize.x = size.y * imageAspect;
		}
		ImVec2 imageMin(origin.x + (size.x - imageSize.x) * 0.5f, origin.y + (size.y - imageSize.y) * 0.5f);
		ImVec2 imageMax(imageMin.x + imageSize.x, imageMin.y + imageSize.y);
		drawList->AddImage(reinterpret_cast<void*>(static_cast<intptr_t>(m_welcomeTexture->GetTextureID())), imageMin, imageMax, ImVec2(0, 1), ImVec2(1, 0));
	}
	drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(12, 20, 30, 8));
	drawList->AddRectFilledMultiColor(
		origin,
		ImVec2(origin.x + size.x, origin.y + size.y),
		IM_COL32(0, 0, 0, 60),
		IM_COL32(0, 0, 0, 8),
		IM_COL32(0, 0, 0, 45),
		IM_COL32(0, 0, 0, 110)
	);

	float panelTop = std::max(250.0f, size.y * 0.40f);
	ImVec2 panelMin(origin.x + 44.0f, origin.y + panelTop - 34.0f);
	ImVec2 panelMax(origin.x + 428.0f, origin.y + std::min(size.y - 76.0f, panelTop + 430.0f));
	drawList->AddRectFilled(panelMin, panelMax, IM_COL32(4, 10, 16, 76), 8.0f);
	drawList->AddRect(panelMin, panelMax, IM_COL32(95, 170, 230, 72), 8.0f);
	drawList->AddLine(ImVec2(panelMin.x, panelMin.y + 56.0f), ImVec2(panelMax.x, panelMin.y + 56.0f), IM_COL32(105, 190, 255, 90), 1.0f);

	ImGui::SetCursorPos(ImVec2(64.0f, panelTop));
	ImGui::BeginGroup();
	ImGui::TextColored(ImVec4(0.46f, 0.74f, 1.0f, 1.0f), "PROCEDURAL WORLD GENERATOR");
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.78f, 0.86f, 0.92f, 1.0f), "Infinite worlds. Procedurally generated.");
	ImGui::TextColored(ImVec4(0.78f, 0.86f, 0.92f, 1.0f), "Visually tuned. Technically yours.");
	ImGui::Spacing();
	ImGui::Spacing();

	auto glassButton = [&](const char* id, const char* title, const char* subtitle, const char* hint) -> bool
	{
		ImVec2 buttonSize(324.0f, 62.0f);
		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton(id, buttonSize);
		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();
		bool clicked = ImGui::IsItemClicked();
		ImU32 fill = hovered ? IM_COL32(20, 56, 82, active ? 150 : 118) : IM_COL32(8, 18, 28, 82);
		ImU32 border = hovered ? IM_COL32(105, 200, 255, 190) : IM_COL32(120, 170, 210, 86);
		ImVec2 max(cursor.x + buttonSize.x, cursor.y + buttonSize.y);
		drawList->AddRectFilled(cursor, max, fill, 6.0f);
		drawList->AddRect(cursor, max, border, 6.0f, 0, hovered ? 1.7f : 1.0f);
		drawList->AddText(ImVec2(cursor.x + 18.0f, cursor.y + 12.0f), IM_COL32(220, 238, 250, 245), title);
		drawList->AddText(ImVec2(cursor.x + 18.0f, cursor.y + 35.0f), IM_COL32(158, 180, 194, 230), subtitle);
		drawList->AddText(ImVec2(cursor.x + buttonSize.x - 34.0f, cursor.y + 22.0f), hovered ? IM_COL32(130, 215, 255, 255) : IM_COL32(170, 190, 205, 190), hint);
		ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + buttonSize.y + 10.0f));
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
		return clicked;
	};

	if (glassButton("welcome_new_world", "NEW WORLD", "Generate a fresh procedural terrain", ">"))
	{
		RequestProjectAction(PendingProjectAction::NEW_PROJECT);
	}
	if (glassButton("welcome_open_world", "OPEN WORLD", "Load an existing .pwg project", ">"))
	{
		RequestProjectAction(PendingProjectAction::OPEN_PROJECT);
	}
	if (glassButton("welcome_continue", "CONTINUE", "Enter the current editor session", ">"))
	{
		m_showWelcomeScreen = false;
	}

	if (!m_recentProjects.empty())
	{
		ImGui::Spacing();
		ImGui::TextDisabled("Recent projects");
		for (const auto& path : m_recentProjects)
		{
			std::string label = std::filesystem::path(path).filename().string();
			if (ImGui::SmallButton(label.c_str()))
			{
				RequestProjectAction(PendingProjectAction::OPEN_RECENT, path);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", path.c_str());
			}
		}
	}
	if (!m_projectStatus.empty())
	{
		ImGui::Spacing();
		ImGui::TextDisabled("%s", m_projectStatus.c_str());
	}
	ImGui::EndGroup();

	float rightX = origin.x + size.x - 360.0f;
	float rightTop = origin.y + 70.0f;
	drawList->AddText(ImVec2(rightX, rightTop), IM_COL32(160, 180, 194, 185), "GENERATION GRAPH");
	drawList->AddLine(ImVec2(rightX + 138.0f, rightTop + 7.0f), ImVec2(origin.x + size.x - 70.0f, rightTop + 7.0f), IM_COL32(130, 160, 180, 95), 1.0f);
	auto graphNode = [&](float x, float y, const char* label)
	{
		ImVec2 a(origin.x + x, origin.y + y);
		ImVec2 b(a.x + 108.0f, a.y + 38.0f);
		drawList->AddRectFilled(a, b, IM_COL32(8, 16, 24, 68), 5.0f);
		drawList->AddRect(a, b, IM_COL32(120, 170, 210, 70), 5.0f);
		drawList->AddText(ImVec2(a.x + 12.0f, a.y + 11.0f), IM_COL32(185, 205, 220, 190), label);
	};
	graphNode(size.x - 360.0f, 116.0f, "Noise");
	graphNode(size.x - 360.0f, 170.0f, "Erosion");
	graphNode(size.x - 212.0f, 143.0f, "Height");
	graphNode(size.x - 212.0f, 206.0f, "Water");
	drawList->AddBezierCubic(ImVec2(origin.x + size.x - 252.0f, origin.y + 135.0f), ImVec2(origin.x + size.x - 230.0f, origin.y + 135.0f), ImVec2(origin.x + size.x - 236.0f, origin.y + 156.0f), ImVec2(origin.x + size.x - 212.0f, origin.y + 162.0f), IM_COL32(95, 180, 240, 120), 1.2f);
	drawList->AddBezierCubic(ImVec2(origin.x + size.x - 252.0f, origin.y + 189.0f), ImVec2(origin.x + size.x - 230.0f, origin.y + 190.0f), ImVec2(origin.x + size.x - 240.0f, origin.y + 215.0f), ImVec2(origin.x + size.x - 212.0f, origin.y + 225.0f), IM_COL32(95, 180, 240, 95), 1.2f);

	float statsTop = origin.y + size.y * 0.50f;
	ImVec2 statsMin(origin.x + size.x - 330.0f, statsTop);
	ImVec2 statsMax(origin.x + size.x - 70.0f, statsTop + 146.0f);
	drawList->AddRectFilled(statsMin, statsMax, IM_COL32(7, 14, 22, 70), 6.0f);
	drawList->AddRect(statsMin, statsMax, IM_COL32(120, 170, 210, 65), 6.0f);
	drawList->AddText(ImVec2(statsMin.x + 16.0f, statsMin.y + 14.0f), IM_COL32(150, 170, 184, 180), "WORLD PREVIEW");
	drawList->AddText(ImVec2(statsMin.x + 16.0f, statsMin.y + 46.0f), IM_COL32(190, 210, 224, 190), "Terrain     procedural");
	drawList->AddText(ImVec2(statsMin.x + 16.0f, statsMin.y + 72.0f), IM_COL32(190, 210, 224, 190), "Materials   PBR layers");
	drawList->AddText(ImVec2(statsMin.x + 16.0f, statsMin.y + 98.0f), IM_COL32(190, 210, 224, 190), "Erosion     particle bake");

	const char* quote = "\"The best way to predict a world is to generate it.\"";
	ImVec2 quoteSize = ImGui::CalcTextSize(quote);
	drawList->AddText(ImVec2(origin.x + (size.x - quoteSize.x) * 0.5f, origin.y + size.y - 72.0f), IM_COL32(198, 210, 220, 175), quote);

	ImGui::SetCursorPos(ImVec2(64.0f, size.y - 46.0f));
	ImGui::TextDisabled("PWG v1.0.0  |  64-bit  |  OpenGL");

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void pwg::Gui::DrawAboutWindow()
{
	if (!m_showAbout)
	{
		return;
	}

	ImGui::Begin("About PWG", &m_showAbout, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Procedural World Generation");
	ImGui::Text("Project files: .pwg YAML");
	ImGui::Text("UI style: Dark Graphite");
	ImGui::End();
}

void pwg::Gui::DrawUnsavedChangesPopup()
{
	if (m_pendingAction != PendingProjectAction::NONE && m_isProjectDirty && !ImGui::IsPopupOpen("Unsaved Changes"))
	{
		ImGui::OpenPopup("Unsaved Changes");
	}

	if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Current project has unsaved changes.");
		ImGui::Spacing();

		if (ImGui::Button("Save", ImVec2(110.0f, 0.0f)))
		{
			if (SaveProject())
			{
				ExecutePendingProjectAction();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Discard", ImVec2(110.0f, 0.0f)))
		{
			MarkProjectClean();
			ExecutePendingProjectAction();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)))
		{
			m_pendingAction = PendingProjectAction::NONE;
			m_pendingPath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
	else if (m_pendingAction != PendingProjectAction::NONE && !m_isProjectDirty)
	{
		ExecutePendingProjectAction();
	}
}

void pwg::Gui::HandleWindowCloseRequest()
{
	if (!glfwWindowShouldClose(m_window.GetWindow()) || m_allowWindowClose)
	{
		return;
	}

	glfwSetWindowShouldClose(m_window.GetWindow(), GLFW_FALSE);
	RequestProjectAction(PendingProjectAction::EXIT_APP);
}

void pwg::Gui::CaptureProjectLayout()
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return;
	}

	size_t dataSize = 0;
	const char* data = ImGui::SaveIniSettingsToMemory(&dataSize);
	if (data && dataSize > 0)
	{
		editor->SetProjectLayoutIni(std::string(data, dataSize));
	}
}

void pwg::Gui::ApplyProjectLayout()
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return;
	}

	const std::string& layout = editor->GetProjectLayoutIni();
	if (!layout.empty())
	{
		ImGui::LoadIniSettingsFromMemory(layout.c_str(), layout.size());
	}
}

bool pwg::Gui::NewProject()
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return false;
	}

	editor->NewProject();
	m_dockLayoutInitialized = false;
	m_currentProjectPath.clear();
	m_projectStatus = "New project";
	MarkProjectClean();
	m_showWelcomeScreen = false;
	return true;
}

bool pwg::Gui::OpenProject()
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return false;
	}

	std::string path = OpenProjectDialog();
	if (path.empty())
	{
		return false;
	}

	return OpenProjectFromPath(path);
}

bool pwg::Gui::OpenProjectFromPath(const std::string& path)
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor || path.empty())
	{
		return false;
	}

	try
	{
		if (ProjectSerializer::Load(*editor, path))
		{
			m_currentProjectPath = path;
			m_projectStatus = "Opened: " + std::filesystem::path(path).filename().string();
			AddRecentProject(path);
			ApplyProjectLayout();
			m_dockLayoutInitialized = !editor->GetProjectLayoutIni().empty();
			MarkProjectClean();
			m_showWelcomeScreen = false;
			return true;
		}
	}
	catch (const std::exception& e)
	{
		m_projectStatus = std::string("Open failed: ") + e.what();
		return false;
	}

	m_projectStatus = "Open failed: invalid PWG project";
	return false;
}

bool pwg::Gui::SaveProject()
{
	if (m_currentProjectPath.empty())
	{
		return SaveProjectAs();
	}

	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return false;
	}

	try
	{
		CaptureProjectLayout();
		if (ProjectSerializer::Save(*editor, m_currentProjectPath))
		{
			m_projectStatus = "Saved: " + std::filesystem::path(m_currentProjectPath).filename().string();
			AddRecentProject(m_currentProjectPath);
			MarkProjectClean();
			return true;
		}
	}
	catch (const std::exception& e)
	{
		m_projectStatus = std::string("Save failed: ") + e.what();
		return false;
	}

	m_projectStatus = "Save failed";
	return false;
}

bool pwg::Gui::SaveProjectAs()
{
	EditorScene* editor = m_scene->GetEditorScene();
	if (!editor)
	{
		return false;
	}

	std::string path = SaveProjectDialog();
	if (path.empty())
	{
		return false;
	}

	std::filesystem::path projectPath(path);
	if (projectPath.extension().empty())
	{
		projectPath.replace_extension(".pwg");
	}

	try
	{
		CaptureProjectLayout();
		if (ProjectSerializer::Save(*editor, projectPath.string()))
		{
			m_currentProjectPath = projectPath.string();
			m_projectStatus = "Saved: " + projectPath.filename().string();
			AddRecentProject(m_currentProjectPath);
			MarkProjectClean();
			return true;
		}
	}
	catch (const std::exception& e)
	{
		m_projectStatus = std::string("Save failed: ") + e.what();
		return false;
	}

	m_projectStatus = "Save failed";
	return false;
}

void pwg::Gui::RequestProjectAction(PendingProjectAction action, const std::string& path)
{
	m_pendingAction = action;
	m_pendingPath = path;
	if (!m_isProjectDirty)
	{
		ExecutePendingProjectAction();
	}
}

void pwg::Gui::ExecutePendingProjectAction()
{
	PendingProjectAction action = m_pendingAction;
	std::string path = m_pendingPath;
	m_pendingAction = PendingProjectAction::NONE;
	m_pendingPath.clear();

	switch (action)
	{
	case PendingProjectAction::NEW_PROJECT:
		NewProject();
		break;
	case PendingProjectAction::OPEN_PROJECT:
		OpenProject();
		break;
	case PendingProjectAction::OPEN_RECENT:
		OpenProjectFromPath(path);
		break;
	case PendingProjectAction::EXIT_APP:
		m_allowWindowClose = true;
		glfwSetWindowShouldClose(m_window.GetWindow(), GLFW_TRUE);
		break;
	default:
		break;
	}
}

void pwg::Gui::MarkProjectDirty()
{
	m_isProjectDirty = true;
}

void pwg::Gui::MarkProjectClean()
{
	m_isProjectDirty = false;
	m_suppressDirtyDetection = true;
}

void pwg::Gui::AddRecentProject(const std::string& path)
{
	if (path.empty())
	{
		return;
	}

	std::string value = std::filesystem::absolute(path).string();
	m_recentProjects.erase(
		std::remove(m_recentProjects.begin(), m_recentProjects.end(), value),
		m_recentProjects.end()
	);
	m_recentProjects.insert(m_recentProjects.begin(), value);
	if (m_recentProjects.size() > 8)
	{
		m_recentProjects.resize(8);
	}
	SaveRecentProjects();
}

void pwg::Gui::LoadRecentProjects()
{
	m_recentProjects.clear();
	std::string path = RecentProjectsPath();
	if (!std::filesystem::exists(path))
	{
		return;
	}

	try
	{
		YAML::Node root = YAML::LoadFile(path);
		auto projects = root["recentProjects"];
		if (!projects)
		{
			return;
		}

		for (const auto& project : projects)
		{
			std::string projectPath = project.as<std::string>();
			if (std::filesystem::exists(projectPath))
			{
				m_recentProjects.push_back(projectPath);
			}
		}
	}
	catch (...)
	{
		m_recentProjects.clear();
	}
}

void pwg::Gui::SaveRecentProjects()
{
	YAML::Node root;
	for (const auto& path : m_recentProjects)
	{
		root["recentProjects"].push_back(path);
	}

	std::ofstream stream(RecentProjectsPath());
	if (stream.is_open())
	{
		stream << root;
	}
}

std::string pwg::Gui::RecentProjectsPath()
{
	return "../assets/configs/recent_projects.yaml";
}

std::string pwg::Gui::OpenProjectDialog()
{
	return platform::OpenProjectDialog();
}

std::string pwg::Gui::SaveProjectDialog()
{
	return platform::SaveProjectDialog();
}

