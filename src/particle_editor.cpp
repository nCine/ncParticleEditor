#include <ncine/config.h>
#if !NCINE_WITH_LUA
	#error nCine must have Lua integration enabled for this application to work
#endif
#if !NCINE_WITH_IMGUI
	#error nCine must have ImGui integration enabled for this application to work
#endif

#include "particle_editor.h"
#include "particle_editor_lua.h"

#include <ncine/Application.h>
#include <ncine/Texture.h>
#include <ncine/Sprite.h>
#include <ncine/ParticleSystem.h>
#include <ncine/IInputManager.h>
#include <ncine/FileSystem.h>

#ifdef WITH_CRASHRPT
	#include "CrashRptWrapper.h"
#endif

namespace {

const char *ConfigFile = "config.lua";

}

nc::IAppEventHandler *createAppEventHandler()
{
	return new MyEventHandler;
}

MyEventHandler::MyEventHandler()
    : loader_(nctl::makeUnique<LuaLoader>()),
      sysStates_(4), texNames_(4),
      textures_(4), texturesToDelete_(4),
      particleSystems_(4)
{
	nc::IInputManager::setHandler(this);
}

void MyEventHandler::onPreInit(nc::AppConfiguration &config)
{
#ifdef WITH_CRASHRPT
	CrashRptWrapper::install();
#endif

#if defined(__ANDROID__)
	config.dataPath() = "asset::";
#elif defined(__EMSCRIPTEN__)
	config.dataPath() = "/";
#else
	#ifdef PACKAGE_DEFAULT_DATA_DIR
	config.dataPath() = PACKAGE_DEFAULT_DATA_DIR;
	#else
	config.dataPath() = "data/";
	#endif
#endif

	configFile_ = ConfigFile;
	if (nc::fs::isReadableFile(configFile_.data()) == false)
	{
		logString_.formatAppend("Config file \"%s\" is not accessible or does not exist\n", configFile_.data());
		configFile_ = nc::fs::joinPath(nc::fs::dataPath(), ConfigFile);
	}

	LuaLoader::Config &luaConfig = loader_->config();
	if (nc::fs::isReadableFile(configFile_.data()))
	{
		if (loader_->loadConfig(configFile_.data()))
			logString_.formatAppend("Loaded config file \"%s\"\n", configFile_.data());
		else
			logString_.formatAppend("Could not load config file \"%s\"\n", configFile_.data());
	}
	else
	{
		logString_.formatAppend("Config file \"%s\" is not accessible or does not exist\n", configFile_.data());
		configFile_ = ConfigFile;
	}

	if (logString_.capacity() < luaConfig.logMaxSize)
	{
		nctl::String temp = nctl::String(luaConfig.logMaxSize);
		temp.assign(logString_);
		logString_ = nctl::move(temp);
	}

	if (nc::fs::isDirectory(luaConfig.scriptsPath.data()) == false)
		luaConfig.scriptsPath = nc::fs::joinPath(nc::fs::dataPath(), "scripts");
	if (nc::fs::isDirectory(luaConfig.texturesPath.data()) == false)
		luaConfig.texturesPath = nc::fs::joinPath(nc::fs::dataPath(), "textures");
	if (nc::fs::isDirectory(luaConfig.backgroundsPath.data()) == false)
		luaConfig.backgroundsPath = nc::fs::joinPath(nc::fs::dataPath(), "backgrounds");

	config.resolution.set(luaConfig.width, luaConfig.height);
	config.inFullscreen = luaConfig.fullscreen;
	config.isResizable = luaConfig.resizable;
	config.frameLimit = luaConfig.frameLimit;
	config.useBufferMapping = luaConfig.useBufferMapping;
	config.vboSize = luaConfig.vboSize;
	config.iboSize = luaConfig.iboSize;
	config.withVSync = luaConfig.withVSync;

	config.windowTitle = "ncParticleEditor";
	config.windowIconFilename = "icon48.png";
}

void MyEventHandler::onInit()
{
#ifdef WITH_CRASHRPT
	// CrashRptWrapper::emulateCrash();
#endif

	backgroundImagePosition_.set(nc::theApplication().width() * 0.5f, nc::theApplication().height() * 0.5f);
	parentPosition_.set(nc::theApplication().width() * 0.5f, nc::theApplication().height() * 0.5f);
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	dummy_ = nctl::makeUnique<nc::SceneNode>(&rootNode, parentPosition_.x, parentPosition_.y);

	const LuaLoader::Config &luaConfig = loader_->config();
	if (luaConfig.startupScriptName.isEmpty() == false)
	{
		const nctl::String startupScript = luaConfig.scriptsPath + luaConfig.startupScriptName;
		if (nc::fs::isReadableFile(startupScript.data()))
		{
			load(startupScript.data());
			filename_ = luaConfig.startupScriptName;
			pushRecentFile(filename_);
		}
	}
	autoEmission_ = luaConfig.autoEmissionOnStart;

	configureGui();

#ifdef __EMSCRIPTEN__
	loader_->localFileLoad.setLoadedCallback([](const nc::EmscriptenLocalFile &localFile, void *userData) {
		MyEventHandler *eventHandler = reinterpret_cast<MyEventHandler *>(userData);
		if (localFile.size() > 0)
			eventHandler->load(localFile.filename(), &localFile);
	}, this);

	loader_->localFileLoadConfig.setLoadedCallback([](const nc::EmscriptenLocalFile &localFile, void *userData) {
		LuaLoader *loader = reinterpret_cast<LuaLoader *>(userData);
		if (localFile.size() > 0)
			loader->loadConfig(localFile.filename(), &localFile);
	}, loader_.get());
#endif
}

void MyEventHandler::onFrameStart()
{
	applyConfig();
	deleteUnusedTextures();

	createGuiMainWindow();
	createGuiConfigWindow();
	createGuiLogWindow();

	if (autoEmission_)
		emitParticles();
}

void MyEventHandler::onShutdown()
{
#ifdef WITH_CRASHRPT
	CrashRptWrapper::uninstall();
#endif
}

void MyEventHandler::onKeyPressed(const nc::KeyboardEvent &event)
{
	if (event.mod & nc::KeyMod::CTRL)
	{
		if (event.sym == nc::KeySym::N1)
			showMainWindow_ = !showMainWindow_;
		else if (event.sym == nc::KeySym::N2)
			showConfigWindow_ = !showConfigWindow_;
		else if (event.sym == nc::KeySym::N3)
			showLogWindow_ = !showLogWindow_;
		else if (event.sym == nc::KeySym::N)
		{
			if (menuNewEnabled())
				menuNew();
		}
		else if (event.sym == nc::KeySym::O)
			menuOpen();
		else if (event.sym == nc::KeySym::S)
		{
			if (menuSaveEnabled())
				menuSave();
		}
		else if (event.sym == nc::KeySym::Q)
			menuQuit();
	}
	else if (event.sym == nc::KeySym::ESCAPE)
		closeModalsAndAbout();
}

void MyEventHandler::emitParticles(unsigned int index)
{
	nc::ParticleSystem *particleSystem = particleSystems_[index].get();
	ParticleSystemGuiState &s = sysStates_[index];

	if (s.active && (s.emitDelay == 0.0f || (s.emitDelay > 0.0f && s.lastEmissionTime.secondsSince() > s.emitDelay)))
	{
		particleSystem->emitParticles(s.init);
		s.lastEmissionTime = nc::TimeStamp::now();
	}
}

void MyEventHandler::emitParticles()
{
	for (unsigned int i = 0; i < particleSystems_.size(); i++)
		emitParticles(i);
}

void MyEventHandler::killParticles(unsigned int index)
{
	nc::ParticleSystem *particleSystem = particleSystems_[index].get();
	ParticleSystemGuiState &s = sysStates_[index];

	particleSystem->killParticles();
	s.lastEmissionTime = nc::TimeStamp::now();
}

void MyEventHandler::killParticles()
{
	for (unsigned int i = 0; i < particleSystems_.size(); i++)
		killParticles(i);
}

bool MyEventHandler::load(const char *filename)
#ifdef __EMSCRIPTEN__
{
	return load(filename, nullptr);
}

bool MyEventHandler::load(const char *filename, const nc::EmscriptenLocalFile *localFile)
#endif
{
	LuaLoader::State loaderState;
#ifndef __EMSCRIPTEN__
	if (loader_->load(filename, loaderState) == false)
#else
	if (loader_->load(filename, loaderState, localFile) == false)
#endif
	{
		logString_.formatAppend("Could not load project file \"%s\"\n", filename);
		return false;
	}

	if (loaderState.systems.isEmpty())
		return false;
	else
		clearData();

	background_ = loaderState.background.color;
	nc::theApplication().gfxDevice().setClearColor(background_);

	backgroundImageName_ = loaderState.background.imageName;
	backgroundImagePosition_ = loaderState.background.imageNormalizedPosition * nc::Vector2f(nc::theApplication().width(), nc::theApplication().height());
	backgroundImageScale_ = loaderState.background.imageScale;
	backgroundImageScaleLock_ = (loaderState.background.imageScale.x == loaderState.background.imageScale.y);
	backgroundImageLayer_ = loaderState.background.imageLayer;
	backgroundImageColor_ = loaderState.background.imageColor;
	backgroundImageRect_ = loaderState.background.imageRect;
	backgroundImageFlippedX = loaderState.background.imageFlippedX;
	backgroundImageFlippedY = loaderState.background.imageFlippedY;
	if (backgroundImageName_.isEmpty() == false)
	{
		loadBackgroundImage(backgroundImageName_);
		applyBackgroundImageProperties();
	}
	else
		deleteBackgroundImage();

	parentPosition_ = loaderState.normalizedAbsPosition * nc::Vector2f(nc::theApplication().width(), nc::theApplication().height());
	dummy_->setPosition(parentPosition_);

	for (int systemIndex = 0; systemIndex < loaderState.systems.size(); systemIndex++)
	{
		LuaLoader::State::ParticleSystem &src = loaderState.systems[systemIndex];
		sysStates_[systemIndex] = {};
		ParticleSystemGuiState &dest = sysStates_[systemIndex];

		dest.name = src.name;
		dest.numParticles = src.numParticles;
		dest.texRect = src.texRect;
		dest.anchorPoint = src.anchorPoint;
		dest.flippedX = src.flippedX;
		dest.flippedY = src.flippedY;
		dest.blendingPreset = src.blendingPreset;

		for (unsigned int texIndex = 0; texIndex < texNames_.size(); texIndex++)
		{
			if (texNames_[texIndex] == src.textureName)
			{
				dest.texture = textures_[texIndex].get();
				nctl::String texName = src.textureName;
				break;
			}
		}
		if (dest.texture == nullptr)
		{
			texIndex_ = textures_.size();
			texNames_[texIndex_] = nctl::String(MaxStringLength);
			texNames_[texIndex_] = src.textureName;
			const bool result = createTexture(texIndex_);
			if (result == false)
				return false;
			dest.texture = textures_[texIndex_].get();
		}

		dest.position = src.position;
		dest.layer = src.layer;
		dest.inLocalSpace = src.inLocalSpace;
		dest.active = src.active;

		createParticleSystem(systemIndex);

		for (unsigned int i = 0; i < src.colorSteps.size(); i++)
		{
			const LuaLoader::State::ColorStep &step = src.colorSteps[i];
			dest.colorAffector->addColorStep(step.age, step.color);
		}

		dest.baseScale = src.sizeStepBaseScale;
		dest.baseScaleLock = (dest.baseScale.x == dest.baseScale.y);
		dest.sizeAffector->setBaseScale(src.sizeStepBaseScale);
		for (unsigned int i = 0; i < src.sizeSteps.size(); i++)
		{
			const LuaLoader::State::SizeStep &step = src.sizeSteps[i];
			dest.sizeAffector->addSizeStep(step.age, step.scale);
		}

		for (unsigned int i = 0; i < src.rotationSteps.size(); i++)
		{
			const LuaLoader::State::RotationStep &step = src.rotationSteps[i];
			dest.rotationAffector->addRotationStep(step.age, step.angle);
		}

		for (unsigned int i = 0; i < src.positionSteps.size(); i++)
		{
			const LuaLoader::State::PositionStep &step = src.positionSteps[i];
			dest.positionAffector->addPositionStep(step.age, step.position);
		}

		for (unsigned int i = 0; i < src.velocitySteps.size(); i++)
		{
			const LuaLoader::State::VelocityStep &step = src.velocitySteps[i];
			dest.velocityAffector->addVelocityStep(step.age, step.velocity);
		}

		dest.init = src.init;
		dest.emitDelay = src.emitDelay;
		dest.lastEmissionTime = nc::TimeStamp::now();

		if (dest.init.emitterRotation)
			dest.rotationCurrentItem = 0;

		if (systemIndex == 0)
		{
			spriteState_.texture = dest.texture;
			spriteState_.texRect = dest.texRect;
			spriteState_.anchorPoint = dest.anchorPoint;
			spriteState_.flippedX = dest.flippedX;
			spriteState_.flippedY = dest.flippedY;
			spriteState_.blendingPreset = dest.blendingPreset;
		}
	}

	logString_.formatAppend("Loaded project file \"%s\"\n", filename);
	return true;
}

void MyEventHandler::save(const char *filename)
{
	LuaLoader::State loaderState;

	loaderState.background.color = background_;
	loaderState.background.imageName = backgroundImageName_;
	loaderState.background.imageNormalizedPosition.x = backgroundImagePosition_.x / nc::theApplication().width();
	loaderState.background.imageNormalizedPosition.y = backgroundImagePosition_.y / nc::theApplication().height();
	loaderState.background.imageScale = backgroundImageScale_;
	loaderState.background.imageLayer = backgroundImageLayer_;
	loaderState.background.imageColor = backgroundImageColor_;
	loaderState.background.imageRect = backgroundImageRect_;
	loaderState.background.imageFlippedX = backgroundImageFlippedX;
	loaderState.background.imageFlippedY = backgroundImageFlippedY;

	loaderState.normalizedAbsPosition.x = parentPosition_.x / nc::theApplication().width();
	loaderState.normalizedAbsPosition.y = parentPosition_.y / nc::theApplication().height();

	for (unsigned int index = 0; index < sysStates_.size(); index++)
	{
		const ParticleSystemGuiState &src = sysStates_[index];
		LuaLoader::State::ParticleSystem dest;

		nctl::String *textureName = nullptr;
		for (unsigned int i = 0; i < textures_.size(); i++)
		{
			if (textures_[i].get() == src.texture)
			{
				textureName = &texNames_[i];
				break;
			}
		}
		FATAL_ASSERT(textureName != nullptr);

		dest.name = src.name;
		dest.numParticles = src.numParticles;
		dest.textureName = *textureName;
		dest.texRect = src.texRect;
		dest.anchorPoint = src.anchorPoint;
		dest.flippedX = src.flippedX;
		dest.flippedY = src.flippedY;
		dest.blendingPreset = src.blendingPreset;
		dest.position = src.position;
		dest.layer = src.layer;
		dest.inLocalSpace = src.inLocalSpace;
		dest.active = src.active;

		if (src.colorAffector->steps().isEmpty() == false)
		{
			for (unsigned int i = 0; i < src.colorAffector->steps().size(); i++)
			{
				const nc::ColorAffector::ColorStep &srcStep = src.colorAffector->steps()[i];
				LuaLoader::State::ColorStep destStep;
				destStep.age = srcStep.age;
				destStep.color = srcStep.color;
				dest.colorSteps.pushBack(destStep);
			}
		}

		if (src.sizeAffector->steps().isEmpty() == false)
		{
			dest.sizeStepBaseScale = src.sizeAffector->baseScale();
			for (unsigned int i = 0; i < src.sizeAffector->steps().size(); i++)
			{
				const nc::SizeAffector::SizeStep &srcStep = src.sizeAffector->steps()[i];
				LuaLoader::State::SizeStep destStep;
				destStep.age = srcStep.age;
				destStep.scale = srcStep.scale;
				dest.sizeSteps.pushBack(destStep);
			}
		}

		if (src.rotationAffector->steps().isEmpty() == false)
		{
			for (unsigned int i = 0; i < src.rotationAffector->steps().size(); i++)
			{
				const nc::RotationAffector::RotationStep &srcStep = src.rotationAffector->steps()[i];
				LuaLoader::State::RotationStep destStep;
				destStep.age = srcStep.age;
				destStep.angle = srcStep.angle;
				dest.rotationSteps.pushBack(destStep);
			}
		}

		if (src.positionAffector->steps().isEmpty() == false)
		{
			for (unsigned int i = 0; i < src.positionAffector->steps().size(); i++)
			{
				const nc::PositionAffector::PositionStep &srcStep = src.positionAffector->steps()[i];
				LuaLoader::State::PositionStep destStep;
				destStep.age = srcStep.age;
				destStep.position = srcStep.position;
				dest.positionSteps.pushBack(destStep);
			}
		}

		if (src.velocityAffector->steps().isEmpty() == false)
		{
			for (unsigned int i = 0; i < src.velocityAffector->steps().size(); i++)
			{
				const nc::VelocityAffector::VelocityStep &srcStep = src.velocityAffector->steps()[i];
				LuaLoader::State::VelocityStep destStep;
				destStep.age = srcStep.age;
				destStep.velocity = srcStep.velocity;
				dest.velocitySteps.pushBack(destStep);
			}
		}

		dest.init = src.init;
		dest.emitDelay = src.emitDelay;
		loaderState.systems.pushBack(dest);
	}

	loader_->save(filename, loaderState);
	logString_.formatAppend("Saved project file \"%s\"\n", filename);
}

void MyEventHandler::applyConfig()
{
	const LuaLoader::Config &cfg = loader_->config();
	nc::Application::RenderingSettings &settings = nc::theApplication().renderingSettings();
	settings.batchingEnabled = cfg.batching;
	settings.cullingEnabled = cfg.culling;

	applyGuiStyleConfig();
}

void MyEventHandler::clearData()
{
	for (unsigned int i = 0; i < textures_.size(); i++)
		texturesToDelete_.pushBack(nctl::move(textures_[i]));
	textures_.clear();
	texNames_.clear();

	for (unsigned int i = 0; i < particleSystems_.size(); i++)
		particleSystems_[i].reset(nullptr);
	particleSystems_.clear();
	sysStates_.clear();

	logString_.formatAppend("Destroyed all textures and particle systems\n");
}

void MyEventHandler::pushRecentFile(const nctl::String &filename)
{
	int i = recentFileIndexStart_;
	while (i != recentFileIndexEnd_)
	{
		if (recentFilenames_[i] == filename)
			return;
		i = (i + 1) % MaxRecentFiles;
	}

	recentFilenames_[recentFileIndexEnd_] = filename;
	recentFileIndexEnd_ = (recentFileIndexEnd_ + 1) % MaxRecentFiles;
	if (recentFileIndexEnd_ == recentFileIndexStart_)
		recentFileIndexStart_ = (recentFileIndexStart_ + 1) % MaxRecentFiles;
}

bool MyEventHandler::loadBackgroundImage(const nctl::String &filename)
{
	const LuaLoader::Config &luaConfig = loader_->config();

	nctl::String filepath(MaxStringLength);
	filepath = filename;
	if (nc::fs::isReadableFile(filepath.data()) == false)
		filepath = nc::fs::joinPath(luaConfig.backgroundsPath, filename);

	if (nc::fs::isReadableFile(filepath.data()))
	{
		backgroundTexture_ = nctl::makeUnique<nc::Texture>(filepath.data());
		if (backgroundSprite_ == nullptr)
			backgroundSprite_ = nctl::makeUnique<nc::Sprite>(&nc::theApplication().rootNode(), backgroundTexture_.get());
		else
			backgroundSprite_->setTexture(backgroundTexture_.get());
		logString_.formatAppend("Loaded background image \"%s\"\n", filepath.data());
		return true;
	}

	logString_.formatAppend("Cannot load background image \"%s\"\n", filepath.data());
	return false;
}

void MyEventHandler::deleteBackgroundImage()
{
	if (backgroundSprite_ || backgroundTexture_)
	{
		backgroundSprite_.reset(nullptr);
		backgroundTexture_.reset(nullptr);
		logString_.formatAppend("Background image destroyed\n");
	}
}

bool MyEventHandler::applyBackgroundImageProperties()
{
	if (backgroundSprite_)
	{
		backgroundSprite_->setPosition(backgroundImagePosition_);
		backgroundSprite_->setScale(backgroundImageScale_);
		backgroundSprite_->setLayer(static_cast<unsigned short>(backgroundImageLayer_));
		backgroundSprite_->setColor(backgroundImageColor_);
		backgroundSprite_->setTexRect(backgroundImageRect_);
		backgroundSprite_->setFlippedX(backgroundImageFlippedX);
		backgroundSprite_->setFlippedY(backgroundImageFlippedY);
	}

	return (backgroundSprite_ != nullptr);
}

unsigned int MyEventHandler::retrieveTexture(unsigned int particleSystemIndex)
{
	unsigned int index = 0;
	for (unsigned int i = 0; i < textures_.size(); i++)
	{
		if (textures_[i].get() == sysStates_[particleSystemIndex].texture)
		{
			index = i;
			break;
		}
	}
	return index;
}

bool MyEventHandler::createTexture(unsigned int index)
{
	const LuaLoader::Config &luaConfig = loader_->config();

	nctl::String filepath(MaxStringLength);
	filepath = texNames_[index];
	if (nc::fs::isReadableFile(filepath.data()) == false)
		filepath = nc::fs::joinPath(luaConfig.texturesPath, texNames_[index]);

	if (nc::fs::isReadableFile(filepath.data()))
	{
		textures_[index] = nctl::makeUnique<nc::Texture>(filepath.data());
		logString_.formatAppend("Loaded texture \"%s\" at index #%u\n", filepath.data(), index);
		return true;
	}

	logString_.formatAppend("Cannot load texture \"%s\" at index #%u\n", filepath.data(), index);
	return false;
}

void MyEventHandler::destroyTexture(unsigned int index)
{
	texturesToDelete_.pushBack(nctl::move(textures_[index]));
	for (unsigned int i = index; i < textures_.size() - 1; i++)
		textures_[i] = nctl::move(textures_[i + 1]);
	textures_.setSize(textures_.size() - 1);

	for (unsigned int i = index; i < texNames_.size() - 1; i++)
		texNames_[i] = texNames_[i + 1];
	texNames_.setSize(texNames_.size() - 1);

	logString_.formatAppend("Destroyed texture at index #%u\n", index);
}

void MyEventHandler::deleteUnusedTextures()
{
	for (nctl::UniquePtr<nc::Texture> &texture : texturesToDelete_)
		texture.reset(nullptr);
	texturesToDelete_.clear();
}

void MyEventHandler::createParticleSystem(unsigned int index)
{
	ParticleSystemGuiState &s = sysStates_[index];

	nc::Texture *texture = textures_[texIndex_].get();
	const nc::Recti texRect(0, 0, texture->width(), texture->height());
	particleSystems_[index] = nctl::makeUnique<nc::ParticleSystem>(dummy_.get(), unsigned(s.numParticles), texture, texRect);
	particleSystems_[index]->setPosition(s.position);
	particleSystems_[index]->setLayer(static_cast<unsigned short>(s.layer));
	particleSystems_[index]->setInLocalSpace(s.inLocalSpace);

	nctl::UniquePtr<nc::ColorAffector> colAffector = nctl::makeUnique<nc::ColorAffector>();
	s.colorAffector = colAffector.get();
	particleSystems_[index]->addAffector(nctl::move(colAffector));

	nctl::UniquePtr<nc::SizeAffector> sizeAffector = nctl::makeUnique<nc::SizeAffector>();
	s.sizeAffector = sizeAffector.get();
	particleSystems_[index]->addAffector(nctl::move(sizeAffector));

	nctl::UniquePtr<nc::RotationAffector> rotAffector = nctl::makeUnique<nc::RotationAffector>();
	s.rotationAffector = rotAffector.get();
	particleSystems_[index]->addAffector(nctl::move(rotAffector));

	nctl::UniquePtr<nc::PositionAffector> posAffector = nctl::makeUnique<nc::PositionAffector>();
	s.positionAffector = posAffector.get();
	particleSystems_[index]->addAffector(nctl::move(posAffector));

	nctl::UniquePtr<nc::VelocityAffector> velAffector = nctl::makeUnique<nc::VelocityAffector>();
	s.velocityAffector = velAffector.get();
	particleSystems_[index]->addAffector(nctl::move(velAffector));

	logString_.formatAppend("Created a new particle system at index #%u\n", index);
}

void MyEventHandler::cloneParticleSystem(unsigned int srcIndex, unsigned int destIndex, unsigned int numParticles)
{
	// Get the destination first to let the array extend to a new capacity before getting the source
	ParticleSystemGuiState &dest = sysStates_[destIndex];
	const ParticleSystemGuiState &src = sysStates_[srcIndex];

	dest.name = src.name;
	dest.active = src.active;

	dest.texture = src.texture;
	dest.texRect = src.texRect;
	particleSystems_[destIndex] = nctl::makeUnique<nc::ParticleSystem>(dummy_.get(), numParticles, dest.texture, dest.texRect);
	dest.position = src.position;
	particleSystems_[destIndex]->setPosition(dest.position);
	dest.layer = src.layer;
	particleSystems_[destIndex]->setLayer(static_cast<unsigned short>(dest.layer));
	dest.inLocalSpace = src.inLocalSpace;
	particleSystems_[destIndex]->setInLocalSpace(dest.inLocalSpace);
	dest.anchorPoint = src.anchorPoint;
	particleSystems_[destIndex]->setAnchorPoint(dest.anchorPoint);
	dest.flippedX = src.flippedX;
	particleSystems_[destIndex]->setFlippedX(dest.flippedX);
	dest.flippedY = src.flippedY;
	particleSystems_[destIndex]->setFlippedY(dest.flippedY);
	dest.blendingPreset = src.blendingPreset;
	particleSystems_[destIndex]->setBlendingPreset(dest.blendingPreset);

	nctl::UniquePtr<nc::ColorAffector> colAffector = nctl::makeUnique<nc::ColorAffector>();
	dest.colorAffector = colAffector.get();
	particleSystems_[destIndex]->addAffector(nctl::move(colAffector));
	for (unsigned int i = 0; i < src.colorAffector->steps().size(); i++)
		dest.colorAffector->steps()[i] = src.colorAffector->steps()[i];

	nctl::UniquePtr<nc::SizeAffector> sizeAffector = nctl::makeUnique<nc::SizeAffector>();
	dest.sizeAffector = sizeAffector.get();
	particleSystems_[destIndex]->addAffector(nctl::move(sizeAffector));
	dest.sizeAffector->setBaseScale(src.sizeAffector->baseScale());
	dest.baseScaleLock = src.baseScaleLock;
	dest.sizeValueLock = src.sizeValueLock;
	for (unsigned int i = 0; i < src.sizeAffector->steps().size(); i++)
		dest.sizeAffector->steps()[i] = src.sizeAffector->steps()[i];

	nctl::UniquePtr<nc::RotationAffector> rotAffector = nctl::makeUnique<nc::RotationAffector>();
	dest.rotationAffector = rotAffector.get();
	particleSystems_[destIndex]->addAffector(nctl::move(rotAffector));
	for (unsigned int i = 0; i < src.rotationAffector->steps().size(); i++)
		dest.rotationAffector->steps()[i] = src.rotationAffector->steps()[i];

	nctl::UniquePtr<nc::PositionAffector> posAffector = nctl::makeUnique<nc::PositionAffector>();
	dest.positionAffector = posAffector.get();
	particleSystems_[destIndex]->addAffector(nctl::move(posAffector));
	for (unsigned int i = 0; i < src.positionAffector->steps().size(); i++)
		dest.positionAffector->steps()[i] = src.positionAffector->steps()[i];

	nctl::UniquePtr<nc::VelocityAffector> velAffector = nctl::makeUnique<nc::VelocityAffector>();
	dest.velocityAffector = velAffector.get();
	particleSystems_[destIndex]->addAffector(nctl::move(velAffector));
	for (unsigned int i = 0; i < src.velocityAffector->steps().size(); i++)
		dest.velocityAffector->steps()[i] = src.velocityAffector->steps()[i];

	dest.init = src.init;
	dest.emitDelay = src.emitDelay;

	logString_.formatAppend("Cloned particle system at index #%u to index #%u\n", srcIndex, destIndex);
}

void MyEventHandler::destroyParticleSystem(unsigned int index)
{
	particleSystems_[index].reset(nullptr);
	for (unsigned int i = index; i < particleSystems_.size() - 1; i++)
		particleSystems_[i] = nctl::move(particleSystems_[i + 1]);
	particleSystems_.setSize(particleSystems_.size() - 1);

	for (unsigned int i = index; i < sysStates_.size() - 1; i++)
		sysStates_[i] = sysStates_[i + 1];
	sysStates_.setSize(sysStates_.size() - 1);

	logString_.formatAppend("Destroyed particle system at index #%u\n", index);
}
