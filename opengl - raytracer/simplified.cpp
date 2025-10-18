#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <thread>
#include <windows.h>

const double PI = 3.14159265358979323846;
long frames = 0;
const int resolution = 4;
const int width = 1000; // Better if a multiple of resolution
const int height = 1000;
bool random = true;
float ray_distance = 1000;
float ray_speed = 8; // lifetime = ray speed / ray degredation
float scope = 1;
int num_threads = 14;

struct Circle {
    float x;
    float y;
    float radius;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    Circle(int sx, int sy, int s) : x(sx), y(sy), radius(s) {}
};

struct point {
    float x;
    float y;
    point(float sx, float sy) : x(sx), y(sy) {}
};

class Ray {
    public:
    float x;
    float y;
    float angle;
    bool done = false;
    float r = 0;
    float g = 0;
    float b = 0;
    float distance_traveled = 0;
    std::vector<point> path;
    Ray(float sx, float sy, float sa) : x(sx), y(sy), angle(sa) {path.push_back(point(x, y)); path.push_back(point(x+cosf(angle), y+sinf(angle)));}
    Ray() {}
    void runRay(int num_circles, const std::vector<Circle>& circles) {
        while (!done) {
            step(num_circles, circles);
        }
    }
    void step(int num_circles, std::vector<Circle> circles) {
        if (!done) {
            move();
            distance_traveled += ray_speed;
            checkCollisions(num_circles, circles);
            path.push_back(point(x, y));
            if (!inBounds() || distance_traveled > ray_distance) {
                done = true;
            }

        }
    }

    private:
    void move() {
        x += cosf(angle)*ray_speed;
        y += sinf(angle)*ray_speed;
    }
    void checkCollisions(int num_circles, std::vector<Circle> circles) {
        radialCollision(circles, num_circles);
    }
    void radialCollision(std::vector<Circle> objects, int size) {
        for (int i = 0; i < size; i++) {
            float dx = objects[i].x-x;
            float dy = objects[i].y-y;
            float dist = hypot(dx, dy);
            if (dist < objects[i].radius) { // Not real reflection physics, just pointing away
                r = objects[i].r;
                g = objects[i].g;
                b = objects[i].b;
                done = true;
                break;
            }
        }
    }
    bool inBounds() {
        if (x < 0 || x > width || y < 0 || y > height) {
            return false;
        }
        return true;
    }
};

const int num_rays = int(ceil(width/resolution));
float angle_difference = scope/num_rays;
Ray rays[num_rays];

void drawCircle(float cx, float cy, float r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * PI * float(i) / float(num_segments);
        float x = r * std::cos(theta);
        float y = r * std::sin(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

struct camera {
    int x;
    int y;
    float angle;
    camera(float sx, float sy, float sa) : x(sx), y(sy), angle(sa) {}
};

void renderRays(Ray rays[], int size) {
    glColor4f(0.6f, 0.6f, 0.6f, 0.5f);
    glBegin(GL_LINES);

    for (int j = 0; j < size; j++) {
        for (int i = 1; i < rays[j].path.size()-1; i++) {
            glVertex2f(rays[j].path[i].x, rays[j].path[i].y);
            glVertex2f(rays[j].path[i+1].x, rays[j].path[i+1].y);
        }
    }

    glEnd();
}

void drawPath(std::vector<point> path) {
    glColor4f(0.6f, 0.6f, 0.6f, 0.5f);
    glBegin(GL_LINES);

    for (int i = 1; i < path.size()-1; i++) {
        glVertex2f(path[i].x, path[i].y);
        glVertex2f(path[i+1].x, path[i+1].y);
    }

    glEnd();
}

class World {
    public:
    camera viewport;
    std::vector<Circle> circles;
    World(int cx, int cy, float angle, std::vector<Circle> circs) : viewport(camera(cx, cy, angle)), circles(circs) {}
    void initializeRays(int size, Ray rays[]) {
        float angle = viewport.angle + scope/2;
        for (int i = 0; i < size; i++) {
            rays[i] = Ray(viewport.x, viewport.y, angle);
            angle -= angle_difference;
        }
    }
    void runRays(Ray rays[], int size) {
        for (int i = 0; i < size; i++) {
            rays[i].runRay(circles.size(), circles);
        }
    }
    void renderWorld() {
        for (int i = 0; i < circles.size(); i++) {
            glColor4f(circles[i].r, circles[i].g, circles[i].b, 1.0f);
            drawCircle(circles[i].x, circles[i].y, circles[i].radius, 50);
        }
    }
    void addCircle(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        double x;
        double y;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;
        circles.push_back(Circle(xpos, ypos, 0));
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwGetCursorPos(window, &x, &y);
            y = height - y;
            circles[circles.size()-1].radius = int(hypot(x-xpos, y-ypos));
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
            renderWorld();
            initializeRays(num_rays, rays);
            runRays(rays, num_rays);
            renderRays(rays, num_rays);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
};

void drawVision(Ray rays[], int size) {
    glBegin(GL_POINTS);

    for (int j = 0; j < size; j++) {
        glColor3f(rays[j].r, rays[j].g, rays[j].b);
        glVertex2f(resolution/2+resolution*j, resolution/2);
    }

    glEnd();
}

GLFWwindow* initiate_window() {
    GLFWwindow* window = glfwCreateWindow(width, height, "2D raytracer", NULL, NULL);
    return window;
}



int main() {

    if (random) {
        srand(std::time(0));
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = initiate_window();

    glfwMakeContextCurrent(window);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPointSize(resolution);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetMouseButtonCallback(window, NULL);
    glfwSetWindowPos(window, 100, 75);

    std::vector<Circle> circles;
    World environment = World(width/4, height/2, 0, circles);
    environment.initializeRays(num_rays, rays);
    glfwSetWindowUserPointer(window, &environment);
    glfwSetMouseButtonCallback(window, NULL);

    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        environment.initializeRays(num_rays, rays);

        environment.runRays(rays, num_rays);

        renderRays(rays, num_rays);
        
        drawVision(rays, num_rays);

        environment.renderWorld();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}