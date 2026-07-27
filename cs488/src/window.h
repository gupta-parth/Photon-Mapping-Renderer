#pragma once

#include "globals.h"
#include "image.h"
#include "input.h"
#include "random.h"
#include "scene.h"
#include "shading.h"

// you probably do not need to modify this in A0 to A3.
class OpenGLInit {
public:
	OpenGLInit() {
		// initialize GLFW
		if (!glfwInit()) {
			std::cerr << "Failed to initialize GLFW." << std::endl;
			exit(-1);
		}

		// create a window
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		globalGLFWindow = glfwCreateWindow(globalWidth, globalHeight, "Welcome to CS488/688!", NULL, NULL);
		if (globalGLFWindow == NULL) {
			std::cerr << "Failed to open GLFW window." << std::endl;
			glfwTerminate();
			exit(-1);
		}

		// make OpenGL context for the window
		glfwMakeContextCurrent(globalGLFWindow);

		// initialize GLEW
		glewExperimental = true;
		if (glewInit() != GLEW_OK) {
			std::cerr << "Failed to initialize GLEW." << std::endl;
			glfwTerminate();
			exit(-1);
		}

		// set callback functions for events
		glfwSetKeyCallback(globalGLFWindow, keyFunc);
		glfwSetMouseButtonCallback(globalGLFWindow, mouseButtonFunc);
		glfwSetCursorPosCallback(globalGLFWindow, cursorPosFunc);

		// create shader
		FSDraw = glCreateProgram();
		GLuint s = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(s, 1, &PFSDrawSource, 0);
		glCompileShader(s);
		glAttachShader(FSDraw, s);
		glLinkProgram(FSDraw);

		// create texture
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &GLFrameBufferTexture);
		glBindTexture(GL_TEXTURE_2D, GLFrameBufferTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, globalWidth, globalHeight, 0, GL_LUMINANCE, GL_FLOAT, 0);

		// initialize some OpenGL state (will not change)
		glDisable(GL_DEPTH_TEST);

		glUseProgram(FSDraw);
		glUniform1i(glGetUniformLocation(FSDraw, "input_tex"), 0);

		GLint dims[4];
		glGetIntegerv(GL_VIEWPORT, dims);
		const float BufInfo[4] = { float(dims[2]), float(dims[3]), 1.0f / float(dims[2]), 1.0f / float(dims[3]) };
		glUniform4fv(glGetUniformLocation(FSDraw, "BufInfo"), 1, BufInfo);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GLFrameBufferTexture);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	virtual ~OpenGLInit() {
		glfwTerminate();
	}
};



// main window
// you probably do not need to modify this in A0 to A3.
class CS488Window {
public:
	// put this first to make sure that the glInit's constructor is called before the one for CS488Window
	OpenGLInit glInit;

	CS488Window() {}
	virtual ~CS488Window() {}

	void(*process)() = NULL;

	void start() const {
		globalScene.preCalc();
		while (glfwWindowShouldClose(globalGLFWindow) == GL_FALSE) {
			glfwPollEvents();
			globalViewDir = normalize(globalLookat - globalEye);
			globalRight = normalize(cross(globalViewDir, globalUp));

			if (globalRenderType == RENDER_RAYTRACE) {
				globalScene.Raytrace();
			} else if (globalRenderType == RENDER_IMAGE) {
				if (process) process();
			}

			// drawing the frame buffer via OpenGL (you don't need to touch this)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, globalWidth, globalHeight, GL_RGB, GL_FLOAT, &FrameBuffer.pixels[0][0]);
			glRecti(1, 1, -1, -1);
			glfwSwapBuffers(globalGLFWindow);
			globalFrameCount++;
			PCG32::rand();
		}
	}
};



