#ifndef PARTICLE_HANDLER_H
#include "particleHandler.h"
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _GLIBCXX_STDLIB_H
#include <stdlib.h>
#endif
vec2f operator+=(vec2f& a, const vec2f& b){
	a.x += b.x;
	a.y += b.y;
	return a;
}
vec2f operator+(const vec2f& a, const vec2f& b){
	vec2f temp;
	temp.x = a.x + b.x;
	temp.y = a.y + b.y;
	return temp;
}
vec2f operator-(const vec2f& a, const vec2f& b){
	vec2f temp;
	temp.x = a.x - b.x;
	temp.y = a.y - b.y;
	return temp;
}
float operator*(const vec2f& a, const vec2f& b){
	return ((float)(a.x*b.x+a.y*b.y));
}
vec2f operator*(vec2f& a, float b){
	vec2f temp;
	temp.x = b*a.x;
	temp.y = b*a.y;
	return temp;
}
node* createNode(particle* particle,vec2f upperBounds, vec2f lowerBounds){
	node* nodePtr = (node*)malloc(sizeof(node));
	nodePtr->lowerBounds = lowerBounds;
	nodePtr->upperBounds = upperBounds;
	nodePtr->particlePtr = particle;
	return nodePtr;
}
particle* createParticle(float x, float y, float mass){
	particle* particlePtr = (particle*)malloc(sizeof(particle));
	particlePtr->position = {x,y};
	particlePtr->mass = mass;
	return particlePtr;
}
void moveParticle(particle* particle, vec2f accel){
	particle->accel += accel;
	particle->position+=particle->accel;
}
void printParticle(particle* particle){
	printf("X: %03.2f, Y: %03.2f, Mass: %03.2f\n",particle->position.x,particle->position.y,particle->mass);
}
