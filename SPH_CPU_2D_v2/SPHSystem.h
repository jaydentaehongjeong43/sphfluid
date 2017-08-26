#ifndef __SPHSYSTEM_H__
#define __SPHSYSTEM_H__

#include "Structure.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <functional>

#define PI glm::pi<float>()
#define INF 1E-12f

class SPHSystem
{
public:

	SPHSystem();
	~SPHSystem();
	void initFluid();
	void addSingleParticle(glm::vec2 pos, glm::vec2 vel);
	glm::ivec2 calcCellPos(glm::vec2 pos) const;
	unsigned calcCellHash(glm::ivec2 pos) const;

	//kernel function
	float poly6(float r2) const { return 315.0f / (64.0f * PI * pow(kernel, 9)) * pow(kernel * kernel - r2, 3); }
	float spiky(float r) const { return -45.0f / (PI * pow(kernel, 6)) * (kernel - r) * (kernel - r); }
	float visco(float r) const { return 45.0f / (PI * pow(kernel, 6)) * (kernel - r); }

	//animation
	void compTimeStep();
	void buildGrid();
	void _compDensPressure_process(Particle& p, unsigned hash);
	void compDensPressure();
	void _compForce_processCell(Particle* p, unsigned hash);
	void forEachParticle(std::function<void(Particle&)> func);
	void clearAcceleration();
	void computeAcceleration();
	void compForce();
	void advection();
	void animation();

	//getter
	unsigned getNumParticle() const { return numParticle; }
	glm::vec2 getWorldSize() const { return worldSize; }
	Particle const* getParticles() const { return particles.data(); }
	Cell const* getCells() const { return cells.data(); }

private:
	float kernel;
	float mass;

	unsigned maxParticle;
	unsigned numParticle;

	glm::ivec2 gridSize;
	glm::vec2 worldSize;
	float cellSize;
	unsigned totCell;

	//params
	glm::vec2 gravity;
	float stiffness;
	float restDensity;
	float timeStep;
	float wallDamping;
	float viscosity;

	std::vector<Particle> particles;
	std::vector<Cell> cells;
};

#endif
