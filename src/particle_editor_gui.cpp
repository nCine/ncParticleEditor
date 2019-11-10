#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#include <ncine/imgui.h>

#include "particle_editor.h"
#include "particle_editor_gui_labels.h"
#include "particle_editor_lua.h"
#include <ncine/Application.h>
#include <ncine/Texture.h>
#include <ncine/Sprite.h>
#include <ncine/ParticleSystem.h>
#include <ncine/IFile.h>

#include "version.h"
#include <ncine/version.h>
#include "texture_strings.h"
#include "script_strings.h"

namespace {

const float PlotHeight = 50.0f;
const char *amountItems[] = { "Constant", "Min/Max" };
const char *lifeItems[] = { "Constant", "Min/Max" };
const char *positionItems[] = { "Constant", "Min/Max", "Radius" };
const char *velocityItems[] = { "Constant", "Min/Max", "Scale" };
const char *rotationItems[] = { "Emitter", "Constant", "Min/Max" };

static bool requestCloseModal = false;
static bool openModal = false;
static bool saveAsModal = false;
static bool showAboutWindow = false;
static bool allowOverwrite = false;

void showHelpMarker(const char *description)
{
	ImGui::TextDisabled("%s", Labels::HELP_MARKER);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(description);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

int inputTextCallback(ImGuiInputTextCallbackData *data)
{
	nctl::String *string = reinterpret_cast<nctl::String *>(data->UserData);
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		// Resize string callback
		ASSERT(data->Buf == string->data());
		string->setLength(data->BufTextLen);
		data->Buf = string->data();
	}
	return 0;
}

}

bool MyEventHandler::menuNewEnabled()
{
	return (particleSystems_.isEmpty() == false ||
	        textures_.isEmpty() == false);
}

void MyEventHandler::menuNew()
{
	clearData();
}

void MyEventHandler::menuOpen()
{
#ifndef __EMSCRIPTEN__
	openModal = true;
#else
	if (loader_->localFileLoad.isLoading() == false)
		loader_->localFileLoad.load(".lua");
#endif
}

bool MyEventHandler::menuSaveEnabled()
{
	return (particleSystems_.isEmpty() == false &&
	        filename_.isEmpty() == false);
}

void MyEventHandler::menuSave()
{
#ifndef __EMSCRIPTEN__
	const LuaLoader::Config &luaConfig = loader_->config();
	nctl::String filePath = joinPath(luaConfig.scriptsPath, filename_);
	save(filePath.data());
#else
	save(filename_.data());
#endif
}

void MyEventHandler::menuQuit()
{
	nc::theApplication().quit();
}

void MyEventHandler::closeModalsAndAbout()
{
	requestCloseModal = true;
	showAboutWindow = false;
}

void MyEventHandler::configureGui()
{
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#ifdef WITH_FONTAWESOME
	// Merge icons from Font Awesome into the default font
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF((nc::IFile::dataPath() + "fonts/" FONT_ICON_FILE_NAME_FAS).data(), 12.0f, &icons_config, icons_ranges);
#endif
}

void MyEventHandler::createGuiMainWindow()
{
	// Keyboard shortcuts can show pop-ups even when the main window is hidden
	createGuiPopups();

	if (showMainWindow_)
	{
		const ImVec2 windowSize = ImVec2(400.0f, 400.0f);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
		ImGui::Begin("ncParticleEditor", &showMainWindow_, ImGuiWindowFlags_MenuBar);

		createGuiMenus();

		createGuiBackground();
		createGuiTextures();
		createGuiManageSystems();

		if (particleSystems_.size() > 0)
		{
			createGuiParticleSystem();
			createGuiColorAffector();
			createGuiSizeAffector();
			createGuiRotationAffector();
			createGuiPositionAffector();
			createGuiVelocityAffector();
			createGuiEmission();
		}

		ImGui::Separator();
		ImGui::End();
	}
}

void MyEventHandler::createGuiMenus()
{
	ImGui::PushID("Menu");
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem(Labels::New, "CTRL + N", false, menuNewEnabled()))
				menuNew();

			if (ImGui::MenuItem(Labels::Open, "CTRL + O"))
				menuOpen();

			const bool openRecentEnabled = recentFilenames_.size() > 0;
			if (ImGui::BeginMenu(Labels::OpenRecent, openRecentEnabled))
			{
				int i = recentFileIndexStart_;
				while (i != recentFileIndexEnd_)
				{
					if (ImGui::MenuItem(recentFilenames_[i].data()))
					{
						const LuaLoader::Config &luaConfig = loader_->config();
						nctl::String filePath = joinPath(luaConfig.scriptsPath, recentFilenames_[i]);
						if (load(filePath.data()))
							filename_ = recentFilenames_[i];
						break;
					}
					i = (i + 1) % MaxRecentFiles;
				}

				ImGui::Separator();
				if (ImGui::MenuItem(Labels::ClearRecents))
				{
					recentFileIndexStart_ = 0;
					recentFileIndexEnd_ = 0;
					recentFilenames_.clear();
				}

				ImGui::EndMenu();
			}

			const bool openBundledEnabled = ScriptStrings::Count > 0;
			if (ImGui::BeginMenu(Labels::OpenBundled, openBundledEnabled))
			{
				for (unsigned int i = 0; i < ScriptStrings::Count; i++)
				{
					if (ImGui::MenuItem(ScriptStrings::Names[i]))
					{
						const LuaLoader::Config &luaConfig = loader_->config();
						nctl::String filePath = joinPath(luaConfig.scriptsPath, ScriptStrings::Names[i]);
						if (load(filePath.data()))
						{
							filename_ = ScriptStrings::Names[i];
							pushRecentFile(filename_);
						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem(Labels::Save, "CTRL + S", false, menuSaveEnabled()))
				menuSave();

			const bool saveAsEnabled = (particleSystems_.isEmpty() == false);
			if (ImGui::MenuItem(Labels::SaveAs, nullptr, false, saveAsEnabled))
				saveAsModal = true;

			ImGui::Separator();

			if (ImGui::MenuItem(Labels::Quit, "CTRL + Q"))
				menuQuit();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem(Labels::Main, "CTRL + 1", &showMainWindow_);
			ImGui::MenuItem(Labels::Config, "CTRL + 2", &showConfigWindow_);
			ImGui::MenuItem(Labels::Log, "CTRL + 3", &showLogWindow_);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("?"))
		{
			if (ImGui::MenuItem(Labels::About))
				showAboutWindow = true;

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiPopups()
{
	if (openModal)
		ImGui::OpenPopup("Open##Modal");
	else if (saveAsModal)
		ImGui::OpenPopup("Save As##Modal");
	else if (showAboutWindow)
	{
		ImGui::Begin("About", &showAboutWindow, ImGuiWindowFlags_AlwaysAutoResize);
#ifdef WITH_GIT_VERSION
		ImGui::Text("ncParticleEditor %s (%s)", VersionStrings::Version, VersionStrings::GitBranch);
#endif
		ImGui::Text("ncParticleEditor compiled on %s at %s", __DATE__, __TIME__);
		ImGui::Separator();
		ImGui::Text("Based on nCine %s (%s)", nc::VersionStrings::Version, nc::VersionStrings::GitBranch);
		ImGui::Text("nCine compiled on %s at %s", nc::VersionStrings::CompilationDate, nc::VersionStrings::CompilationTime);
		ImGui::End();
	}

	if (ImGui::BeginPopupModal("Open##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Enter the name of the file to load");
		ImGui::Separator();

		if (!ImGui::IsAnyItemActive())
			ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("", filename_.data(), MaxStringLength, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize,
		                     inputTextCallback, &filename_) || ImGui::Button(Labels::Ok))
		{
			const LuaLoader::Config &luaConfig = loader_->config();
			nctl::String filePath = joinPath(luaConfig.scriptsPath, filename_);
			if (nc::IFile::access(filePath.data(), nc::IFile::AccessMode::READABLE) && load(filePath.data()))
			{
				pushRecentFile(filename_);
				requestCloseModal = true;
			}
			else
			{
				filename_ = Labels::LoadingError;
				logString_.formatAppend("Could not load project file \"%s\"\n", filePath.data());
			}
		}
		ImGui::SetItemDefaultFocus();

		ImGui::SameLine();
		if (ImGui::Button(Labels::Cancel))
			requestCloseModal = true;

		if (requestCloseModal)
		{
			ImGui::CloseCurrentPopup();
			openModal = false;
			requestCloseModal = false;
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Save As##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Enter the name of the file to save");
		ImGui::Separator();

		if (!ImGui::IsAnyItemActive())
			ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("", filename_.data(), MaxStringLength, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize,
		                     inputTextCallback, &filename_) || ImGui::Button(Labels::Ok))
		{
#ifndef __EMSCRIPTEN__
			const LuaLoader::Config &luaConfig = loader_->config();
			nctl::String filePath = joinPath(luaConfig.scriptsPath, filename_);
			if (nc::IFile::access(filePath.data(), nc::IFile::AccessMode::READABLE) && allowOverwrite == false)
			{
				filename_ = Labels::FileExists;
				logString_.formatAppend("Could not overwrite existing file \"%s\"\n", filePath.data());
			}
			else
			{
				save(filePath.data());
				pushRecentFile(filename_);
				requestCloseModal = true;
			}
#else
			save(filename_.data());
			requestCloseModal = true;
#endif
		}
		ImGui::SetItemDefaultFocus();

		ImGui::SameLine();
		if (ImGui::Button(Labels::Cancel))
			requestCloseModal = true;
#ifndef __EMSCRIPTEN__
		ImGui::SameLine();
		ImGui::Checkbox("Allow Overwrite", &allowOverwrite);
#endif

		if (requestCloseModal)
		{
			ImGui::CloseCurrentPopup();
			saveAsModal = false;
			requestCloseModal = false;
		}

		ImGui::EndPopup();
	}
}

void MyEventHandler::createGuiBackground()
{
	const LuaLoader::Config &cfg = loader_->config();

	widgetName_.format(Labels::Background);
	if (backgroundTexture_)
		widgetName_.formatAppend(" (%s)", backgroundImageName_.data());
	widgetName_.append("###Background");
	ImGui::PushID("Background");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		ImGui::ColorEdit4("Color", background_.data(), ImGuiColorEditFlags_NoAlpha);
		nc::theApplication().gfxDevice().setClearColor(background_);

		ImGui::Separator();
		ImGui::InputText("Image to load", backgroundImageName_.data(), MaxStringLength,
		                 ImGuiInputTextFlags_CallbackResize, inputTextCallback, &backgroundImageName_);

		if (ImGui::Button(Labels::Load) && backgroundImageName_.isEmpty() == false)
		{
			if (loadBackgroundImage(backgroundImageName_))
				backgroundImageRect_ = backgroundTexture_->rect();
			else
				backgroundImageName_ = Labels::LoadingError;
		}
		ImGui::SameLine();
		if (ImGui::Button(Labels::Delete))
			deleteBackgroundImage();

		if (backgroundTexture_)
		{
			ImGui::SliderFloat("Image Pos X", &backgroundImagePosition_.x, 0.0f, nc::theApplication().width());
			ImGui::SliderFloat("Image Pos Y", &backgroundImagePosition_.y, 0.0f, nc::theApplication().height());
			ImGui::SliderFloat("Image Scale", &backgroundImageScale_, 0.0f, cfg.maxBackgroundImageScale);
			ImGui::SliderInt("Image Layer", &backgroundImageLayer_, 0, cfg.maxRenderingLayer);
			ImGui::ColorEdit4("Image Color", backgroundImageColor_.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);

			int minX = backgroundImageRect_.x;
			int maxX = minX + backgroundImageRect_.w;
			int minY = backgroundImageRect_.y;
			int maxY = minY + backgroundImageRect_.h;
			ImGui::DragIntRange2("Image Rect X", &minX, &maxX, 1.0f, 0, backgroundTexture_->width());
			ImGui::DragIntRange2("Image Rect Y", &minY, &maxY, 1.0f, 0, backgroundTexture_->height());
			backgroundImageRect_.x = minX;
			backgroundImageRect_.w = maxX - minX;
			backgroundImageRect_.y = minY;
			backgroundImageRect_.h = maxY - minY;
			ImGui::SameLine();
			widgetName_.format("%s##Rect", Labels::Reset);
			if (ImGui::Button(widgetName_.data()))
				backgroundImageRect_ = nc::Recti(0, 0, backgroundTexture_->width(), backgroundTexture_->height());

			applyBackgroundImageProperties();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiTextures()
{
	widgetName_.format(Labels::Textures);
	if (textures_.isEmpty() == false)
		widgetName_.formatAppend(" (%s: #%u of %u)", texStates_[texIndex_].name.data(), texIndex_, textures_.size());
	widgetName_.append("###Textures");
	ImGui::PushID("Textures");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		if (TextureStrings::Count > 0)
		{
			static int currentComboTexture = 0;
			if (ImGui::Combo("Bundled textures", &currentComboTexture, TextureStrings::Names, TextureStrings::Count))
				texFilename_ = TextureStrings::Names[currentComboTexture];
		}
		ImGui::InputText("Image to load", texFilename_.data(), MaxStringLength,
		                 ImGuiInputTextFlags_CallbackResize, inputTextCallback, &texFilename_);
		if (ImGui::Button(Labels::Load) && texFilename_.isEmpty() == false)
		{
			texIndex_ = textures_.size();
			texStates_[texIndex_].name = texFilename_;

			if (createTexture(texIndex_) == false)
			{
				texIndex_ = textures_.size() - 1;
				texFilename_ = Labels::LoadingError;
			}
			else
			{
				texFilename_.clear();
				texStates_[texIndex_].texRect = textures_[texIndex_]->rect();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(Labels::Delete) && textures_.size() > 0 && texIndex_ < textures_.size())
		{
			// Check if the texture is in use by some system
			bool canDelete = true;
			for (const ParticleSystemGuiState &state : sysStates_)
			{
				if (state.texture == textures_[texIndex_].get())
				{
					canDelete = false;
					break;
				}
			}

			if (canDelete)
			{
				destroyTexture(texIndex_);
				texIndex_--;
			}
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::InputInt("Index", &texIndex_, 1, 10, textures_.isEmpty() ? ImGuiInputTextFlags_ReadOnly : 0);
		if (textures_.isEmpty())
			texIndex_ = 0;
		else if (texIndex_ < 0)
			texIndex_ = 0;
		else if (texIndex_ > textures_.size() - 1)
			texIndex_ = textures_.size() - 1;

		if (textures_.size() > 0)
		{
			ImGui::SameLine();
			if (ImGui::Button(Labels::Assign) && particleSystems_.isEmpty() == false)
			{
				sysStates_[systemIndex_].texture = textures_[texIndex_].get();
				sysStates_[systemIndex_].texRect = texStates_[texIndex_].texRect;
				particleSystems_[systemIndex_]->setTexture(textures_[texIndex_].get());
				particleSystems_[systemIndex_]->setTexRect(texStates_[texIndex_].texRect);
			}
			ImGui::SameLine();
			showHelpMarker("Also applies current texture rectangle");
			ImGui::SameLine();
			if (ImGui::Button(Labels::Retrieve) && particleSystems_.isEmpty() == false)
			{
				texIndex_ = retrieveTexture(systemIndex_);
				texStates_[texIndex_].texRect = sysStates_[systemIndex_].texRect;
			}

			nc::Texture &tex = *textures_[texIndex_];
			TextureGuiState &t = texStates_[texIndex_];

			const float texWidth = static_cast<float>(tex.width());
			const float texHeight = static_cast<float>(tex.height());
			if (t.showRect)
			{
				ImGui::Image(tex.imguiTexId(), ImVec2(t.texRect.w, t.texRect.h),
				             ImVec2(t.texRect.x / texWidth, t.texRect.y / texHeight),
				             ImVec2((t.texRect.x + t.texRect.w) / texWidth, (t.texRect.y + t.texRect.h) / texHeight));
			}
			else
				ImGui::Image(tex.imguiTexId(), ImVec2(texWidth, texHeight));

			int minX = t.texRect.x;
			int maxX = minX + t.texRect.w;
			ImGui::DragIntRange2("Rect X", &minX, &maxX, 1.0f, 0, tex.width());
			ImGui::SameLine();
			ImGui::Checkbox("Show##Rect", &t.showRect);
			int minY = t.texRect.y;
			int maxY = minY + t.texRect.h;
			ImGui::DragIntRange2("Rect Y", &minY, &maxY, 1.0f, 0, tex.height());
			t.texRect.x = minX;
			t.texRect.w = maxX - minX;
			t.texRect.y = minY;
			t.texRect.h = maxY - minY;
			ImGui::SameLine();
			widgetName_.format("%s##Rect", Labels::Reset);
			if (ImGui::Button(widgetName_.data()))
				t.texRect = nc::Recti(0, 0, tex.width(), tex.height());

			ImGui::Text("Name: %s", texStates_[texIndex_].name.data());
			ImGui::Text("Width: %d", tex.width());
			ImGui::SameLine();
			ImGui::Text("Height: %d", tex.height());
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiManageSystems()
{
	widgetName_.format(Labels::ManageSystems);
	if (particleSystems_.isEmpty() == false)
	{
		if (sysStates_[systemIndex_].name.isEmpty() == false)
			widgetName_.formatAppend(" (%s: #%u of %u)", sysStates_[systemIndex_].name.data(), systemIndex_, particleSystems_.size());
		else
			widgetName_.formatAppend(" (#%u of %u)", systemIndex_, particleSystems_.size());
	}
	widgetName_.append("###ManageSystems");
	ImGui::PushID("ManageSystems");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		if (ImGui::Button(Labels::New))
		{
			if (textures_.isEmpty() == false)
			{
				systemIndex_ = particleSystems_.size();
				sysStates_[systemIndex_] = {};
				createParticleSystem(systemIndex_);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button(Labels::Delete) && particleSystems_.size() > 0 && systemIndex_ < particleSystems_.size())
		{
			destroyParticleSystem(systemIndex_);
			systemIndex_--;
		}

		ImGui::SameLine();
		if (ImGui::Button(Labels::Clone) && particleSystems_.size() > 0 && systemIndex_ < particleSystems_.size())
		{
			int srcSystemIndex = systemIndex_;
			systemIndex_ = particleSystems_.size();
			cloneParticleSystem(srcSystemIndex, systemIndex_, sysStates_[srcSystemIndex].numParticles);
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(80);
		ImGui::InputInt("Index", &systemIndex_, 1, 10, particleSystems_.isEmpty() ? ImGuiInputTextFlags_ReadOnly : 0);
		if (particleSystems_.isEmpty())
			systemIndex_ = 0;
		if (systemIndex_ < 0)
			systemIndex_ = 0;
		else if (systemIndex_ > particleSystems_.size() - 1)
			systemIndex_ = particleSystems_.size() - 1;

		if (particleSystems_.isEmpty() == false)
		{
			ImGui::SameLine();
			ImGui::Checkbox("Active", &sysStates_[systemIndex_].active);

			ImGui::InputText("Name", sysStates_[systemIndex_].name.data(), MaxStringLength,
			                 ImGuiInputTextFlags_CallbackResize, inputTextCallback, &sysStates_[systemIndex_].name);
			ImGui::SliderFloat("Pos X", &parentPosition_.x, 0.0f, nc::theApplication().width());
			ImGui::SliderFloat("Pos Y", &parentPosition_.y, 0.0f, nc::theApplication().height());
			dummy_->setPosition(parentPosition_);
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiParticleSystem()
{
	const LuaLoader::Config &cfg = loader_->config();
	nc::ParticleSystem *particleSystem = particleSystems_[systemIndex_].get();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	widgetName_.format("%s (%d particles)###ParticleSystem", Labels::ParticleSystem, s.numParticles);
	ImGui::PushID("ParticleSystem");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		ImGui::SliderInt("Particles", &s.numParticles, 1, cfg.maxNumParticles);
		ImGui::SameLine();
		if (ImGui::Button(Labels::Apply) && s.numParticles != particleSystem->numParticles())
		{
			unsigned int tempSystemIndex = particleSystems_.size();
			cloneParticleSystem(systemIndex_, tempSystemIndex, 1);
			cloneParticleSystem(tempSystemIndex, systemIndex_, s.numParticles);
			destroyParticleSystem(tempSystemIndex);
			particleSystem = particleSystems_[systemIndex_].get();
		}
		ImGui::SameLine();
		showHelpMarker("Applies the new number by creating a temporary clone and thus preserving the system state");

		ImGui::SliderFloat("Rel Pos X", &s.position.x, -cfg.systemPositionRange, cfg.systemPositionRange);
		ImGui::SameLine();
		if (ImGui::Button(Labels::Reset))
			s.position.set(0.0f, 0.0f);
		ImGui::SliderFloat("Rel Pos Y", &s.position.y, -cfg.systemPositionRange, cfg.systemPositionRange);
		particleSystem->setPosition(s.position);
		ImGui::SliderInt("Layer", &s.layer, 0, cfg.maxRenderingLayer);
		particleSystem->setLayer(static_cast<unsigned int>(s.layer));
		ImGui::Checkbox("Local Space", &s.inLocalSpace);
		particleSystem->setInLocalSpace(s.inLocalSpace);
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiColorAffector()
{
	nc::ParticleSystem *particleSystem = particleSystems_[systemIndex_].get();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	widgetName_.format(Labels::ColorAffector);
	if (s.colorAffector->steps().isEmpty() == false)
		widgetName_.formatAppend(" (%u steps)", s.colorAffector->steps().size());
	widgetName_.append("###ColorAffector");
	ImGui::PushID("ColorAffector");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		if (s.colorAffector->steps().isEmpty() == false)
		{
			createGuiColorPlot(s);
			ImGui::Separator();
		}

		int stepId = 0;
		for (nc::ColorAffector::ColorStep &step : s.colorAffector->steps())
		{
			widgetName_.format("Step %d", stepId);
			if (ImGui::TreeNodeEx(widgetName_.data(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (stepId > 0 && step.age < s.colorAffector->steps()[stepId - 1].age)
					step.age = s.colorAffector->steps()[stepId - 1].age;

				widgetName_.format("Color##%d", stepId);
				ImGui::ColorEdit4(widgetName_.data(), step.color.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
				widgetName_.format("Age##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.age, 0.0f, 1.0f);
				ImGui::TreePop();
			}
			stepId++;
		}

		if (s.colorAffector->steps().isEmpty() == false)
			ImGui::Separator();
		if (ImGui::TreeNodeEx("New Step", ImGuiTreeNodeFlags_DefaultOpen))
		{
			widgetName_.format("Color##%d", stepId);
			ImGui::ColorEdit4(widgetName_.data(), s.colorValue.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
			widgetName_.format("Age##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.colorAge, 0.0f, 1.0f);

			if (ImGui::Button(Labels::Add))
			{
				for (int i = static_cast<int>(s.colorAffector->steps().size()) - 1; i >= -1; i--)
				{
					const bool placeNotFound = (i > -1) ? s.colorAffector->steps()[i].age > s.colorAge : false;
					if (placeNotFound)
						s.colorAffector->steps()[i + 1] = s.colorAffector->steps()[i];
					else
					{
						s.colorAffector->steps()[i + 1].age = s.colorAge;
						s.colorAffector->steps()[i + 1].color = s.colorValue;
						break;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(Labels::Remove) && s.colorAffector->steps().size() > 0)
				s.colorAffector->steps().setSize(s.colorAffector->steps().size() - 1);
			ImGui::SameLine();
			if (ImGui::Button(Labels::RemoveAll) && s.colorAffector->steps().size() > 0)
				s.colorAffector->steps().clear();
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiColorPlot(const ParticleSystemGuiState &s)
{
	static float redValues[NumPlotValues];
	static float greenValues[NumPlotValues];
	static float blueValues[NumPlotValues];
	static float alphaValues[NumPlotValues];

	if (ImGui::TreeNodeEx("Plot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		unsigned int prevIndex = 0;
		nc::ColorAffector::ColorStep *prevStep = &s.colorAffector->steps()[0];
		for (nc::ColorAffector::ColorStep &step : s.colorAffector->steps())
		{
			const unsigned int index = static_cast<unsigned int>(step.age * NumPlotValues);
			for (unsigned int i = prevIndex; i < index; i++)
			{
				const float factor = (i - prevIndex) / static_cast<float>(index - prevIndex);
				redValues[i] = prevStep->color.r() + factor * (step.color.r() - prevStep->color.r());
				greenValues[i] = prevStep->color.g() + factor * (step.color.g() - prevStep->color.g());
				blueValues[i] = prevStep->color.b() + factor * (step.color.b() - prevStep->color.b());
				alphaValues[i] = prevStep->color.a() + factor * (step.color.a() - prevStep->color.a());
			}
			prevIndex = index;
			prevStep = &step;
		}
		for (unsigned int i = prevIndex; i < NumPlotValues; i++)
		{
			redValues[i] = prevStep->color.r();
			greenValues[i] = prevStep->color.g();
			blueValues[i] = prevStep->color.b();
			alphaValues[i] = prevStep->color.a();
		}

		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PlotLines("Red Steps", redValues, NumPlotValues, 0, nullptr, 0.0f, 1.0f, ImVec2(0.0f, PlotHeight));
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
		ImGui::PlotLines("Green Steps", greenValues, NumPlotValues, 0, nullptr, 0.0f, 1.0f, ImVec2(0.0f, PlotHeight));
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
		ImGui::PlotLines("Blue Steps", blueValues, NumPlotValues, 0, nullptr, 0.0f, 1.0f, ImVec2(0.0f, PlotHeight));
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::PlotLines("Alpha Steps", alphaValues, NumPlotValues, 0, nullptr, 0.0f, 1.0f, ImVec2(0.0f, PlotHeight));
		ImGui::PopStyleColor();
		ImGui::TreePop();
	}
}

void MyEventHandler::createGuiSizeAffector()
{
	const LuaLoader::Config &cfg = loader_->config();
	nc::ParticleSystem *particleSystem = particleSystems_[systemIndex_].get();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	widgetName_.format(Labels::SizeAffector);
	if (s.sizeAffector->steps().isEmpty() == false)
		widgetName_.formatAppend(" (%u steps)", s.sizeAffector->steps().size());
	widgetName_.append("###SizeAffector");
	ImGui::PushID("SizeAffector");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		ImGui::SliderFloat("Base Scale", &s.baseScale, cfg.minParticleScale, cfg.maxParticleScale);
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
			s.baseScale = 1.0f;
		s.sizeAffector->setBaseScale(s.baseScale);
		ImGui::Separator();

		if (s.sizeAffector->steps().isEmpty() == false)
		{
			createGuiSizePlot(s);
			ImGui::Separator();
		}

		int stepId = 0;
		for (nc::SizeAffector::SizeStep &step : s.sizeAffector->steps())
		{
			widgetName_.format("Step %d", stepId);
			if (ImGui::TreeNodeEx(widgetName_.data(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (stepId > 0 && step.age < s.sizeAffector->steps()[stepId - 1].age)
					step.age = s.sizeAffector->steps()[stepId - 1].age;

				widgetName_.format("Scale##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.scale, cfg.minParticleScale, cfg.maxParticleScale);
				widgetName_.format("Age##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.age, 0.0f, 1.0f);
				ImGui::TreePop();
			}
			stepId++;
		}

		if (s.sizeAffector->steps().isEmpty() == false)
			ImGui::Separator();
		if (ImGui::TreeNodeEx("New Step", ImGuiTreeNodeFlags_DefaultOpen))
		{
			widgetName_.format("Scale##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.sizeValue, cfg.minParticleScale, cfg.maxParticleScale);
			ImGui::SameLine();
			widgetName_.format("Reset##%d", stepId);
			if (ImGui::Button(widgetName_.data()))
				s.sizeValue = 1.0f;
			widgetName_.format("Age##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.sizeAge, 0.0f, 1.0f);

			if (ImGui::Button(Labels::Add))
			{
				for (int i = static_cast<int>(s.sizeAffector->steps().size()) - 1; i >= -1; i--)
				{
					const bool placeNotFound = (i > -1) ? s.sizeAffector->steps()[i].age > s.sizeAge : false;
					if (placeNotFound)
						s.sizeAffector->steps()[i + 1] = s.sizeAffector->steps()[i];
					else
					{
						s.sizeAffector->steps()[i + 1].age = s.sizeAge;
						s.sizeAffector->steps()[i + 1].scale = s.sizeValue;
						break;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(Labels::Remove) && s.sizeAffector->steps().size() > 0)
				s.sizeAffector->steps().setSize(s.sizeAffector->steps().size() - 1);
			ImGui::SameLine();
			if (ImGui::Button(Labels::RemoveAll) && s.sizeAffector->steps().size() > 0)
				s.sizeAffector->steps().clear();
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiSizePlot(const ParticleSystemGuiState &s)
{
	const LuaLoader::Config &cfg = loader_->config();
	static float values[NumPlotValues];

	if (ImGui::TreeNodeEx("Plot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		unsigned int prevIndex = 0;
		nc::SizeAffector::SizeStep *prevStep = &s.sizeAffector->steps()[0];
		for (nc::SizeAffector::SizeStep &step : s.sizeAffector->steps())
		{
			const unsigned int index = static_cast<unsigned int>(step.age * NumPlotValues);
			for (unsigned int i = prevIndex; i < index; i++)
			{
				const float factor = (i - prevIndex) / static_cast<float>(index - prevIndex);
				values[i] = prevStep->scale + factor * (step.scale - prevStep->scale);
			}
			prevIndex = index;
			prevStep = &step;
		}
		for (unsigned int i = prevIndex; i < NumPlotValues; i++)
			values[i] = prevStep->scale;

		ImGui::PlotLines("Size Steps", values, NumPlotValues, 0, nullptr, cfg.minParticleScale, cfg.maxParticleScale, ImVec2(0.0f, PlotHeight));
		ImGui::TreePop();
	}
}

void MyEventHandler::createGuiRotationAffector()
{
	const LuaLoader::Config &cfg = loader_->config();
	nc::ParticleSystem *particleSystem = particleSystems_[systemIndex_].get();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	widgetName_.format(Labels::RotationAffector);
	if (s.rotationAffector->steps().isEmpty() == false)
		widgetName_.formatAppend(" (%u steps)", s.rotationAffector->steps().size());
	widgetName_.append("###RotationAffector");
	ImGui::PushID("RotationAffector");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		if (s.rotationAffector->steps().isEmpty() == false)
		{
			createGuiRotationPlot(s);
			ImGui::Separator();
		}

		int stepId = 0;
		for (nc::RotationAffector::RotationStep &step : s.rotationAffector->steps())
		{
			widgetName_.format("Step %d", stepId);
			if (ImGui::TreeNodeEx(widgetName_.data(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (stepId > 0 && step.age < s.rotationAffector->steps()[stepId - 1].age)
					step.age = s.rotationAffector->steps()[stepId - 1].age;

				widgetName_.format("Angle##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.angle, cfg.minParticleAngle, cfg.maxParticleAngle);
				widgetName_.format("Age##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.age, 0.0f, 1.0f);
				ImGui::TreePop();
			}
			stepId++;
		}

		if (s.rotationAffector->steps().isEmpty() == false)
			ImGui::Separator();
		if (ImGui::TreeNodeEx("New Step", ImGuiTreeNodeFlags_DefaultOpen))
		{
			widgetName_.format("Angle##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.rotValue, cfg.minParticleAngle, cfg.maxParticleAngle);
			ImGui::SameLine();
			widgetName_.format("Reset##%d", stepId);
			if (ImGui::Button(widgetName_.data()))
				s.rotValue = 0.0f;
			widgetName_.format("Age##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.rotAge, 0.0f, 1.0f);

			if (ImGui::Button(Labels::Add))
			{
				for (int i = static_cast<int>(s.rotationAffector->steps().size()) - 1; i >= -1; i--)
				{
					const bool placeNotFound = (i > -1) ? s.rotationAffector->steps()[i].age > s.rotAge : false;
					if (placeNotFound)
						s.rotationAffector->steps()[i + 1] = s.rotationAffector->steps()[i];
					else
					{
						s.rotationAffector->steps()[i + 1].age = s.rotAge;
						s.rotationAffector->steps()[i + 1].angle = s.rotValue;
						break;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(Labels::Remove) && s.rotationAffector->steps().size() > 0)
				s.rotationAffector->steps().setSize(s.rotationAffector->steps().size() - 1);
			ImGui::SameLine();
			if (ImGui::Button(Labels::RemoveAll) && s.rotationAffector->steps().size() > 0)
				s.rotationAffector->steps().clear();
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiRotationPlot(const ParticleSystemGuiState &s)
{
	const LuaLoader::Config &cfg = loader_->config();
	static float values[NumPlotValues];

	if (ImGui::TreeNodeEx("Plot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		unsigned int prevIndex = 0;
		nc::RotationAffector::RotationStep *prevStep = &s.rotationAffector->steps()[0];
		for (nc::RotationAffector::RotationStep &step : s.rotationAffector->steps())
		{
			const unsigned int index = static_cast<unsigned int>(step.age * NumPlotValues);
			for (unsigned int i = prevIndex; i < index; i++)
			{
				const float factor = (i - prevIndex) / static_cast<float>(index - prevIndex);
				values[i] = prevStep->angle + factor * (step.angle - prevStep->angle);
			}
			prevIndex = index;
			prevStep = &step;
		}
		for (unsigned int i = prevIndex; i < NumPlotValues; i++)
			values[i] = prevStep->angle;

		ImGui::PlotLines("Rotation Steps", values, NumPlotValues, 0, nullptr, cfg.minParticleAngle, cfg.maxParticleAngle, ImVec2(0.0f, PlotHeight));
		ImGui::TreePop();
	}
}

void MyEventHandler::createGuiPositionAffector()
{
	const LuaLoader::Config &cfg = loader_->config();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	widgetName_.format(Labels::PositionAffector);
	if (s.positionAffector->steps().isEmpty() == false)
		widgetName_.formatAppend(" (%u steps)", s.positionAffector->steps().size());
	widgetName_.append("###PositionAffector");
	ImGui::PushID("PositionAffector");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		if (s.positionAffector->steps().isEmpty() == false)
		{
			createGuiPositionPlot(s);
			ImGui::Separator();
		}

		int stepId = 0;
		for (nc::PositionAffector::PositionStep &step : s.positionAffector->steps())
		{
			widgetName_.format("Step %d", stepId);
			if (ImGui::TreeNodeEx(widgetName_.data(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (stepId > 0 && step.age < s.positionAffector->steps()[stepId - 1].age)
					step.age = s.positionAffector->steps()[stepId - 1].age;

				widgetName_.format("Position X##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.position.x, -cfg.positionRange, cfg.positionRange);
				widgetName_.format("Position Y##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.position.y, -cfg.positionRange, cfg.positionRange);
				widgetName_.format("Age##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.age, 0.0f, 1.0f);
				ImGui::TreePop();
			}
			stepId++;
		}

		if (s.positionAffector->steps().isEmpty() == false)
			ImGui::Separator();
		if (ImGui::TreeNodeEx("New Step", ImGuiTreeNodeFlags_DefaultOpen))
		{
			widgetName_.format("Position X##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.positionValue.x, -cfg.positionRange, cfg.positionRange);
			ImGui::SameLine();
			widgetName_.format("Reset##%d", stepId);
			if (ImGui::Button(widgetName_.data()))
				s.positionValue.set(0.0f, 0.0f);
			widgetName_.format("Position Y##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.positionValue.y, -cfg.positionRange, cfg.positionRange);
			widgetName_.format("Age##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.positionAge, 0.0f, 1.0f);

			if (ImGui::Button(Labels::Add))
			{
				for (int i = static_cast<int>(s.positionAffector->steps().size()) - 1; i >= -1; i--)
				{
					const bool placeNotFound = (i > -1) ? s.positionAffector->steps()[i].age > s.positionAge : false;
					if (placeNotFound)
						s.positionAffector->steps()[i + 1] = s.positionAffector->steps()[i];
					else
					{
						s.positionAffector->steps()[i + 1].age = s.positionAge;
						s.positionAffector->steps()[i + 1].position = s.positionValue;
						break;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(Labels::Remove) && s.positionAffector->steps().size() > 0)
				s.positionAffector->steps().setSize(s.positionAffector->steps().size() - 1);
			ImGui::SameLine();
			if (ImGui::Button(Labels::RemoveAll) && s.positionAffector->steps().size() > 0)
				s.positionAffector->steps().clear();
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiPositionPlot(const ParticleSystemGuiState &s)
{
	const LuaLoader::Config &cfg = loader_->config();
	static float xValues[NumPlotValues];
	static float yValues[NumPlotValues];

	if (ImGui::TreeNodeEx("Plot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		unsigned int prevIndex = 0;
		nc::PositionAffector::PositionStep *prevStep = &s.positionAffector->steps()[0];
		for (nc::PositionAffector::PositionStep &step : s.positionAffector->steps())
		{
			const unsigned int index = static_cast<unsigned int>(step.age * NumPlotValues);
			for (unsigned int i = prevIndex; i < index; i++)
			{
				const float factor = (i - prevIndex) / static_cast<float>(index - prevIndex);
				xValues[i] = prevStep->position.x + factor * (step.position.x - prevStep->position.x);
				yValues[i] = prevStep->position.y + factor * (step.position.y - prevStep->position.y);
			}
			prevIndex = index;
			prevStep = &step;
		}
		for (unsigned int i = prevIndex; i < NumPlotValues; i++)
		{
			xValues[i] = prevStep->position.x;
			yValues[i] = prevStep->position.y;
		}

		ImGui::PlotLines("X Position Steps", xValues, NumPlotValues, 0, nullptr, -cfg.positionRange, cfg.positionRange, ImVec2(0.0f, PlotHeight));
		ImGui::PlotLines("Y Position Steps", yValues, NumPlotValues, 0, nullptr, -cfg.positionRange, cfg.positionRange, ImVec2(0.0f, PlotHeight));
		ImGui::TreePop();
	}
}

void MyEventHandler::createGuiVelocityAffector()
{
	const LuaLoader::Config &cfg = loader_->config();
	nc::ParticleSystem *particleSystem = particleSystems_[systemIndex_].get();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	widgetName_.format(Labels::VelocityAffector);
	if (s.velocityAffector->steps().isEmpty() == false)
		widgetName_.formatAppend(" (%u steps)", s.velocityAffector->steps().size());
	widgetName_.append("###VelocityAffector");
	ImGui::PushID("VelocityAffector");
	if (ImGui::CollapsingHeader(widgetName_.data()))
	{
		if (s.velocityAffector->steps().isEmpty() == false)
		{
			createGuiVelocityPlot(s);
			ImGui::Separator();
		}

		int stepId = 0;
		for (nc::VelocityAffector::VelocityStep &step : s.velocityAffector->steps())
		{
			widgetName_.format("Step %d", stepId);
			if (ImGui::TreeNodeEx(widgetName_.data(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (stepId > 0 && step.age < s.velocityAffector->steps()[stepId - 1].age)
					step.age = s.velocityAffector->steps()[stepId - 1].age;

				widgetName_.format("Velocity X##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.velocity.x, -cfg.velocityRange, cfg.velocityRange);
				widgetName_.format("Velocity Y##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.velocity.y, -cfg.velocityRange, cfg.velocityRange);
				widgetName_.format("Age##%d", stepId);
				ImGui::SliderFloat(widgetName_.data(), &step.age, 0.0f, 1.0f);
				ImGui::TreePop();
			}
			stepId++;
		}

		if (s.velocityAffector->steps().isEmpty() == false)
			ImGui::Separator();
		if (ImGui::TreeNodeEx("New Step", ImGuiTreeNodeFlags_DefaultOpen))
		{
			widgetName_.format("Velocity X##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.velocityValue.x, -cfg.velocityRange, cfg.velocityRange);
			ImGui::SameLine();
			widgetName_.format("Reset##%d", stepId);
			if (ImGui::Button(widgetName_.data()))
				s.velocityValue.set(0.0f, 0.0f);
			widgetName_.format("Velocity Y##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.velocityValue.y, -cfg.velocityRange, cfg.velocityRange);
			widgetName_.format("Age##%d", stepId);
			ImGui::SliderFloat(widgetName_.data(), &s.velocityAge, 0.0f, 1.0f);

			if (ImGui::Button(Labels::Add))
			{
				for (int i = static_cast<int>(s.velocityAffector->steps().size()) - 1; i >= -1; i--)
				{
					const bool placeNotFound = (i > -1) ? s.velocityAffector->steps()[i].age > s.velocityAge : false;
					if (placeNotFound)
						s.velocityAffector->steps()[i + 1] = s.velocityAffector->steps()[i];
					else
					{
						s.velocityAffector->steps()[i + 1].age = s.velocityAge;
						s.velocityAffector->steps()[i + 1].velocity = s.velocityValue;
						break;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button(Labels::Remove) && s.velocityAffector->steps().size() > 0)
				s.velocityAffector->steps().setSize(s.velocityAffector->steps().size() - 1);
			ImGui::SameLine();
			if (ImGui::Button(Labels::RemoveAll) && s.velocityAffector->steps().size() > 0)
				s.velocityAffector->steps().clear();
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::createGuiVelocityPlot(const ParticleSystemGuiState &s)
{
	const LuaLoader::Config &cfg = loader_->config();
	static float xValues[NumPlotValues];
	static float yValues[NumPlotValues];

	if (ImGui::TreeNodeEx("Plot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		unsigned int prevIndex = 0;
		nc::VelocityAffector::VelocityStep *prevStep = &s.velocityAffector->steps()[0];
		for (nc::VelocityAffector::VelocityStep &step : s.velocityAffector->steps())
		{
			const unsigned int index = static_cast<unsigned int>(step.age * NumPlotValues);
			for (unsigned int i = prevIndex; i < index; i++)
			{
				const float factor = (i - prevIndex) / static_cast<float>(index - prevIndex);
				xValues[i] = prevStep->velocity.x + factor * (step.velocity.x - prevStep->velocity.x);
				yValues[i] = prevStep->velocity.y + factor * (step.velocity.y - prevStep->velocity.y);
			}
			prevIndex = index;
			prevStep = &step;
		}
		for (unsigned int i = prevIndex; i < NumPlotValues; i++)
		{
			xValues[i] = prevStep->velocity.x;
			yValues[i] = prevStep->velocity.y;
		}

		ImGui::PlotLines("X Velocity Steps", xValues, NumPlotValues, 0, nullptr, -cfg.velocityRange, cfg.velocityRange, ImVec2(0.0f, PlotHeight));
		ImGui::PlotLines("Y Velocity Steps", yValues, NumPlotValues, 0, nullptr, -cfg.velocityRange, cfg.velocityRange, ImVec2(0.0f, PlotHeight));
		ImGui::TreePop();
	}
}

void MyEventHandler::createGuiEmission()
{
	const LuaLoader::Config &cfg = loader_->config();
	ParticleSystemGuiState &s = sysStates_[systemIndex_];

	const float columnWidth = ImGui::GetContentRegionAvail().x * 0.75f;
	ImGui::PushID("Emission");
	if (ImGui::CollapsingHeader(Labels::Emission))
	{
		ImGui::PushID("Amount");
		const unsigned int numParticles = particleSystems_[systemIndex_]->numParticles();
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		if (s.amountCurrentItem == 0)
		{
			s.init.rndAmount.x = (s.init.rndAmount.x + s.init.rndAmount.y) / 2;
			ImGui::SliderInt("Amount", &s.init.rndAmount.x, 1, numParticles);
			s.init.rndAmount.y = s.init.rndAmount.x;
		}
		else if (s.amountCurrentItem == 1)
			ImGui::DragIntRange2("Amount", &s.init.rndAmount.x, &s.init.rndAmount.y, 1, 1, numParticles, "Min: %d", "Max: %d");

		ImGui::NextColumn();
		ImGui::Combo("##AmountCombo", &s.amountCurrentItem, amountItems, IM_ARRAYSIZE(amountItems));
		ImGui::Columns(1);
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::PushID("Life");
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		if (s.lifeCurrentItem == 0)
		{
			s.init.rndLife.x = (s.init.rndLife.x + s.init.rndLife.y) * 0.5f;
			ImGui::SliderFloat("Life", &s.init.rndLife.x, 0.0f, cfg.maxRandomLife, "%.2fs");
			s.init.rndLife.y = s.init.rndLife.x;
		}
		else if (s.lifeCurrentItem == 1)
			ImGui::DragFloatRange2("Life", &s.init.rndLife.x, &s.init.rndLife.y, 0.01f, 0.0f, cfg.maxRandomLife, "Min: %.2fs", "Max: %.2fs");

		ImGui::NextColumn();
		ImGui::SetNextItemWidth(80);
		ImGui::Combo("##LifeCombo", &s.lifeCurrentItem, lifeItems, IM_ARRAYSIZE(lifeItems));
		ImGui::Columns(1);
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::PushID("Position");
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		if (s.positionCurrentItem == 0)
		{
			s.init.rndPositionX.x = (s.init.rndPositionX.x + s.init.rndPositionX.y) * 0.5f;
			ImGui::SliderFloat("Position X", &s.init.rndPositionX.x, -cfg.randomPositionRange, cfg.randomPositionRange);
			s.init.rndPositionX.y = s.init.rndPositionX.x;

			s.init.rndPositionY.x = (s.init.rndPositionY.x + s.init.rndPositionY.y) * 0.5f;
			ImGui::SliderFloat("Position Y", &s.init.rndPositionY.x, -cfg.randomPositionRange, cfg.randomPositionRange);
			s.init.rndPositionY.y = s.init.rndPositionY.x;
		}
		else if (s.positionCurrentItem == 1)
		{
			ImGui::DragFloatRange2("Position X", &s.init.rndPositionX.x, &s.init.rndPositionX.y, 1.0f, -cfg.randomPositionRange, cfg.randomPositionRange, "Min: %.1f", "Max: %.1f");
			ImGui::DragFloatRange2("Position Y", &s.init.rndPositionY.x, &s.init.rndPositionY.y, 1.0f, -cfg.randomPositionRange, cfg.randomPositionRange, "Min: %.1f", "Max: %.1f");
		}
		else if (s.positionCurrentItem == 2)
		{
			float centerX = (s.init.rndPositionX.x + s.init.rndPositionX.y) * 0.5f;
			float centerY = (s.init.rndPositionY.x + s.init.rndPositionY.y) * 0.5f;
			float radius = ((s.init.rndPositionX.y - s.init.rndPositionX.x) + (s.init.rndPositionY.y - s.init.rndPositionY.x)) * 0.25f;

			ImGui::SliderFloat("Position X", &centerX, -cfg.randomPositionRange, cfg.randomPositionRange);
			ImGui::SliderFloat("Position Y", &centerY, -cfg.randomPositionRange, cfg.randomPositionRange);
			ImGui::SliderFloat("Radius", &radius, 0.0f, cfg.randomPositionRange * 2.0f);

			if (radius < 0.0f)
				radius = 0.0f;
			s.init.setPositionAndRadius(centerX, centerY, radius);
		}

		ImGui::NextColumn();
		ImGui::Combo("##PositionCombo", &s.positionCurrentItem, positionItems, IM_ARRAYSIZE(positionItems));
		if (ImGui::Button(Labels::Reset))
		{
			s.init.rndPositionX.set(0.0f, 0.0f);
			s.init.rndPositionY.set(0.0f, 0.0f);
		}
		ImGui::Columns(1);
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::PushID("Velocity");
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		static nc::Vector2f scale(1.0f, 1.0f);
		if (s.velocityCurrentItem == 0)
		{
			s.init.rndVelocityX.x = (s.init.rndVelocityX.x + s.init.rndVelocityX.y) * 0.5f;
			ImGui::SliderFloat("Velocity X", &s.init.rndVelocityX.x, -cfg.randomVelocityRange, cfg.randomVelocityRange);
			s.init.rndVelocityX.y = s.init.rndVelocityX.x;

			s.init.rndVelocityY.x = (s.init.rndVelocityY.x + s.init.rndVelocityY.y) * 0.5f;
			ImGui::SliderFloat("Velocity Y", &s.init.rndVelocityY.x, -cfg.randomVelocityRange, cfg.randomVelocityRange);
			s.init.rndVelocityY.y = s.init.rndVelocityY.x;
		}
		else if (s.velocityCurrentItem == 1)
		{
			ImGui::DragFloatRange2("Velocity X", &s.init.rndVelocityX.x, &s.init.rndVelocityX.y, 1.0f, -cfg.randomVelocityRange, cfg.randomVelocityRange, "Min: %.1f", "Max: %.1f");
			ImGui::DragFloatRange2("Velocity Y", &s.init.rndVelocityY.x, &s.init.rndVelocityY.y, 1.0f, -cfg.randomVelocityRange, cfg.randomVelocityRange, "Min: %.1f", "Max: %.1f");
		}
		else if (s.velocityCurrentItem == 2)
		{
			float velX = s.init.rndVelocityX.x;
			if (scale.x != 0.0f)
				velX /= scale.x;
			else if (scale.y != 0.0f)
				velX = s.init.rndVelocityX.y / scale.y;

			float velY = s.init.rndVelocityY.x;
			if (scale.x != 0.0f)
				velY /= scale.x;
			else if (scale.y != 0.0f)
				velY = s.init.rndVelocityY.y / scale.y;

			ImGui::SliderFloat("Velocity X", &velX, -cfg.randomVelocityRange, cfg.randomVelocityRange);
			ImGui::SliderFloat("Velocity Y", &velY, -cfg.randomVelocityRange, cfg.randomVelocityRange);
			ImGui::DragFloatRange2("Scale", &scale.x, &scale.y, 1.0f, -cfg.randomVelocityRange, cfg.randomVelocityRange, "Min: %.1f", "Max: %.1f");

			if (scale.x > scale.y)
				nctl::swap(scale.x, scale.y);
			s.init.setVelocityAndScale(velX, velY, scale.x, scale.y);
		}
		ImGui::NextColumn();
		ImGui::Combo("##VelocityCombo", &s.velocityCurrentItem, velocityItems, IM_ARRAYSIZE(velocityItems));
		if (ImGui::Button(Labels::Reset))
		{
			scale.set(1.0f, 1.0f);
			s.init.rndVelocityX.set(0.0f, 0.0f);
			s.init.rndVelocityY.set(0.0f, 0.0f);
		}
		ImGui::Columns(1);
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::PushID("Rotation");
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		if (s.rotationCurrentItem == 0)
			ImGui::Text("Rotation set by emitter");
		else if (s.rotationCurrentItem == 1)
		{
			s.init.rndRotation.x = (s.init.rndRotation.x + s.init.rndRotation.y) * 0.5f;
			ImGui::SliderFloat("Rotation", &s.init.rndRotation.x, 0.0f, 180.0f);
			s.init.rndRotation.y = s.init.rndRotation.x;
		}
		else if (s.rotationCurrentItem == 2)
			ImGui::DragFloatRange2("Rotation", &s.init.rndRotation.x, &s.init.rndRotation.y, 1.0f, 0.0f, 180.0f, "Min: %.1f", "Max: %.1f");
		ImGui::NextColumn();
		ImGui::Combo("##RotationCombo", &s.rotationCurrentItem, rotationItems, IM_ARRAYSIZE(rotationItems));
		s.init.emitterRotation = (s.rotationCurrentItem == 0);
		ImGui::Columns(1);
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::PushID("Delay");
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::SliderFloat("Delay", &s.emitDelay, 0.0f, cfg.maxDelay, "%.2fs");
		ImGui::NextColumn();
		if (ImGui::Button("As life"))
			s.emitDelay = (s.init.rndLife.x + s.init.rndLife.y) * 0.5f;
		ImGui::Columns(1);
		ImGui::PopID();
	}
	ImGui::PopID();
	sanitizeParticleInit(s.init);

	ImGui::Separator();
	ImGui::PushID("Emit");
	if (ImGui::Button(Labels::Emit))
		emitParticles();
	ImGui::SameLine();
	if (ImGui::Button(Labels::Kill))
		killParticles();
	ImGui::SameLine();
	ImGui::Checkbox("Auto", &autoEmission_);
	createGuiEmissionPlot();

	if (particleSystems_.size() > 1)
	{
		if (ImGui::TreeNode("Particle Systems"))
		{
			for (unsigned int i = 0; i < particleSystems_.size(); i++)
			{
				widgetName_ = Labels::Emit;
				widgetName_.formatAppend("##%u", i);
				if (ImGui::Button(widgetName_.data()))
					emitParticles(i);
				ImGui::SameLine();
				widgetName_ = Labels::Kill;
				widgetName_.formatAppend("##%u", i);
				if (ImGui::Button(widgetName_.data()))
					killParticles(i);
				ImGui::SameLine();
				widgetName_.format("Active##%u", i);
				ImGui::Checkbox(widgetName_.data(), &sysStates_[i].active);
				ImGui::SameLine();
				ImGui::Text("#%u", i);
				ImGui::SameLine();
				ImGui::Text("Layer: %d", sysStates_[i].layer);
				ImGui::SameLine();
				ImGui::Text("Alive: %u/%u", particleSystems_[i]->numAliveParticles(), particleSystems_[i]->numParticles());
				if (sysStates_[i].name.isEmpty() == false)
				{
					ImGui::SameLine();
					ImGui::Text("Name: %s", sysStates_[i].name.data());
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void MyEventHandler::sanitizeParticleInit(nc::ParticleInitializer &init)
{
	const LuaLoader::Config &cfg = loader_->config();
	const unsigned int numParticles = particleSystems_[systemIndex_]->numParticles();

	// Sort and clamp of `rndAmount`
	if (init.rndAmount.x > init.rndAmount.y)
		nctl::swap(init.rndAmount.x, init.rndAmount.y);

	if (init.rndAmount.x < 1)
		init.rndAmount.x = 1;
	if (init.rndAmount.y > numParticles)
		init.rndAmount.y = numParticles;

	// Sort and clamp of `rndLife`
	if (init.rndLife.x > init.rndLife.y)
		nctl::swap(init.rndLife.x, init.rndLife.y);

	if (init.rndLife.x < 0.0f)
		init.rndLife.x = 0.0f;
	if (init.rndLife.y > cfg.maxRandomLife)
		init.rndLife.y = cfg.maxRandomLife;

	// Sort and clamp of `rndPositionX`
	if (init.rndPositionX.x > init.rndPositionX.y)
		nctl::swap(init.rndPositionX.x, init.rndPositionX.y);

	if (init.rndPositionX.x < -cfg.randomPositionRange)
		init.rndPositionX.x = -cfg.randomPositionRange;
	if (init.rndPositionX.y > cfg.randomPositionRange)
		init.rndPositionX.y = cfg.randomPositionRange;

	// Sort and clamp of `rndPositionY`
	if (init.rndPositionY.x > init.rndPositionY.y)
		nctl::swap(init.rndPositionY.x, init.rndPositionY.y);

	if (init.rndPositionY.x < -cfg.randomPositionRange)
		init.rndPositionY.x = -cfg.randomPositionRange;
	if (init.rndPositionY.y > cfg.randomPositionRange)
		init.rndPositionY.y = cfg.randomPositionRange;

	// Sort and clamp of `rndVelocityX`
	if (init.rndVelocityX.x > init.rndVelocityX.y)
		nctl::swap(init.rndVelocityX.x, init.rndVelocityX.y);

	if (init.rndVelocityX.x < -cfg.randomVelocityRange)
		init.rndVelocityX.x = -cfg.randomVelocityRange;
	if (init.rndVelocityX.y > cfg.randomVelocityRange)
		init.rndVelocityX.y = cfg.randomVelocityRange;

	// Sort and clamp of `rndVelocityY`
	if (init.rndVelocityY.x > init.rndVelocityY.y)
		nctl::swap(init.rndVelocityY.x, init.rndVelocityY.y);

	if (init.rndVelocityY.x < -cfg.randomVelocityRange)
		init.rndVelocityY.x = -cfg.randomVelocityRange;
	if (init.rndVelocityY.y > cfg.randomVelocityRange)
		init.rndVelocityY.y = cfg.randomVelocityRange;

	// Sort and clamp of `rndRotation`
	if (init.rndRotation.x > init.rndRotation.y)
		nctl::swap(init.rndRotation.x, init.rndRotation.y);

	if (init.rndRotation.x < 0.0)
		init.rndRotation.x = 0.0f;
	if (init.rndRotation.y > 180.0f)
		init.rndRotation.y = 180.0f;
}

void MyEventHandler::createGuiEmissionPlot()
{
	static nc::TimeStamp lastUpdateTime;

	static unsigned int index = 0;
	static float values[NumPlotValues];
	static float interval = nc::theApplication().interval();
	static unsigned int aliveParticles = 0;
	static unsigned int totalParticles = 0;

	if (lastUpdateTime.secondsSince() > 0.1f)
	{
		interval = nc::theApplication().interval();
		aliveParticles = 0;
		totalParticles = 0;
		for (nctl::UniquePtr<nc::ParticleSystem> &particleSystem : particleSystems_)
		{
			aliveParticles += particleSystem->numAliveParticles();
			totalParticles += particleSystem->numParticles();
		}
		values[index] = aliveParticles;
		index = (index + 1) % NumPlotValues;
		lastUpdateTime = nc::TimeStamp::now();
	}

	ImGui::SameLine();
	ImGui::Text("FPS: %.0f (%.2fs)", 1.0f / interval, interval);
	ImGui::PlotLines("", values, NumPlotValues, 0, nullptr, 0, totalParticles, ImVec2(0.0f, 25.0f));
	ImGui::SameLine();
	ImGui::Text("Alive: %u/%u", aliveParticles, totalParticles);
}

void MyEventHandler::createGuiConfigWindow()
{
	if (showConfigWindow_)
	{
		const ImVec2 windowSize = ImVec2(450.0f, 450.0f);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
		ImGui::Begin("Config", &showConfigWindow_, 0);

		LuaLoader::Config &cfg = loader_->config();

#ifdef __ANDROID__
		ImGui::Text("Screen Width: %d", nc::theApplication().widthInt());
		ImGui::Text("Screen Height: %d", nc::theApplication().heightInt());
		ImGui::Text("Resizable: false");
		ImGui::SameLine();
		ImGui::Text("Fullscreen: true");
#else
		static int selectedVideoMode = -1;
		const nc::IGfxDevice::VideoMode currentVideoMode = nc::theApplication().gfxDevice().currentVideoMode();
		if (cfg.fullscreen == false)
		{
			ImGui::SliderInt("Window Width", &cfg.width, 0, currentVideoMode.width);
			ImGui::SliderInt("Window Height", &cfg.height, 0, currentVideoMode.height);
			ImGui::Checkbox("Resizable", &cfg.resizable);
			ImGui::SameLine();
			selectedVideoMode = -1;
		}
		else
		{
			unsigned int currentVideoModeIndex = 0;
			const unsigned int numVideoModes = nc::theApplication().gfxDevice().numVideoModes();
			comboVideoModes_.clear();
			for (unsigned int i = 0; i < numVideoModes; i++)
			{
				const nc::IGfxDevice::VideoMode &mode = nc::theApplication().gfxDevice().videoMode(i);
				comboVideoModes_.formatAppend("%ux%u, %uHz", mode.width, mode.height, mode.refreshRate);
				comboVideoModes_.setLength(comboVideoModes_.length() + 1);

				if (mode == currentVideoMode)
					currentVideoModeIndex = i;
			}
			comboVideoModes_.setLength(comboVideoModes_.length() + 1);
			// Append a second '\0' to signal the end of the combo item list
			comboVideoModes_[comboVideoModes_.length() - 1] = '\0';

			if (selectedVideoMode < 0)
				selectedVideoMode = currentVideoModeIndex;

			ImGui::Combo("Video Mode", &selectedVideoMode, comboVideoModes_.data());
			cfg.width = nc::theApplication().gfxDevice().videoMode(selectedVideoMode).width;
			cfg.height = nc::theApplication().gfxDevice().videoMode(selectedVideoMode).height;
		}

		ImGui::Checkbox("Fullscreen", &cfg.fullscreen);
		ImGui::SameLine();
		if (ImGui::Button(Labels::Apply))
		{
			nc::theApplication().gfxDevice().setFullScreen(cfg.fullscreen);
			if (cfg.fullscreen == false)
				nc::theApplication().gfxDevice().setResolution(cfg.width, cfg.height);
			else
				nc::theApplication().gfxDevice().setVideoMode(selectedVideoMode);
		}
		ImGui::SameLine();
		if (ImGui::Button(Labels::Current))
		{
			cfg.width = nc::theApplication().widthInt();
			cfg.height = nc::theApplication().heightInt();
			cfg.fullscreen = nc::theApplication().gfxDevice().isFullScreen();
			cfg.resizable = nc::theApplication().gfxDevice().isResizable();
			selectedVideoMode = -1;
		}
#endif

		ImGui::NewLine();
		ImGui::Checkbox("Buffer Mapping", &cfg.useBufferMapping);
		ImGui::SameLine();
		ImGui::Checkbox("Vertical Sync", &cfg.withVSync);
		int frameLimit = cfg.frameLimit;
		ImGui::SliderInt("Frame Limit", &frameLimit, 0, 240);
		cfg.frameLimit = frameLimit < 0 ? 0 : frameLimit;
		int vboSize = cfg.vboSize / 1024;
		ImGui::SliderInt("VBO Size", &vboSize, 0, 1024, "%d KB");
		cfg.vboSize = vboSize * 1024;
		int iboSize = cfg.iboSize / 1024;
		ImGui::SliderInt("IBO Size", &iboSize, 0, 256, "%d KB");
		cfg.iboSize = iboSize * 1024;

		ImGui::Checkbox("Batching", &cfg.batching);
		ImGui::SameLine();
		ImGui::Checkbox("Culling", &cfg.culling);

		ImGui::NewLine();
		int saveFileSize = cfg.saveFileMaxSize / 1024;
		ImGui::SliderInt("Savefile Size", &saveFileSize, 0, 128, "%d KB");
		cfg.saveFileMaxSize = saveFileSize * 1024;
		int logStringSize = cfg.logMaxSize / 1024;
		ImGui::SliderInt("Log Size", &logStringSize, 0, 64, "%d KB");
		cfg.logMaxSize = logStringSize * 1024;

		ImGui::NewLine();
		ImGui::Checkbox("Auto Emission On Start", &cfg.autoEmissionOnStart);
		ImGui::InputText("Start-up Script Name", cfg.startupScriptName.data(), MaxStringLength,
		                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize, inputTextCallback, &cfg.startupScriptName);

		ImGui::NewLine();
		ImGui::InputText("Scripts Path", cfg.scriptsPath.data(), MaxStringLength,
		                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize, inputTextCallback, &cfg.scriptsPath);
		ImGui::InputText("Textures Path", cfg.texturesPath.data(), MaxStringLength,
		                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize, inputTextCallback, &cfg.texturesPath);
		ImGui::InputText("Backgrounds Path", cfg.backgroundsPath.data(), MaxStringLength,
		                 ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackResize, inputTextCallback, &cfg.backgroundsPath);

		loader_->sanitizeInitValues();

		ImGui::NewLine();
		if (ImGui::TreeNode("GUI Limits"))
		{
			ImGui::PushItemWidth(100.0f);
			ImGui::InputFloat("Max Background Image Scale", &cfg.maxBackgroundImageScale);
			ImGui::InputInt("Max Rendering Layer", &cfg.maxRenderingLayer);
			ImGui::InputInt("Max Number of Particles", &cfg.maxNumParticles);
			ImGui::InputFloat("System Position Range", &cfg.systemPositionRange);
			ImGui::InputFloat("Min Particle Scale", &cfg.minParticleScale);
			ImGui::SameLine();
			ImGui::InputFloat("Max Particle Scale", &cfg.maxParticleScale);
			ImGui::InputFloat("Min Particle Angle", &cfg.minParticleAngle);
			ImGui::SameLine();
			ImGui::InputFloat("Max Particle Angle", &cfg.maxParticleAngle);
			ImGui::InputFloat("Position Range", &cfg.positionRange);
			ImGui::InputFloat("Velocity Range", &cfg.velocityRange);
			ImGui::InputFloat("Max Random Life", &cfg.maxRandomLife);
			ImGui::InputFloat("Random Position Range", &cfg.randomPositionRange);
			ImGui::InputFloat("Random Velocity Range", &cfg.randomVelocityRange);
			ImGui::InputFloat("Max Delay", &cfg.maxDelay);
			ImGui::PopItemWidth();

			loader_->sanitizeGuiLimits();
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("GUI Style"))
		{
			ImGui::Combo("Theme", &cfg.styleIndex, "Dark\0Light\0Classic\0");

			ImGui::SliderFloat("Frame Rounding", &cfg.frameRounding, 0.0f, 12.0f, "%.0f");

			ImGui::Checkbox("Window Border", &cfg.windowBorder);
			ImGui::SameLine();
			ImGui::Checkbox("Frame Border", &cfg.frameBorder);
			ImGui::SameLine();
			ImGui::Checkbox("Popup Border", &cfg.popupBorder);

			ImGui::SetNextItemWidth(100);
			ImGui::DragFloat("Scaling", &cfg.scaling, 0.005f, 0.5f, 2.0f, "%.1f");
			ImGui::SameLine();
			if (ImGui::Button(Labels::Reset))
			{
#ifdef __ANDROID__
				cfg.scaling = 2.0f;
#else
				cfg.scaling = 1.0f;
#endif
			}

			loader_->sanitizeGuiStyle();
			ImGui::TreePop();
		}

		ImGui::NewLine();
		if (ImGui::Button(Labels::Load))
		{
#ifndef __EMSCRIPTEN__
			loader_->loadConfig(configFile_.data());
			logString_.formatAppend("Loaded config file \"%s\"\n", configFile_.data());
#else
			if (loader_->localFileLoadConfig.isLoading() == false)
				loader_->localFileLoadConfig.load(".lua");
#endif
		}
		ImGui::SameLine();
		if (ImGui::Button(Labels::Save))
		{
			loader_->saveConfig(configFile_.data());
			logString_.formatAppend("Saved config file \"%s\"\n", configFile_.data());
		}
#ifdef __EMSCRIPTEN__
		ImGui::SameLine();
		if (ImGui::Button(Labels::Reset))
		{
			loader_->loadConfig(configFile_.data());
			logString_.formatAppend("Loaded config file \"%s\"\n", configFile_.data());
		}
#endif

		ImGui::End();
	}
}

void MyEventHandler::createGuiLogWindow()
{
	if (showLogWindow_)
	{
		const ImVec2 windowSize = ImVec2(600.0f, 300.0f);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
		ImGui::Begin("Log", &showLogWindow_, 0);

		ImGui::BeginChild("scrolling", ImVec2(0.0f, -1.2f * ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::TextUnformatted(logString_.data());
		ImGui::EndChild();
		ImGui::Separator();
		if (ImGui::Button(Labels::Clear))
			logString_.clear();
		ImGui::SameLine();
		ImGui::Text("Length: %u / %u", logString_.length(), logString_.capacity());

		ImGui::End();
	}
}

void MyEventHandler::applyGuiStyleConfig()
{
	const LuaLoader::Config &cfg = loader_->config();

	switch (cfg.styleIndex)
	{
		case 0: ImGui::StyleColorsDark(); break;
		case 1: ImGui::StyleColorsLight(); break;
		case 2: ImGui::StyleColorsClassic(); break;
	}

	ImGuiStyle &style = ImGui::GetStyle();
	style.FrameRounding = cfg.frameRounding;
	// Make `GrabRounding` always the same value as `FrameRounding`
	style.GrabRounding = style.FrameRounding;
	style.WindowBorderSize = cfg.windowBorder ? 1.0f : 0.0f;
	style.FrameBorderSize = cfg.frameBorder ? 1.0f : 0.0f;
	style.PopupBorderSize = cfg.popupBorder ? 1.0f : 0.0f;
	ImGui::GetIO().FontGlobalScale = cfg.scaling;
}
