#version 440 core
// #extension GL_ARB_enhanced_layouts : require

#define PI 3.141592
#define INF 0.000001

struct Particle
{
  vec2 pos;
  vec2 vel;
  vec2 acc;
  vec2 ev;
  float dens;
  float pres;
  int next;
  int pad; // unused
};

struct Cell
{
  int head;
};

layout(std430, binding = 1) buffer Particles 
{ 
  Particle particles[]; 
};

layout(std430, binding = 2) buffer Cells 
{ 
  Cell cells[]; 
};

layout(xfb_buffer = 0, xfb_stride = 48) out VertOut
{
  layout(xfb_offset = 0) vec2 pos;
  layout(xfb_offset = 8) vec2 vel;
  layout(xfb_offset = 16) vec2 acc;
  layout(xfb_offset = 24) vec2 ev;
  layout(xfb_offset = 32) float dens;
  layout(xfb_offset = 36) float pres;
  layout(xfb_offset = 40) int next;
  layout(xfb_offset = 44) int pad; // unused
}
vertOut;

// uniform isampler2DRect cells;
uniform float cellSize;
uniform int gridWidth;
uniform int gridHeight;
uniform float mass;
uniform float restDensity;
uniform float stiffness;
uniform float kernel;
uniform float viscosity;
uniform float timeStep;
uniform float wallDamping;
uniform vec2 gravity;
uniform vec2 worldSize;

ivec2 calcCellPos(vec2 pos)
{
	return ivec2(int(pos.x / cellSize), int(pos.y / cellSize));
}

int calcCellHash(ivec2 pos)
{
	if (pos.x < 0 || pos.x >= gridWidth || pos.y < 0 || pos.y >= gridHeight) return -1;
	return pos.y * gridWidth + pos.x;
}

float poly6(float r2) { return 315.0 / (64.0 * PI * pow(kernel, 9.0)) * pow(kernel * kernel - r2, 3.0); }
float spiky(float r) { return -45.0 / (PI * pow(kernel, 6.0)) * (kernel - r) * (kernel - r); }
float visco(float r) { return 45.0 / (PI * pow(kernel, 6.0)) * (kernel - r); }

void main() {
  Particle p = particles[gl_VertexID];
  p.dens = 0.0;
  p.pres = 0.0;
  ivec2 cellPos = calcCellPos(p.pos);
  for (int xoffset = -1; xoffset <=1; ++xoffset) {
    for (int yoffset = -1; yoffset <=1; ++yoffset) {
      ivec2 nearPos = cellPos + ivec2(xoffset, yoffset);
      int hash = calcCellHash(nearPos);
      if (hash == -1) continue;
      int neighborIdx = cells[hash].head;
      while (neighborIdx >= 0)
      {
        Particle neighbor = particles[neighborIdx];
        vec2 distVec = p.pos - neighbor.pos;
        float dist2 = dot(distVec, distVec);
        // if too close or out of kernel, then skip
        if (dist2 > INF && dist2 < kernel * kernel)
          p.dens = p.dens + mass * poly6(dist2);
        neighborIdx = neighbor.next;
      }
    }
  }
  p.dens = p.dens + mass * poly6(0.0); // self
  p.pres = (pow(p.dens / restDensity, 7) - 1) * stiffness;

  p.acc = vec2(0.0, 0.0);
  for (int xoffset = -1; xoffset <=1; ++xoffset) {
    for (int yoffset = -1; yoffset <=1; ++yoffset) {
      ivec2 nearPos = cellPos + ivec2(xoffset, yoffset);
      uint hash = calcCellHash(nearPos);
      if (hash == -1) continue;
      int neighborIdx = cells[hash].head;
      while (neighborIdx >= 0)
      {
        Particle neighbor = particles[neighborIdx];
        vec2 distVec = neighbor.pos - p.pos;
        float dist2 = dot(distVec, distVec);

        // if too close or out of kernel, then skip
        if (dist2 > INF && dist2 < kernel * kernel)
        {
          float dist = sqrt(dist2);
          float V = mass / p.dens;

          float tempForce = V * (p.pres + neighbor.pres) * spiky(dist);
          p.acc = p.acc - distVec * tempForce / dist;

          vec2 relVel = p.ev - neighbor.ev;
          tempForce = V * viscosity * visco(dist);
          p.acc = p.acc + relVel * tempForce;
        }

        neighborIdx = neighbor.next;
      }
    }
  }
  p.acc = p.acc / p.dens + gravity;
  
  p.vel = p.vel + p.acc * timeStep;
  p.pos = p.pos + p.vel * timeStep;

  if (p.pos.x < 0.0)
  {
    p.vel.x = p.vel.x * wallDamping;
    p.pos.x = 0.0;
  }
  if (p.pos.x >= worldSize.x)
  {
    p.vel.x = p.vel.x * wallDamping;
    p.pos.x = worldSize.x - 0.0001;
  }
  if (p.pos.y < 0.0)
  {
    p.vel.y = p.vel.y * wallDamping;
    p.pos.y = 0.0;
  }
  if (p.pos.y >= worldSize.y)
  {
    p.vel.y = p.vel.y * wallDamping;
    p.pos.y = worldSize.y - 0.0001;
  }

  p.ev = (p.ev + p.vel) / 2.0;

  vertOut.pos  = p.pos;
  vertOut.vel  = p.vel;
  vertOut.acc  = p.acc;
  vertOut.ev   = p.ev;
  vertOut.dens = p.dens;
  vertOut.pres = p.pres;
  vertOut.next = p.next;
  vertOut.pad = p.pad = 0;
}