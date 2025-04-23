// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale)
{
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}
string slurp(string fileName); // forward declaration

class like
{
public:
  int i, j; // string names serves to refer to them //i and j r two particles

  float energy;
};


class buddies
{
public:
  int i, j; // string names serves to refer to them //i and j r two particles

  float vibes;
};


class spring
{
public:
  int i, j; // string names serves to refer to them //i and j r two particles

  float length;    // resting length
  float stiffness; // resting stiffness
};

struct AlloApp : App
{
  Parameter pointSize{"/pointSize", "", 2.0, 0.0, 20.0};
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  Parameter dragFactor{"/dragFactor", "", 0.1, 0.0, 0.9};
  Parameter repulsionFactor{"/repulsionFactor", "", 0.1, 0.0, 10.9};
  Parameter boundarySize{"/boundSize", "", 1.0, 0.0, 10.9};
  //

  ShaderProgram pointShader;

  //  simulation state
  Mesh mesh; // position *is inside the mesh* mesh.vertices() are the positions
  vector<Vec3f> velocity;
  vector<Vec3f> force;
  vector<float> mass;

  std::vector<spring> spring_list; // need to make it a member // vector holds a bunch of spring lists
  std::vector<like> like_list;

  void onInit() override
  {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize);  // add parameter to GUI
    gui.add(timeStep);   // add parameter to GUI
    gui.add(dragFactor); // add parameter to GUI
    gui.add(repulsionFactor); // add parameter to GUI
    gui.add(boundarySize); // add parameter to GUI
    //
  }

  void onCreate() override
  {
    // compile shaders
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    // set initial conditions of the simulation
    //

    // c++11 "lambda" function
    auto randomColor = []()
    { return HSV(rnd::uniform(0.50, 0.70), rnd::uniform(), rnd::uniform()); };

    mesh.primitive(Mesh::POINTS);
    // does 1000 work on your system? how many can you make before you get a low
    // frame rate? do you need to use <1000?
    for (int _ = 0; _ < 1000; _++)
    {
      mesh.vertex(randomVec3f(5));
      mesh.color(randomColor());

      // float m = rnd::uniform(3.0, 0.5);
      float m = 3 + rnd::normal() / 2;
      if (m < 0.5)
        m = 0.5;
      mass.push_back(m);

      // using a simplified volume/size relationship
      mesh.texCoord(pow(m, 1.0f / 3), 0); // s, t

      // separate state arrays
      velocity.push_back(randomVec3f(0.1));
      force.push_back(randomVec3f(1));
    }

    nav().pos(0, 0, 10);
  }

  bool freeze = false;
  void onAnimate(double dt) override
  {
    if (freeze)
      return;

    // compute spring force

    for (int k = 0; k < spring_list.size(); ++k)
    {
      auto spring = spring_list[k];
      // positions of the particle pair...
      Vec3f a = mesh.vertices()[spring.i]; 
      Vec3f b = mesh.vertices()[spring.j];
      Vec3f displacement = b - a;
      float distance = displacement.mag();
      Vec3f f = displacement.normalize() * spring.stiffness * (distance - spring.length); // if u have a normalization it sets the length to one
      force[spring.i] += f;
      force[spring.j] -= f;
    }


    for (int k = 0; k < mesh.vertices().size(); ++k)
    {

     // float bound = floor(rnd::uniform(5.0f, 10.0f));
      Vec3f a = mesh.vertices()[k]; // make b the origin 
      Vec3f b = Vec3f(0,0,0);
      Vec3f displacement = b - a;
      float distance = displacement.mag();
      Vec3f f = displacement.normalize() * (distance - boundarySize); 
      force[k] += f;
    }

    for (int k = 0; k < like_list.size(); ++k)
    {
      auto like = like_list[k];
      // positions of the particle pair...
      Vec3f a = mesh.vertices()[like.i]; // hw is building springs between a and the origin not b
      Vec3f b = mesh.vertices()[like.j];
      Vec3f displacement = b - a;
      Vec3f f = displacement.normalize() * like.energy; //
      force[like.i] += f;
      force[like.j] += f; // make them both same so that theyre asymettrical
    }


    // Calculate forces


    // repulsion (culombs law)
    for (int i = 0; i < mesh.vertices().size(); ++i)
    {
      for (int j = i + 1; j < mesh.vertices().size(); ++j)
      {
        if (i == j) continue;

        Vec3f a = mesh.vertices()[i];
        Vec3f b = mesh.vertices()[j];
        Vec3f displacement = a - b;

        float distSqr = displacement.magSqr(); // alternatively float distance = displacement.mag(); --> (distance * distance);
        float forceUnit = repulsionFactor / distSqr; 
        forceUnit = min(forceUnit, 1.0f); // add a boundary to limit the maximum strength


        Vec3f direction = displacement.normalize();
        Vec3f f = direction * forceUnit;
        force[i] += f;
        force[j] -= f;

        // i and j are a pair
        // apply and equal and possible force
        // as they get futher apart they influence each other much less
        // as they get close to each other they influence each other a lot more

        // limit large forces... if the force is too large, ignore it // they should slide thru each other
      }
    }


    //

    // XXX you put code here that calculates gravitational forces and sets
    // accelerations These are pair-wise. Each unique pairing of two particles
    // These are equal but opposite: A exerts a force on B while B exerts that
    // same amount of force on A (but in the opposite direction!) Use a nested
    // for loop to visit each pair once The time complexity is O(n*n)
    //
    // Vec3f has lots of operations you might use...
    // • +=
    // • -=
    // • +
    // • -
    // • .normalize() ~ Vec3f points in the direction as it did, but has length 1
    // • .normalize(float scale) ~ same but length `scale`
    // • .mag() ~ length of the Vec3f
    // • .magSqr() ~ squared length of the Vec3f
    // • .dot(Vec3f f)
    // • .cross(Vec3f f)

    // drag
    for (int i = 0; i < velocity.size(); i++)
    {
      force[i] += -velocity[i] * dragFactor;
    }

    // Integration
    //
    vector<Vec3f> &position(mesh.vertices());
    for (int i = 0; i < velocity.size(); i++)
    {
      // "semi-implicit" Euler integration
      velocity[i] += force[i] / mass[i] * timeStep;
      position[i] += velocity[i] * timeStep;
    }

    // clear all accelerations (IMPORTANT!!)
    for (auto &a : force)
      a.set(0);
  }

  bool onKeyDown(const Keyboard &k) override
  {
    if (k.key() == ' ')
    {
      freeze = !freeze;
    }

    if (k.key() == '1')
    {
      // introduce some "random" forces
      for (int i = 0; i < velocity.size(); i++)
      {
        // F = ma
        force[i] += randomVec3f(1);
      }
    }

    if (k.key() == '2')
    {
      // choose 2 particles at random
      int i = rnd::uniform(mesh.vertices().size());
      int j = rnd::uniform(mesh.vertices().size());
      while (i == j)
      {
        j = rnd::uniform(mesh.vertices().size());
      }

      // i and j are different particles ...
      spring_list.push_back({i, j, 1.0, 5.0}); // default length and stiffness
    }

    if (k.key() == '3')
    {
      // choose 2 particles at random
      int i = rnd::uniform(mesh.vertices().size());
      int j = rnd::uniform(mesh.vertices().size());
      while (i == j)
      {
        j = rnd::uniform(mesh.vertices().size());
      }

      like_list.push_back({i, j, 0.1});
    }


    return true;
  }

  void onDraw(Graphics &g) override
  {
    g.clear(0.3);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);

    // reset to the default shader if we want to draw something else

    g.color(1.0, 1.0, 0.0); // resets shader...

    Mesh springs(Mesh::LINES); // need to fill this part out
    for (int k = 0; k < spring_list.size(); ++k)
    {
      auto spring = spring_list[k];
      Vec3f a = mesh.vertices()[spring.i];
      Vec3f b = mesh.vertices()[spring.j];
      springs.vertex(a);
      springs.vertex(b);
    }

    g.draw(springs);
  }
};

int main()
{
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

string slurp(string fileName)
{
  fstream file(fileName);
  string returnValue = "";
  while (file.good())
  {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
