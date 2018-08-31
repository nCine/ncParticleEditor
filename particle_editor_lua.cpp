#include "particle_editor_lua.h"
#include "Colorf.h"
#include "LuaUtils.h"
#include "LuaRectUtils.h"
#include "LuaVector2Utils.h"
#include "LuaColorUtils.h"
#include "IFile.h"

///////////////////////////////////////////////////////////
// CONSTRUCTORS and DESTRUCTOR
///////////////////////////////////////////////////////////

LuaLoader::LuaLoader()
	: luaState_(nc::LuaStateManager::ApiType::NONE,
		nc::LuaStateManager::StatisticsTracking::DISABLED,
		nc::LuaStateManager::StandardLibraries::NOT_LOADED),
	  configLoaded_(false)
{

}

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

namespace {

void indent(nctl::String &string, int amount)
{
	FATAL_ASSERT(amount >= 0);
	for (int i = 0; i < amount; i++)
		string.append("\t");
}

const unsigned int ConfigFileVersion = 2;
const unsigned int ProjectFileVersion = 3;

namespace Names
{

const char *projectVersion = "project_version"; // version 3

const char *normalizedAbsPosition = "normalized_absolute_position"; // version 2

const char *particleSystems = "particle_systems";

const char *name = "name"; // version 3
const char *numParticles = "num_particles";
const char *texture = "texture";
const char *texRext = "texture_rect";
const char *relativePosition = "relative_position";
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

const char *cfgVersion = "config_version"; // version 2
const char *cfgWidth = "width";
const char *cfgHeight = "height";
const char *cfgFullscreen = "fullscreen";
const char *cfgVboSize = "vbo_size";
const char *cfgIboSize = "ibo_size";
const char *cfgBatching = "batching";
const char *cfgCulling = "culling";

}

}

bool LuaLoader::loadConfig(const char *filename)
{
	lua_State *L = luaState_.state();
	luaState_.run(filename);

	nc::LuaUtils::tryRetrieveGlobal<int32_t>(L, Names::cfgWidth, config_.width);
	nc::LuaUtils::tryRetrieveGlobal<int32_t>(L, Names::cfgHeight, config_.height);
	nc::LuaUtils::tryRetrieveGlobal<bool>(L, Names::cfgFullscreen, config_.fullscreen);
	nc::LuaUtils::tryRetrieveGlobal<unsigned long>(L, Names::cfgVboSize, config_.vboSize);
	nc::LuaUtils::tryRetrieveGlobal<unsigned long>(L, Names::cfgIboSize, config_.iboSize);
	nc::LuaUtils::tryRetrieveGlobal<bool>(L, Names::cfgBatching, config_.batching);
	nc::LuaUtils::tryRetrieveGlobal<bool>(L, Names::cfgCulling, config_.culling);

	configLoaded_ = true;
	return configLoaded_;
}

bool LuaLoader::saveConfig(const char *filename)
{
	nctl::String file(4096);
	int amount = 0;

	indent(file, amount); file.formatAppend("%s = %u\n", Names::cfgVersion, ConfigFileVersion);
	indent(file, amount); file.formatAppend("%s = %d\n", Names::cfgWidth, config_.width);
	indent(file, amount); file.formatAppend("%s = %d\n", Names::cfgHeight, config_.height);
	indent(file, amount); file.formatAppend("%s = %s\n", Names::cfgFullscreen, config_.fullscreen ? "true" : "false");
	indent(file, amount); file.formatAppend("%s = %lu\n", Names::cfgVboSize, config_.vboSize);
	indent(file, amount); file.formatAppend("%s = %lu\n", Names::cfgIboSize, config_.iboSize);
	indent(file, amount); file.formatAppend("%s = %s\n", Names::cfgBatching, config_.batching ? "true" : "false");
	indent(file, amount); file.formatAppend("%s = %s\n", Names::cfgCulling, config_.culling ? "true" : "false");

	nctl::UniquePtr<nc::IFile> fileHandle = nc::IFile::createFileHandle(filename);
	fileHandle->open(nc::IFile::OpenMode::WRITE | nc::IFile::OpenMode::BINARY);
	fileHandle->write(file.data(), file.length());
	fileHandle->close();

	return true;
}

bool LuaLoader::load(const char *filename, State &state)
{
	lua_State *L = luaState_.state();
	luaState_.run(filename);

	unsigned int version = 1;
	nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, Names::projectVersion, version);

	if (nc::LuaUtils::tryRetrieveGlobalTable(L, Names::normalizedAbsPosition))
		state.normalizedAbsPosition = nc::LuaVector2fUtils::retrieveTable(L, -1);
	else
		state.normalizedAbsPosition = nc::Vector2f(0.5f, 0.5f); // center of the screen
	nc::LuaUtils::pop(L);

	nc::LuaUtils::retrieveGlobalTable(L, Names::particleSystems);
	const unsigned int numSystems = nc::LuaUtils::rawLen(L, -1);

	for (unsigned int systemIndex = 0; systemIndex < numSystems; systemIndex++)
	{
		State::ParticleSystem s;

		nc::LuaUtils::rawGeti(L, -1, systemIndex + 1); // Lua arrays start from index 1

		if (version == 3)
			s.name = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::name);

		s.numParticles = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::numParticles);
		s.textureName = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::texture);
		s.texRect = nc::LuaRectiUtils::retrieveTableField(L, -1, Names::texRext);
		s.position = nc::LuaVector2fUtils::retrieveTableField(L, -1, Names::relativePosition);
		s.inLocalSpace = nc::LuaUtils::retrieveField<bool>(L, -1, Names::inLocalSpace);
		s.active = nc::LuaUtils::retrieveField<bool>(L, -1, Names::active);

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
	nctl::String file(4096);
	int amount = 0;

	indent(file, amount); file.formatAppend("%s = %u\n", Names::projectVersion, ProjectFileVersion);

	indent(file, amount); file.formatAppend("%s = {x = %f, y = %f}\n", Names::normalizedAbsPosition, state.normalizedAbsPosition.x, state.normalizedAbsPosition.y);

	indent(file, amount); file.formatAppend("%s =\n", Names::particleSystems);
	indent(file, amount); file.append("{\n");
	amount++;

	const unsigned int numSystems = state.systems.size();
	for (unsigned int index = 0; index < numSystems; index++)
	{
		const State::ParticleSystem &sysState = state.systems[index];
		const bool isLastSystem = (index == numSystems - 1);
		indent(file, amount); file.append("{\n");

		amount++;
		indent(file, amount); file.formatAppend("%s = \"%s\",\n", Names::name, sysState.name.data());
		indent(file, amount); file.formatAppend("%s = %d,\n", Names::numParticles, sysState.numParticles);
		indent(file, amount); file.formatAppend("%s = \"%s\",\n", Names::texture, sysState.textureName.data());
		indent(file, amount); file.formatAppend("%s = {x = %d, y = %d, w = %d, h = %d},\n", Names::texRext,
		                                        sysState.texRect.x, sysState.texRect.y, sysState.texRect.w, sysState.texRect.h);
		indent(file, amount); file.formatAppend("%s = {x = %f, y = %f},\n", Names::relativePosition, sysState.position.x, sysState.position.y);
		indent(file, amount); file.formatAppend("%s = %s,\n", Names::inLocalSpace, sysState.inLocalSpace ? "true" : "false");
		indent(file, amount); file.formatAppend("%s = %s,\n", Names::active, sysState.active ? "true" : "false");
		file.append("\n");

		if (sysState.colorSteps.isEmpty() == false)
		{
			indent(file, amount); file.formatAppend("%s =\n", Names::colorSteps);
			indent(file, amount); file.append("{\n");
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
			indent(file, amount); file.append("},\n");
			file.append("\n");
		}

		if (sysState.sizeSteps.isEmpty() == false)
		{
			indent(file, amount); file.formatAppend("%s =\n", Names::sizeSteps);
			indent(file, amount); file.append("{\n");
			amount++;
			indent(file, amount); file.formatAppend("%s = %f,\n", Names::baseScale, sysState.sizeStepBaseScale);
			for (unsigned int i = 0; i < sysState.sizeSteps.size(); i++)
			{
				const State::SizeStep &step = sysState.sizeSteps[i];
				const bool isLastStep = (i == sysState.sizeSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, %f}%s\n", step.age, step.scale, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount); file.append("},\n");
			file.append("\n");
		}

		if (sysState.rotationSteps.isEmpty() == false)
		{
			indent(file, amount); file.formatAppend("%s =\n", Names::rotationSteps);
			indent(file, amount); file.append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.rotationSteps.size(); i++)
			{
				const State::RotationStep &step = sysState.rotationSteps[i];
				const bool isLastStep = (i == sysState.rotationSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, %f}%s\n", step.age, step.angle, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount); file.append("},\n");
			file.append("\n");
		}

		if (sysState.positionSteps.isEmpty() == false)
		{
			indent(file, amount); file.formatAppend("%s =\n", Names::positionSteps);
			indent(file, amount); file.append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.positionSteps.size(); i++)
			{
				const State::PositionStep &step = sysState.positionSteps[i];
				const bool isLastStep = (i == sysState.positionSteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, {x = %f, y = %f}}%s\n", step.age, step.position.x, step.position.y, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount); file.append("},\n");
			file.append("\n");
		}

		if (sysState.velocitySteps.isEmpty() == false)
		{
			indent(file, amount); file.formatAppend("%s =\n", Names::velocitySteps);
			indent(file, amount); file.append("{\n");
			amount++;
			for (unsigned int i = 0; i < sysState.velocitySteps.size(); i++)
			{
				const State::VelocityStep &step = sysState.velocitySteps[i];
				const bool isLastStep = (i == sysState.velocitySteps.size() - 1);
				indent(file, amount);
				file.formatAppend("{%f, {x = %f, y = %f}}%s\n", step.age, step.velocity.x, step.velocity.y, isLastStep ? "" : ",");
			}
			amount--;
			indent(file, amount); file.append("},\n");
			file.append("\n");
		}

		indent(file, amount); file.formatAppend("%s =\n", Names::emission);
		indent(file, amount); file.append("{\n");
		amount++;
		indent(file, amount); file.formatAppend("%s = {%d, %d},\n", Names::amount, sysState.init.rndAmount.x, sysState.init.rndAmount.y);
		indent(file, amount); file.formatAppend("%s = {%f, %f},\n", Names::life, sysState.init.rndLife.x, sysState.init.rndLife.y);
		indent(file, amount); file.formatAppend("%s = {%f, %f},\n", Names::positionX, sysState.init.rndPositionX.x, sysState.init.rndPositionX.y);
		indent(file, amount); file.formatAppend("%s = {%f, %f},\n", Names::positionY, sysState.init.rndPositionY.x, sysState.init.rndPositionY.y);
		indent(file, amount); file.formatAppend("%s = {%f, %f},\n", Names::velocityX, sysState.init.rndVelocityX.x, sysState.init.rndVelocityX.y);
		indent(file, amount); file.formatAppend("%s = {%f, %f},\n", Names::velocityY, sysState.init.rndVelocityY.x, sysState.init.rndVelocityY.y);
		indent(file, amount); file.formatAppend("%s = {%f, %f},\n", Names::rotation, sysState.init.rndRotation.x, sysState.init.rndRotation.y);
		indent(file, amount); file.formatAppend("%s = %s,\n", Names::emitterRotation, sysState.init.emitterRotation ? "true" : "false");
		indent(file, amount); file.formatAppend("%s = %f\n", Names::delay, sysState.emitDelay);
		amount--;
		indent(file, amount); file.append("}\n");

		amount--;
		indent(file, amount); file.formatAppend("}%s\n", isLastSystem ? "" : ",\n");
	}
	amount--;
	indent(file, amount); file.append("}\n");

	nctl::UniquePtr<nc::IFile> fileHandle = nc::IFile::createFileHandle(filename);
	fileHandle->open(nc::IFile::OpenMode::WRITE | nc::IFile::OpenMode::BINARY);
	fileHandle->write(file.data(), file.length());
	fileHandle->close();
}
