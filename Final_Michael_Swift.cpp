/*
* This code is used to draw primitives that are then moved, expanded and rotated
* to create an illuminated textured chair with a user operated camera by holding left alt and dragging mouse while holding right mouse button
* the option to rotate the camera around the object by holding down a key the s key
* shaders are used to add color and texture to the primitives
* Author: Michael Swift
*/
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SOIL2/SOIL2.h>

using namespace std;

int width, height;
const double PI = 3.14159;
const float toRadians = PI / 180.0f;

// Declare Input Callback Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mode);

// Declare View Matrix
glm::mat4 viewMatrix;

// Camera Field of View
GLfloat fov = 45.0f;

void initiateCamera();
void spinCamera();

// Define Camera Attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 1.0f, 4.0f); 
glm::vec3 target = glm::vec3(-0.375f, 0.5f, 0.4);
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target); 
glm::vec3 worldUp = glm::vec3(0.0, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight)); 
glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 

// Declare target prototype
glm::vec3 getTarget();

// Camera Transformation Prototype
void TransformCamera();

// Boolean array for keys and mouse buttons
bool keys[1024], mouseButtons[3];

// Booleans to check camera transformation
bool isPanning = false, isOrbiting = false;

// Pitch and Yaw
GLfloat radius = 3.0f, rawYaw = 0.0f, rawPitch = 0.0f, degYaw, degPitch;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat lastX = 320, lastY = 240, xChange, yChange; 

bool firstMouseMove = true; 

// Light source position
glm::vec3 lightPosition(1.0f, 1.0f, 1.0f);

// Draw Primitive(s)
void draw()
{
	GLenum mode = GL_TRIANGLES;
	GLsizei indices = 6;
	glDrawElements(mode, indices, GL_UNSIGNED_BYTE, nullptr);


}

// Create and Compile Shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	glShaderSource(shaderID, 1, &src, nullptr);
	glCompileShader(shaderID);

	return shaderID;

}

// Create Program Object
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	// Create program object
	GLuint shaderProgram = glCreateProgram();

	// Attach vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	// Link shaders to create executable
	glLinkProgram(shaderProgram);

	// Delete compiled vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	// Return Shader Program
	return shaderProgram;

}

/*
* Main function to create window where keycallbacks are used to interact with the camera around the objects drawn
*/
int main(void)
{
	//Demensions for window
	width = 640; height = 480;
	GLFWwindow* window;
	
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Main Window", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Set input callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
		cout << "Error!" << endl;

	GLfloat lampVertices[] = {
		-0.5, -0.5, 0.0, // index 0

		-0.5, 0.5, 0.0,   // index 1

		0.5, -0.5, 0.0,  // index 2	

		0.5, 0.5, 0.0    // index 3
	};

	GLubyte lampIndices[] = {
		0, 1, 2,
		1, 2, 3
	};

	GLfloat vertices[] = {

		// Triangle charateristics each index location and color used for shader
		-0.25, -0.25, 0.0,// index 0
		1.0, 0.0, 0.0, 
		0.0, 0.0, 
		0.0f, 0.0f, 1.0f, 

		-0.25, 0.25, 0.0, // index 1
		0.0, 1.0, 0.0, 
		0.0, 1.0, 
		0.0f, 0.0f, 1.0f, 

		0.25, -0.25, 0.0, // index 2	
		0.0, 0.0, 1.0, 
		1.0, 0.0, 
		0.0f, 0.0f, 1.0f, 

			
		0.25, 0.25, 0.0,  // index 3	
		1.0, 0.0, 1.0, 
		1.0, 1.0, 
		0.0f, 0.0f, 1.0f 
	};

	// Define element indices
	GLubyte indices[] = {
		0, 1, 2,
		1, 2, 3
	};

	// Plane Transformations for each plane, front, right, back and left
	// Chair Leg BR 
	glm::vec3 planePositions[] = {
		glm::vec3(0.125f,  0.60f,  0.125f), 
		glm::vec3(0.25f,  0.60f,  0.0f), 
		glm::vec3(0.125f,  0.60f,  -0.125f), 
		glm::vec3(0.0f, 0.60f,  0.0f) 
	};

	// Chair leg BL
	glm::vec3 planePositions2[] = {
		glm::vec3(-0.875f,  0.60f,  0.125f), 
		glm::vec3(-0.75f,  0.60f,  0.0f), 
		glm::vec3(-0.875f,  0.60f,  -0.125f), 
		glm::vec3(-1.0f, 0.60f,  0.0f)  
	};

	// Chair leg FL
	glm::vec3 planePositions3[] = {
		glm::vec3(-0.875f,  0.0f,  0.925f), 
		glm::vec3(-0.75f,  0.0f,  0.8f), 
		glm::vec3(-0.875f,  0.0f,  0.675f), 
		glm::vec3(-1.0f, 0.0f,  0.8f)  
	};

	//Chair leg FR
	glm::vec3 planePositions4[] = {
		glm::vec3(0.125f,  0.0f,  0.925f),
		glm::vec3(0.25f,  0.0f,  0.8f), 
		glm::vec3(0.125f,  0.0f,  0.675f), 
		glm::vec3(0.0f, 0.0f,  0.8f)  
	};
	
	// Plane Positions for chair seat, front, right, back, left, top and bottom
		glm::vec3 planePositions5[] = {
		glm::vec3(-0.375f,  0.65f,  .93f), 
		glm::vec3(0.25f,  0.65f,  0.40f), 
		glm::vec3(-0.375f,  0.65f,  -0.125f), 
		glm::vec3(-1.0f, 0.65f,  0.40f),  
		glm::vec3(-0.375f, 0.76f, 0.40f), 
		glm::vec3(-0.375f, 0.54f, 0.40f) 
	};

	// Plane Positions for chair back, front, back and top
	glm::vec3 planePositions6[] = {
		glm::vec3(-0.375f,  1.51f,  0.125f), 
		glm::vec3(-0.375f,  1.51f,  -0.125f), 
		glm::vec3(-0.375f, 1.975f, 0.0f) 
		
	};


	// Plane rotations to create 3D objects
	glm::float32 planeRotations[] = {
		0.0f, 90.0f, 180.0f, -90.0f
	};

	glm::float32 planeRotations2[] = {
		0.0f, 180.0f, -90.0f
	};

	glm::float32 planeRotations3[] = {
		0.0f, 90.0f, 180.0f, -90.0f, -90.f, 90.f
	};

	
	glEnable(GL_DEPTH_TEST);


	// Create VBO and EBO for each 3D object, floor and light source that is processed in the shader
	GLuint cubeVBO, cubeEBO, cubeVAO, floorVBO, floorEBO, floorVAO, lampVBO, lampEBO, lampVAO;

	glGenBuffers(1, &cubeVBO); 
	glGenBuffers(1, &cubeEBO); 

	glGenBuffers(1, &floorVBO); 
	glGenBuffers(1, &floorEBO); 

	glGenBuffers(1, &lampVBO); 
	glGenBuffers(1, &lampEBO); 
	
	glGenVertexArrays(1, &cubeVAO); 
	glGenVertexArrays(1, &floorVAO); 
	glGenVertexArrays(1, &lampVAO); 

	glBindVertexArray(cubeVAO);

	// VBO and EBO Placed in User-Defined VAO
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO); 
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 
	
	// Specify attribute location and layout to GPU for 3D objects
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);
	 
	glBindVertexArray(0); 

	//Define floor VAO
	glBindVertexArray(floorVAO);

	glBindBuffer(GL_ARRAY_BUFFER, floorVBO); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO); 
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

	// Specify attribute location and layout to GPU for floor
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);
	
	glBindVertexArray(0);

	// Define Lamp VAO
	glBindVertexArray(lampVAO);

	glBindBuffer(GL_ARRAY_BUFFER, lampVBO); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO); 

	glBufferData(GL_ARRAY_BUFFER, sizeof(lampVertices), lampVertices, GL_STATIC_DRAW); 
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lampIndices), lampIndices, GL_STATIC_DRAW); 
	
	// Specify attribute location and layout to GPU for floor
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	// Load Textures
	int crateTextWidth, crateTextHeight, gridTextWidth, gridTextHeight;
	unsigned char* crateImage = SOIL_load_image("wood.jpg", &crateTextWidth, &crateTextHeight, 0, SOIL_LOAD_RGB);
	unsigned char* gridImage = SOIL_load_image("grid.png", &gridTextWidth, &gridTextHeight, 0, SOIL_LOAD_RGB);


	// Generate Textures
	GLuint crateTexture;
	glGenTextures(1, &crateTexture);
	glBindTexture(GL_TEXTURE_2D, crateTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, crateTextWidth, crateTextHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, crateImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(crateImage);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint gridTexture;
	glGenTextures(1, &gridTexture);
	glBindTexture(GL_TEXTURE_2D, gridTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gridTextWidth, gridTextHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, gridImage);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(gridImage);
	glBindTexture(GL_TEXTURE_2D, 0);


	// Vertex shader source code
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"layout(location = 1) in vec3 aColor;"
		"layout(location = 2) in vec2 texCoord;"
		"layout(location = 3) in vec3 normal;"
		"out vec3 oColor;"
		"out vec2 oTexCoord;"
		"out vec3 oNormal;"
		"out vec3 fragPos;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"oColor = aColor;"
		"oNormal = mat3(transpose(inverse(model))) * normal;"
		"fragPos = vec3(model * vec4(vPosition, 1.0f));"
		"oTexCoord = texCoord;"
		"}\n";

	// Fragment shader source code
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec3 oColor;"
		"in vec2 oTexCoord;"
		"in vec3 oNormal;"
		"in vec3 fragPos;"
		"out vec4 fragColor;"
		"uniform sampler2D myTexture;"
		"uniform vec3 objectColor;"
		"uniform vec3 lightColor;"
		"uniform vec3 lightPos;"
		"uniform vec3 viewPos;"
		"void main()\n"
		"{\n"
		"//Ambient\n"
		"float ambientStrength = 0.4f;"
		"vec3 ambient = ambientStrength * lightColor;"
		"//Diffuse\n"
		"vec3 norm = normalize(oNormal);"
		"vec3 lightDir = normalize(lightPos - fragPos);"
		"float diff = max(dot(norm, lightDir), 0.0);"
		"vec3 diffuse = diff * lightColor;"
		"//Specularity\n"
		"float specularStr = 1.5f;"
		"vec3 viewDir = normalize(viewPos - fragPos);"
		"vec3 reflectDir = reflect(-lightDir, norm);"
		"float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);"
		"vec3 specular = specularStr * spec * lightColor;"
		"vec3 result = (ambient + diffuse + specular) * objectColor;"
		"fragColor = texture(myTexture, oTexCoord) * vec4(result, 1.0f);"
		"}\n";

	// Lamp Vertex shader source code
	string lampVertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec3 vPosition;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);"
		"}\n";

	// Lamp Fragment shader source code
	string lampFragmentShaderSource =
		"#version 330 core\n"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"fragColor = vec4(1.0f);"
		"}\n";

	// Create Shader Program
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
	GLuint lampShaderProgram = CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource);

	
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use Shader Program exe and select VAO before drawing 
		glUseProgram(shaderProgram); 
		glm::mat4 projectionMatrix;			   		
		viewMatrix = glm::lookAt(cameraPosition, getTarget(), worldUp);
		projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

		// Get matrix's uniform location and set matrix
		GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

		// Get light and object color, and light position location
		GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
		GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
		GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
		GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

		// Assign Light and Object Colors
		glUniform3f(objectColorLoc, 0.76f, 0.60f, 0.32f);
		glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); 

		// Set light position
		glUniform3f(lightPosLoc, lightPosition.x, lightPosition.y, lightPosition.z);

		// Specify view position
		glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glBindTexture(GL_TEXTURE_2D, crateTexture); 
		glBindVertexArray(cubeVAO);  

		// Create back right leg for chair
		for (GLuint i = 0; i < 4; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions[i]);
			modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.50f, 5.5f, 0.50f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		}
		glBindVertexArray(0); 

		glBindTexture(GL_TEXTURE_2D, crateTexture); 
		glBindVertexArray(cubeVAO); 
		
		// Create back left leg for chair
		for (GLuint i = 0; i < 4; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions2[i]);
			modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.50f, 5.5f, 0.50f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		}
		glBindVertexArray(0); 
	
		glBindTexture(GL_TEXTURE_2D, crateTexture); 
		glBindVertexArray(cubeVAO);  

		// Create front left leg of chair
		for (GLuint i = 0; i < 4; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions3[i]);
			modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.50f, 3.0f, 0.50f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		}
		glBindVertexArray(0); 

		glBindTexture(GL_TEXTURE_2D, crateTexture); 
		glBindVertexArray(cubeVAO); 

		// Create back left leg of chair
		for (GLuint i = 0; i < 4; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions4[i]);
			modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.50f, 3.0f, 0.50f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		}
		glBindVertexArray(0); 
		
		glBindTexture(GL_TEXTURE_2D, crateTexture); 
		glBindVertexArray(cubeVAO);  

		// Create chair seat
		for (GLuint i = 0; i < 6; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions5[i]);
			modelMatrix = glm::rotate(modelMatrix, planeRotations3[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(2.1f, 0.45f, 2.50f));
			if (i >= 4)
				modelMatrix = glm::rotate(modelMatrix, planeRotations3[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		}
		glBindVertexArray(0); 
				
		glBindTexture(GL_TEXTURE_2D, crateTexture); 
		glBindVertexArray(cubeVAO); 
        
		// Chair Back
		for (GLuint i = 0; i < 3; i++)
		{
			glm::mat4 modelMatrix;
			modelMatrix = glm::translate(modelMatrix, planePositions6[i]);
			modelMatrix = glm::rotate(modelMatrix, planeRotations2[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(2.5f, 1.85f, 1.0f));
			if (i >= 2) {
				modelMatrix = glm::rotate(modelMatrix, planeRotations2[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.20f, 2.5f, 1.0f));					
			}
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		}
		glBindVertexArray(0); 
		
		// Create grid textured floor
		glBindTexture(GL_TEXTURE_2D, gridTexture); 
		glBindVertexArray(floorVAO);
		glm::mat4 modelMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-.4f, -0.75f, 0.1f));
		modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(5.f, 5.f, 5.f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		draw();
		glBindVertexArray(0); 
		glUseProgram(0); 

		// Used to display the light source for adjusting lighting
		/*
		glUseProgram(lampShaderProgram);

		// Get matrix's uniform location and set matrix
		GLint lampModelLoc = glGetUniformLocation(lampShaderProgram, "model");
		GLint lampViewLoc = glGetUniformLocation(lampShaderProgram, "view");
		GLint lampProjLoc = glGetUniformLocation(lampShaderProgram, "projection");

		glUniformMatrix4fv(lampViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(lampProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glBindVertexArray(lampVAO); // User-defined VAO must be called before draw. 

		// Transform planes to sides of lamp
		
		for (GLuint i = 0; i < 4; i++)
		{
			glm::mat4 modelMatrix;

			modelMatrix = glm::translate(modelMatrix, planePositions[i] / glm::vec3(8.f, 8.f, 8.f) + lightPosition);
			modelMatrix = glm::rotate(modelMatrix, planeRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(.125f, .125f, .125f)); //shrink lamp
			
			glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			// Draw primitive(s)
			draw();
		}

		// Unbind Shader exe and VOA after drawing per frame
		glBindVertexArray(0); //Incase different VAO wii be used after

		glUseProgram(0);
		*/
						 
		glfwSwapBuffers(window);
		glfwPollEvents();
		TransformCamera();
	}

	//Clear GPU resources
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &cubeEBO);
	glDeleteVertexArrays(1, &floorVAO);
	glDeleteBuffers(1, &floorVBO);
	glDeleteBuffers(1, &floorEBO);
	
	glfwTerminate();
	return 0;
}

// Define input functions
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	// Close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Assign true to Element ASCII if key pressed
	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE) 
		keys[key] = false;

}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Clamp FOV to prevent camera distortion
	if (fov >= 1.0f && fov <= 55.0f)
		fov -= yoffset * 0.01;

	// Default FOV
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 55.0f)
		fov = 55.0f;

}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouseMove)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}
	// Calculate mouse offset 
	xChange = xpos - lastX;
	yChange = lastY - ypos; 
	lastX = xpos;
	lastY = ypos;


	if (isOrbiting)
	{
		// Update raw yaw and pitch with mouse movement
		rawYaw += xChange;
		rawPitch += yChange;

		// Convert yaw and pitch to degrees, and clamp pitch
		degYaw = glm::radians(rawYaw);
		degPitch = glm::clamp(glm::radians(rawPitch), -glm::pi<float>() / 2.f + .1f, glm::pi<float>() / 2.f - .1f);

		cameraPosition.x = target.x + radius * cosf(degPitch) * sinf(degYaw);
		cameraPosition.y = target.y + radius * sinf(degPitch);
		cameraPosition.z = target.z + radius * cosf(degPitch) * cosf(degYaw);
	}
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mode)
{
	if (action == GLFW_PRESS)
		mouseButtons[button] = true;
	else if (action == GLFW_RELEASE)
		mouseButtons[button] = false;
}

// Define getTarget function
glm::vec3 getTarget() {
	return target;
}

/* 
* Left alt key and left mouse button to move camera
* F key to reset camera
* S key to spin camera around object
*/
void TransformCamera()
{
	// Orbit camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButtons[GLFW_MOUSE_BUTTON_LEFT])
		isOrbiting = true;
	else
		isOrbiting = false;

	if (keys[GLFW_KEY_F])
		initiateCamera();

	if (keys[GLFW_KEY_S])
		spinCamera();
}

// Reset camera
void initiateCamera()
{	cameraPosition = glm::vec3(0.0f, 1.0f, 4.0f); 
    target = glm::vec3(-0.375f, 0.5f, 0.4);
	cameraDirection = glm::normalize(cameraPosition - cameraDirection); 
	worldUp = glm::vec3(0.0, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight)); 
	CameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
}

// Rotate camera around object
void spinCamera()
{
	cameraPosition = glm::vec3(0.0f, 1.0f, 0.0f) + glm::vec3(3.5f * sin(glfwGetTime()), 0.0f, 3.5f * cos(glfwGetTime()));
	target = glm::vec3(-0.375f, 0.5f, 0.4);
	cameraDirection = glm::normalize(cameraPosition - cameraDirection);
	worldUp = glm::vec3(0.0, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
	CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
}