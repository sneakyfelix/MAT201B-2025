#include "al/app/al_App.hpp"
#include "al/graphics/al_Image.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

#include <fstream>
#include <string>
#include <map>
#include <sstream>

using namespace al;


Vec3f randVec3() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
RGB randColor() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }



// used a llm for this --> 

Vec3f hsvToCylindrical(float h, float s, float v) {
    float theta = h * M_2PI;
    return Vec3f(cos(theta) * s, v, sin(theta) * s);
}

std::string slurp(const std::string& filename) {  // only a declaration
    std::ifstream f(filename);
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
}

// data container for layouts
struct Layout {
    std::vector<Vec3f> positions;
    std::vector<Color> colors;
};

class MyApp : public App {
    Mesh displayMesh;
    ShaderProgram shader;
    Parameter pointSize{"pointSize", 0.004, 0.0005, 0.015};

    Layout currentLayout, nextLayout;
    std::vector<Vec3f> interpolated;

    float transitionTime = 1.0;
    float elapsed = 0.0;
    bool transitioning = false;

    std::map<std::string, Layout> layouts;

    void onInit() override {
        auto gui = GUIDomain::enableGUI(defaultWindowDomain())->newGUI();
        gui.add(pointSize);  // add parameter to GUI
    }

    void loadLayouts(const Image& img) {
        Layout imageLayout, rgbLayout, hsvLayout, myLayout;

        int w = img.width();
        int h = img.height();

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                auto px = img.at(x, y);
                float r = px.r / 255.0f;
                float g = px.g / 255.0f;
                float b = px.b / 255.0f;

                Vec3f posNorm = Vec3f(float(x) / w, float(y) / h, 0.0f);
                Color color(r, g, b);
                HSV hsv(color);

                imageLayout.positions.push_back(posNorm);
                rgbLayout.positions.push_back(Vec3f(r, g, b));
                hsvLayout.positions.push_back(hsvToCylindrical(hsv.h, hsv.s, hsv.v));
                myLayout.positions.push_back(randVec3());

                imageLayout.colors.push_back(color);
                rgbLayout.colors.push_back(color);
                hsvLayout.colors.push_back(color);
                myLayout.colors.push_back(color);
            }
        }

        // sets up mesh and initial data
        currentLayout = imageLayout;
        nextLayout = imageLayout;
        interpolated = imageLayout.positions;

        displayMesh.primitive(Mesh::POINTS);
        for (int i = 0; i < interpolated.size(); ++i) {
            displayMesh.vertex(interpolated[i]);
            displayMesh.color(currentLayout.colors[i]);
            displayMesh.texCoord(0.1, 0);
        }

        // stores layouts
        layouts["image"] = imageLayout;
        layouts["rgb"] = rgbLayout;
        layouts["hsv"] = hsvLayout;
        layouts["mine"] = myLayout;
    }

    void startTransition(const std::string& name) {
        if (!layouts.count(name)) return;

        nextLayout = layouts[name];

        for (int i = 0; i < nextLayout.colors.size(); ++i) {
            displayMesh.colors()[i] = nextLayout.colors[i];
        }

        elapsed = 0.0;
        transitioning = true;
    }

    void onCreate() override {
        Image img("../rainbow.jpg");
        if (img.width() == 0) {
            std::cerr << "Image failed to load.\n";
            exit(1);
        }

        loadLayouts(img);
        nav().pos(0, 0, 5);

        if (!shader.compile(slurp("../point-vertex.glsl"),
                            slurp("../point-fragment.glsl"),
                            slurp("../point-geometry.glsl"))) {
            std::cerr << "Shader failed to compile.\n";
            exit(1);
        }
    }

    void onAnimate(double dt) override {
        if (!transitioning) return;

        elapsed += dt;
        float t = elapsed / transitionTime;
        if (t >= 1.0f) {
            t = 1.0f;
            transitioning = false;
            currentLayout = nextLayout;
        }

        for (int i = 0; i < interpolated.size(); ++i) {
            interpolated[i] = lerp(currentLayout.positions[i], nextLayout.positions[i], t);
            displayMesh.vertices()[i] = interpolated[i];
        }
    }

    void onDraw(Graphics& g) override {
        g.clear(0.1);
        g.shader(shader);
        g.shader().uniform("pointSize", pointSize);
        g.blending(true);
        g.blendTrans();
        g.depthTesting(true);
        g.draw(displayMesh);
    }

    bool onKeyDown(const Keyboard& k) override {
        if (k.key() == 'q') {  
          std::cout << "Exiting..." << std::endl; 
          quit(); 
        }
        if (k.key() == '1') { 
          startTransition("image");
         }
        if (k.key() == '2') { 
          startTransition("rgb"); 
        }
        if (k.key() == '3') {
          startTransition("hsv");
        }
        if (k.key() == '4') {
          startTransition("mine");
        }

        if (k.key() == ' ') {
            displayMesh.reset();
            interpolated.clear();
            currentLayout.positions.clear();
            currentLayout.colors.clear();
            for (int i = 0; i < 100; ++i) {
                Vec3f pos = randVec3();
                Color c = randColor();
                displayMesh.vertex(pos);
                displayMesh.color(c);
                displayMesh.texCoord(0.1, 0);
                interpolated.push_back(pos);
                currentLayout.positions.push_back(pos);
                currentLayout.colors.push_back(c);
            }
        }
        return true;
    }
};

int main() {
    MyApp().start();
}

