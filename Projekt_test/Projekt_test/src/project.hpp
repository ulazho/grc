#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <list>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"
#include <vector>
#include <numeric>

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "objload.h"
#include "SOIL/stb_image_aug.h"
#include <Windows.h>
#include <iomanip> 
#include <sstream>
#include "SOIL/SOIL.h"
using namespace std;

bool rKeyWasPressed = false;



namespace texture {
	GLuint earth;
	GLuint mars;
	GLuint jupiter;
	GLuint neptune;
	GLuint saturn;
	GLuint mercury;
	GLuint venus;
	GLuint uranus;
	GLuint moon;
	GLuint ship;
	GLuint sun;
	GLuint metal;
	GLuint earth_nmap;
	GLuint mars_nmap;
	GLuint jupiter_nmap;
	GLuint neptune_nmap;
	GLuint saturn_nmap;
	GLuint mercury_nmap;
	GLuint venus_nmap;
	GLuint uranus_nmap;
	GLuint moon_nmap;
}


GLuint program;
GLuint programSun;
GLuint programTex;
GLuint programEarth;
GLuint programProcTex;
GLuint programSkybox;
GLuint cubemapTexture;
GLuint programUnmap;
Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;
Core::RenderContext cubeContext;
GLuint empty_nmap_texture = -1;
glm::vec3 cameraPos = glm::vec3(-4.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);

glm::vec3 spaceshipPos = glm::vec3(-4.f, 0, 0);
glm::vec3 spaceshipDir = glm::vec3(1.f, 0.f, 0.f);
GLuint VAO, VBO;

float aspectRatio = 1.777777777777778f;
glm::vec3 lightColor = glm::vec3(0.9, 0.7, 0.8);


const double totalTime = 90.0;
double remainingTime = totalTime;

struct Planet {
	glm::vec3 startPosition;
	glm::vec3 modelScale;
	bool isActivated;
	GLuint textureID;
	std::string name;
	glm::vec3 currentPlanetPos;
	bool interacted;
	GLuint nmap_texture;
	bool isNmap;

};

std::vector<Planet> planets;
std::vector<Planet*> targetPlanets;


glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);

	return cameraMatrix;
}

glm::mat4 createPerspectiveMatrix()
{

	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 20.;
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		1,0.,0.,0.,
		0.,aspectRatio,0.,0.,
		0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
		0.,0.,-1.,0.,
		});


	perspectiveMatrix = glm::transpose(perspectiveMatrix);

	return perspectiveMatrix;
}

void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID, bool isHighlighted) {
	glUseProgram(programTex);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programTex, "transformation"), 1, GL_FALSE, glm::value_ptr(transformation));
	glUniformMatrix4fv(glGetUniformLocation(programTex, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glUniform3f(glGetUniformLocation(programTex, "lightPos"), 0, 0, 0);
	Core::SetActiveTexture(textureID, "colorTexture", programTex, 0);
	glUniform1i(glGetUniformLocation(programTex, "isHighlighted"), isHighlighted ? GL_TRUE : GL_FALSE);
	glm::vec3 highlightColor = isHighlighted ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
	glUniform3fv(glGetUniformLocation(programTex, "highlightColor"), 1, glm::value_ptr(highlightColor));
	glUniform1f(glGetUniformLocation(programTex, "metallic"), 0.5);
	glUniform1f(glGetUniformLocation(programTex, "roughness"), 0.5);
	glUniform3fv(glGetUniformLocation(programTex, "albedo"), 1, glm::value_ptr(glm::vec3(1.0, 0.5, 0.31)));

	Core::DrawContext(context);
}


void drawObjectTextureWithNMAP(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID, bool isHighlighted, GLuint nmap_texture) {
	glUseProgram(programUnmap);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programUnmap, "transformation"), 1, GL_FALSE, glm::value_ptr(transformation));
	glUniformMatrix4fv(glGetUniformLocation(programUnmap, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

	glUniform3f(glGetUniformLocation(programUnmap, "lightPos"), 0, 0, 0);
	Core::SetActiveTexture(textureID, "colorTexture", programUnmap, 0);
	glUniform1i(glGetUniformLocation(programUnmap, "isHighlighted"), isHighlighted ? GL_TRUE : GL_FALSE);
	glm::vec3 highlightColor = isHighlighted ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
	glUniform3fv(glGetUniformLocation(programUnmap, "highlightColor"), 1, glm::value_ptr(highlightColor));

	glUniform1f(glGetUniformLocation(programUnmap, "metallic"), 0.3);
	glUniform1f(glGetUniformLocation(programUnmap, "roughness"), 0.5);
	glUniform3fv(glGetUniformLocation(programUnmap, "albedo"), 1, glm::value_ptr(glm::vec3(1.0, 0.5, 0.31)));
	glUniform1i(glGetUniformLocation(programUnmap, "normalMap"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, nmap_texture);


	Core::DrawContext(context);
}


void drawSunTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID, bool isHighlighted) {
	glUseProgram(programSun);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programSun, "transformation"), 1, GL_FALSE, glm::value_ptr(transformation));
	glUniformMatrix4fv(glGetUniformLocation(programSun, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	glUniform3f(glGetUniformLocation(programSun, "lightPos"), 0, 0, 0);
	Core::SetActiveTexture(textureID, "colorTexture", programSun, 0);
	glUniform1i(glGetUniformLocation(programSun, "isHighlighted"), isHighlighted ? GL_TRUE : GL_FALSE);
	glm::vec3 highlightColor = isHighlighted ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
	glUniform3fv(glGetUniformLocation(programSun, "highlightColor"), 1, glm::value_ptr(highlightColor));

	Core::DrawContext(context);
}

void drawObjectSkybox(Core::RenderContext& context) {
	glDisable(GL_DEPTH_TEST);
	glUseProgram(programSkybox);
	glm::mat4 view = createCameraMatrix();
	glm::mat4 projection = createPerspectiveMatrix();
	glm::mat4 viewProjectionMatrix = projection * glm::mat4(glm::mat3(view));
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "transformation"), 1, GL_FALSE, glm::value_ptr(viewProjectionMatrix));
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	Core::DrawContext(cubeContext);
	glEnable(GL_DEPTH_TEST);


}

void renderScene(GLFWwindow* window)
{
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 transformation;
	float time = glfwGetTime();

	drawObjectSkybox(cubeContext);

	drawSunTexture(sphereContext, glm::mat4(), texture::sun, FALSE);

	float rot = 1.f;


	for (auto& planet : planets) {



		glm::mat4 modelMatrix = glm::eulerAngleY(time / rot) * glm::translate(planet.startPosition) * glm::eulerAngleY(time) * glm::scale(planet.modelScale);
		drawObjectTextureWithNMAP(sphereContext, modelMatrix, planet.textureID, planet.isActivated, planet.nmap_texture);
		planet.currentPlanetPos = glm::vec3(modelMatrix * glm::vec4(0, 0, 0, 1.0f));
		rot += 0.5f;
	}



	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
	glm::mat4 specshipCameraRotrationMatrix = glm::mat4({
		spaceshipSide.x,spaceshipSide.y,spaceshipSide.z,0,
		spaceshipUp.x,spaceshipUp.y,spaceshipUp.z ,0,
		-spaceshipDir.x,-spaceshipDir.y,-spaceshipDir.z,0,
		0.,0.,0.,15.,
		});


	drawObjectTexture(shipContext,
		glm::translate(spaceshipPos) * specshipCameraRotrationMatrix * glm::eulerAngleY(glm::pi<float>()),
		texture::ship,
		FALSE
	);

	glUseProgram(0);
	glfwSwapBuffers(window);

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
}
void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	context.initFromAssimpMesh(scene->mMeshes[0]);
}


void initializePlanets() {

	planets.push_back(Planet{ glm::vec3(1.5f, 0, 0), glm::vec3(0.1f), false, texture::mercury, "Mercury", glm::vec3(1.5f, 0, 0), false, texture::mercury_nmap, FALSE });
	planets.push_back(Planet{ glm::vec3(2.3f, 0, 0), glm::vec3(0.15f), false, texture::venus, "Venus", glm::vec3(2.3f, 0, 0), false, texture::venus_nmap, FALSE });
	planets.push_back(Planet{ glm::vec3(4.f, 0, 0), glm::vec3(0.25f), false, texture::earth, "Earth", glm::vec3(4.f, 0, 0), false, texture::earth_nmap, TRUE });
	planets.push_back(Planet{ glm::vec3(6.6f, 0, 0), glm::vec3(0.27f), false, texture::mars, "Mars", glm::vec3(6.6f, 0, 0), false, texture::mars_nmap, FALSE });
	planets.push_back(Planet{ glm::vec3(7.4f, 0, 0), glm::vec3(0.43f), false, texture::jupiter, "Jupiter", glm::vec3(7.4f, 0, 0), false, texture::jupiter_nmap, FALSE });
	planets.push_back(Planet{ glm::vec3(11.f, 0, 0), glm::vec3(0.47f), false, texture::saturn, "Saturn", glm::vec3(11.f, 0, 0), false, texture::saturn_nmap, FALSE });
	planets.push_back(Planet{ glm::vec3(12.f, 0, 0), glm::vec3(0.32f), false, texture::uranus, "Uranus", glm::vec3(12.f, 0, 0), false, texture::uranus_nmap, FALSE });
	planets.push_back(Planet{ glm::vec3(14.f, 0, 0), glm::vec3(0.3f), false, texture::neptune, "Neptune", glm::vec3(14.f, 0, 0), false, texture::neptune_nmap, FALSE });
}

void selectRandomPlanet() {
	srand(static_cast<unsigned int>(time(nullptr)));
	std::vector<int> selectedIndexes;

	int randIndex = (int)glfwGetTime() % planets.size();

	if (std::find(selectedIndexes.begin(), selectedIndexes.end(), randIndex) == selectedIndexes.end()) {
		selectedIndexes.push_back(randIndex);
		planets[randIndex].isActivated = true;
		targetPlanets.push_back(&planets[randIndex]);
	}

}



GLuint loadCubemap(std::vector<std::string> faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height;
	unsigned char* image;

	for (GLuint i = 0; i < faces.size(); i++)
	{
		image = SOIL_load_image(faces[i].c_str(), &width, &height, 0, SOIL_LOAD_RGB);
		if (image)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			SOIL_free_image_data(image);
		}
		else
		{
			std::cout << "failed: " << faces[i] << std::endl;
			SOIL_free_image_data(image);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void init(GLFWwindow* window) {


	std::vector<std::string> faces
	{
		"./textures/skybox/bkg3_back6.png",
		"./textures/skybox/bkg3_front5.png",
		"./textures/skybox/bkg3_bottom4.png",
		"./textures/skybox/bkg3_top3.png",
		"./textures/skybox/bkg3_right1.png",
		"./textures/skybox/bkg3_left2.png"
	};
	cubemapTexture = loadCubemap(faces);


	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glEnable(GL_DEPTH_TEST);
	program = shaderLoader.CreateProgram("shaders/shader.vert", "shaders/shader.frag");
	programTex = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programEarth = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programProcTex = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_sun.vert", "shaders/shader_sun.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");

	programUnmap = shaderLoader.CreateProgram("shaders/shader_unmap.vert", "shaders/shader_unmap.frag");

	texture::earth = Core::LoadTexture("textures/earth.png");
	texture::moon = Core::LoadTexture("textures/mars.jpg");
	texture::jupiter = Core::LoadTexture("textures/jupiter.jpg");
	texture::mars = Core::LoadTexture("textures/mars.jpg");
	texture::neptune = Core::LoadTexture("textures/neptune.jpg");
	texture::uranus = Core::LoadTexture("textures/uranus.jpg");
	texture::mercury = Core::LoadTexture("textures/mercury.jpg");
	texture::saturn = Core::LoadTexture("textures/saturn.jpg");
	texture::venus = Core::LoadTexture("textures/venus.jpg");
	texture::sun = Core::LoadTexture("textures/sun.jpg");
	texture::metal = Core::LoadTexture("textures/metal.jpg");
	texture::ship = Core::LoadTexture("textures/spaceship.png");

	texture::earth_nmap = Core::LoadTexture("textures/earth_normalmap.png");
	texture::jupiter_nmap = Core::LoadTexture("textures/jupiter_normalmap.png");
	texture::mars_nmap = Core::LoadTexture("textures/mars_normalmap.png");
	texture::uranus_nmap = Core::LoadTexture("textures/uranus_normalmap.png");
	texture::mercury_nmap = Core::LoadTexture("textures/mercury_normalmap.png");
	texture::saturn_nmap = Core::LoadTexture("textures/saturn_normalmap.png");
	texture::venus_nmap = Core::LoadTexture("textures/venus_normalmap.png");
	texture::neptune_nmap = Core::LoadTexture("textures/neptune_normalmap.png");
	texture::saturn_nmap = Core::LoadTexture("textures/saturn_normalmap.png");




	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);
	loadModelToContext("./models/cube.obj", cubeContext);

	initializePlanets();
	selectRandomPlanet();
}

bool checkAllPlanetsDeactivated() {
	for (const auto& planet : planets) {
		if (planet.isActivated) {
			return false;
		}
	}
	return true;
}



void shutdown(GLFWwindow* window)
{
	shaderLoader.DeleteProgram(program);
}



void processInput(GLFWwindow* window)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.003f;
	float moveSpeed = 0.009f;
	static bool rKeyPressedLastFrame = false;


	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		spaceshipPos += spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		spaceshipPos -= spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		spaceshipPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		spaceshipPos -= spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));



	cameraPos = spaceshipPos - 1.5 * spaceshipDir + glm::vec3(0, 1, 0) * 0.5f;
	cameraDir = spaceshipDir;



}



void ShowLoseDialog(GLFWwindow* window) {
	int msgboxID = MessageBox(
		NULL,
		TEXT("You lose! Game Over."),
		TEXT("Game Over"),
		MB_ICONWARNING | MB_OK
	);

	if (msgboxID == IDOK) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void ShowWinDialog(GLFWwindow* window) {
	int msgboxID = MessageBox(
		NULL,
		TEXT("You win! Game Over."),
		TEXT("Game Over"),
		MB_ICONWARNING | MB_OK
	);

	if (msgboxID == IDOK) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void renderLoop(GLFWwindow* window) {

	while (!glfwWindowShouldClose(window)) {



		for (auto& planet : planets) {
			float planetRadius = std::max(std::max(planet.modelScale.x, planet.modelScale.y), planet.modelScale.z);
			float collisionDistance = 0.1f + planetRadius;
			float safeDistance = collisionDistance + 0.2f;

			if (glm::distance(spaceshipPos, planet.currentPlanetPos) < collisionDistance && !planet.isActivated) {
				ShowLoseDialog(window);
				break;
			}
			else if (glm::distance(spaceshipPos, planet.currentPlanetPos) < collisionDistance && planet.isActivated) {
				ShowWinDialog(window);
				break;
			}


		}

		processInput(window);

		renderScene(window);

		glfwPollEvents();
	}
}