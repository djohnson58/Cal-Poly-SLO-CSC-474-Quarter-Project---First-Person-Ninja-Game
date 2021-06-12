/*
CPE/CSC 474 Lab base code Eckhardt/Dahl
based on CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt

------------------------------------------------------

Cal Poly SLO
CSC 474 Computer animation
Prof. Christian Eckhardt
Quater Project - "First Person Ninja Game"
by Davin Johnson
6/11/2021
*/

#define _USE_MATH_DEFINES

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <glad/glad.h>
#include "SmartTexture.h"
#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"
#include "Shape.h"
#include "skmesh.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// assimp
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/vector3.h"
#include "assimp/scene.h"
#include <assimp/mesh.h>

#define NUM_ENEMIES 10
#define NUM_PROJECTILES 5
#define ENEMY_MAX_HEALTH 6
#define NUM_COLLECTIBLES 6

using namespace std;
using namespace glm;
using namespace Assimp;
enum playerState {ALIVE, DEAD, INVULN};

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

class camera
{
public:
	glm::vec3 pos, rot, start;
	int w, a, s, d, space, lctrl, shift;
	float charHeight;
	//mouse vars
	vec3 lookAtPoint, cameraUp, cameraRight;

	//mouse tracker
	bool firstMouse, mousePressed, look;
	float lastX, lastY, xoffset, yoffset;
	double xPos, yPos, xStartPos, yStartPos;

	bool invuln = false;

	//window info
	int width, height;

	camera()
	{
		w = a = s = d = space = lctrl = shift = 0;
		start = glm::vec3(0, -1.4, -58);
		pos = start;
		rot = glm::vec3(0, 0, 0);
		charHeight = pos.y;
		firstMouse = true;
		mousePressed = false;
		look = true;
		lastX = lastY = xoffset = yoffset = 0;
		xPos = yPos = xStartPos = yStartPos = 0;
		width = height = 0;
		lookAtPoint = cameraUp = cameraRight = vec3(0);
	}
	glm::mat4 process(double ftime, GLFWwindow* window, vec3 mapScale, playerState* pState, bool gameWin)
	{
		checkPlayerDeadOrWin(window, pState, gameWin);

		if (look) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwGetCursorPos(window, &xPos, &yPos);
			getCameraLookAt(width, height);
		}
		float speed = 0;
		float strafeSpeed = 0;
		//move camera forward or back
		if (w == 1)
		{
			speed = 8.5 * ftime;
		}
		else if (s == 1)
		{
			speed = -5.5 * ftime;
		}
		//move camera left or right
		if (d == 1)
		{
			strafeSpeed = -7 * ftime;
		}
		else if (a == 1)
		{
			strafeSpeed = 7 * ftime;
		}
		//jump and crouch
		if (space == 1)
		{
			//not implemented
		}
		else if (lctrl == 1)
		{
			charHeight = -.8;
			speed *= .3;
			strafeSpeed *= .3;
		}
		else {
			charHeight = start.y;
		}
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::vec4 dir = glm::vec4(strafeSpeed, 0, speed, 1);
		dir = dir * R;
		R = Rx * R;
		pos += glm::vec3(dir.x, 0, dir.z);
		pos.y = charHeight;

		if (pos.x > mapScale.x)
			pos.x = mapScale.x;
		else if (pos.x < -mapScale.x)
			pos.x = -mapScale.x;
		if (pos.z > mapScale.z)
			pos.z = mapScale.z;
		else if (pos.z < -mapScale.z)
			pos.z = -mapScale.z;

		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R * T;
	}
	void getCameraLookAt(int width, int height)
	{
		if (mousePressed)
		{
			if (firstMouse)
			{
				lastX = xPos;
				lastY = yPos;
				firstMouse = false;
			}
			xoffset = xPos - lastX;
			yoffset = lastY - yPos;
			lastX = xPos;
			lastY = yPos;

			float sensitivity = 1.5f;
			xoffset *= sensitivity;
			yoffset *= sensitivity;

			rot.y += (M_PI) * (xoffset / width);
			rot.x -= (M_PI) * (yoffset / width);
			if (rot.x < -M_PI / 2)
				rot.x = -M_PI / 2;
			else if (rot.x > M_PI / 2)
				rot.x = M_PI / 2;

		}
		else
		{
			lastX = xPos;
			lastY = yPos;
		}

	}
	void reset(GLFWwindow* window) {
		pos = start;
		rot = vec3(0);
		firstMouse = true;
		glfwSetCursorPos(window, xStartPos, yStartPos);
	}
	void checkPlayerDeadOrWin(GLFWwindow* window, playerState* pState, bool gameWin) {
		if (*pState == DEAD || gameWin) {
			w = 0;
			s = 0;
			a = 0;
			d = 0;
			space = 0;
			lctrl = 0;
			look = false;
		}
	}
	void getDirPos(vec3& position, vec3& dir) {
		position = -pos;
		mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		vec4 dir4 = glm::vec4(0, 0, -1, 0);
		dir4 = dir4 * Ry;
		dir = vec3(dir4);
	}
};

camera mycam;

class enemy
{
public:
	vec3 startPos, pos, moveDir, respawnPos;
	int animation, detectDist, stealthDetectDist, health;
	bool idle, turned;
	double timer;
	float dirRad;

	enemy(vec3 startingPosition) {

		startPos = respawnPos = pos = startingPosition;
		moveDir = vec3(0, 0, 1);
		animation = 3;
		idle = true;
		turned = false;
		timer = rand() % 10;
		dirRad = 0;
		detectDist = 10;
		stealthDetectDist = 2;
		health = ENEMY_MAX_HEALTH;
	}
	glm::mat4 update(double ftime, vec3 playerPos, vec3 mapScale, playerState* pState, vector<pair<vec3, double>>* smokePositions, bool gameWin) {

		checkKillPlayer(playerPos, pState, gameWin);
		checkDead(smokePositions);

		timer += ftime;
		float move = 0;
		if (idle) { //enemy not targeting player
			if ((!mycam.lctrl && glm::distance(pos, playerPos) <= detectDist) || 
				(mycam.lctrl && glm::distance(pos, playerPos) <= stealthDetectDist)) {
				idle = false;
			}
			else {
				if (((int)timer % 10) < 5) { //stand idle
					animation = 3;
					move = 0;
					turned = false;
				}
				else { //walk randomly
					animation = 2;
					move = 1.5;
				}
				if (((int)timer % 10) == 5 && !turned) {
					turned = true;
					dirRad += (5. * M_PI) / 6.;
					moveDir.x = sin(dirRad);
					moveDir.z = cos(dirRad);
				}
			}
		}
		else { //enemy has spotted player
			animation = 1;
			move = 6.0;
			if ((!mycam.lctrl && glm::distance(pos, playerPos) > 20) ||
				(mycam.lctrl && glm::distance(pos, playerPos) > 10)) {
				idle = true;
			}
			else {
				vec3 self = vec3(pos.x, 0, pos.z);
				vec3 player = vec3(playerPos.x, 0, playerPos.z);
				moveDir = player - self;
				moveDir = normalize(moveDir);
			}
			timer = 0;
		}

		float angle = atan(moveDir.x, moveDir.z);
		glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
		pos += glm::vec3(moveDir.x * move * (float)ftime, 0, moveDir.z * move * (float)ftime);

		// constrain enemy to map area
		if (pos.x > mapScale.x)
			pos.x = mapScale.x;
		else if (pos.x < -mapScale.x)
			pos.x = -mapScale.x;
		if (pos.z > mapScale.z)
			pos.z = mapScale.z;
		else if (pos.z < -mapScale.z)
			pos.z = -mapScale.z;

		glm::mat4 T = glm::translate(glm::mat4(1), pos);

		return T * R;
	}
	void checkKillPlayer(vec3 playerPos, playerState* pState, bool gameWin) {
		vec3 pPos = vec3(playerPos.x, 0, playerPos.z);
		if (glm::distance(pos, pPos) <= 0.5 && *pState != INVULN && !gameWin) {
			*pState = DEAD;
		}
	}
	void checkDead(vector<pair<vec3, double>>* smokePositions) {
		//check health and determine if enemy should die
		if (health <= 0) {
			smokePositions->push_back(make_pair(vec3(pos.x, 1.2, pos.z), 0.));
			respawn();
		}
	}
	void damage(int amt) {
		health -= amt;
		idle = false;
	}
	void respawn() {
		respawnPos = respawnPos * -1.0f;
		pos = respawnPos;
		health = ENEMY_MAX_HEALTH;
		idle = true;
		cout << "enemy killed, respawning at: (" << respawnPos.x << ", " << respawnPos.y << ", " << respawnPos.z << ")" << endl;
	}
	void reset() {
		pos = startPos;
		respawnPos = startPos;
		health = ENEMY_MAX_HEALTH;
		idle = true;
	}
};

class projectile
{
public:
	vec3 startPos, pos, moveDir;
	bool active;
	double flightTimer;
	float spawnOffset, moveSpeed, spinSpeed;

	projectile(vec3 startingPosition) {

		startPos = pos = startingPosition;
		moveDir = vec3(0);
		active = false;
		flightTimer = 0;
		spawnOffset = 0.1f;
		moveSpeed = 10.5f;
		spinSpeed = 10.0f;
	}
	glm::mat4 update(double ftime, vector<enemy> &enemies) {

		checkHitEnemy(enemies);

		flightTimer += ftime;

		if (flightTimer >= 3.0) {
			reset();
		}

		if (active) {
			pos.y = 1.1f;
			pos += glm::vec3(moveDir.x * moveSpeed * (float)ftime, 0, moveDir.z * moveSpeed * (float)ftime);
		}

		float spinRot = flightTimer * spinSpeed;
		glm::mat4 R = glm::rotate(glm::mat4(1.0f), spinRot, glm::vec3(0, 1, 0));

		glm::mat4 T = glm::translate(glm::mat4(1), pos);

		return T * R;
	}
	void checkHitEnemy(vector<enemy> &enemies) {
		//check if collision with any enemies
		vec3 thisPos = vec3(pos.x, 0, pos.z);
		for (int i = 0; i < enemies.size(); i++) {
			if (distance(thisPos, enemies[i].pos) <= 0.3f) {
				cout << "enemy hit with projectile!" << endl;
				enemies[i].damage(2);
				reset();
				break;
			}
		}
	}
	void spawn(vec3 playerPos, vec3 playerDir) {
		pos.x = playerPos.x + playerDir.x * spawnOffset;
		pos.z = playerPos.z + playerDir.z * spawnOffset;
		vec3 movementDir = normalize(playerDir);
		moveDir.x = movementDir.x;
		moveDir.z = movementDir.z;
		active = true;
	}
	void reset() {
		pos = startPos;
		flightTimer = 0;
		active = false;
		moveDir = vec3(0);
	}
};

class Application : public EventCallbacks
{
public:
	WindowManager * windowManager = nullptr;

	// shader programs
	std::shared_ptr<Program> psky, skinProg, mapObjectsProg, cubeProg, ptree, animTexProg;

	// skinned meshes
	SkinnedMesh ninjaMesh;
	SkinnedMesh katanaHands;
	SkinnedMesh katanaOneHand;
	SkinnedMesh leftHand;

	// textures
	shared_ptr<SmartTexture> skyTex, templeTex, groundTex, rockTex, UITex, UIWinTex, UICheatTex, katanaTex, shurikenTex, smokeTex, crateTex;
	
	// shapes
	shared_ptr<Shape> skyShape, templeShape, cubeShape, quadShape, rockShape, treeShape, katanaShape, shurikenShape, tCubeShape;

	//game info
	playerState pState = ALIVE;
	vec3 mapScale = vec3(60, .5, 60);
	vector<enemy> enemies;
	vector<projectile> shurikens;
	vector<pair<vec3, double>> smokePositions;
	vector<vec3> collectibles;
	bool playerWin = false;
	bool playerAttack = false;
	bool swung = false;
	bool playerThrow = false;
	bool thrown = false;
	int collectibleCnt = 0;

	//deforming info
	vector<vec3> boundingBoxActual, boundingBoxInitial;
	vec3 minv, maxv, Apt, Bpt, Cpt, Dpt, Ept, Fpt, Gpt, Hpt;
	vec3  AptDraw, BptDraw, CptDraw, DptDraw, EptDraw, FptDraw, GptDraw, HptDraw;
	vector<float> pVals;
	vector<vec3> newVerts;

	//mouse input val
	bool firstClick = true;

	//object placement testing values
	bool spaceBar = false;
	float xVal = 0;
	float yVal = 0;
	float zVal = 0;
	float xAngle = 0;
	float yAngle = 0;
	float zAngle = 0;
	
	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		if (pState == DEAD || playerWin) {
			if (key == GLFW_KEY_R && action == GLFW_PRESS)
			{
				//reset the game
				mycam.reset(window);
				for (int i = 0; i < enemies.size(); i++) {
					enemies[i].reset();
				}
				pState = ALIVE;
				mycam.look = true;
				playerWin = false;
				makeCollectibles();
			}
		}
		else {
			if (key == GLFW_KEY_W && action == GLFW_PRESS)
			{
				mycam.w = 1;
			}
			if (key == GLFW_KEY_W && action == GLFW_RELEASE)
			{
				mycam.w = 0;
			}
			if (key == GLFW_KEY_S && action == GLFW_PRESS)
			{
				mycam.s = 1;
			}
			if (key == GLFW_KEY_S && action == GLFW_RELEASE)
			{
				mycam.s = 0;
			}
			if (key == GLFW_KEY_A && action == GLFW_PRESS)
			{
				mycam.a = 1;
			}
			if (key == GLFW_KEY_A && action == GLFW_RELEASE)
			{
				mycam.a = 0;
			}
			if (key == GLFW_KEY_D && action == GLFW_PRESS)
			{
				mycam.d = 1;
			}
			if (key == GLFW_KEY_D && action == GLFW_RELEASE)
			{
				mycam.d = 0;
			}
			if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
			{
				mycam.space = 1;
				spaceBar = true;
			}
			if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
			{
				mycam.space = 0;
				spaceBar = false;
			}
			if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
			{
				mycam.lctrl = 1;
			}
			if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
			{
				mycam.lctrl = 0;
			}
			if (key == GLFW_KEY_E && action == GLFW_PRESS)
			{
				if (pState != DEAD)
					playerThrow = true;
			}
			if (key == GLFW_KEY_UP && action == GLFW_PRESS)
			{
				if (spaceBar) {
					yAngle += 0.1;
				}
				else
					yVal += 0.05;
			}
			if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
			{
				if (spaceBar) {
					yAngle -= 0.1;
				}
				else
					yVal -= 0.05;
			}
			if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
			{
				if (spaceBar) {
					xAngle += 0.1;
				}
				else
					xVal += 0.05;
			}
			if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
			{
				if (spaceBar) {
					xAngle -= 0.1;
				}
				else
					xVal -= 0.05;
			}
			if (key == GLFW_KEY_COMMA && action == GLFW_PRESS)
			{
				if (spaceBar) {
					zAngle += 0.1;
				}
				else
					zVal += 0.05;
			}
			if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS)
			{
				if (spaceBar) {
					zAngle -= 0.1;
				}
				else
					zVal -= 0.05;
			}
		}

		//shift key for locking/unlocking mouse to window
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
		{
			mycam.shift = 1;
		}
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
		{
			mycam.shift = 0;
		}

		if (key == GLFW_KEY_H && action == GLFW_PRESS)
		{
			if (pState == ALIVE)
				pState = INVULN;
			else if (pState == INVULN)
				pState = ALIVE;
		}

		// testing key
		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		{
			//knifeHandMesh.print_bones();
			//cout << "katana translate vector is: vec3(" << xVal << ", " << yVal << ", " << zVal << ")" << endl;
			//cout << "katana rotate vector is: vec3(" << xAngle << ", " << yAngle << ", " << zAngle << ")" << endl;
			//cout << "cameraPos: vec3(" << -mycam.pos.x << ", " << -mycam.pos.y << ", " << -mycam.pos.z << ")" << endl;
			if (pState == ALIVE) {
				cout << "PlayerState: ALIVE" << endl;
			}
			else if (pState == DEAD) {
				cout << "PlayerState: DEAD" << endl;
			}
			else {
				cout << "PlayerState: INVULN" << endl;
			}
			/*cout << "look: " << (mycam.look ? "true" : "false") << endl;
			cout << "Cursor Pos: " << mycam.xPos << ", " << mycam.yPos << endl;
			cout << "cursor start pos: " << mycam.xStartPos << ", " << mycam.yStartPos << endl;*/
		}
	}

	// callback for the mouse
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			if (mycam.shift) {
				toggleWindowLock(window);
			}
			if (pState != DEAD && !playerThrow && !playerWin) 
				playerAttack = true;
		}
	}

	void toggleWindowLock(GLFWwindow* window) {
		mycam.mousePressed = !mycam.mousePressed;
		if (mycam.mousePressed)
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		if (firstClick) {
			glfwGetCursorPos(window, &mycam.xStartPos, &mycam.yStartPos);
			firstClick = false;
		}

	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}	
	
	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom(const std::string& resourceDirectory)
	{
		// Initialize meshes
		if (!ninjaMesh.LoadMesh(resourceDirectory + "/Ninja.fbx")) {
			printf("Mesh load failed\n");
			return;
		}
		if (!katanaHands.LoadMesh(resourceDirectory + "/Arms.fbx")) {
			printf("Mesh load failed\n");
			return;
		}
		katanaHands.addDiffuseTexture(resourceDirectory + "/armTex.png", 0);
		katanaHands.SetAnimation(1);

		if (!katanaOneHand.LoadMesh(resourceDirectory + "/ArmAttack1.fbx")) {
			printf("Mesh load failed\n");
			return;
		}
		katanaOneHand.addDiffuseTexture(resourceDirectory + "/armTex.png", 0);
		katanaOneHand.SetAnimation(1);

		if (!leftHand.LoadMesh(resourceDirectory + "/LeftArmThrow.fbx")) {
			printf("Mesh load failed\n");
			return;
		}
		leftHand.addDiffuseTexture(resourceDirectory + "/armTex.png", 0);
		leftHand.SetAnimation(0);
	
		skyShape = make_shared<Shape>();
		skyShape->loadMesh(resourceDirectory + "/sphere.obj");
		skyShape->resize();
		skyShape->init();

		templeShape = make_shared<Shape>();
		templeShape->loadMesh(resourceDirectory + "/Japanese_Temple.obj");
		templeShape->resize();
		templeShape->init();

		cubeShape = make_shared<Shape>();
		cubeShape->loadMesh(resourceDirectory + "/cube.obj");
		cubeShape->resize();
		cubeShape->init();

		quadShape = make_shared<Shape>();
		quadShape->loadMesh(resourceDirectory + "/quad.obj");
		quadShape->resize();
		quadShape->init();

		rockShape = make_shared<Shape>();
		rockShape->loadMesh(resourceDirectory + "/rock_base_LP.obj");
		rockShape->resize();
		rockShape->init();

		katanaShape = make_shared<Shape>();
		katanaShape->loadMesh(resourceDirectory + "/katana/katana.obj");
		katanaShape->resize();
		katanaShape->init();

		shurikenShape = make_shared<Shape>();
		shurikenShape->loadMesh(resourceDirectory + "/ninjaStar/NinjaStar.obj");
		shurikenShape->resize();
		shurikenShape->init();

		tCubeShape = make_shared<Shape>();
		tCubeShape->loadMesh(resourceDirectory + "/texCube.obj");
		tCubeShape->resize();
		tCubeShape->init();

		treeShape = make_shared<Shape>();
		string mtldir = resourceDirectory + "/Trees1/";
		treeShape->loadMesh(resourceDirectory + "/Trees1/tree_mango_var01.obj", &mtldir, stbiload);
		treeShape->resize();
		treeShape->init();

		// Initialize textures
		auto strSky = resourceDirectory + "/sky.jpg";
		skyTex = SmartTexture::loadTexture(strSky, true);
		if (!skyTex)
			cerr << "error: texture " << strSky << " not found" << endl;

		auto strTemple = resourceDirectory + "/Japanese_Temple_Paint2_Japanese_Shrine_Mat_AlbedoTransparency.png";
		templeTex = SmartTexture::loadTexture(strTemple, false);
		if (!templeTex)
			cerr << "error: texture " << strTemple << " not found" << endl;

		auto strRock = resourceDirectory + "/rock_BaseColor.png";
		rockTex = SmartTexture::loadTexture(strRock, false);
		if (!rockTex)
			cerr << "error: texture " << strRock << " not found" << endl;

		auto strGround = resourceDirectory + "/Grassy_Walk.jpg";
		groundTex = SmartTexture::loadTexture(strGround, false);
		if (!groundTex)
			cerr << "error: texture " << strGround << " not found" << endl;

		auto strUI = resourceDirectory + "/GameOverText.png";
		UITex = SmartTexture::loadTexture(strUI, false);
		if (!UITex)
			cerr << "error: texture " << strUI << " not found" << endl;

		strUI = resourceDirectory + "/GameWinText.png";
		UIWinTex = SmartTexture::loadTexture(strUI, false);
		if (!UIWinTex)
			cerr << "error: texture " << strUI << " not found" << endl;

		strUI = resourceDirectory + "/GameCheatText.png";
		UICheatTex = SmartTexture::loadTexture(strUI, false);
		if (!UICheatTex)
			cerr << "error: texture " << strUI << " not found" << endl;

		auto strKatana = resourceDirectory + "/katana/Diffuse_A.png";
		katanaTex = SmartTexture::loadTexture(strKatana, false);
		if (!katanaTex)
			cerr << "error: texture " << strKatana << " not found" << endl;

		auto strShuriken = resourceDirectory + "/ninjaStar/ninjaStarTex.png";
		shurikenTex = SmartTexture::loadTexture(strShuriken, false);
		if (!shurikenTex)
			cerr << "error: texture " << strShuriken << " not found" << endl;

		auto strSmoke = resourceDirectory + "/smokeAnimTiles.png";
		smokeTex = SmartTexture::loadTexture(strSmoke, false);
		if (!smokeTex)
			cerr << "error: texture " << strSmoke << " not found" << endl;

		auto strCrate = resourceDirectory + "/crate2_diffuse.png";
		crateTex = SmartTexture::loadTexture(strCrate, false);
		if (!crateTex)
			cerr << "error: texture " << strCrate << " not found" << endl;

		// Initialize deformation values for treeShape
		for (int i = 0; i < treeShape->posBuf[1].size(); i += 3) {
			if (treeShape->posBuf[1][i + 1] > -1) {
				minv = vec3(treeShape->posBuf[1][i], treeShape->posBuf[1][i + 1], treeShape->posBuf[1][i + 2]);
				break;
			}
		}

		maxv = minv;

		for (int i = 0; i < treeShape->posBuf[1].size(); i += 3) {
			if (treeShape->posBuf[1][i + 1] > 0) {
				if (treeShape->posBuf[1][i] < minv.x)
					minv.x = treeShape->posBuf[1][i];
				if (treeShape->posBuf[1][i + 1] < minv.y)
					minv.y = treeShape->posBuf[1][i + 1];
				if (treeShape->posBuf[1][i + 2] < minv.z)
					minv.z = treeShape->posBuf[1][i + 2];

				if (treeShape->posBuf[1][i] > maxv.x)
					maxv.x = treeShape->posBuf[1][i];
				if (treeShape->posBuf[1][i + 1] > maxv.y)
					maxv.y = treeShape->posBuf[1][i + 1];
				if (treeShape->posBuf[1][i + 2] > maxv.z)
					maxv.z = treeShape->posBuf[1][i + 2];
			}
		}

		float w = maxv.x - minv.x;
		float h = maxv.y - minv.y;
		float d = maxv.z - minv.z;

		for (int i = 0; i < treeShape->posBuf[1].size(); i += 3) {
			if (treeShape->posBuf[1][i + 1] > 0) {
				pVals.push_back((treeShape->posBuf[1][i] - minv.x) / w);
				pVals.push_back((treeShape->posBuf[1][i + 1] - minv.y) / h);
				pVals.push_back((treeShape->posBuf[1][i + 2] - minv.z) / d);
			}
			else {
				pVals.push_back(0);
				pVals.push_back(0);
				pVals.push_back(0);
			}
		}

		Apt = vec3(minv.x, maxv.y, minv.z);
		Bpt = vec3(minv.x, maxv.y, maxv.z);
		Cpt = maxv;
		Dpt = vec3(maxv.x, maxv.y, minv.z);
		Ept = minv;
		Fpt = vec3(minv.x, minv.y, maxv.z);
		Gpt = vec3(maxv.x, minv.y, maxv.z);
		Hpt = vec3(maxv.x, minv.y, minv.z);

		boundingBoxActual.push_back(Apt);//0
		boundingBoxActual.push_back(Bpt);//1
		boundingBoxActual.push_back(Cpt);//2
		boundingBoxActual.push_back(Dpt);//3
		boundingBoxActual.push_back(Ept);//4
		boundingBoxActual.push_back(Fpt);//5
		boundingBoxActual.push_back(Gpt);//6
		boundingBoxActual.push_back(Hpt);//7
	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		// Initialize the GLSL programs
		psky = std::make_shared<Program>();
		psky->setVerbose(true);
		psky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!psky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		
		psky->addUniform("P");
		psky->addUniform("V");
		psky->addUniform("M");
		psky->addUniform("tex");
		psky->addUniform("camPos");
		psky->addAttribute("vertPos");
		psky->addAttribute("vertNor");
		psky->addAttribute("vertTex");

		skinProg = std::make_shared<Program>();
		skinProg->setVerbose(true);
		skinProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
		if (!skinProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		
		skinProg->addUniform("P");
		skinProg->addUniform("V");
		skinProg->addUniform("M");
		skinProg->addUniform("tex");
		skinProg->addUniform("camPos");
		skinProg->addAttribute("vertPos");
		skinProg->addAttribute("vertNor");
		skinProg->addAttribute("vertTex");
		skinProg->addAttribute("BoneIDs");
		skinProg->addAttribute("Weights");

		mapObjectsProg = std::make_shared<Program>();
		mapObjectsProg->setVerbose(true);
		mapObjectsProg->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!mapObjectsProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		mapObjectsProg->addUniform("P");
		mapObjectsProg->addUniform("V");
		mapObjectsProg->addUniform("M");
		mapObjectsProg->addUniform("tex");
		mapObjectsProg->addUniform("campos");
		mapObjectsProg->addAttribute("vertPos");
		mapObjectsProg->addAttribute("vertNor");
		mapObjectsProg->addAttribute("vertTex");

		cubeProg = std::make_shared<Program>();
		cubeProg->setVerbose(true);
		cubeProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		if (!cubeProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		cubeProg->addUniform("P");
		cubeProg->addUniform("V");
		cubeProg->addUniform("M");
		cubeProg->addUniform("tex");
		cubeProg->addUniform("campos");
		cubeProg->addAttribute("vertPos");
		cubeProg->addAttribute("vertNor");
		cubeProg->addAttribute("vertTex");

		ptree = std::make_shared<Program>();
		ptree->setVerbose(true);
		ptree->setShaderNames(resourceDirectory + "/tree_vertex.glsl", resourceDirectory + "/tree_frag.glsl");
		if (!ptree->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		ptree->addUniform("P");
		ptree->addUniform("V");
		ptree->addUniform("M");
		ptree->addUniform("campos");
		ptree->addAttribute("vertPos");
		ptree->addAttribute("vertNor");
		ptree->addAttribute("vertTex");

		animTexProg = std::make_shared<Program>();
		animTexProg->setVerbose(true);
		animTexProg->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/textureAnim_fragment.glsl");
		if (!animTexProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		animTexProg->addUniform("P");
		animTexProg->addUniform("V");
		animTexProg->addUniform("M");
		animTexProg->addUniform("tex");
		animTexProg->addUniform("campos");
		animTexProg->addUniform("texoff");
		animTexProg->addUniform("texoff_last");
		animTexProg->addUniform("t");
		animTexProg->addAttribute("vertPos");
		animTexProg->addAttribute("vertNor");
		animTexProg->addAttribute("vertTex");

		//create enemies
		vec3 playerPos = vec3(-mycam.pos.x, 0, -mycam.pos.z);
		float spawnXBounds = mapScale.x - 10;
		float spawnZBounds = mapScale.z - 10;
		cout << "Generating Enemy Spawn Locations..." << endl;
		for (int i = 0; i < NUM_ENEMIES; i++) {
			vec3 newPos = vec3(rand() % (int)((spawnXBounds * 2) + 1) - spawnXBounds, 0, rand() % (int)((spawnZBounds * 2) + 1) - spawnZBounds);
			if (glm::distance(newPos, playerPos) <= 15) {
				i--;
			}
			else {
				enemies.push_back(enemy(newPos));
			}
		}
		cout << "...done" << endl;

		//create projectiles
		for (int i = 0; i < NUM_PROJECTILES; i++) {
			shurikens.push_back(projectile(vec3(0, -10, 0)));
		}

		//create collectibles
		makeCollectibles();
	}

	void makeCollectibles() {
		collectibles.clear();
		collectibles.push_back(vec3(0, 0.75, 0));
		collectibles.push_back(vec3(-32.6934, 0.75, 34.1226));
		collectibles.push_back(vec3(-33.2572, 0.75, -35.1597));
		collectibles.push_back(vec3(5.25035, 0.75, -43.6434));
		collectibles.push_back(vec3(38.0797, 0.75, 8.75321));
		collectibles.push_back(vec3(55.1883, 0.75, 50.6933));
		collectibleCnt = 0;
	}

	void checkCollectibles(vec3 playerPos) {
		for (int i = 0; i < collectibles.size(); i++) {
			if (distance(playerPos, collectibles[i]) <= 2.0f) {
				collectibleCnt++;
				cout << "Collectible Get! Current Cnt: " << collectibleCnt << " / " << NUM_COLLECTIBLES << endl;
				collectibles.erase(collectibles.begin() + i);
				break;
			}
		}
	}

	void checkWin() {
		if (collectibleCnt == NUM_COLLECTIBLES) {
			playerWin = true;
			mycam.invuln = false;
			mycam.look = false;
		}
	}

	void swingSword(vec3 playerPos) {
		vec3 pPos = playerPos;
		pPos.y = 0;
		for (int i = 0; i < enemies.size(); i++) {
			if (distance(pPos, enemies[i].pos) <= 3.5f && pState != DEAD) {
				cout << "enemy hit with sword!" << endl;
				if (mycam.lctrl && enemies[i].idle) {
					enemies[i].damage(6);
				}
				else {
					enemies[i].damage(3);
				}
				break;
			}
		}
	}

	void throwStar(vec3 pos, vec3 dir) {
		for (int i = 0; i < shurikens.size(); i++) {
			if (!shurikens[i].active) {
				shurikens[i].spawn(pos, dir);
				break;
			}
		}
	}

	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		double frametime = get_last_elapsed_time();
		double attackFtime = frametime;
		static double totaltime = 0;
		static double attackTimer = 0;
		static double attack2Timer = 0;
		static double treeRotTimer = 0;
		totaltime += frametime;

		checkCollectibles(-mycam.pos);
		checkWin();

		//handle sword attack input
		if (playerAttack) {
			playerThrow = false;
			attack2Timer = 0;
			leftHand.SetAnimTotalTime(0);
			attackTimer += frametime;
		}
		if (attackTimer >= 0.45 && !swung) {
			swung = true;
			swingSword(-mycam.pos);
		}
		if (attackTimer >= 1.201) {
			attackTimer = 0;
			playerAttack = false;
			swung = false;
			katanaHands.SetAnimTotalTime(0);
		}

		//handle shuriken attack input
		if (playerThrow) {
			attack2Timer += frametime;
		}
		if (attack2Timer >= 0.2 && !thrown) {
			thrown = true;
			vec3 pPos, throwDir;
			mycam.getDirPos(pPos, throwDir);
			throwStar(pPos, throwDir);
		}
		if (attack2Timer >= 0.65) {
			attack2Timer = 0;
			playerThrow = false;
			thrown = false;
			leftHand.SetAnimTotalTime(0);
		}

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				
		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = mycam.process(frametime, windowManager->getHandle(), mapScale, &pState, playerWin);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);		
		if (width < height)
			{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect,  1.0f / aspect, -2.0f, 100.0f);
			}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		auto sangle = -3.1415926f / 2.0f;
		glm::mat4 RotateXSky = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 camp = -mycam.pos;
		glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
		glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));

		M = TransSky  * RotateXSky * SSky;
		glm::mat4 Trans, rotX, rotY, rotZ, Scale;

		// Draw the sky using GLSL.
		psky->bind();
		GLuint texLoc = glGetUniformLocation(psky->pid, "tex");
		skyTex->bind(texLoc);	
		glUniformMatrix4fv(psky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(psky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(psky->getUniform("camPos"), 1, &mycam.pos[0]);

		glDisable(GL_DEPTH_TEST);
		skyShape->draw(psky, false);
		glEnable(GL_DEPTH_TEST);	
		skyTex->unbind();
		psky->unbind();

		//draw ninja enemies
		skinProg->bind();

		texLoc = glGetUniformLocation(skinProg->pid, "tex");
		sangle = -3.1415926f / 2.0f;
		glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.03f, 0.03f, 0.03f));
		for (int i = 0; i < enemies.size(); i++) {
			glm::mat4 TransRot = enemies[i].update(frametime, -mycam.pos, mapScale, &pState, &smokePositions, playerWin);
			M = TransRot * Scale;

			glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			ninjaMesh.SetAnimation(enemies[i].animation);
			ninjaMesh.setBoneTransformations(skinProg->pid, frametime * 3.7);
			if ((pState == ALIVE || pState == INVULN) && !playerWin)
				ninjaMesh.Render(texLoc);
		}

		skinProg->unbind();

		//draw ground
		cubeProg->bind();
		texLoc = glGetUniformLocation(cubeProg->pid, "tex");
		groundTex->bind(texLoc);
		glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(cubeProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(cubeProg->getUniform("campos"), 1, &mycam.pos[0]);
		Trans = glm::translate(glm::mat4(1.0f), vec3(0, -.5, 0));
		Scale = glm::scale(glm::mat4(1.0f), mapScale);
		M = Trans * Scale;
		glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		cubeShape->draw(cubeProg, false);
		groundTex->unbind();
		cubeProg->unbind();

		//draw UI elements
		mapObjectsProg->bind();
		texLoc = glGetUniformLocation(mapObjectsProg->pid, "tex");
		glUniformMatrix4fv(mapObjectsProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(mapObjectsProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		mat4 Vi = glm::inverse(V);

		Trans = glm::translate(glm::mat4(1.0f), vec3(0, 0.07, -0.5));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
		M = Vi * Trans * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		if (pState == DEAD && !playerWin) {
			UITex->bind(texLoc);
			quadShape->draw(mapObjectsProg, false);
		}
		else if (playerWin) {
			UIWinTex->bind(texLoc);
			quadShape->draw(mapObjectsProg, false);
		}
		else if (pState == INVULN) {
			UICheatTex->bind(texLoc);
			quadShape->draw(mapObjectsProg, false);
		}
		UITex->unbind();

		//draw temples
		templeTex->bind(texLoc);
		Trans = glm::translate(glm::mat4(1.0f), vec3(-3, 10, -33));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(10, 10, 10));
		M = Trans * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		templeShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-44, 10, -13));
		sangle = 3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		templeShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(35, 10, 48));
		sangle = 3.1415926f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		templeShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(50, 10, -38));
		sangle = -3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		templeShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-10, 10, 27));
		sangle = 3.1415926f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		templeShape->draw(mapObjectsProg, false);
		templeTex->unbind();
		
		//draw rocks
		rockTex->bind(texLoc);
		Trans = glm::translate(glm::mat4(1.0f), vec3(9, 2, -35));
		rotY = glm::rotate(glm::mat4(1.0f), 0.f, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-28, 3, 33));
		sangle = -3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(3, 7, 2));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-38, 4.8, 39));
		sangle = 3.1415926f / 6.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 11, 3));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-42, 3, 2));
		sangle = 3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(3, 8, 2));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-38, 3, 11));
		sangle = (3.1415926f / 6.0f) * 4.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(3, 8, 2));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-35, 4, -35));
		sangle = 3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(15, 20, 15));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(39, 3, 15));
		sangle = 3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(6, 14, 4));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(41, 4, 5));
		sangle = -3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(2, 9, 3));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(33, 1, 8));
		sangle = -3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(3, 3, 4));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-12, .75, 8));
		sangle = -3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(3, 2, 2));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		rockShape->draw(mapObjectsProg, false);

		rockTex->unbind();

		//collectible crates
		crateTex->bind(texLoc);

		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.5, 0.5, 0.5));
		sangle = 0.5f * totaltime;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		for (int i = 0; i < collectibles.size(); i++) {
			Trans = glm::translate(glm::mat4(1.0f), collectibles[i]);
			M = Trans * rotY * Scale;
			glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			tCubeShape->draw(mapObjectsProg, false);
		}
		crateTex->unbind();


		//draw ninja stars
		shurikenTex->bind(texLoc);
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1, 0.1, 0.1));
		for (int i = 0; i < shurikens.size(); i++) {
			if (shurikens[i].active) {
				M = shurikens[i].update(frametime, enemies) * Scale;
				glUniformMatrix4fv(mapObjectsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
				shurikenShape->draw(mapObjectsProg, false);
			}
		}
		shurikenTex->unbind();

		mapObjectsProg->unbind();

		//draw trees
		float s = 0.5f;
		treeRotTimer = frametime * s;
		float angle = treeRotTimer * sin(totaltime) * 0.5f;
		glm::mat4 rotPts = glm::rotate(glm::mat4(1.0f), angle, vec3(0, 1, 0));

		boundingBoxActual[0] = rotPts * vec4(boundingBoxActual[0], 1);
		boundingBoxActual[1] = rotPts * vec4(boundingBoxActual[1], 1);
		boundingBoxActual[2] = rotPts * vec4(boundingBoxActual[2], 1);
		boundingBoxActual[3] = rotPts * vec4(boundingBoxActual[3], 1);

		//calculate new vertices
		newVerts.clear();
		vec3 ABpt, DCpt, EFpt, HGpt, EFABpt, HGDCpt, newPt;

		for (int i = 0; i < treeShape->posBuf[1].size(); i += 3) {
			if (treeShape->posBuf[1][i + 1] > 0) {
				ABpt = (boundingBoxActual[0] * (1 - pVals[i + 2])) + (boundingBoxActual[1] * pVals[i + 2]);
				DCpt = (boundingBoxActual[3] * (1 - pVals[i + 2])) + (boundingBoxActual[2] * pVals[i + 2]);
				EFpt = (boundingBoxActual[4] * (1 - pVals[i + 2])) + (boundingBoxActual[5] * pVals[i + 2]);
				HGpt = (boundingBoxActual[7] * (1 - pVals[i + 2])) + (boundingBoxActual[6] * pVals[i + 2]);

				EFABpt = (EFpt * (1 - pVals[i + 1])) + (ABpt * pVals[i + 1]);
				HGDCpt = (HGpt * (1 - pVals[i + 1])) + (DCpt * pVals[i + 1]);

				newPt = (EFABpt * (1 - pVals[i])) + (HGDCpt * pVals[i]);
				newVerts.push_back(newPt);
			}
			else {
				newVerts.push_back(vec3(treeShape->posBuf[1][i], treeShape->posBuf[1][i + 1], treeShape->posBuf[1][i + 2]));
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, treeShape->posBufID[1]);
		//update the vertex array with the updated points
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)* newVerts.size(), newVerts.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		ptree->bind();
		glUniformMatrix4fv(ptree->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(ptree->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(ptree->getUniform("campos"), 1, &mycam.pos[0]);
		Trans = glm::translate(glm::mat4(1.0f), vec3(-8, 5, -24));
		sangle = -3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), 0.0f, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(6, 5, 36));
		sangle = -3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), 0.0f, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(10, 6, 30));
		sangle = 3.1415926f;
		rotY = glm::rotate(glm::mat4(1.0f), 0.0f, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 6, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(26, 5, -36));
		sangle = -3.1415926f / 3.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(24, 6, -28));
		sangle = 3.1415926f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 6, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(32, 5, -11));
		sangle = 3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 5, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(38, 4, -8));
		sangle = -3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(5, 4, 5));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-42, 8, 19));
		sangle = 3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(6, 8, 6));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-34, 6, 29));
		sangle = -3.1415926f / 2.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(8, 6, 10));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(12, 8, 9));
		sangle = 3.1415926f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(6, 8, 6));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(-15, 4, -2));
		sangle = 3.1415926f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(4, 4, 4));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);

		Trans = glm::translate(glm::mat4(1.0f), vec3(6, 4, -11));
		sangle = -3.1415926f / 4.0f;
		rotY = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(3, 4, 3));
		M = Trans * rotY * Scale;
		glUniformMatrix4fv(ptree->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		treeShape->draw(ptree, false);
		ptree->unbind();

		//billboard animation ---------------

		//draw smoke puffs
		animTexProg->bind();

		texLoc = glGetUniformLocation(animTexProg->pid, "tex");
		smokeTex->bind(texLoc);
		glUniform3fv(animTexProg->getUniform("campos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(animTexProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(animTexProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);

		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(2, 2, 2));
		Vi = glm::transpose(V);
		Vi[0][3] = 0;
		Vi[1][3] = 0;
		Vi[2][3] = 0;

		static vec2 to = vec2(0, 0);
		static vec2 tol = vec2(0, 0);

		for (int i = 0; i < smokePositions.size(); i++) {
			Trans = glm::translate(glm::mat4(1.0f), smokePositions[i].first);
			M = Trans * Vi * Scale;
			glUniformMatrix4fv(animTexProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);

			smokePositions[i].second += frametime;
			double ttime = smokePositions[i].second * 30.0;
			int ittime = (int)ttime;
			int x = ittime % 12;
			int y = ittime / 12;
			to.x = x;
			to.y = y;

			if (to.x != 0) {
				tol.x = to.x - 1;
				tol.y = to.y;
			}
			else {
				tol.x = 11;
				tol.y = to.y - 1;
			}

			float t = ttime - ittime;

			glUniform1f(animTexProg->getUniform("t"), t);
			glUniform2fv(animTexProg->getUniform("texoff"), 1, &to.x);
			glUniform2fv(animTexProg->getUniform("texoff_last"), 1, &tol.x);
			quadShape->draw(animTexProg, false);

		}

		for (int i = 0; i < smokePositions.size(); i++) {
			if (smokePositions[i].second >= 2.7) {
				smokePositions.erase(smokePositions.begin() + i);
				i--;
			}
		}

		smokeTex->unbind();
		animTexProg->unbind();

		//draw hand
		skinProg->bind();

		texLoc = glGetUniformLocation(skinProg->pid, "tex");
		glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		sangle = 3.1415926f;
		rotX = glm::rotate(glm::mat4(1.0f), mycam.rot.x, vec3(1, 0, 0));
		rotY = glm::rotate(glm::mat4(1.0f), -mycam.rot.y + sangle, vec3(0, 1, 0));
		Trans = glm::translate(glm::mat4(1.0f), -mycam.pos);
		glm::mat4 t2 = glm::translate(glm::mat4(1.0f), vec3(-0.35, -0.2, 1.5));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.005, 0.005, 0.005));
		M = Trans * rotY * rotX * t2 * Scale;
		glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);

		glClear(GL_DEPTH_BUFFER_BIT);

		if (!playerThrow) {
			if (!playerAttack) {
				attackFtime = 0;
			}
			katanaHands.setBoneTransformations(skinProg->pid, attackFtime * 55.);
			katanaHands.Render(texLoc);
		}
		else {
			katanaOneHand.setBoneTransformations(skinProg->pid, 0);
			katanaOneHand.Render(texLoc);

			mat4 leftHandTrans = glm::translate(mat4(1), vec3(0.1, 1.15, -0.85));
			leftHand.AddBoneTransformation("Left_Ombro.001", leftHandTrans);

			leftHand.setBoneTransformations(skinProg->pid, attackFtime * 75.);
			leftHand.Render(texLoc);
		}

		skinProg->unbind();

		//draw sword in hand
		cubeProg->bind();
		texLoc = glGetUniformLocation(cubeProg->pid, "tex");
		katanaTex->bind(texLoc);
		glUniformMatrix4fv(cubeProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(cubeProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(cubeProg->getUniform("campos"), 1, &mycam.pos[0]);
		sangle = 3.1415926f * 9.0f / 7.0f;
		Scale = glm::scale(glm::mat4(1.0f), vec3(1.5, 1.5, 1.5));

		mat4 rightHand = katanaHands.GetMountMatrix("Right_Ombro.007");

		Trans = glm::translate(glm::mat4(1.0f), vec3(-0.7, 0.2, 0.6));
		rotX = glm::rotate(glm::mat4(1.0f), -0.5f, vec3(1, 0, 0));
		rotY = glm::rotate(glm::mat4(1.0f), -2.3f, vec3(0, 1, 0));
		rotZ = glm::rotate(glm::mat4(1.0f), -0.3f, vec3(0, 0, 1));

		M = M * rightHand * Trans * rotZ * rotY * rotX * Scale;
		glUniformMatrix4fv(cubeProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		katanaShape->draw(cubeProg, false);

		katanaTex->unbind();

		cubeProg->unbind();

	}
};

//******************************************************************************************
int main(int argc, char **argv)
{
	srand(time(NULL));

	std::string resourceDir = "../resources"; // Where the resources are loaded from
	std::string missingTexture = "missing.jpg";
	
	SkinnedMesh::setResourceDir(resourceDir);
	SkinnedMesh::setDefaultTexture(missingTexture);
	
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom(resourceDir);

	//lock cursor into game window at game start
	application->toggleWindowLock(windowManager->getHandle());
	mycam.reset(windowManager->getHandle());

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

