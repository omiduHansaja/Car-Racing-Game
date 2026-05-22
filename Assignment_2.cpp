//G21328038_Omidu
// Assignment_2.cpp: A program using the TL-Engine

#include <TL-Engine.h>// TL-Engine include file and namespace
#include <iostream>
#include <vector>

using namespace tle;
using namespace std;

//Constants defining the car's speed and physics
const float kMaxSpeed = 66.67f;//Maximum forward speed in units per second
const float kMaxReverseSpeed = 33.34f;
const float kThrustForce = 120.0f;
const float kReverseThrustForce = 60.0f;
const float kDragCoefficient = 0.02f;

//Constants defining various sizes and scales used in the game
const float kScale = 0.5f;
const float kCheckpointSize = 10.0f;
const float kCarRadius = 1.0f;
const float kBoxWidth = 10.0f;
const float kBoxDepth = 10.0f;
const int kNumCheckpoints = 4;
const float kVectorMultiplier = 1.0f;
const float kTankRadius = 1.5f;
const float kSkyBoxY = -840.0f;
const float kphConverter = 3.6f;// Convertor from units per second to kmp/h

//Constants defining the starting positions of the car
const float kCarStartX = 0.0f;
const float kCarStartY = 0.0f;
const float kCarStartZ = -20.0f;

//Constant defined which speed the car can steer
const float kSteerSpeed = 75.0f;

//Constants defining the starting positions of the camera and speed
const float kCameraSpeed = 50.0f;
const float kCameraStartX = 0.0f;
const float kCameraStartY = 10.0f;
const float kCameraStartZ = -30.0f;

//Constant defined the Pi value for angle calculations
const float kPI = 3.14f;

// Struct defining 2D vectors
struct Vector2D {
    float x;
    float y;
};

//Function to add two 2D vectors
Vector2D addVector2D(const Vector2D& v1, const Vector2D& v2) {
    Vector2D result;
    result.x = v1.x + v2.x;//Add X components
    result.y = v1.y + v2.y;//Add Y components
    return result;
}

//Function to scale a 2D vector by a scaler value
Vector2D scalerMultiplier(const Vector2D& v, float scalar) {
    Vector2D result;
    result.x = v.x * scalar;//Scale X component
    result.y = v.y * scalar;//Scale Y component
    return result;
}

//Function to calculate the thrust vector based on the car's position and thrust force
Vector2D calculateThrustVector(IModel* car, float thrust) {
    float angle = -car->GetY() * kPI / 180.0f;
    return { sin(angle) * thrust, cos(angle) * thrust };
}

//Function to calculate the drag vector based on the car's momentum and frame time
Vector2D calculateDragVector(const Vector2D& momentum, float frameTime) {
    float speed = sqrt(momentum.x * momentum.x + momentum.y * momentum.y);
    if (speed > 0.0f) {
        // Calculate drag force
        float dragForce = kDragCoefficient * speed * speed * frameTime;
        return scalerMultiplier(momentum, -dragForce / speed);
    }
    return { 0.0f, 0.0f };//No drag if speed is 0
}

//Function for sphere to box collision detection
bool sphereToBox(float sphereX, float sphereZ, float radius, IModel* box, float boxWidth, float boxDepth) {
    float boxX = box->GetX();
    float boxZ = box->GetZ();

    //Calculate the closest point on the box to the sphere
    float closestX = max(boxX - boxWidth / 2, min(sphereX, boxX + boxWidth / 2));
    float closestZ = max(boxZ - boxDepth / 2, min(sphereZ, boxZ + boxDepth / 2));

    float dx = closestX - sphereX;//Calculate the distance in X
    float dz = closestZ - sphereZ;

    //To check if the distance from the sphere(car) to the cleanest point is less than the sphere's radius
    return (dx * dx + dz * dz) < (radius * radius);
}

//Function for sphere to sphere collision detection
bool sphereToSphere(float x1, float y1, float z1, float radius1, float x2, float y2, float z2, float radius2) {
    float dx = x1 - x2;//distance in X
    float dy = y1 - y2;
    float dz = z1 - z2;
    float distance = sqrt(dx * dx + dy * dy + dz * dz);//Calculate the distance between the centers

    //Check if the distance is less than the sum of the radii
    return distance < (radius1 + radius2);
}

//Function to resolve collisions by reflecting the momentum and moving the car away from the collision point
void resolveCollision(IModel* car, Vector2D& momentum, float normalX, float normalZ) {

    //Reflect the momentum vector over the collision normal
    float reflectedX = momentum.x - 2 * (momentum.x * normalX) * normalX;
    float reflectedZ = momentum.y - 2 * (momentum.y * normalZ) * normalZ;

    //Update momentum components
    momentum.x = reflectedX;
    momentum.y = reflectedZ;

    //Moves the car slightly away from the collision point to prevent getting stuck
    car->MoveLocalX(-normalX * 0.1f);
    car->MoveLocalZ(-normalZ * 0.1f);
}

//Struct defining the checkpoint
struct checkpoint {
    IModel* model;
    bool passed;//To keep track the checkpoint has been passed
};

//Struct to define the tank
struct tank {
    IModel* tankmodel;
    bool buried;
};

int main() {

    // Create a 3D engine (using TLX engine here) and open a window for it
    I3DEngine* myEngine = New3DEngine(kTLX);
    myEngine->StartWindowed();

    // Add default folder for meshes and other media
    myEngine->AddMediaFolder("C:\\ProgramData\\TL-Engine\\Media");
    myEngine->AddMediaFolder("C:\\Users\\User\\Desktop\\Game concepts\\Assignment(sem 2) materials\\Models");
    myEngine->AddMediaFolder("C:\\Users\\User\\Desktop\\Game concepts\\Assignment(sem 2) materials\\Extra Models");

    /**** Set up your scene here ****/

    IMesh* carMesh = myEngine->LoadMesh("race2.x");
    IModel* car = carMesh->CreateModel(kCarStartX, kCarStartY, kCarStartZ);

    IMesh* skyboxMesh = myEngine->LoadMesh("Skybox 07.x");
    IModel* skybox = skyboxMesh->CreateModel(0, kSkyBoxY, 0);

    IMesh* floorMesh = myEngine->LoadMesh("ground.x");
    IModel* floor = floorMesh->CreateModel(0.0f, 0.0f, 0.0f);

    IMesh* checkpointMesh = myEngine->LoadMesh("Checkpoint.x");
    IMesh* isleMesh = myEngine->LoadMesh("IsleStraight.x");
    IMesh* wallMesh = myEngine->LoadMesh("Wall.x");
    IMesh* tankMesh = myEngine->LoadMesh("TankSmall1.x");

    //Camera setup
    ICamera* myCamera = myEngine->CreateCamera(kManual);
    myCamera->SetX(kCameraStartX);
    myCamera->SetY(kCameraStartY);
    myCamera->SetZ(kCameraStartZ);
    myCamera->AttachToParent(car);

    //To add the ui backdrop
    ISprite* uiBackdrop = myEngine->CreateSprite("ui_backdrop.jpg", 0, 650);

    //To load he font to display text on screen
    IFont* myFont = myEngine->LoadFont("Comic Sans MS", 32);

    //Make checkpoints with their models and "passed" as the struct
    checkpoint checkpoints[kNumCheckpoints] = {
        {checkpointMesh->CreateModel(0, 0, 0), false},
        {checkpointMesh->CreateModel(10, 0, 120), false},
        {checkpointMesh->CreateModel(50, 0, 100), false},
        {checkpointMesh->CreateModel(25, 0, 56), false}
    };

    checkpoints[1].model->RotateY(90);

    //Create isles
    IModel* isles[6] = {
        isleMesh->CreateModel(-10, 0, 40),
        isleMesh->CreateModel(10, 0, 40),
        isleMesh->CreateModel(-10, 0, 56),
        isleMesh->CreateModel(10, 0, 56),
        isleMesh->CreateModel(-10, 0, 72),
        isleMesh->CreateModel(10, 0, 72)
    };

    //Create walls
    IModel* walls[4] = {
        wallMesh->CreateModel(-10, 0, 48),
        wallMesh->CreateModel(10, 0, 48),
        wallMesh->CreateModel(-10, 0, 64),
        wallMesh->CreateModel(10, 0, 64)
    };

    //Create tanks
    tank tanks[4] = {
        {tankMesh->CreateModel(5, 0, 30), false},
        {tankMesh->CreateModel(15, 0, 70), false},
        {tankMesh->CreateModel(20, 0, 110), false},
        {tankMesh->CreateModel(12,-0.75, 50), true}
    };
    tanks[3].tankmodel->RotateX(45);

    //Declare momentum and thrust vectors
    Vector2D momentum = { 0.0f, 0.0f };
    Vector2D thrust = { 0.0f, 0.0f };

    //To check the camera's mode
    bool isFirstPersonCam = false;

    //Enum to define the game states
    enum GameStates { start, countdown, racing, gameOver };

    GameStates gameState = start;
    float countdownTimer = 0.0f;

    //Initial game text displayed at the start of the game
    string gameText = "Hit Space to Start";

    int currentCheckpoint = 0;

    int carHealth = 100;
    // The main game loop, repeat until engine is stopped
    while (myEngine->IsRunning()) {

        // Draw the scene
        myEngine->DrawScene();
        float frameTime = myEngine->Timer();
        /**** Update your scene each frame here ****/

        //Handle game states
        if (gameState == start) {
            if (myEngine->KeyHit(Key_Space)) {
                gameState = countdown;
                countdownTimer = 3.0f;//Set countdown timer to 3 seconds
            }
        }
        else if (gameState == countdown) {
            countdownTimer -= frameTime;
            if (countdownTimer <= 0) {
                gameState = racing;
                gameText = "Go!";
            }
            else if (countdownTimer <= 1) {
                gameText = "1";
            }
            else if (countdownTimer <= 2) {
                gameText = "2";
            }
            else {
                gameText = "3";
            }
        }
        else if (gameState == racing) {
            //Check if the car is still alive
            if (carHealth > 0) {
                //Move the car forward
                if (myEngine->KeyHeld(Key_W)) {
                    thrust = calculateThrustVector(car, kThrustForce * frameTime);//Calculate thrust vector
                    momentum = addVector2D(momentum, thrust);//update momentum
                }
                else if (myEngine->KeyHeld(Key_S)) {
                    thrust = calculateThrustVector(car, kReverseThrustForce * frameTime);//Calculate reverse thrust vector
                    thrust = { -thrust.x, -thrust.y };//Invert the thrust direction
                    momentum = addVector2D(momentum, thrust);
                }
                if (myEngine->KeyHeld(Key_A)) {
                    car->RotateY(-kSteerSpeed * frameTime);//Rotate car left
                }
                if (myEngine->KeyHeld(Key_D)) {
                    car->RotateY(kSteerSpeed * frameTime);
                }
            }

            Vector2D drag = calculateDragVector(momentum, frameTime);//Calculate drag
            momentum = addVector2D(momentum, drag);//Update momentum with drag

            //Calculate the speed and direction of car
            float speed = sqrt(momentum.x * momentum.x + momentum.y * momentum.y);//Calculate speed from momentum
            float direction = 1.0f;

            //Determine direction based on momentum
            if (momentum.x != 0.0f || momentum.y != 0.0f) {
                direction = (momentum.y >= 0.0f) ? 1.0f : -1.0f;
            }

            //Limit speed to maximum forward speed
            if (direction > 0.0f && speed > kMaxSpeed) {
                momentum = scalerMultiplier(momentum, kMaxSpeed / speed);
            }

            //Limit speed to maximum reverse speed
            else if (direction < 0.0f && speed > kMaxReverseSpeed) {
                Vector2D reverseDir = scalerMultiplier(momentum, 1.0f / speed);
                momentum = scalerMultiplier(reverseDir, kMaxReverseSpeed);
            }

            //Move the car based on momentum
            car->MoveLocalZ(momentum.y * frameTime);
            car->MoveLocalX(momentum.x * frameTime);

            //Checkpoint collision 
            if (!checkpoints[currentCheckpoint].passed &&
                sphereToBox(car->GetX(), car->GetZ(), kCarRadius, checkpoints[currentCheckpoint].model, kCheckpointSize, kCheckpointSize)) {
                checkpoints[currentCheckpoint].passed = true;//Mark checkpoint as passed
                currentCheckpoint++;//Continue to next checkpoint
                if (currentCheckpoint == kNumCheckpoints) {
                    gameState = gameOver;
                    gameText = "Game Over.";
                }
                else if (currentCheckpoint == 3) {
                    gameText = "Stage 3 complete";
                }
                else if (currentCheckpoint == 2) {
                    gameText = "Stage 2 complete";
                }
                else if (currentCheckpoint == 1) {
                    gameText = "Stage 1 complete";
                }
            }

            //Check for collisions with checkpoints
            for (int i = 0; i < kNumCheckpoints; ++i) {
                if (sphereToSphere(car->GetX(), car->GetY(), car->GetZ(), kCarRadius, checkpoints[i].model->GetX(),
                    checkpoints[i].model->GetY(), checkpoints[i].model->GetZ(), kCarRadius)) {

                    resolveCollision(car, momentum, checkpoints[i].model->GetX() - car->GetX(),//Resolve collision
                        checkpoints[i].model->GetZ() - car->GetZ());
                    carHealth--;//Decrease car health
                }
            }
            //Check for collisions with isles
            for (int i = 0; i < 6; ++i) {
                if (sphereToBox(car->GetX(), car->GetZ(), kCarRadius, isles[i], kBoxWidth, kBoxDepth)) {
                    resolveCollision(car, momentum, isles[i]->GetX() - car->GetX(), isles[i]->GetZ() - car->GetZ());
                    carHealth--;
                }
            }
            //Check for collisions with walls
            for (int i = 0; i < 4; ++i) {
                if (sphereToBox(car->GetX(), car->GetZ(), kCarRadius, walls[i], kBoxWidth, kBoxDepth)) {
                    resolveCollision(car, momentum, walls[i]->GetX() - car->GetX(), walls[i]->GetZ() - car->GetZ());
                    carHealth--;
                }
            }

            //To check if the car is damaged
            if (carHealth <= 0) {
                gameState = gameOver;
                gameText = "Game Over";
            }

            //To calculate the display speed of the car in kmp/h
            float displaySpeed = speed * kphConverter;

            //To draw the speed and health information on screen
            myFont->Draw("Speed: " + to_string(static_cast<int>(displaySpeed)) + " km/h", 250, 650, kBlack);
            myFont->Draw("Health: " + to_string(carHealth), 500, 650, kRed);
        }

        //Camera controls
        if (myEngine->KeyHeld(Key_Up)) {
            myCamera->MoveLocalZ(kCameraSpeed * frameTime);
        }
        if (myEngine->KeyHeld(Key_Down)) {
            myCamera->MoveLocalZ(-kCameraSpeed * frameTime);
        }
        if (myEngine->KeyHeld(Key_Right)) {
            myCamera->MoveLocalX(kCameraSpeed * frameTime);
        }
        if (myEngine->KeyHeld(Key_Left)) {
            myCamera->MoveLocalX(-kCameraSpeed * frameTime);
        }

        //Set the camera to third person view(default)
        if (myEngine->KeyHit(Key_1)) {
            isFirstPersonCam = false;
            myCamera->ResetOrientation();
            myCamera->SetLocalPosition(kCameraStartX, kCameraStartY, kCameraStartZ);
            myCamera->AttachToParent(car);
        }
        //Set the first person cam
        if (myEngine->KeyHit(Key_2)) {
            isFirstPersonCam = true;
            myCamera->DetachFromParent();
            myCamera->SetLocalPosition(0.0f, 2.0f, 0.0f);
            myCamera->RotateY(180);
            myCamera->AttachToParent(car);
        }

        //Handle mouse capture 
        int mouseX = myEngine->GetMouseMovementX();
        int mouseY = myEngine->GetMouseMovementY();

        if (mouseX != 0) {
            myCamera->RotateLocalY(mouseX * frameTime * 50.0f);
        }
        if (mouseY != 0) {
            float currentRotationX = myCamera->GetLocalX();
            float newRotationX = currentRotationX + (mouseY * frameTime * 50.0f);

            //To limit the camera rotation angle
            if (newRotationX > -80.0f && newRotationX < 80.0f) {
                myCamera->RotateLocalX(mouseY * frameTime * 50.0f);
            }
        }

        //Draw the current game text on screen
        myFont->Draw(gameText, 0, 650, kBlue);

        //To exit from the game(if the user wants to)
        if (myEngine->KeyHit(Key_Escape)) {
            myEngine->Stop();
        }
    }

    myEngine->Delete();

}