#ifndef PARTICLEHANDLER_H
#define PARTICLEHANDLER_H
typedef struct vec2f{
	float x;
	float y;
}vec2f;
typedef struct vec3f{
	float x;
	float y;
	float z;
}vec3f;
typedef struct particle{
	vec2f position;
	float mass;
	vec2f accel;
}particle;
typedef struct node{
	vec2f lowerBounds;
	vec2f upperBounds;
	particle* particlePtr;
	node* left;
	node* right;
}node;
vec2f operator+=(vec2f& a, const vec2f& b);
vec2f operator+(vec2f& a, const vec2f& b);
vec2f operator-(const vec2f& a, const vec2f& b);
float operator*(const vec2f& a, const vec2f& b);
vec2f operator*(const vec2f& a, float b);
node* createNode(particle* particle,vec2f upperBounds, vec2f lowerBounds);
particle* createParticle(float x, float y, float mass);
void printParticle(particle* particle);
void moveParticle(particle* particle, vec2f accel);

#endif
