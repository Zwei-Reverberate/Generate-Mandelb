#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"


const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 1000;
float screenRatio = SCR_WIDTH / SCR_HEIGHT;
auto screenSize = glm::vec2(SCR_WIDTH, SCR_HEIGHT);

#pragma region render preparing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

double lastX = SCR_WIDTH / 2.0f;
double lastY = SCR_HEIGHT / 2.0f;

bool firstMouse = true;

float NEAR_PLANE = 0.1f;
float FAR_PLANE = 100.0f;

#pragma endregion


void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#pragma  region Uniform and Vertex
struct UniformStructure
{
	// Renderer
	float maxRaySteps = 1000.0;
	float baseMinDistance = 0.00001;
	float minDistance = baseMinDistance;
	int minDistanceFactor = 0;
	int fractalIters = 100;
	float bailLimit = 5.0;

	// Mandelbulb
	float power = 12.0;
	int derivativeBias = 1;
	bool julia = false;
	glm::vec3 juliaC = glm::vec3(0.86, 0.23, -0.5);
	bool mandelbulbOn = true;

	// Tetra
	int tetraFactor = 1;
	float tetraScale = 1.0;

	float fudgeFactor = 1.0;
	float noiseFactor = 0.9;
	glm::vec3 bgColor = glm::vec3(0.8, 0.85, 1.0);
	glm::vec3 glowColor = glm::vec3(0.75, 0.9, 1.0);
	float glowFactor = 1.0;
	bool showBgGradient = true;
	bool recursiveTetraOn = false;

	glm::vec4 orbitStrength = glm::vec4(-1.0, -1.8, -1.4, 1.3);
	glm::vec3 otColor0 = glm::vec3(0.3, 0.5, 0.2);
	glm::vec3 otColor1 = glm::vec3(0.6, 0.2, 0.5);
	glm::vec3 otColor2 = glm::vec3(0.25, 0.7, 0.9);
	glm::vec3 otColor3 = glm::vec3(0.2, 0.45, 0.25);
	glm::vec3 otColorBase = glm::vec3(0.3, 0.6, 0.76);
	float otBaseStrength = 0.5;
	float otDist0to1 = 0.3;
	float otDist1to2 = 1.0;
	float otDist2to3 = 0.4;
	float otDist3to0 = 0.2;
	float otCycleIntensity = 5.0;
	float otPaletteOffset = 0.0;

	int shadowRayMinStepsTaken = 5;
	glm::vec3 lightPos = glm::vec3(3.0, 3.0, 10.0);
	float shadowBrightness = 0.2f;
	bool lightSource = false;
	float phongShadingMixFactor = 1.0;
	float ambientIntensity = 1.0;
	float diffuseIntensity = 1.0;
	float specularIntensity = 1.0;
	float shininess = 32.0;
	bool gammaCorrection = false;
};
const float quadArray[4][2] =
{
	{-1.0f, -1.0f},
	{ 1.0f, -1.0f},
	{-1.0f,  1.0f},
	{ 1.0f,  1.0f}
};
glm::mat4x2 quad = glm::make_mat4x2(&quadArray[0][0]);
#pragma endregion

int main()
{
	#pragma region Init glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Mandelbulb", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
#pragma endregion

	glEnable(GL_DEPTH_TEST);
	Shader shader = Shader("VertexShader.vert", "FragmentShader.frag");

	#pragma region VAO, VBO
	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), &quad[0][0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(0);
#pragma endregion

	UniformStructure u;

	float step = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		#pragma region render setting
		float currentFrame = float(glfwGetTime());
		deltaTime = 0.1 * (currentFrame - lastFrame);
		lastFrame = currentFrame;
		processInput(window);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
#pragma endregion

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, NEAR_PLANE, FAR_PLANE);
		view = camera.GetViewMatrix();

		// view = glm::translate(view, glm::vec3(0.0, 0.0, step));
		view = glm::rotate(view, glm::radians(step), glm::vec3(1, 1, 0));
		step = step + 0.1f;
	    // u.power = u.power + 0.05f;

		glBindVertexArray(VAO);
		#pragma region shader setting
		shader.use();
		shader.setMat4("model", model);
		shader.setMat4("view", view);
		shader.setMat4("projection", projection);
		shader.setFloat("u_nearPlane", NEAR_PLANE);
		shader.setFloat("u_farPlane", FAR_PLANE);
		shader.setFloat("u_time", currentFrame);
		shader.setFloat("u_screenRatio", screenRatio);
		shader.setVec2("u_screenSize", screenSize);

		shader.setFloat("u_maxRaySteps", u.maxRaySteps);
		shader.setFloat("u_minDistance", u.minDistance);
		shader.setInt("u_fractalIters", u.fractalIters);
		shader.setFloat("u_bailLimit", u.bailLimit);

		shader.setBool("u_mandelbulbOn", u.mandelbulbOn);
		shader.setInt("u_derivativeBias", u.derivativeBias);
		shader.setFloat("u_power", u.power);
		shader.setBool("u_julia", u.julia);
		shader.setVec3("u_juliaC", u.juliaC);

		shader.setInt("u_tetraFactor", u.recursiveTetraOn ? u.tetraFactor : 0);
		shader.setFloat("u_tetraScale", u.tetraScale);

		shader.setVec4("u_orbitStrength", u.orbitStrength);
		shader.setVec3("u_color0", u.otColor0);
		shader.setVec3("u_color1", u.otColor1);
		shader.setVec3("u_color2", u.otColor2);
		shader.setVec3("u_color3", u.otColor3);
		shader.setVec3("u_colorBase", u.otColorBase);
		shader.setFloat("u_baseColorStrength", u.otBaseStrength);
		shader.setFloat("u_otDist0to1", u.otDist0to1);
		shader.setFloat("u_otDist1to2", u.otDist1to2);
		shader.setFloat("u_otDist2to3", u.otDist2to3);
		shader.setFloat("u_otDist3to0", u.otDist3to0);
		shader.setFloat("u_otCycleIntensity", u.otCycleIntensity);
		shader.setFloat("u_otPaletteOffset", u.otPaletteOffset);

		shader.setInt("u_shadowRayMinStepsTaken", u.shadowRayMinStepsTaken);
		shader.setInt("u_lightSource", u.lightSource);
		shader.setFloat("u_phongShadingMixFactor", u.phongShadingMixFactor);
		shader.setVec3("u_lightPos", u.lightPos);

		shader.setInt("u_shadowBrightness", u.shadowBrightness);
		shader.setVec3("u_bgColor", u.bgColor);
		shader.setVec3("u_glowColor", u.glowColor);
		shader.setFloat("u_glowFactor", u.glowFactor);
		shader.setVec3("u_CameraPos", camera.Position);
		shader.setInt("u_showBgGradient", u.showBgGradient);
		shader.setFloat("u_noiseFactor", u.noiseFactor);
		shader.setFloat("u_fudgeFactor", u.fudgeFactor);
		shader.setFloat("u_ambientIntensity", u.ambientIntensity);
		shader.setFloat("u_diffuseIntensity", u.diffuseIntensity);
		shader.setFloat("u_specularIntensity", u.specularIntensity);
		shader.setFloat("u_shininess", u.shininess);
		shader.setInt("u_gammaCorrection", u.gammaCorrection);
		#pragma endregion

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	// If it is the first time to enter, record the current mouse position to initialize the mouse position
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// Calculate the position difference between the current frame and the previous 
	double xoffset = 0.1f * (xpos - lastX);
	double yoffset = 0.1f * (lastY - ypos);

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}