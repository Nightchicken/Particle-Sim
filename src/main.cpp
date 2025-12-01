#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include "particleHandler.h"
#include <pthread.h>
#include <GLFW/glfw3.h>
//Borrowed from Google
const uint32_t width = 500;
const uint32_t height = 500;
void Start() {
  if (!glfwInit()) {
    return;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window =
      glfwCreateWindow(width, height, "WebGPU window", nullptr, nullptr);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    // TODO: Render a triangle using WebGPU.
  }
}

//End of borrowed code
int main(){
	int arraySize = 128;
	Start();
	particle* particleArray[arraySize];
	for (int i = 0; i < arraySize; i++){
		particleArray[i] = createParticle(0, 0, 1);
	}
	for (int i = 0; i < arraySize; i++){
		printParticle(particleArray[i]);
	}
	for (int i = 0; i < arraySize; i++){
		free(particleArray[i]);
	}
	return 0;
}
