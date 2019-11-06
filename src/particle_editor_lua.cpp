#include "particle_editor_lua.h"
#include <ncine/Colorf.h>
#include <ncine/LuaStateManager.h>
#include <ncine/LuaUtils.h>
#include <ncine/LuaRectUtils.h>
#include <ncine/LuaVector2Utils.h>
#include <ncine/LuaColorUtils.h>
#include <ncine/IFile.h>

#ifdef __EMSCRIPTEN__
	#include <ncine/EmscriptenLocalFile.h>
#endif

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

namespace {

nctl::String &indent(nctl::String &string, int amount)
{
	FATAL_ASSERT(amount >= 0);
	for (int i = 0; i < amount; i++)
		string.append("\t");

	return string;
}

const unsigned int ProjectFileVersion = 5;
const unsigned int ConfigFileVersion = 11;

namespace Names {

	const char *version = "project_version"; // version 3

	const char *normalizedAbsPosition = "normalized_absolute_position"; // version 2
	const char *particleSystems = "particle_systems";
	const char *backgroundProperties = "background_properties"; // version 4
	const char *backgroundColor = "background_color"; // version 4
	const char *backgroundImage = "background_image"; // version 4
	const char *backgroundImageNormalizedPosition = "background_image_normalized_position"; // version 4
	const char *backgroundImageScale = "background_image_scale"; // version 4
	const char *backgroundImageLayer = "background_image_layer"; // version 4
	const char *backgroundImageColor = "background_image_color"; // version 5
	const char *backgroundImageRect = "background_image_rect"; // version 5

	const char *name = "name"; // version 3
	const char *numParticles = "num_particles";
	const char *texture = "texture";
	const char *texRext = "texture_rect";
	const char *relativePosition = "relative_position";
	const char *layer = "layer";
	const char *inLocalSpace = "local_space";
	const char *active = "active";

	const char *colorSteps = "color_steps";
	const char *sizeSteps = "size_steps";
	const char *baseScale = "base_scale";
	const char *rotationSteps = "rotation_steps";
	const char *positionSteps = "position_steps";
	const char *velocitySteps = "velocity_steps";

	const char *emission = "emission";
	const char *amount = "amount";
	const char *life = "life";
	const char *positionX = "position_x";
	const char *positionY = "position_y";
	const char *velocityX = "velocity_x";
	const char *velocityY = "velocity_y";
	const char *rotation = "rotation";
	const char *emitterRotation = "emitter_rotation";
	const char *delay = "delay";

}

namespace CfgNames {

	const char *version = "config_version"; // version 2
	const char *width = "width";
	const char *height = "height";
	const char *fullscreen = "fullscreen";
	const char *resizable = "resizable"; // version 8
	const char *frameLimit = "frame_limit"; // version 10
	const char *useBufferMapping = "buffer_mapping"; // version 9
	const char *vboSize = "vbo_size";
	const char *iboSize = "ibo_size";
	const char *withVSync = "vsync"; // version 9
	const char *batching = "batching";
	const char *culling = "culling";
	const char *saveFileMaxSize = "savefile_maxsize"; // version 3
	const char *logMaxSize = "log_maxsize"; // version 5
	const char *startupScriptName = "startup_script_name"; // version 11
	const char *autoEmissionOnStart = "auto_emission_on_start"; // version 11

	const char *scriptsPath = "scripts_path"; // version 6
	const char *backgroundsPath = "backgrounds_path"; // version 6
	const char *texturesPath = "textures_path"; // version 6

	const char *guiLimits = "gui_limits"; // version 3
	const char *maxBackgroundImageScale = "max_background_image_scale"; // version 4
	const char *maxRenderingLayer = "max_rendering_layer"; // version 4
	const char *maxNumParticles = "max_num_particles"; // version 3
	const char *systemPositionRange = "system_position_range"; // version 3
	const char *minParticleScale = "min_particle_scale"; // version 3
	const char *maxParticleScale = "max_particle_scale"; // version 3
	const char *minParticleAngle = "min_particle_angle"; // version 3
	const char *maxParticleAngle = "max_particle_angle"; // version 3
	const char *positionRange = "position_range"; // version 3
	const char *velocityRange = "velocity_range"; // version 3
	const char *maxRandomLife = "max_random_life"; // version 3
	const char *randomPositionRange = "random_position_range"; // version 3
	const char *randomVelocityRange = "random_velocity_range"; // version 3
	const char *maxDelay = "max_delay"; // version 3

	const char *guiStyle = "gui_style"; // version 7
	const char *styleIndex = "style_index"; // version 7
	const char *frameRounding = "frame_rounding"; // version 7
	const char *windowBorder = "window_border"; // version 7
	const char *frameBorder = "frame_border"; // version 7
	const char *popupBorder = "popup_border"; // version 7
	const char *scaling = "scaling"; // version 7

}

}

void LuaLoader::sanitizeInitValues()
{
	if (config_.width < 640)
		config_.width = 640;
	if (config_.height < 480)
		config_.height = 480;

	if (config_.frameLimit > 240)
		config_.frameLimit = 240;

	if (config_.vboSize < 256 * 1024)
		config_.vboSize = 256 * 1024;
	if (config_.iboSize < 32 * 1024)
		config_.iboSize = 32 * 1024;

	if (config_.saveFileMaxSize < 8 * 1024)
		config_.saveFileMaxSize = 8 * 1024;
	if (config_.logMaxSize < 4 * 1024)
		config_.logMaxSize = 4 * 1024;
}

void LuaLoader::sanitizeGuiLimits()
{
	if (config_.maxRenderingLayer > 65535)
		config_.maxRenderingLayer = 65535;
	else if (config_.maxRenderingLayer < 0)
		config_.maxRenderingLayer = 0;

	if (config_.maxNumParticles == 0)
		config_.maxNumParticles = 1;
	else if (config_.maxNumParticles < 0)
		config_.maxNumParticles *= -1;

	if (config_.minParticleScale < 0)
		config_.minParticleScale *= -1;
	if (config_.maxParticleScale < 0)
		config_.maxParticleScale *= -1;
	if (config_.maxRandomLife < 0)
		config_.maxRandomLife *= -1;
	if (config_.maxDelay < 0)
		config_.maxDelay *= -1;

	if (config_.minParticleScale > config_.maxParticleScale)
	{
		float temp = config_.minParticleScale;
		config_.minParticleScale = config_.maxParticleScale;
		config_.maxParticleScale = temp;
	}
}

void LuaLoader::sanitizeGuiStyle()
{
	if (config_.styleIndex < 0)
		config_.styleIndex = 0;
	else if (config_.styleIndex > 2)
		config_.styleIndex = 2;

	if (config_.frameRounding < 0.0f)
		config_.frameRounding = 0.0f;
	else if (config_.frameRounding > 12.0f)
		config_.frameRounding = 12.0f;

	if (config_.scaling < 0.5f)
		config_.scaling = 0.5f;
	else if (config_.scaling > 2.0f)
		config_.scaling = 2.0f;
}

bool LuaLoader::loadConfig(const char *filename)
#ifdef __EMSCRIPTEN__
{
	return loadConfig(filename, nullptr);
}

bool LuaLoader::loadConfig(const char *filename, const nc::EmscriptenLocalFile *localFile)
#endif
{
	createNewState();
#ifndef __EMSCRIPTEN__
	if (luaState_->run(filename) == false)
#else
	const bool loaded = (localFile != nullptr)
	                        ? luaState_->runFromMemory(localFile->data(), localFile->size(), localFile->filename())
	                        : luaState_->run(filename);

	if (loaded == false)
#endif
		return false;
	lua_State *L = luaState_->state();

	unsigned int version = 1;
	nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, CfgNames::version, version);

	nc::LuaUtils::tryRetrieveGlobal<int32_t>(L, CfgNames::width, config_.width);
	nc::LuaUtils::tryRetrieveGlobal<int32_t>(L, CfgNames::height, config_.height);
	nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::fullscreen, config_.fullscreen);
	nc::LuaUtils::tryRetrieveGlobal<unsigned long>(L, CfgNames::vboSize, config_.vboSize);
	nc::LuaUtils::tryRetrieveGlobal<unsigned long>(L, CfgNames::iboSize, config_.iboSize);
	nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::batching, config_.batching);
	nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::culling, config_.culling);

	if (version >= 10)
		nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, CfgNames::frameLimit, config_.frameLimit);

	if (version >= 9)
	{
		nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::useBufferMapping, config_.useBufferMapping);
		nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::withVSync, config_.withVSync);
	}

	if (version >= 8)
		nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::resizable, config_.resizable);

	if (version >= 3)
		nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, CfgNames::saveFileMaxSize, config_.saveFileMaxSize);

	if (version >= 5)
		nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, CfgNames::logMaxSize, config_.logMaxSize);

	if (version >= 11)
	{
		config_.startupScriptName = nc::LuaUtils::retrieveGlobal<const char *>(L, CfgNames::startupScriptName);
		nc::LuaUtils::tryRetrieveGlobal<bool>(L, CfgNames::autoEmissionOnStart, config_.autoEmissionOnStart);
	}

	config_.scriptsPath = "scripts/";
	config_.texturesPath = "textures/";
	config_.backgroundsPath = "backgrounds/";
	if (version >= 6)
	{
		config_.scriptsPath = nc::LuaUtils::retrieveGlobal<const char *>(L, CfgNames::scriptsPath);
		config_.texturesPath = nc::LuaUtils::retrieveGlobal<const char *>(L, CfgNames::texturesPath);
		config_.backgroundsPath = nc::LuaUtils::retrieveGlobal<const char *>(L, CfgNames::backgroundsPath);
	}

	if (version >= 3)
	{
		if (nc::LuaUtils::tryRetrieveGlobalTable(L, CfgNames::guiLimits))
		{
			nc::LuaUtils::tryRetrieveField<int32_t>(L, -1, CfgNames::maxNumParticles, config_.maxNumParticles);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::systemPositionRange, config_.systemPositionRange);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::minParticleScale, config_.minParticleScale);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::maxParticleScale, config_.maxParticleScale);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::minParticleAngle, config_.minParticleAngle);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::maxParticleAngle, config_.maxParticleAngle);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::positionRange, config_.positionRange);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::velocityRange, config_.velocityRange);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::maxRandomLife, config_.maxRandomLife);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::randomPositionRange, config_.randomPositionRange);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::randomVelocityRange, config_.randomVelocityRange);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::maxDelay, config_.maxDelay);

			if (version >= 4)
			{
				nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::maxBackgroundImageScale, config_.maxBackgroundImageScale);
				nc::LuaUtils::tryRetrieveField<int32_t>(L, -1, CfgNames::maxRenderingLayer, config_.maxRenderingLayer);
			}
		}
		nc::LuaUtils::pop(L);
	}

	if (version >= 7)
	{
		if (nc::LuaUtils::tryRetrieveGlobalTable(L, CfgNames::guiStyle))
		{
			nc::LuaUtils::tryRetrieveField<int>(L, -1, CfgNames::styleIndex, config_.styleIndex);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::frameRounding, config_.frameRounding);
			nc::LuaUtils::tryRetrieveField<bool>(L, -1, CfgNames::windowBorder, config_.windowBorder);
			nc::LuaUtils::tryRetrieveField<bool>(L, -1, CfgNames::frameBorder, config_.frameBorder);
			nc::LuaUtils::tryRetrieveField<bool>(L, -1, CfgNames::popupBorder, config_.popupBorder);
			nc::LuaUtils::tryRetrieveField<float>(L, -1, CfgNames::scaling, config_.scaling);
		}
		nc::LuaUtils::pop(L);
	}

	sanitizeInitValues();
	sanitizeGuiLimits();
	sanitizeGuiStyle();

	return true;
}

bool LuaLoader::saveConfig(const char *filename)
{
	sanitizeInitValues();
	sanitizeGuiLimits();

	nctl::String file(4096);
	int amount = 0;

	indent(file, amount).formatAppend("%s = %u\n", CfgNames::version, ConfigFileVersion);
	indent(file, amount).formatAppend("%s = %d\n", CfgNames::width, config_.width);
	indent(file, amount).formatAppend("%s = %d\n", CfgNames::height, config_.height);
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::fullscreen, config_.fullscreen ? "true" : "false");
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::resizable, config_.resizable ? "true" : "false");
	indent(file, amount).formatAppend("%s = %u\n", CfgNames::frameLimit, config_.frameLimit);
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::useBufferMapping, config_.useBufferMapping ? "true" : "false");
	indent(file, amount).formatAppend("%s = %lu\n", CfgNames::vboSize, config_.vboSize);
	indent(file, amount).formatAppend("%s = %lu\n", CfgNames::iboSize, config_.iboSize);
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::withVSync, config_.withVSync ? "true" : "false");
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::batching, config_.batching ? "true" : "false");
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::culling, config_.culling ? "true" : "false");
	indent(file, amount).formatAppend("%s = %u\n", CfgNames::saveFileMaxSize, config_.saveFileMaxSize);
	indent(file, amount).formatAppend("%s = %u\n", CfgNames::logMaxSize, config_.logMaxSize);
	indent(file, amount).formatAppend("%s = \"%s\"\n", CfgNames::startupScriptName, config_.startupScriptName.data());
	indent(file, amount).formatAppend("%s = %s\n", CfgNames::autoEmissionOnStart, config_.autoEmissionOnStart ? "true" : "false");

	indent(file, amount).formatAppend("%s = \"%s\"\n", CfgNames::scriptsPath, config_.scriptsPath.data());
	indent(file, amount).formatAppend("%s = \"%s\"\n", CfgNames::texturesPath, config_.texturesPath.data());
	indent(file, amount).formatAppend("%s = \"%s\"\n", CfgNames::backgroundsPath, config_.backgroundsPath.data());

	indent(file, amount).append("\n");
	indent(file, amount).formatAppend("%s =\n", CfgNames::guiLimits);
	indent(file, amount).append("{\n");
	amount++;

	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::maxBackgroundImageScale, config_.maxBackgroundImageScale);
	indent(file, amount).formatAppend("%s = %d,\n", CfgNames::maxRenderingLayer, config_.maxRenderingLayer);
	indent(file, amount).formatAppend("%s = %d,\n", CfgNames::maxNumParticles, config_.maxNumParticles);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::systemPositionRange, config_.systemPositionRange);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::minParticleScale, config_.minParticleScale);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::maxParticleScale, config_.maxParticleScale);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::minParticleAngle, config_.minParticleAngle);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::maxParticleAngle, config_.maxParticleAngle);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::positionRange, config_.positionRange);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::velocityRange, config_.velocityRange);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::maxRandomLife, config_.maxRandomLife);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::randomPositionRange, config_.randomPositionRange);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::randomVelocityRange, config_.randomVelocityRange);
	indent(file, amount).formatAppend("%s = %f\n", CfgNames::maxDelay, config_.maxDelay);

	amount--;
	indent(file, amount).append("}\n");

	indent(file, amount).append("\n");
	indent(file, amount).formatAppend("%s =\n", CfgNames::guiStyle);
	indent(file, amount).append("{\n");
	amount++;

	indent(file, amount).formatAppend("%s = %d,\n", CfgNames::styleIndex, config_.styleIndex);
	indent(file, amount).formatAppend("%s = %f,\n", CfgNames::frameRounding, config_.frameRounding);
	indent(file, amount).formatAppend("%s = %s,\n", CfgNames::windowBorder, config_.windowBorder ? "true" : "false");
	indent(file, amount).formatAppend("%s = %s,\n", CfgNames::frameBorder, config_.frameBorder ? "true" : "false");
	indent(file, amount).formatAppend("%s = %s,\n", CfgNames::popupBorder, config_.popupBorder ? "true" : "false");
	indent(file, amount).formatAppend("%s = %f\n", CfgNames::scaling, config_.scaling);

	amount--;
	indent(file, amount).append("}\n");

#ifndef __EMSCRIPTEN__
	nctl::UniquePtr<nc::IFile> fileHandle = nc::IFile::createFileHandle(filename);
	fileHandle->open(nc::IFile::OpenMode::WRITE | nc::IFile::OpenMode::BINARY);
	fileHandle->write(file.data(), file.length());
	fileHandle->close();
#else
	nc::EmscriptenLocalFile localFileSave;
	localFileSave.write(file.data(), file.length());
	localFileSave.save(filename);
#endif

	return true;
}

bool LuaLoader::load(const char *filename, State &state)
#ifdef __EMSCRIPTEN__
{
	return load(filename, state, nullptr);
}

bool LuaLoader::load(const char *filename, State &state, const nc::EmscriptenLocalFile *localFile)
#endif
{
	createNewState();
#ifndef __EMSCRIPTEN__
	if (luaState_->run(filename) == false)
#else
	const bool loaded = (localFile != nullptr)
	                        ? luaState_->runFromMemory(localFile->data(), localFile->size(), localFile->filename())
	                        : luaState_->run(filename);

	if (loaded == false)
#endif
		return false;
	lua_State *L = luaState_->state();

	unsigned int version = 1;
	nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, Names::version, version);

	state.normalizedAbsPosition = nc::Vector2f(0.5f, 0.5f); // center of the screen as default
	if (nc::LuaUtils::tryRetrieveGlobalTable(L, Names::normalizedAbsPosition))
		state.normalizedAbsPosition = nc::LuaVector2fUtils::retrieveTable(L, -1);
	nc::LuaUtils::pop(L);

	state.background.color = nc::Colorf::Black; // black background as default
	state.background.imageNormalizedPosition = nc::Vector2f(0.5f, 0.5f); // center of the screen as default
	state.background.imageScale = 1.0f;
	state.background.imageLayer = 0;
	if (version >= 4)
	{
		nc::LuaUtils::retrieveGlobalTable(L, Names::backgroundProperties);

		state.background.color = nc::LuaColorUtils::retrieveTableField(L, -1, Names::backgroundColor);
		state.background.imageName = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::backgroundImage);
		state.background.imageNormalizedPosition = nc::LuaVector2fUtils::retrieveTableField(L, -1, Names::backgroundImageNormalizedPosition);
		state.background.imageScale = nc::LuaUtils::retrieveField<float>(L, -1, Names::backgroundImageScale);
		state.background.imageLayer = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::backgroundImageLayer);

		if (version >= 5)
		{
			state.background.imageColor = nc::LuaColorUtils::retrieveTableField(L, -1, Names::backgroundImageColor);
			state.background.imageRect = nc::LuaRectiUtils::retrieveTableField(L, -1, Names::backgroundImageRect);
		}

		nc::LuaUtils::pop(L);
	}

	nc::LuaUtils::retrieveGlobalTable(L, Names::particleSystems);
	const unsigned int numSystems = nc::LuaUtils::rawLen(L, -1);

	for (unsigned int systemIndex = 0; systemIndex < numSystems; systemIndex++)
	{
		State::ParticleSystem s;

		nc::LuaUtils::rawGeti(L, -1, systemIndex + 1); // Lua arrays start from index 1

		if (version >= 3)
			s.name = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::name);

		s.numParticles = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::numParticles);
		s.textureName = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::texture);
		s.texRect = nc::LuaRectiUtils::retrieveTableField(L, -1, Names::texRext);
		s.position = nc::LuaVector2fUtils::retrieveTableField(L, -1, Names::relativePosition);
		s.inLocalSpace = nc::LuaUtils::retrieveField<bool>(L, -1, Names::inLocalSpace);
		s.active = nc::LuaUtils::retrieveField<bool>(L, -1, Names::active);

		s.layer = 1;
		if (version >= 4)
			s.layer = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::layer);

		if (nc::LuaUtils::tryRetrieveFieldTable(L, -1, Names::colorSteps))
		{
			const size_t numSteps = nc::LuaUtils::rawLen(L, -1);
			for (unsigned int i = 0; i < numSteps; i++)
			{
				State::ColorStep step;
				nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

				nc::LuaUtils::rawGeti(L, -1, 1); // Lua arrays start from index 1
				step.age = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::rawGeti(L, -1, 2);
				step.color = nc::LuaColorUtils::retrieveTable(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::pop(L);
				s.colorSteps.pushBack(step);
			}
		}
		nc::LuaUtils::pop(L);

		s.sizeStepBaseScale = 1.0f; // default if no size steps are found
		if (nc::LuaUtils::tryRetrieveFieldTable(L, -1, Names::sizeSteps))
		{
			s.sizeStepBaseScale = nc::LuaUtils::retrieveField<float>(L, -1, Names::baseScale);

			const size_t numSteps = nc::LuaUtils::rawLen(L, -1);
			for (unsigned int i = 0; i < numSteps; i++)
			{
				State::SizeStep step;
				nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

				nc::LuaUtils::rawGeti(L, -1, 1); // Lua arrays start from index 1
				step.age = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::rawGeti(L, -1, 2);
				step.scale = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::pop(L);
				s.sizeSteps.pushBack(step);
			}
		}
		nc::LuaUtils::pop(L);

		if (nc::LuaUtils::tryRetrieveFieldTable(L, -1, Names::rotationSteps))
		{
			const size_t numSteps = nc::LuaUtils::rawLen(L, -1);
			for (unsigned int i = 0; i < numSteps; i++)
			{
				State::RotationStep step;
				nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

				nc::LuaUtils::rawGeti(L, -1, 1); // Lua arrays start from index 1
				step.age = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::rawGeti(L, -1, 2);
				step.angle = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::pop(L);
				s.rotationSteps.pushBack(step);
			}
		}
		nc::LuaUtils::pop(L);

		if (nc::LuaUtils::tryRetrieveFieldTable(L, -1, Names::positionSteps))
		{
			const size_t numSteps = nc::LuaUtils::rawLen(L, -1);
			for (unsigned int i = 0; i < numSteps; i++)
			{
				State::PositionStep step;
				nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

				nc::LuaUtils::rawGeti(L, -1, 1); // Lua arrays start from index 1
				step.age = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::rawGeti(L, -1, 2);
				step.position = nc::LuaVector2fUtils::retrieveTable(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::pop(L);
				s.positionSteps.pushBack(step);
			}
		}
		nc::LuaUtils::pop(L);

		if (nc::LuaUtils::tryRetrieveFieldTable(L, -1, Names::velocitySteps))
		{
			const size_t numSteps = nc::LuaUtils::rawLen(L, -1);
			for (unsigned int i = 0; i < numSteps; i++)
			{
				State::VelocityStep step;
				nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

				nc::LuaUtils::rawGeti(L, -1, 1); // Lua arrays start from index 1
				step.age = nc::LuaUtils::retrieve<float>(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::rawGeti(L, -1, 2);
				step.velocity = nc::LuaVector2fUtils::retrieveTable(L, -1);
				nc::LuaUtils::pop(L);

				nc::LuaUtils::pop(L);
				s.velocitySteps.pushBack(step);
			}
		}
		nc::LuaUtils::pop(L);

		nc::LuaUtils::retrieveFieldTable(L, -1, Names::emission);
		s.init.rndAmount = nc::LuaVector2iUtils::retrieveArrayField(L, -1, Names::amount);
		s.init.rndLife = nc::LuaVector2fUtils::retrieveArrayField(L, -1, Names::life);
		s.init.rndPositionX = nc::LuaVector2fUtils::retrieveArrayField(L, -1, Names::positionX);
		s.init.rndPositionY = nc::LuaVector2fUtils::retrieveArrayField(L, -1, Names::positionY);
		s.init.rndVelocityX = nc::LuaVector2fUtils::retrieveArrayField(L, -1, Names::velocityX);
		s.init.rndVelocityY = nc::LuaVector2fUtils::retrieveArrayField(L, -1, Names::velocityY);
		s.init.rndRotation = nc::LuaVector2fUtils::retrieveArrayField(L, -1, Names::rotation);
		s.init.emitterRotation = nc::LuaUtils::retrieveField<bool>(L, -1, Names::emitterRotation);
		s.emitDelay = nc::LuaUtils::retrieveField<float>(L, -1, Names::delay);
		nc::LuaUtils::pop(L);

		state.systems.pushBack(s);
		nc::LuaUtils::pop(L);
	}

	return true;
}

void LuaLoader::save(const char *filename, const State &state)
{
	nctl::String file(config_.saveFileMaxSize);
	int amount = 0;

	indent(file, amount).formatAppend("%s = %u\n", Names::version, ProjectFileVersion);
	indent(file, amount).formatAppend("%s = {x = %f, y = %f}\n", Names::normalizedAbsPosition, state.normalizedAbsPosition.x, state.normalizedAbsPosition.y);
	file.append("\n");

	indent(file, amount).formatAppend("%s =\n", Names::backgroundProperties);
	indent(file, amount).append("{\n");
	amount++;

	const nc::Colorf &bgColor = state.background.color;
	indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f, a = %f},\n", Names::backgroundColor, bgColor.r(), bgColor.g(), bgColor.b(), 1.0f);
	indent(file, amount).formatAppend("%s = \"%s\",\n", Names::backgroundImage, state.background.imageName.data());
	indent(file, amount).formatAppend("%s = {x = %f, y = %f},\n", Names::backgroundImageNormalizedPosition,
	                                  state.background.imageNormalizedPosition.x, state.background.imageNormalizedPosition.y);
	indent(file, amount).formatAppend("%s = %f,\n", Names::backgroundImageScale, state.background.imageScale);
	indent(file, amount).formatAppend("%s = %d,\n", Names::backgroundImageLayer, state.background.imageLayer);
	const nc::Colorf &imgColor = state.background.imageColor;
	indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f, a = %f},\n", Names::backgroundImageColor, imgColor.r(), imgColor.g(), imgColor.b(), imgColor.a());
	indent(file, amount).formatAppend("%s = {x = %d, y = %d, w = %d, h = %d}\n", Names::backgroundImageRect,
	                                  state.background.imageRect.x, state.background.imageRect.y, state.background.imageRect.w, state.background.imageRect.h);

	amount--;
	indent(file, amount).append("}\n");
	file.append("\n");

	indent(file, amount).formatAppend("%s =\n", Names::particleSystems);
	indent(file, amount).append("{\n");
	amount++;
	const unsigned int numSystems = state.systems.size();
	for (unsigned int index = 0; index < numSystems; index++)
	{
		const State::ParticleSystem &sysState = state.systems[index];
		const bool isLastSystem = (index == numSystems - 1);
		indent(file, amount).append("{\n");

		amount++;
		indent(file, amount).formatAppend("%s = \"%s\",\n", Names::name, sysState.name.data());
		indent(file, amount).formatAppend("%s = %d,\n", Names::numParticles, sysState.numParticles);
		indent(file, amount).formatAppend("%s = \"%s\",\n", Names::texture, sysState.textureName.data());
		indent(file, amount).formatAppend("%s = {x = %d, y = %d, w = %d, h = %d},\n", Names::texRext,
		                                  sysState.texRect.x, sysState.texRect.y, sysState.texRect.w, sysState.texRect.h);
		indent(file, amount).formatAppend("%s = {x = %f, y = %f},\n", Names::relativePosition, sysState.position.x, sysState.position.y);
		indent(file, amount).formatAppend("%s = %d,\n", Names::layer, sysState.layer);
		indent(file, amount).formatAppend("%s = %s,\n", Names::inLocalSpace, sysState.inLocalSpace ? "true" : "false");
		indent(file, amount).formatAppend("%s = %s,\n", Names::active, sysState.active ? "true" : "false");
		file.append("\n");

		if (sysState.colorSteps.isEmpty() == false)
		{
			indent(file, amount).formatAppend("%s =\n", Names::colorSteps);
			indent(file, amount).append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.colorSteps.size(); i++)
			{
				const State::ColorStep &step = sysState.colorSteps[i];
				const bool isLastStep = (i == sysState.colorSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, {r = %f, g = %f, b = %f, a = %f}}%s\n",
				                  step.age, step.color.r(), step.color.g(), step.color.b(), step.color.a(), isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount).append("},\n");
			file.append("\n");
		}

		if (sysState.sizeSteps.isEmpty() == false)
		{
			indent(file, amount).formatAppend("%s =\n", Names::sizeSteps);
			indent(file, amount).append("{\n");
			amount++;
			indent(file, amount).formatAppend("%s = %f,\n", Names::baseScale, sysState.sizeStepBaseScale);
			for (unsigned int i = 0; i < sysState.sizeSteps.size(); i++)
			{
				const State::SizeStep &step = sysState.sizeSteps[i];
				const bool isLastStep = (i == sysState.sizeSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, %f}%s\n", step.age, step.scale, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount).append("},\n");
			file.append("\n");
		}

		if (sysState.rotationSteps.isEmpty() == false)
		{
			indent(file, amount).formatAppend("%s =\n", Names::rotationSteps);
			indent(file, amount).append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.rotationSteps.size(); i++)
			{
				const State::RotationStep &step = sysState.rotationSteps[i];
				const bool isLastStep = (i == sysState.rotationSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, %f}%s\n", step.age, step.angle, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount).append("},\n");
			file.append("\n");
		}

		if (sysState.positionSteps.isEmpty() == false)
		{
			indent(file, amount).formatAppend("%s =\n", Names::positionSteps);
			indent(file, amount).append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.positionSteps.size(); i++)
			{
				const State::PositionStep &step = sysState.positionSteps[i];
				const bool isLastStep = (i == sysState.positionSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, {x = %f, y = %f}}%s\n", step.age, step.position.x, step.position.y, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount).append("},\n");
			file.append("\n");
		}

		if (sysState.velocitySteps.isEmpty() == false)
		{
			indent(file, amount).formatAppend("%s =\n", Names::velocitySteps);
			indent(file, amount).append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.velocitySteps.size(); i++)
			{
				const State::VelocityStep &step = sysState.velocitySteps[i];
				const bool isLastStep = (i == sysState.velocitySteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, {x = %f, y = %f}}%s\n", step.age, step.velocity.x, step.velocity.y, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount).append("},\n");
			file.append("\n");
		}

		indent(file, amount).formatAppend("%s =\n", Names::emission);
		indent(file, amount).append("{\n");
		amount++;
		indent(file, amount).formatAppend("%s = {%d, %d},\n", Names::amount, sysState.init.rndAmount.x, sysState.init.rndAmount.y);
		indent(file, amount).formatAppend("%s = {%f, %f},\n", Names::life, sysState.init.rndLife.x, sysState.init.rndLife.y);
		indent(file, amount).formatAppend("%s = {%f, %f},\n", Names::positionX, sysState.init.rndPositionX.x, sysState.init.rndPositionX.y);
		indent(file, amount).formatAppend("%s = {%f, %f},\n", Names::positionY, sysState.init.rndPositionY.x, sysState.init.rndPositionY.y);
		indent(file, amount).formatAppend("%s = {%f, %f},\n", Names::velocityX, sysState.init.rndVelocityX.x, sysState.init.rndVelocityX.y);
		indent(file, amount).formatAppend("%s = {%f, %f},\n", Names::velocityY, sysState.init.rndVelocityY.x, sysState.init.rndVelocityY.y);
		indent(file, amount).formatAppend("%s = {%f, %f},\n", Names::rotation, sysState.init.rndRotation.x, sysState.init.rndRotation.y);
		indent(file, amount).formatAppend("%s = %s,\n", Names::emitterRotation, sysState.init.emitterRotation ? "true" : "false");
		indent(file, amount).formatAppend("%s = %f\n", Names::delay, sysState.emitDelay);
		amount--;
		indent(file, amount).append("}\n");

		amount--;
		indent(file, amount).formatAppend("}%s\n", isLastSystem ? "" : ",\n");
	}
	amount--;
	indent(file, amount).append("}\n");

#ifndef __EMSCRIPTEN__
	nctl::UniquePtr<nc::IFile> fileHandle = nc::IFile::createFileHandle(filename);
	fileHandle->open(nc::IFile::OpenMode::WRITE | nc::IFile::OpenMode::BINARY);
	fileHandle->write(file.data(), file.length());
	fileHandle->close();
#else
	nc::EmscriptenLocalFile localFileSave;
	localFileSave.write(file.data(), file.length());
	localFileSave.save(filename);
#endif
}

///////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
///////////////////////////////////////////////////////////

void LuaLoader::createNewState()
{
	luaState_ = nctl::makeUnique<nc::LuaStateManager>(
	    nc::LuaStateManager::ApiType::NONE,
	    nc::LuaStateManager::StatisticsTracking::DISABLED,
	    nc::LuaStateManager::StandardLibraries::NOT_LOADED);
}
