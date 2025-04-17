#include "al/app/al_App.hpp"
#include "al/graphics/al_Image.hpp" 
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"
using namespace al;
#include <fstream> // for slurp()
#include <string> // for slurp()

Vec3f rvec() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
RGB rcolor() { return RGB(rnd::uniform(), rnd::uniform(), rnd::uniform()); }

std::string slurp(std::string fileName); // only a declaration
class MyApp : public App {
    Mesh grid, rgb, hsl, mine;
    Mesh mesh;
    Mesh imageMesh; 
    Mesh rgbCubeMesh;
    Mesh randomMesh;

    ShaderProgram shader;
    Parameter pointSize{"pointSize", 0.004, 0.0005, 0.015};

    void onInit() override {
        auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
        auto &gui = GUIdomain->newGUI();
        gui.add(pointSize);
    }

    void onCreate() override {
        auto image = Image("../rainbow.jpg");
        if (image.width() == 0) {
            std::cout << "Image not found" << std::endl;
            exit(1);
        }

        mesh.primitive(Mesh::POINTS);
        imageMesh.primitive(Mesh::POINTS); //

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                auto pixel = image.at(x, y);
                float px = float(x) / image.width();
                float py = float(y) / image.height();
                Color c(pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0);

                mesh.vertex(px, py, 0);
                mesh.color(c);
                mesh.texCoord(0.1, 0);

                imageMesh.vertex(px, py, 0);  
                imageMesh.color(c);
                imageMesh.texCoord(0.1, 0);
            }
        }

        rgbCubeMesh.primitive(Mesh::POINTS);

        for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
        auto pixel = image.at(x, y);
        float r = pixel.r / 255.0f;
        float g = pixel.g / 255.0f;
        float b = pixel.b / 255.0f;

        rgbCubeMesh.vertex(r, g, b);        
        rgbCubeMesh.color(r, g, b);          
        rgbCubeMesh.texCoord(0.1, 0);
    }
}

randomMesh.primitive(Mesh::POINTS);

for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
        auto pixel = image.at(x, y);
        float r = pixel.r / 255.0f;
        float g = pixel.g / 255.0f;
        float b = pixel.b / 255.0f;

        Vec3f randPos = rvec(); 
        randomMesh.vertex(randPos);
        randomMesh.color(r, g, b);
        randomMesh.texCoord(0.1, 0);
    }
}


        nav().pos(0, 0, 5);

        if (!shader.compile(slurp("../point-vertex.glsl"), slurp("../point-fragment.glsl"), slurp("../point-geometry.glsl"))) {
            printf("Shader failed to compile\n");
            exit(1);
        }
    }

    void onAnimate(double dt) override {
        
    }

    void onDraw(Graphics& g) override {
        g.clear(0.1);
        g.shader(shader);
        g.shader().uniform("pointSize", pointSize);
        g.blending(true);
        g.blendTrans();
        g.depthTesting(true);
        g.draw(mesh);
    }

    bool onKeyDown(const Keyboard& k) override {
        if (k.key() == 'q') {
            std::cout << "Exiting..." << std::endl;
            quit();
        }

        if (k.key() == ' ') {
            mesh.reset(); 
            for (int i = 0; i < 100; ++i) {
                mesh.vertex(rvec());
                mesh.color(rcolor());
                mesh.texCoord(0.1, 0);
            }
        }

        if (k.key() == '1') {
            mesh = imageMesh; 
        }

        if (k.key() == '2') {
            mesh = rgbCubeMesh;
        }

        if (k.key() == '4') {
            mesh = randomMesh;
        }
        
        

        return true;
    }
};

int main() { MyApp().start(); }


std::string slurp(std::string fileName) {
    std::fstream file(fileName);
    std::string returnValue = "";
    while (file.good()) {
      std::string line;
      getline(file, line);
      returnValue += line + "\n";
    }
    return returnValue;
  }
