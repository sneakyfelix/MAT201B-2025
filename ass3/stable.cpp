#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/math/al_Vec.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}

void appendMesh(Mesh &target, const Mesh &source) {
  int vertexOffset = target.vertices().size();

  // copy vertices, normals, and colors
  for (int i = 0; i < source.vertices().size(); ++i) {
    target.vertex(source.vertices()[i]);
  }
  for (int i = 0; i < source.normals().size(); ++i) {
    target.normal(source.normals()[i]);
  }
  for (int i = 0; i < source.colors().size(); ++i) {
    target.color(source.colors()[i]);
  }
  for (int i = 0; i < source.indices().size(); ++i) {
    target.index(source.indices()[i] + vertexOffset);
  }
}

Mesh createCatMesh() {
  Mesh cat;
  cat.primitive(Mesh::TRIANGLES);

  auto makeBox = [&](Vec3f scale, Vec3f pos, Color color = Color(1, 0.5, 0)) {
    Mesh m;
    addCube(m);
    m.scale(scale);
    m.translate(pos);
    for (int i = 0; i < m.vertices().size(); i++) {
      m.color(color);
    }
    appendMesh(cat, m);
  };

  // body
  makeBox({0.6f, 0.25f, 0.25f}, {0, 0.0f, 0}, Color(0.45f, 0.27f, 0.07f));

  // head
  makeBox({0.25f, 0.25f, 0.25f}, {0.45f, 0.05f, 0});

  // tail base
  makeBox({0.08f, 0.08f, 0.2f}, {-0.33f, 0.05f, 0.0f});

  // tail tip
  makeBox({0.06f, 0.06f, 0.15f}, {-0.33f, 0.12f, -0.15f}, Color(1));

  // front left leg/paw
  makeBox({0.08f, 0.2f, 0.08f}, {0.2f, -0.225f, 0.15f},
          Color(0.45f, 0.27f, 0.07f));
  makeBox({0.08f, 0.05f, 0.08f}, {0.2f, -0.325f, 0.15f},
          Color(1));  // white paw

  // front right leg/paw
  makeBox({0.08f, 0.2f, 0.08f}, {0.2f, -0.225f, -0.15f});
  makeBox({0.08f, 0.05f, 0.08f}, {0.2f, -0.325f, -0.15f},
          Color(1));  // white paw

  // back left leg/paw
  makeBox({0.08f, 0.2f, 0.08f}, {-0.2f, -0.225f, 0.15f});
  makeBox({0.08f, 0.05f, 0.08f}, {-0.2f, -0.325f, 0.15f},
          Color(1));  // white paw

  // back right leg/paw
  makeBox({0.08f, 0.2f, 0.08f}, {-0.2f, -0.125f, -0.15f});
  makeBox({0.08f, 0.05f, 0.08f}, {-0.2f, -0.225f, -0.15f},
          Color(1));  // white paw

  // ear left
  makeBox({0.05f, 0.08f, 0.05f}, {0.52f, 0.18f, 0.1f});

  // ear right
  makeBox({0.05f, 0.08f, 0.05f}, {0.52f, 0.18f, -0.1f});

  // left eye
  makeBox({0.04f, 0.09f, 0.04f}, {0.60f, 0.12f, 0.06f}, Color(0));

  // right eye
  makeBox({0.04f, 0.09f, 0.04f}, {0.60f, 0.12f, -0.06f}, Color(0));

  cat.generateNormals();
  return cat;
}

struct AlloApp : App {
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  Parameter attractionFactor{"/attraction to cat", "", 0.0, 0.01, 0.6};
  Parameter repulsionFactor{"/revolution treatment", "", 0.0, 0.0, 2.6};

  Light light;
  Material material;  // Necessary for specular highlights

  Mesh catMesh;
  Nav catNav;

  Mesh floor;

  Nav cameraNav;
  int cameraMode = 0;   // 0 = default, 1 = cat, 2 = flea
  int trackedFlea = 0;  // index of flea to follow

  // size, color, species, sex, age, etc.
  std::vector<Nav> fleas;
  std::vector<float> fleaSize;
  std::vector<int> fleaTarget;

  void onInit() override {
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(timeStep);
    gui.add(attractionFactor);
    gui.add(repulsionFactor);
  }

  void onCreate() override {
    addSurface(floor, 100, 100);
    for (int i = 0; i < floor.vertices().size(); ++i) {
      floor.color(0.1, 0.7, 0.1);
    }
    floor.generateNormals();

    nav().pos(0, 0, 10);
    // addSphere(mesh);
    // mesh.translate(0, 0, -0.1);
    catMesh = createCatMesh();
    catMesh.scale(1, 1, 1.3);

    catMesh.generateNormals();

    Vec3f sum;
    for (auto& vert : catMesh.vertices()) {
      sum += vert;
    }
    sum /= catMesh.vertices().size();
    cout << sum << endl;



    light.pos(0, 10, 10);

    catNav.pos(Vec3f(0));

    for (int i = 0; i < 100; ++i) {
      Nav f;
      f.pos(randomVec3f(220.5));  // start far away
      fleas.push_back(f);
      fleaSize.push_back(rnd::uniform(0.008, 0.01));
      fleaTarget.push_back(rnd::uniform(catMesh.vertices().size()));
    }

    nav().pos(0, 0, 5);
  }

  Vec3f owner;
  double time = 0;

  bool paused = true;

  void onAnimate(double dt) override {
    if (paused) return;

    if (cameraMode == 1) {
      float distance = (catNav.pos() - nav().pos()).mag();
      if (distance < 1) {
        nav().moveF(-0.01);
      }
      else if (distance > 3) {
        nav().moveF(0.01);
      }
      nav().faceToward(catNav.pos(), 0.1);
    } else if (cameraMode == 2 && fleas.size() > 0) {
      // follow a flea
      cameraNav.pos(fleas[trackedFlea].pos() + fleas[trackedFlea].uf() * -0.5 + Vec3f(0, 0, 2));
      cameraNav.faceToward(fleas[trackedFlea].pos(), 1.0);
    }

    if (time > 7) {
      time -= 7;
      owner = Vec3f(rnd::uniformS(10), 0, rnd::uniformS(10));
    }
    time += dt;

    catNav.faceToward(owner, Vec3f(0, 1, 0), 0.02);
    catNav.moveF(0.5);
    catNav.step(dt);

    for (int i = 0; i < fleas.size(); ++i) {
      if (fleaTarget[i] >= 0) continue;  // skip flea if has partner

      float closestDist = 200;
      int closestIdx = -1;

      for (int j = 0; j < fleas.size(); ++j) {
        if (i == j) continue;  //  skip if comparing the flea to itself
        if (fleaTarget[j] >= 0) continue;  // skip if flea j is has pair

        float dist = (fleas[i].pos() - fleas[j].pos()).mag();

        if (dist < closestDist) {
          closestDist = dist;
          closestIdx = j;
        }
      }

      if (closestIdx != -1 && closestDist < 5.0f) {
        fleaTarget[i] = closestIdx;
        fleaTarget[closestIdx] = i;
      }
    }

    // pair w/ flea friend
    for (int i = 0; i < fleas.size(); ++i) {
      if (fleaTarget[i] >= 0) {
        fleas[i].faceToward(fleas[fleaTarget[i]].pos(), 0.1);
        fleas[i].nudgeToward(fleas[fleaTarget[i]].pos(), 0.05);

        // randomly break up pairs
        if (rnd::uniform() < 0.0005f) fleaTarget[i] = -1;
      }

      Vec3f catPos = catNav.pos();
      Vec3f fleaPos = fleas[i].pos();
      Vec3f toCat = catPos - fleaPos;
      float distance = toCat.mag();

      if (repulsionFactor > 0 && distance < repulsionFactor) {
        Vec3f correctedPos = catPos + toCat.normalize() * repulsionFactor;
        fleas[i].pos(correctedPos);
      }

      fleas[i].faceToward(catPos, 0.1);
      fleas[i].nudgeToward(catPos, attractionFactor);
      fleas[i].moveF(0.02);
      fleas[i].step(dt);
    }
 

    if (cameraMode == 2 && fleas.size() > 0) {
      cameraNav.pos(fleas[trackedFlea].pos() + fleas[trackedFlea].uf() * -0.5 +
                    Vec3f(0, 0, 2));
      cameraNav.faceToward(fleas[trackedFlea].pos(), 1.0);
    }
  }

  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      paused = !paused;
    }
    if (k.key() == '1') {
      cameraMode = 1;  // cat
    }
    if (k.key() == '2') {
      cameraMode = 2;  // flea
    }
    if (k.key() == '0') {
      cameraMode = 0;  //  freedom
    }
  }

  void onDraw(Graphics &g) override {

    g.clear(255, 255, 255);
    g.depthTesting(true);
    g.lighting(true);
    light.globalAmbient(RGB(0.3));
    light.ambient(RGB(0));
    light.diffuse(RGB(1, 1, 0.5));
    g.light(light);
    material.specular(light.diffuse() * 0.2);
    material.shininess(10);
    g.material(material);

    g.pushMatrix();
    g.translate(catNav.pos());
    g.rotate(catNav.quat());
    g.rotate(-90, 0, 1, 0);
    g.meshColor();
    g.draw(catMesh);
    g.popMatrix();

    Mesh m;
    addSphere(m, 0.1);
    m.generateNormals();
    g.pushMatrix();
    g.translate(owner);
    g.color(1, 0, 0);  // red color
    g.draw(m);
    g.popMatrix();

    for (int i = 0; i < fleas.size(); ++i) {
      g.pushMatrix();
      g.translate(fleas[i].pos());
      g.scale(fleaSize[i]);
      Mesh m;
      addSphere(m);
      m.generateNormals();
      g.color(0, 0, 0);  // red flea
      g.draw(m);
      g.popMatrix();
    }

    g.meshColor();
    g.translate(0, -0.5, 0);
    g.rotate(90, 1, 0, 0);
    g.scale(100);
    g.draw(floor);
  }
};

int main() {
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}