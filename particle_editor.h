#ifndef CLASS_MYEVENTHANDLER
#define CLASS_MYEVENTHANDLER

#include <ncine/IAppEventHandler.h>
#include <ncine/IInputEventHandler.h>
#include <nctl/UniquePtr.h>
#include <nctl/String.h>
#include <nctl/StaticArray.h>
#include <ncine/Vector2.h>
#include <ncine/Colorf.h>
#include <ncine/Rect.h>
#include <ncine/ParticleAffectors.h>
#include <ncine/ParticleInitializer.h>
#include <ncine/Timer.h>

namespace ncine {

class Sprite;
class Texture;
class ParticleSystem;
class SceneNode;

}

class LuaLoader;

namespace nc = ncine;

/// My nCine event handler
class MyEventHandler :
    public nc::IAppEventHandler,
    public nc::IInputEventHandler
{
  public:
	MyEventHandler();
	void onPreInit(nc::AppConfiguration &config) override;
	void onInit() override;
	void onFrameStart() override;
	void onShutdown() override;

	void onKeyPressed(const nc::KeyboardEvent &event) override;

  private:
	static const unsigned int MaxStringLength = 256;

	nc::Colorf background_ = nc::Colorf::Black;
	nctl::String backgroundImageName_ = nctl::String(MaxStringLength);
	nc::Vector2f backgroundImagePosition_ = nc::Vector2f::Zero;
	float backgroundImageScale_ = 1.0f;
	int backgroundImageLayer_ = 0;
	nc::Colorf backgroundImageColor_ = nc::Colorf::White;
	nc::Recti backgroundImageRect_;

	nc::Vector2f parentPosition_ = nc::Vector2f::Zero;
	int systemIndex_ = 0;
	bool autoEmission_ = false;
	struct ParticleSystemGuiState
	{
		nctl::String name = nctl::String(MaxStringLength);
		int numParticles = 128;
		nc::Vector2f position = nc::Vector2f::Zero;
		int layer = 1;
		bool inLocalSpace = false;
		bool active = true;

		nc::Texture *texture = nullptr;
		nc::Recti texRect;

		nc::ColorAffector *colorAffector = nullptr;
		nc::Colorf colorValue = nc::Colorf(1.0f, 1.0f, 1.0f, 1.0f);
		float colorAge = 0.0f;

		nc::SizeAffector *sizeAffector = nullptr;
		float baseScale = 1.0f;
		float sizeValue = 1.0f;
		float sizeAge = 0.0f;

		nc::RotationAffector *rotationAffector = nullptr;
		float rotValue = 0.0f;
		float rotAge = 0.0f;

		nc::PositionAffector *positionAffector = nullptr;
		nc::Vector2f positionValue = nc::Vector2f::Zero;
		float positionAge = 0.0f;

		nc::VelocityAffector *velocityAffector = nullptr;
		nc::Vector2f velocityValue = nc::Vector2f::Zero;
		float velocityAge = 0.0f;

		nc::ParticleInitializer init;
		int amountCurrentItem = 0;
		int lifeCurrentItem = 0;
		int positionCurrentItem = 1;
		int velocityCurrentItem = 1;
		int rotationCurrentItem = 0;
		float emitDelay = (init.rndLife.x + init.rndLife.y) * 0.5f;
		nc::Timer emitTimer;
	};

	nctl::String configFile_ = nctl::String(MaxStringLength);
	nctl::String filename_ = nctl::String(MaxStringLength);
	nctl::String texFilename_ = nctl::String(MaxStringLength);
	static const unsigned int MaxRecentFiles = 6;
	nctl::StaticArray<nctl::String, MaxRecentFiles> recentFilenames_;
	int recentFileIndexStart_ = 0;
	int recentFileIndexEnd_ = 0;

	int texIndex_ = 0;
	struct TextureGuiState
	{
		nctl::String name = nctl::String(MaxStringLength);
		nc::Recti texRect;
		bool showRect = false;
	};

	nctl::String logString_ = nctl::String(4096);

	nctl::UniquePtr<LuaLoader> loader_;

	nctl::Array<ParticleSystemGuiState> sysStates_;
	nctl::Array<TextureGuiState> texStates_;

	nctl::UniquePtr<nc::SceneNode> dummy_;
	nctl::Array<nctl::UniquePtr<nc::Texture>> textures_;
	nctl::Array<nctl::UniquePtr<nc::Texture>> texturesToDelete_;
	nctl::Array<nc::Rectf> rects_;
	nctl::UniquePtr<nc::Texture> backgroundTexture_;
	nctl::UniquePtr<nc::Sprite> backgroundSprite_;
	nctl::Array<nctl::UniquePtr<nc::ParticleSystem>> particleSystems_;
	nctl::String widgetName_ = nctl::String(MaxStringLength);

	static const unsigned int NumPlotValues = 64;

	bool showMainWindow_ = true;
	bool showConfigWindow_ = false;
	bool showLogWindow_ = false;

	bool menuNewEnabled();
	void menuNew();
	void menuOpen();
	bool menuSaveEnabled();
	void menuSave();
	void menuQuit();
	void closeModalsAndAbout();

	void configureGui();
	void createGuiMainWindow();
	void createGuiMenus();
	void createGuiPopups();
	void createGuiBackground();
	void createGuiTextures();
	void createGuiManageSystems();
	void createGuiParticleSystem();
	void createGuiColorAffector();
	void createGuiColorPlot(const ParticleSystemGuiState &s);
	void createGuiSizeAffector();
	void createGuiSizePlot(const ParticleSystemGuiState &s);
	void createGuiRotationAffector();
	void createGuiRotationPlot(const ParticleSystemGuiState &s);
	void createGuiPositionAffector();
	void createGuiPositionPlot(const ParticleSystemGuiState &s);
	void createGuiVelocityAffector();
	void createGuiVelocityPlot(const ParticleSystemGuiState &s);
	void createGuiEmission();
	void sanitizeParticleInit(nc::ParticleInitializer &init);
	void createGuiEmissionPlot();
	void createGuiConfigWindow();
	void createGuiLogWindow();

	void emitParticles(unsigned int index);
	void emitParticles();
	void killParticles(unsigned int index);
	void killParticles();

	bool load(const char *filename);
	void save(const char *filename);
	void pushRecentFile(const nctl::String &filename);

	void applyConfig();
	void applyGuiStyleConfig();
	void clearData();

	bool loadBackgroundImage(const nctl::String &filename);
	void deleteBackgroundImage();
	bool applyBackgroundImageProperties();
	unsigned int retrieveTexture(unsigned int particleSystemIndex);
	bool createTexture(unsigned int index);
	void destroyTexture(unsigned int index);
	void deleteUnusedTextures();

	void createParticleSystem(unsigned int index);
	void cloneParticleSystem(unsigned int srcIndex, unsigned int destIndex, unsigned int numParticles);
	void destroyParticleSystem(unsigned int index);

	nctl::String joinPath(const nctl::String &first, const nctl::String &second);
};

#endif
