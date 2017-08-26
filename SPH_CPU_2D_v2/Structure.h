#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

#include <glm/glm.hpp>

class Particle
{
public:
	glm::vec2 pos;
	glm::vec2 vel;
	glm::vec2 acc;
	glm::vec2 ev;

	float dens;
	float pres;

	Particle* next{nullptr};
};

class Cell
{
public:
	Particle* head;
};

#endif
