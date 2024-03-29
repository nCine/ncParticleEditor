#ifndef CLASS_LUALOADER
#define CLASS_LUALOADER

#include <ncine/IAppEventHandler.h>
#include <ncine/IInputEventHandler.h>
#include <nctl/UniquePtr.h>
#include <nctl/String.h>
#include <nctl/StaticArray.h>
#include <ncine/Vector2.h>
#include <ncine/Vector4.h>
#include <ncine/Rect.h>
#include <ncine/DrawableNode.h>
#include <ncine/ParticleAffectors.h>
#include <ncine/ParticleInitializer.h>
#include <ncine/LuaStateManager.h>

#ifdef __EMSCRIPTEN__
	#include <ncine/EmscriptenLocalFile.h>
#endif

namespace ncine {

class Texture;
class ParticleSystem;
class LuaStateManager;

}

namespace nc = ncine;

/// The particle editor loader/saver
class LuaLoader
{
  public:
	static const unsigned int MaxFilenameLength = 256;

	struct State
	{
		struct ColorStep
		{
			float age;
			nc::Colorf color;
		};

		struct SizeStep
		{
			float age;
			nc::Vector2f scale;
		};

		struct RotationStep
		{
			float age;
			float angle;
		};

		struct PositionStep
		{
			float age;
			nc::Vector2f position;
		};

		struct VelocityStep
		{
			float age;
			nc::Vector2f velocity;
		};

		struct ParticleSystem
		{
			nctl::String name = nctl::String(MaxFilenameLength);
			int numParticles;
			nctl::String textureName = nctl::String(MaxFilenameLength);
			nc::Recti texRect;
			nc::Vector2f anchorPoint;
			bool flippedX;
			bool flippedY;
			nc::DrawableNode::BlendingPreset blendingPreset;
			nc::Vector2f position;
			int layer;
			bool inLocalSpace;
			bool active;
			nctl::Array<ColorStep> colorSteps;
			nc::Vector2f sizeStepBaseScale;
			nctl::Array<SizeStep> sizeSteps;
			nctl::Array<RotationStep> rotationSteps;
			nctl::Array<PositionStep> positionSteps;
			nctl::Array<VelocityStep> velocitySteps;
			nc::ParticleInitializer init;
			float emitDelay;
		};

		struct BackgroundProperties
		{
			nc::Colorf color;
			nctl::String imageName = nctl::String(MaxFilenameLength);
			nc::Vector2f imageNormalizedPosition;
			nc::Vector2f imageScale;
			int imageLayer;
			nc::Colorf imageColor;
			nc::Recti imageRect;
			bool imageFlippedX;
			bool imageFlippedY;
		};

		nc::Vector2f normalizedAbsPosition;
		BackgroundProperties background;
		nctl::Array<ParticleSystem> systems;
	};

	struct Config
	{
		int width = 1280;
		int height = 720;
		bool fullScreen = false;
		bool resizable = false;
		unsigned int frameLimit = 0;
		bool useBufferMapping = false;
		unsigned long vboSize = 256 * 1024;
		unsigned long iboSize = 32 * 1024;
		bool withVSync = true;
		bool batching = true;
		bool culling = true;
		unsigned int saveFileMaxSize = 8 * 1024;
		unsigned int logMaxSize = 4 * 1024;
		nctl::String startupScriptName = nctl::String(MaxFilenameLength);
		bool autoEmissionOnStart = false;

		nctl::String scriptsPath = nctl::String(MaxFilenameLength);
		nctl::String texturesPath = nctl::String(MaxFilenameLength);
		nctl::String backgroundsPath = nctl::String(MaxFilenameLength);

		float maxBackgroundImageScale = 5.0f;
		int maxRenderingLayer = 16;
		int maxNumParticles = 256;
		float systemPositionRange = 200.0f;
		float minParticleScale = 0.0f;
		float maxParticleScale = 2.0f;
		float minParticleAngle = -360.0f;
		float maxParticleAngle = 360.0f;
		float positionRange = 5.0f;
		float velocityRange = 5.0f;
		float maxRandomLife = 5.0f;
		float randomPositionRange = 100.0f;
		float randomVelocityRange = 200.0f;
		float maxDelay = 5.0f;

		int styleIndex = 0;
		float frameRounding = 0.0f;
		bool windowBorder = true;
		bool frameBorder = true;
		bool popupBorder = true;
#ifdef __ANDROID__
		float scaling = 2.0f;
#else
		float scaling = 1.0f;
#endif
	};

#ifdef __EMSCRIPTEN__
	nc::EmscriptenLocalFile localFileLoad;
	nc::EmscriptenLocalFile localFileLoadConfig;
#endif

	inline const Config &config() const { return config_; }
	inline Config &config() { return config_; }
	void sanitizeInitValues();
	void sanitizeGuiLimits();
	void sanitizeGuiStyle();
	bool loadConfig(const char *filename);
#ifdef __EMSCRIPTEN__
	bool loadConfig(const char *filename, const nc::EmscriptenLocalFile *localFile);
#endif
	bool saveConfig(const char *filename);
	bool load(const char *filename, State &state);
#ifdef __EMSCRIPTEN__
	bool load(const char *filename, State &state, const nc::EmscriptenLocalFile *localFile);
#endif
	void save(const char *filename, const State &state);

  private:
	nctl::UniquePtr<nc::LuaStateManager> luaState_;
	Config config_;

	void createNewState();
};

#endif
