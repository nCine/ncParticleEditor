#include "particle_editor.h"
#include "particle_editor_lua.h"

#include "Application.h"
#include "Texture.h"
#include "Sprite.h"
#include "ParticleSystem.h"
#include "IInputManager.h"
#include "IFile.h"

namespace {

const char *ScriptFile = "particles.lua";

}

nc::IAppEventHandler *createAppEventHandler()
{
	return new MyEventHandler;
}

MyEventHandler::MyEventHandler()
	: loader_(nctl::makeUnique<LuaLoader>()),
	  sysStates_(4), texStates_(4),
	  textures_(4), texturesToDelete_(4),
	  particleSystems_(4)
{
	nc::IInputManager::setHandler(this);
}

void MyEventHandler::onPreInit(nc::AppConfiguration &config)
{
#ifdef __ANDROID__
	config.setDataPath("/sdcard/ncparticleeditor/");
#else
	#ifdef PACKAGE_DEFAULT_DATA_DIR
	config.setDataPath(PACKAGE_DEFAULT_DATA_DIR);
	#else
	config.setDataPath("data/");
	#endif
#endif

	configFile_ = "config.lua";
	if (nc::IFile::access(configFile_.data(), nc::IFile::AccessMode::READABLE) == false)
		configFile_ = nc::IFile::dataPath() +  "config.lua";

	scriptsPath_ = "scripts/";
	if (nc::IFile::access(scriptsPath_.data(), nc::IFile::AccessMode::READABLE) == false)
		scriptsPath_ = nc::IFile::dataPath() + "scripts/";

	texturesPath_ = "textures/";
	if (nc::IFile::access(texturesPath_.data(), nc::IFile::AccessMode::READABLE) == false)
		texturesPath_ = nc::IFile::dataPath() + "textures/";

	if (nc::IFile::access(configFile_.data(), nc::IFile::AccessMode::READABLE))
	{
		LuaLoader::Config &luaConfig = loader_->config();
		luaConfig.width = config.xResolution();
		luaConfig.height = config.yResolution();
		luaConfig.fullscreen = config.inFullscreen();
		luaConfig.vboSize = config.vboSize();
		luaConfig.iboSize = config.iboSize();
		luaConfig.batching = true;
		luaConfig.culling = true;

		if (loader_->loadConfig(configFile_.data()))
		{
			config.setResolution(luaConfig.width, luaConfig.height);
			config.setFullScreen(luaConfig.fullscreen);
			config.setVboSize(luaConfig.vboSize);
			config.setIboSize(luaConfig.iboSize);
			logString_.formatAppend("Loaded config file \"%s\"\n", configFile_.data());
		}
		else
			logString_.formatAppend("Could not load config file \"%s\"\n", configFile_.data());
	}

	const unsigned long MinimumVboSize = 256 * 1024;
	if (config.vboSize() < MinimumVboSize)
	{
		logString_.formatAppend("The required VBO size of %lu is too low and it has been raised to %lu bytes\n", config.vboSize(), MinimumVboSize);
		config.setVboSize(MinimumVboSize);
	}

	const unsigned long MinimumIboSize = 32 * 1024;
	if (config.iboSize() < MinimumIboSize)
	{
		logString_.formatAppend("The required IBO size of %lu is too low and it has been raised to %lu bytes\n", config.iboSize(), MinimumIboSize);
		config.setIboSize(MinimumIboSize);
	}

	config.setWindowTitle("ncParticleEditor");
	config.setWindowIconFilename("icon48.png");
}

void MyEventHandler::onInit()
{
	parentPosition_.set(nc::theApplication().width() * 0.5f, nc::theApplication().height() * 0.5f);
	nc::SceneNode &rootNode = nc::theApplication().rootNode();
	dummy_ = nctl::makeUnique<nc::SceneNode>(&rootNode, parentPosition_.x, parentPosition_.y);

	nctl::String initialScript = scriptsPath_ + ScriptFile;
	if (nc::IFile::access(initialScript.data(), nc::IFile::AccessMode::READABLE))
		load(initialScript.data());

	configureGui();
}

void MyEventHandler::onFrameStart()
{
	applyConfig();
	deleteUnusedTextures();
	createGui();

	if (autoEmission_)
		emitParticles();
}

void MyEventHandler::onKeyPressed(const nc::KeyboardEvent &event)
{
	if (event.mod & nc::KeyMod::CTRL)
	{
		if (event.sym == nc::KeySym::N)
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

	if (s.active && (s.emitDelay == 0.0f || (s.emitDelay > 0.0f && s.emitTimer.interval() > s.emitDelay)))
	{
		particleSystem->emitParticles(s.init);
		s.emitTimer.start();
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
	s.emitTimer.start();
}

void MyEventHandler::killParticles()
{
	for (unsigned int i = 0; i < particleSystems_.size(); i++)
		killParticles(i);
}

bool MyEventHandler::load(const char *filename)
{
	LuaLoader::State loaderState;
	if (loader_->load(filename, loaderState) == false)
	{
		logString_.formatAppend("Could not load project file \"%s\"\n", filename);
		return false;
	}

	if (loaderState.systems.isEmpty())
		return false;
	else
		clearData();

	background_ = loaderState.background;
	nc::theApplication().gfxDevice().setClearColor(background_);

	backgroundImageName_ = loaderState.backgroundImageName;
	backgroundImagePosition_ = loaderState.backgroundImageNormalizedPosition * nc::Vector2f(nc::theApplication().width(), nc::theApplication().height());
	backgroundImageScale_ = loaderState.backgroundImageScale;
	backgroundImageLayer_ = loaderState.backgroundImageLayer;
	if (backgroundImageName_.isEmpty() == false)
	{
		loadBackgroundImage(backgroundImageName_);
		if (backgroundSprite_)
		{
			backgroundSprite_->setPosition(backgroundImagePosition_);
			backgroundSprite_->setScale(backgroundImageScale_);
			backgroundSprite_->setLayer(backgroundImageLayer_);
		}
	}
	else
		deleteBackgroundImage();

	parentPosition_ = loaderState.normalizedAbsPosition * nc::Vector2f(nc::theApplication().width(), nc::theApplication().height());
	dummy_->setPosition(parentPosition_);

	for (int systemIndex = 0; systemIndex < loaderState.systems.size(); systemIndex++)
	{
		LuaLoader::State::ParticleSystem &src = loaderState.systems[systemIndex];
		sysStates_[systemIndex] = { };
		ParticleSystemGuiState &dest = sysStates_[systemIndex];

		dest.name = src.name;
		dest.numParticles = src.numParticles;
		dest.texRect = src.texRect;

		for (unsigned int texIndex = 0; texIndex < texStates_.size(); texIndex++)
		{
			if (texStates_[texIndex].name == src.textureName)
			{
				dest.texture = textures_[texIndex].get();
				TextureGuiState &texState = texStates_[texIndex];
				texState.name = src.textureName;
				texState.texRect = dest.texRect;
				break;
			}
		}
		if (dest.texture == nullptr)
		{
			texIndex_ = textures_.size();
			texStates_[texIndex_] = { };
			TextureGuiState &texState = texStates_[texIndex_];
			texState.name = src.textureName;
			const bool result = createTexture(texIndex_);
			if (result == false)
				return false;
			texState.texRect = dest.texRect;
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
		dest.emitTimer.start();

		if (dest.init.emitterRotation)
			dest.rotationCurrentItem = 0;
	}

	logString_.formatAppend("Loaded project file \"%s\"\n", filename);
	return true;
}

void MyEventHandler::save(const char *filename)
{
	LuaLoader::State loaderState;

	loaderState.background = background_;
	loaderState.backgroundImageName = backgroundImageName_;
	loaderState.backgroundImageNormalizedPosition.x = backgroundImagePosition_.x / nc::theApplication().width();
	loaderState.backgroundImageNormalizedPosition.y = backgroundImagePosition_.y / nc::theApplication().height();
	loaderState.backgroundImageScale = backgroundImageScale_;
	loaderState.backgroundImageLayer = backgroundImageLayer_;

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
				textureName = &texStates_[i].name;
				break;
			}
		}
		FATAL_ASSERT(textureName != nullptr);

		dest.name = src.name;
		dest.numParticles = src.numParticles;
		dest.textureName = *textureName;
		dest.texRect = src.texRect;
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
	if (loader_->configLoaded())
	{
		const LuaLoader::Config &cfg = loader_->config();
		nc::Application::RenderingSettings &settings = nc::theApplication().renderingSettings();
		settings.batchingEnabled = cfg.batching;
		settings.cullingEnabled = cfg.culling;
	}
}

void MyEventHandler::clearData()
{
	for (unsigned int i = 0; i < textures_.size(); i++)
		texturesToDelete_.pushBack(nctl::move(textures_[i]));
	textures_.clear();
	texStates_.clear();

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
		i++;
	}

	recentFilenames_[recentFileIndexEnd_] = filename;
	recentFileIndexEnd_ = (recentFileIndexEnd_ + 1) % MaxRecentFiles;
	if (recentFileIndexEnd_ == recentFileIndexStart_)
		recentFileIndexStart_ = (recentFileIndexStart_ + 1) % MaxRecentFiles;
}

bool MyEventHandler::loadBackgroundImage(const nctl::String &filename)
{
	nctl::String filepath(texturesPath_ + filename);

	if (filename.isEmpty() == false && nc::IFile::access(filepath.data(), nc::IFile::AccessMode::READABLE))
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
	TextureGuiState &s = texStates_[index];
	nctl::String filepath(texturesPath_ + s.name);

	if (s.name.isEmpty() == false && nc::IFile::access(filepath.data(), nc::IFile::AccessMode::READABLE))
	{
		textures_[index] = nctl::makeUnique<nc::Texture>(filepath.data());
		s.texRect = textures_[index]->rect();
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

	for (unsigned int i = index; i < texStates_.size() - 1; i++)
		texStates_[i] = texStates_[i + 1];
	texStates_.setSize(texStates_.size() - 1);

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

	s.texture = textures_[texIndex_].get();
	s.texRect = texStates_[texIndex_].texRect;
	particleSystems_[index] = nctl::makeUnique<nc::ParticleSystem>(dummy_.get(), unsigned(s.numParticles), s.texture, s.texRect);
	particleSystems_[index]->setPosition(s.position);
	particleSystems_[index]->setLayer(static_cast<unsigned int>(s.layer));

	nctl::UniquePtr<nc::ColorAffector> colAffector = nctl::makeUnique<nc::ColorAffector>();
	s.colorAffector = colAffector.get();
	particleSystems_[index]->addAffector(nctl::move(colAffector));

	nctl::UniquePtr<nc::SizeAffector> sizeAffector = nctl::makeUnique<nc::SizeAffector>(1.0f);
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
