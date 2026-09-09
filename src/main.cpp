#include "cs488.h"


// setting up lighting
static PointLightSource light;
static void setupLightSource() {
    //light.position = float3(0.5f, 4.0f, 1.0f); // use this for sponza.obj
    light.position = float3(0.0f, 0.35f, 0.08f);
    light.wattage = float3(5.0f, 5.0f, 5.0f);
    globalScene.addLight(&light);
}




// loading .obj file from the command line arguments
static TriangleMesh mesh;
static void setupScene(int argc, const char* argv[]) {
    if (argc > 1) {
        bool objLoadSucceed = mesh.load(argv[1]);
        if (!objLoadSucceed) {
            printf("Invalid .obj file.\n");
            printf("Making a single triangle instead.\n");
            mesh.createSingleTriangle();
        }
    } else {
        printf("Specify .obj file in the command line arguments. Example: CS488.exe cornellbox.obj\n");
        printf("Making a single triangle instead.\n");
        mesh.createSingleTriangle();
    }
    globalScene.addObject(&mesh);
}


int main(int argc, const char* argv[]) {
    setupScene(argc, argv);
    // setupLightSource();


    // Note precalc also adds area light sources 
    globalScene.preCalc();

    // change camera stuff 
    globalEye    = float3(0.0f, 0.8f, 3.2f);
    globalLookat = float3(0.0f, 0.8f, 0.0f);

    globalViewDir = normalize(globalLookat - globalEye);
	globalRight = normalize(cross(globalViewDir, globalUp));
    const int iterations = 50;
    const int photons = 100000;
    const int depth = 20;
    const float radius = 0.02f;
    const float alpha = 0.7f;
    Integrator sppm(iterations, photons, alpha, radius, depth);
    sppm.render(globalScene, FrameBuffer);

    //globalScene.Raytrace();
    FrameBuffer.save("cornell-water-anti-aliased.png");
}
