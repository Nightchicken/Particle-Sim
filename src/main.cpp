#include <stdlib.h>
#include "particleHandler.h"
#include <pthread.h>
int main(){
	int arraySize = 128;
	particle* particleArray[arraySize];
	for (int i = 0; i < arraySize; i++){
		particleArray[i] = createParticle(0, 0, 1);
	}
	for (int i = 0; i < arraySize; i++){
		printParticle(particleArray[i]);
	}
	return 0;
}
