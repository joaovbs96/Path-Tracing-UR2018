
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "brdf/glass.hpp"
#include "brdf/lambertian.hpp"
#include "brdf/metal.hpp"
#include "geometry/geometry_list.hpp"
#include "geometry/sphere.hpp"
#include "sampling/rng.hpp"
#include "sampling/sampling.hpp"
#include "utils/camera.hpp"
#include "utils/framebuffer.hpp"
#include "utils/ray.hpp"
#include "utils/scenes.hpp"

float3 trace(Ray& r, Geometry* scene, uint& seed) {
  float3 throughput = float3(1.0f);

  // iterative version of recursion
  for (int depth = 0; depth < 50; depth++) {
    Hit_Record rec;

    // ray hit
    if (scene->hit(r, rec, 0.001f, FLT_MAX)) {
      // gets next direction
      float3 wi = rec.brdf->sample(rec.p, -r.direction(), rec.normal, seed);

      // accumulates color
      float pdf;
      float3 color = rec.brdf->eval(rec.p, -r.direction(), wi, rec.normal, pdf);
      throughput *= color;

      // updates ray
      r = Ray(rec.p, wi);
    }

    // ray missed, samples background
    else {
      float t = 0.5f * (r.direction().y + 1.0f);
      float3 color = (1.0f - t) * float3(1.0f) + t * float3(0.5f, 0.7f, 1.0f);

      return throughput * color;  // return accumulated color
    }
  }

  // ray reached max depth: cancel recursion
  return float3(0.0f);
}

int main() {
  const int samples = 100;
  stbi_flip_vertically_on_write(1);
  unsigned int seed = tea<64>(2, 5);
  const int width = 200, height = 100;

  Framebuffer fb(width, height);
  Camera cam;
  Geometry_List scene;

  In_One_Weekend(scene, cam, width, height, seed);

  auto t0 = std::chrono::system_clock::now();

  for (int r = 0; r < fb.height(); r++) {
    for (int c = 0; c < fb.width(); c++) {
      float3 color;
      for (int s = 0; s < samples; s++) {
        float u = float(c + rnd(seed)) / float(fb.width());
        float v = float(r + rnd(seed)) / float(fb.height());

        Ray ray = cam.make_ray(u, v, seed);
        color += trace(ray, &scene, seed);
      }

      fb.set_pixel(r, c, sqrt(color / float(samples)));
    }
    printf("Progress: %.2f \r", 100 * (float(r) / height));
  }
  printf("\n");

  fb.save("output.png");

  auto t1 = std::chrono::system_clock::now();
  auto time = std::chrono::duration<float>(t1 - t0).count();
  printf("Rendering time: %2.fs\n", float(time));

  system("pause");
  return 0;
}