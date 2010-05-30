// Game.h
// @author Eric D. Wills

#ifndef __TERRAIN_DEMO_H__
#define __TERRAIN_DEMO_H__

#include "Ogre.h"

#ifndef LINUX
#include <CoreFoundation/CoreFoundation.h>
#endif

class Airplane;
class Display;
class InputListener;
class Level;
class World;

class Game {
private:
    Airplane * airplane;
    InputListener* inputListener;
    Ogre::Root* root;
    Ogre::SceneManager* sceneManager;
    Ogre::RenderWindow* renderWindow;
    Ogre::Camera* camera;
    Ogre::RaySceneQuery* raySceneQuery;
    Ogre::Ray terrainRay;
    Ogre::String mResourcePath;
    Ogre::String levelPath;
    Level *currentLevel;
    World *world;
    Display * display;
    
    bool breaking;

public:
    Game();

    ~Game();

    Ogre::SceneManager* getSceneManager() { return sceneManager; }
    Ogre::String getLevelPath() { return levelPath; }
    bool windowClosed() const { return renderWindow->isClosed(); }
    Airplane * getAirplane() { return airplane; }

    float getTerrainHeightAt(float x, float y);
    Level *getCurrentLevel(){ return currentLevel; };
    World * getWorld() { return world; }
    void init();
    void update(float dt);

    void setBreak();
    bool checkBreak();
private:
    bool setup();
};

#endif
