// Core Libraries
#include <iostream>
#include <string>
#include <math.h>
#include <map> // for std::map
#include <memory> // for std::shared_ptr

// 3rd Party Libraries
#include <GLEW\glew.h>
#include <GLUT\glut.h>
#include <TTK\OBJMesh.h>
#include <TTK\Camera.h>
#include <IL/il.h> // for ilInit()
#include <glm\vec3.hpp>
#include <glm\gtx\color_space.hpp>

// User Libraries
#include "Shader.h"
#include "ShaderProgram.h"
#include "GameObject.h"
#include "FrameBufferObject.h"
#include "TTK\Utilities.h"

// Defines and Core variables
#define FRAMES_PER_SECOND 60
const int FRAME_DELAY = 1000 / FRAMES_PER_SECOND; // Milliseconds per frame

int windowWidth = 800;
int windowHeight = 600;

glm::vec3 mousePosition; // x,y,0
glm::vec3 mousePositionFlipped; // x, height - y, 0

// A few conversions to know
const float degToRad = 3.14159f / 180.0f;
const float radToDeg = 180.0f / 3.14159f;

float deltaTime = 0.0f; // amount of time since last update (set every frame in timer callback)

glm::vec3 position;
float movementSpeed = 5.0f;
glm::vec4 lightPos;

// Cameras
TTK::Camera playerCamera; // the camera you move around with wasd + mouse

// Asset databases
std::map<std::string, std::shared_ptr<TTK::MeshBase>> meshes;
std::map<std::string, std::shared_ptr<GameObject>> gameobjects;

// Materials
std::shared_ptr<Material> defaultMaterial;
std::shared_ptr<Material> toonMaterial;
std::shared_ptr<Material> outlineMaterial;

enum GameMode
{
	DEFAULT,
	TOON,
	TOON_OUTLINES
};

GameMode currentMode = DEFAULT;

void initializeShaders()
{
	std::string shaderPath = "../../Assets/Shaders/";

	// Load shaders

	Shader v_default, v_passThru;
	v_default.loadShaderFromFile(shaderPath + "default_v.glsl", GL_VERTEX_SHADER);
	v_passThru.loadShaderFromFile(shaderPath + "passThru_v.glsl", GL_VERTEX_SHADER);

	Shader f_default, f_toon, f_solidColour;
	f_default.loadShaderFromFile(shaderPath + "default_f.glsl", GL_FRAGMENT_SHADER);
	f_toon.loadShaderFromFile(shaderPath + "toon_f.glsl", GL_FRAGMENT_SHADER);
	f_solidColour.loadShaderFromFile(shaderPath + "solidColour_f.glsl", GL_FRAGMENT_SHADER);

	// Default material that all objects use
	defaultMaterial = std::make_shared<Material>();
	defaultMaterial->shader->attachShader(v_default);
	defaultMaterial->shader->attachShader(f_default);
	defaultMaterial->shader->linkProgram();

	// Toon shaded material
	toonMaterial = std::make_shared<Material>();
	toonMaterial->shader->attachShader(v_default);
	toonMaterial->shader->attachShader(f_toon);
	toonMaterial->shader->linkProgram();

	// Solid colour material (for outlines)
	outlineMaterial = std::make_shared<Material>();
	outlineMaterial->shader->attachShader(v_passThru);
	outlineMaterial->shader->attachShader(f_solidColour);
	outlineMaterial->shader->linkProgram();
}

void initializeScene()
{
	std::string meshPath = "../../Assets/Models/";
	
	std::shared_ptr<TTK::OBJMesh> floorMesh = std::make_shared<TTK::OBJMesh>();
	std::shared_ptr<TTK::OBJMesh> sphereMesh = std::make_shared<TTK::OBJMesh>();
	std::shared_ptr<TTK::OBJMesh> torusMesh = std::make_shared<TTK::OBJMesh>();

	floorMesh->loadMesh(meshPath + "floor.obj");
	sphereMesh->loadMesh(meshPath + "sphere.obj");
	torusMesh->loadMesh(meshPath + "torus.obj");

	// Note: looking up a mesh by it's string name is not the fastest thing,
	// you don't want to do this every frame, once in a while (like now) is fine.
	// If you need you need constant access to a mesh (i.e. you need it every frame),
	// store a reference to it so you don't need to look it up every time.
	meshes["floor"] = floorMesh;
	meshes["sphere"] = sphereMesh;
	meshes["torus"] = torusMesh;

	// Create objects
	gameobjects["floor"] = std::make_shared<GameObject>(glm::vec3(0.0f, 0.0f, 0.0f), floorMesh, defaultMaterial);
	gameobjects["sphere"] = std::make_shared<GameObject>(glm::vec3(0.0f, 5.0f, 0.0f), sphereMesh, defaultMaterial);
	gameobjects["bigTorus"] = std::make_shared<GameObject>(glm::vec3(0.0f, 2.0f, 0.0f), torusMesh, defaultMaterial);
	gameobjects["bigTorus"]->setScale(3.0f);
	
	// Generate a bunch of objects in a circle
	int numObjects = 12;
	float circleStep = 360.f / numObjects;
	float colorStep = 1.0f / numObjects;
	for (int i = 0; i < numObjects; i++)
	{
		glm::vec3 pos, colour = getColorFromHue(colorStep * i);
		std::string name = ("torus" + std::to_string(i));
		pos.x = cos(circleStep*(float)i*degToRad) * 10.0f;
		pos.y = 2.0f;
		pos.z = sin(circleStep*(float)i*degToRad) * 10.0f;
		gameobjects[name] = std::make_shared<GameObject>(pos, torusMesh, defaultMaterial);
		gameobjects[name]->colour = glm::vec4(colour, 1.0f);
	}

	// Set object properties
	gameobjects["sphere"]->colour = glm::vec4(1.0f);
}

void updateScene()
{
	// Move light in simple circular path
	static float ang = 0.0f;

	ang += deltaTime;
	lightPos.x = cos(ang) * 10.0f;
	lightPos.y = cos(ang*4.0f) * 2.0f + 10.0f;
	lightPos.z = sin(ang) * 10.0f;
	lightPos.w = 1.0f;

	gameobjects["sphere"]->setPosition(lightPos);

	// Update all game objects
	for (auto itr = gameobjects.begin(); itr != gameobjects.end(); ++itr)
	{
		auto gameobject = itr->second;

		// Remember: root nodes are responsible for updating all of its children
		// So we need to make sure to only invoke update() for the root nodes.
		// Otherwise some objects would get updated twice in a frame!
		if (gameobject->isRoot())
			gameobject->update(deltaTime);
	}
}

void drawScene(TTK::Camera& cam)
{
	for (auto itr = gameobjects.begin(); itr != gameobjects.end(); ++itr)
	{
		auto gameobject = itr->second;

		if (gameobject->isRoot())
			gameobject->draw(cam);
	}
}

void setMaterialForAllGameObjects(std::shared_ptr<Material> mat)
{
	for (auto itr = gameobjects.begin(); itr != gameobjects.end(); ++itr)
	{
		itr->second->material = mat;
	}
}

void updateTorusColour();

// This is where we draw stuff
void DisplayCallbackFunction(void)
{
	// Update cameras (there's two now!)
	playerCamera.update();

	// Update all gameobjects
	updateScene();

	updateTorusColour();


	switch (currentMode)
	{
		// Regular lambert shading
		case DEFAULT: // press 1
		{
			// Turn Culling off
			glDisable(GL_CULL_FACE);

			// Turn Solid Fill On
			glPolygonMode(GL_FRONT, GL_FILL);

			// Clear back buffer
			FrameBufferObject::clearFrameBuffer(glm::vec4(0.8f, 0.8f, 0.8f, 0.0f));

			// Tell all game objects to use the default material
			setMaterialForAllGameObjects(defaultMaterial);

			// Set material properties
			defaultMaterial->vec4Uniforms["u_lightPos"] = playerCamera.viewMatrix * lightPos;

			// Draw the scene to the back buffer
			drawScene(playerCamera);
		}
		break;

		// Toon shading
		case TOON: // press 2
		{
			// Turn Culling off
			glDisable(GL_CULL_FACE);

			// Turn Solid Fill On
			glPolygonMode(GL_FRONT, GL_FILL);

			// Clear back buffer
			FrameBufferObject::clearFrameBuffer(glm::vec4(0.0f, 0.8f, 0.8f, 0.0f));
	

			// Tell all game objects to use the toon shading material
			setMaterialForAllGameObjects(toonMaterial);

			// Set material properties
			toonMaterial->vec4Uniforms["u_lightPos"] = playerCamera.viewMatrix * lightPos;

			// Draw the scene to the back buffer
			drawScene(playerCamera);
		}
		break;

		// Toon shading + outlines
		case TOON_OUTLINES: // press 3
		{
			// Clear back buffer
			FrameBufferObject::clearFrameBuffer(glm::vec4(0.8f, 0.0f, 0.8f, 0.0f));

			glCullFace(GL_FRONT_FACE);

			glPolygonMode(GL_BACK, GL_LINE);

			glLineWidth(6.0f);

			


			// Tell all game objects to use the toon shading material
			setMaterialForAllGameObjects(outlineMaterial);

			// Set material properties
			outlineMaterial->vec4Uniforms["u_lightPos"] = playerCamera.viewMatrix * lightPos;

			

			// Draw the scene to the back buffer
			drawScene(playerCamera);

			// Turn Culling off
			glCullFace(GL_BACK);

			// Turn Solid Fill On
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			// Tell all game objects to use the toon shading material
			setMaterialForAllGameObjects(toonMaterial);

			// Set material properties
			toonMaterial->vec4Uniforms["u_lightPos"] = playerCamera.viewMatrix * lightPos;

			// Draw the scene to the back buffer
			drawScene(playerCamera);

		}
		break;
	}

	/* Swap Buffers to Make it show up on screen */
	glutSwapBuffers();
}

/* function void KeyboardCallbackFunction(unsigned char, int,int)
* Description:
*   - this handles keyboard input when a button is pressed
*/
void KeyboardCallbackFunction(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // the escape key
	case 'q': // the 'q' key
	case 'Q':
		playerCamera.moveUp();
		break;
	case 'e':
	case 'E':
		playerCamera.moveDown();
	case 'W':
	case 'w':
		playerCamera.moveForward();
		break;
	case 'S':
	case 's':
		playerCamera.moveBackward();
		break;
	case 'A':
	case 'a':
		playerCamera.moveRight();
		break;
	case 'D':
	case 'd':
		playerCamera.moveLeft();
		break;
	}
}

/* function void KeyboardUpCallbackFunction(unsigned char, int,int)
* Description:
*   - this handles keyboard input when a button is lifted
*/
void KeyboardUpCallbackFunction(unsigned char key, int x, int y)
{
	switch (key)
	{
		case '1':
			currentMode = DEFAULT;
		break;

		case '2':
			currentMode = TOON;
		break;

		case '3':
			currentMode = TOON_OUTLINES;
		break;


	default:
		break;
	}
}

/* function TimerCallbackFunction(int value)
* Description:
*  - this is called many times per second
*  - this enables you to animate things
*  - no drawing, just changing the state
*  - changes the frame number and calls for a redisplay
*  - FRAME_DELAY is the number of milliseconds to wait before calling the timer again
*/
void TimerCallbackFunction(int value)
{
	// Calculate new deltaT for potential updates and physics calculations
	static int elapsedTimeAtLastTick = 0;
	int totalElapsedTime = glutGet(GLUT_ELAPSED_TIME);

	deltaTime = totalElapsedTime - elapsedTimeAtLastTick;
	deltaTime /= 1000.0f;
	elapsedTimeAtLastTick = totalElapsedTime;

	/* this call makes it actually show up on screen */
	glutPostRedisplay();
	/* this call gives it a proper frame delay to hit our target FPS */
	glutTimerFunc(FRAME_DELAY, TimerCallbackFunction, 0);
}

/* function WindowReshapeCallbackFunction()
* Description:
*  - this is called whenever the window is resized
*  - and sets up the projection matrix properly
*/
void WindowReshapeCallbackFunction(int w, int h)
{
	/* Update our Window Properties */
	windowWidth = w;
	windowHeight = h;

	playerCamera.winHeight = h;
	playerCamera.winWidth = w;
}


void MouseClickCallbackFunction(int button, int state, int x, int y)
{
	mousePosition.x = x;
	mousePosition.y = y;

	mousePositionFlipped = mousePosition;
	mousePositionFlipped.y = windowHeight - mousePosition.y;
}

void SpecialInputCallbackFunction(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		position.z += movementSpeed * deltaTime;
		break;
	case GLUT_KEY_DOWN:
		position.z -= movementSpeed * deltaTime;
		break;
	case GLUT_KEY_LEFT:
		position.x += movementSpeed * deltaTime;
		break;
	case GLUT_KEY_RIGHT:
		position.x -= movementSpeed * deltaTime;
		break;
	}
}

// Called when the mouse is clicked and moves
void MouseMotionCallbackFunction(int x, int y)
{
	if (mousePosition.length() > 0)
		playerCamera.processMouseMotion(x, y, mousePosition.x, mousePosition.y, deltaTime);

	mousePosition.x = x;
	mousePosition.y = y;

	mousePositionFlipped = mousePosition;
	mousePositionFlipped.y = windowHeight - mousePosition.y;
}

/* function main()
* Description:
*  - this is the main function
*  - does initialization and then calls glutMainLoop() to start the event handler
*/
int main(int argc, char **argv)
{
	// Memory Leak Detection
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	/* initialize the window and OpenGL properly */
	glutInit(&argc, argv);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("Tutorial");

	/* set up our function callbacks */
	glutDisplayFunc(DisplayCallbackFunction);
	glutKeyboardFunc(KeyboardCallbackFunction);
	glutKeyboardUpFunc(KeyboardUpCallbackFunction);
	glutReshapeFunc(WindowReshapeCallbackFunction);
	glutMouseFunc(MouseClickCallbackFunction);
	glutMotionFunc(MouseMotionCallbackFunction);
	glutTimerFunc(1, TimerCallbackFunction, 0);
	glutSpecialFunc(SpecialInputCallbackFunction);

	// Init GLEW
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		std::cout << "TTK::InitializeTTK Error: GLEW failed to init" << std::endl;
	}
	printf("OpenGL version: %s, GLSL version: %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Init IL
	ilInit();

	// Init GL
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Initialize scene
	initializeShaders();
	initializeScene();

	/* Start Game Loop */
	deltaTime = glutGet(GLUT_ELAPSED_TIME);
	deltaTime /= 1000.0f;

	glutMainLoop();

	return 0;
}

void updateTorusColour()
{
	// All hail torus god
	static float t = 0.0f;
	t += deltaTime * 0.1f;
	if (t >= 1.0f) t = 0.0f;
	gameobjects["bigTorus"]->colour = getColorFromHue(t);
}