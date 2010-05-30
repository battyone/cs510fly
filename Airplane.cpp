#include "Airplane.h"

const Ogre::String Airplane::SCENE_NODE_NAME = "Airplane";

static const float MINIMUM_TIME_STEP = 0.01f;

static const float MASS = 18885.f; // NTO mass of F-15 Eagle in kg
static const float WEIGHT = 184000.0f; // NTO weight of F-15 Eagle in newtons
static const float HEIGHT = 5.63f;  // Height of F-15 Eagle in meters
static const float AIR_DENSITY = 1.2f; // Density of air in kg/m^3 (assumed constant)
static const float PLANFORM_AREA = 56.5; // Planform area of F-15 Eagle in m^2
static const float THRUST_DELTA = 5000.0; // Adjust thrust by 5 kN/s
static const Ogre::Radian ROLL_DELTA(Ogre::Math::HALF_PI/4.0f); // Adjust roll by pi/8 rad/s
static const Ogre::Radian PITCH_DELTA(Ogre::Math::HALF_PI/2.0f); // Adjust pitch by pi/4 rad/s
static const Ogre::Radian YAW_DELTA(Ogre::Math::HALF_PI/8.0F); // Adjust yaw by pi/16 rad/s

Airplane::Airplane(Game * game, Ogre::SceneNode * sceneNode) :
    Object(game, sceneNode),
    delay(0.0f),
    position(sceneNode->getPosition() + Ogre::Vector3(0.0f, HEIGHT, 0.0f)),
    orientation(sceneNode->getOrientation()),
    velocity(Ogre::Vector3::ZERO), thrustAmount(0.0f),
    thrustInc(false), thrustDec(false), pitchInc(false), pitchDec(false),
    rollInc(false), rollDec(false), yawInc(false), yawDec(false) { }

Airplane::~Airplane() { }

Ogre::Vector3 Airplane::thrust() const {
    return thrustAmount * Ogre::Vector3::NEGATIVE_UNIT_Z;
}

Ogre::Vector3 Airplane::lift() const {
    // Pretty sure this is wrong.
    const float aoa = orientation.getPitch(false).valueDegrees();
    const float velSquared = velocity.squaredLength();
    const float cl = liftCoefficient(aoa);
    
    const Ogre::Vector3 liftDir = (orientation.Inverse() * velocity)
            .crossProduct(Ogre::Vector3::NEGATIVE_UNIT_X).normalisedCopy();
    
    return 0.5f * AIR_DENSITY * PLANFORM_AREA * velSquared * cl * liftDir;
}

float Airplane::liftCoefficient(float aoa) const {
    // See docs/lift-coefficient.numbers
    
    if (aoa <= 7.0f)
        return 0.11f * aoa;
    else if (aoa < 15.0f)
        return aoa * (-0.0159f * aoa + 0.3581f) - 0.9474;
    else
        return -0.02f * aoa + 1.15;
}

Ogre::Vector3 Airplane::weight() const {
    return WEIGHT * (orientation.Inverse() * Ogre::Vector3::NEGATIVE_UNIT_Y);
}

Ogre::Vector3 Airplane::drag() const {
    const float aoa = orientation.getPitch(false).valueDegrees();
    const float velSquared = velocity.squaredLength();
    const float cd = dragCoefficient(aoa);
    
    return 0.5f * AIR_DENSITY * PLANFORM_AREA * velSquared * cd * -1 * (orientation.Inverse() * velocity);
}

float Airplane::dragCoefficient(float aoa) const {
    // See docs/drag-coefficient.numbers
    
    const float cl = liftCoefficient(aoa);
    return cl * (0.2837 * cl - 0.0693) + 0.0217;
}

Ogre::Vector3 Airplane::netForce() const {
    // Separate these out for ease of debugging

    Ogre::Vector3
        thrust = this->thrust(),
        lift = this->lift(),
        drag = this->drag(),
        weight = this->weight();
    return thrust + lift + drag + weight;
}

void Airplane::increaseThrust() { thrustInc = true; }
void Airplane::decreaseThrust() { thrustDec = true; }
void Airplane::pitchUp() { pitchInc = true; }
void Airplane::pitchDown() { pitchDec = true; }
void Airplane::rollLeft() { rollDec = true; }
void Airplane::rollRight() { rollInc = true; }
void Airplane::yawLeft() { yawInc = true; }
void Airplane::yawRight() { yawDec = true; }

void Airplane::update(float dt) {
    if (dt == 0.0)
        return;

    if (thrustInc) {
        thrustAmount += THRUST_DELTA * dt;
        thrustInc = false;
    }
    if (thrustDec) {
        thrustAmount -= THRUST_DELTA * dt;
        thrustDec = false;
    }
    if (pitchInc) {
        orientation = orientation * Ogre::Quaternion(PITCH_DELTA * dt, Ogre::Vector3::UNIT_X);
        pitchInc = false;
    }
    if (pitchDec) {
        orientation = orientation * Ogre::Quaternion(PITCH_DELTA * dt, Ogre::Vector3::NEGATIVE_UNIT_X);
        pitchDec = false;
    }
    if (rollInc) {
        orientation = orientation * Ogre::Quaternion(ROLL_DELTA * dt, Ogre::Vector3::NEGATIVE_UNIT_Z);
        rollInc = false;
    }
    if (rollDec) {
        orientation = orientation * Ogre::Quaternion(ROLL_DELTA * dt, Ogre::Vector3::UNIT_Z);
        rollDec = false;
    }
    if (yawInc) {
        orientation = orientation * Ogre::Quaternion(YAW_DELTA * dt, Ogre::Vector3::UNIT_Y);
        yawInc = false;
    }
    if (yawDec) {
        orientation = orientation * Ogre::Quaternion(YAW_DELTA * dt, Ogre::Vector3::NEGATIVE_UNIT_Y);
        yawDec = false;
    }

    dt += delay;
    if (dt < MINIMUM_TIME_STEP) {
        delay = dt;
        return;
    }
    delay = 0.0f;

    Ogre::Vector3 relativeNetForce = netForce();
    Ogre::Vector3 absoluteNetForce = orientation.Inverse() * relativeNetForce;

    // Hi, Newton!
    Ogre::Vector3 acceleration = absoluteNetForce / MASS;

    // Velocity Verlet integration
    
    // Step 1: Calculate next position
    position += dt * velocity + 0.5f * dt * dt * acceleration;

    // Step 2: Calculate velocity a *half*-step from now
    const Ogre::Vector3 halfStepVelocity = velocity + 0.5f * dt * acceleration;

    // Step 3a: Project velocity forward another smidgen by Euler method
    velocity += dt * acceleration;

    // Step 3b: Re-evaluate acceleration in new state
    relativeNetForce = netForce();
    absoluteNetForce = orientation.Inverse() * relativeNetForce;
    acceleration = absoluteNetForce / MASS;

    // Step 4: Calculate new velocity from half-step velocity and new acceleration
    velocity = halfStepVelocity + 0.5f * dt * acceleration;

    // TODO We can get away with not normalizing orientation every frame if need be
    orientation.normalise();

    // XXX This seems heavy-handed.
    if (position.y < HEIGHT * 0.5f) {
        position.y = HEIGHT * 0.5f;
        velocity.y = 0.0f;
    }
    
    sceneNode->setOrientation(orientation);
    sceneNode->setPosition(position);

}

Ogre::Radian Airplane::getPitch() const { return orientation.getPitch(); }
Ogre::Radian Airplane::getRoll() const { return orientation.getRoll(); }
Ogre::Radian Airplane::getYaw() const { return orientation.getYaw(); }
