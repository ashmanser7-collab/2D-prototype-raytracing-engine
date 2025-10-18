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
float ray_degredation = 0.0f;
float ray_speed = 8; // lifetime = ray speed / ray degredation
float scope = 1;
int num_threads = 14;

struct gravitationalBody {
    float x;
    float y;
    float radius;
    float gravity;
    gravitationalBody(int sx, int sy, int s, float g) : x(sx), y(sy), radius(s), gravity(g) {}
};
struct Circle {
    float x;
    float y;
    float radius;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float reflectivity = 1.0f;
    Circle(int sx, int sy, int s) : x(sx), y(sy), radius(s) {}
};
struct Quadrilateral {
    int x;
    int y;
    int width;
    int height;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float reflectivity = 1.0f;
    Quadrilateral(int sx, int sy, int w, int h) : x(sx), y(sy), width(w), height(h) {}
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
    float done = 0;
    float r = 0;
    float g = 0;
    float b = 0;
    std::vector<point> path;
    Ray(float sx, float sy, float sa) : x(sx), y(sy), angle(sa) {path.push_back(point(x, y)); path.push_back(point(x+cosf(angle), y+sinf(angle)));}
    Ray() {}
    void runRay(int num_circles, const std::vector<Circle>& circles, int num_quads, const std::vector<Quadrilateral>& quadrilaterals, int num_gravs, std::vector<gravitationalBody> gravs) {
        while (done < 1) {
            step(num_circles, circles, num_quads, quadrilaterals, num_gravs, gravs);
        }
    }
    void step(int num_circles, std::vector<Circle> circles, int num_quads, std::vector<Quadrilateral> quadrilaterals, int num_gravs, std::vector<gravitationalBody> gravs) {
        if (done < 1) {
            move();
            checkCollisions(num_circles, circles, num_quads, quadrilaterals, num_gravs, gravs);
            done += ray_degredation;
            path.push_back(point(x, y));
            if (!inBounds()) {
                done = 1.0f;
            }
        }
    }

    private:
    void move() {
        x += cosf(angle)*ray_speed;
        y += sinf(angle)*ray_speed;
    }
    void checkCollisions(int num_circles, std::vector<Circle> circles, int num_quads, std::vector<Quadrilateral> quadrilaterals, int num_gravs, std::vector<gravitationalBody> gravs) {
        radialCollision(circles, num_circles);
        quadrilateralCollision(quadrilaterals, num_quads);
        gravitationalBodyCollision(gravs, num_gravs);
    }
    void gravitationalBodyCollision(std::vector<gravitationalBody> objects, int size) {
        float new_angle = angle;
        for (int i = 0; i < size; i++) {
            float dx = objects[i].x-x;
            float dy = objects[i].y-y;
            float dist = hypot(dx, dy);
            if (dist < objects[i].radius) {
                r = 0;
                g = 0;
                b = 0;
                done = 1;
                break;
            }

            float target_angle = atan2(dy, dx);

            // Compute angular difference in range [-PI, PI]
            float diff = target_angle - angle;
            while (diff > PI) diff -= 2 * PI;
            while (diff < -PI) diff += 2 * PI;

            // Gravity "pull" — stronger when closer
            float pull = objects[i].gravity / (dist + 1.0f);

            // Apply a small fraction of that difference per step
            new_angle += diff * pull;
        }
        angle = new_angle;
    }
    void radialCollision(std::vector<Circle> objects, int size) {
        for (int i = 0; i < size; i++) {
            float dx = objects[i].x-x;
            float dy = objects[i].y-y;
            float dist = hypot(dx, dy);
            if (dist < objects[i].radius) { // Not real reflection physics, just pointing away

                float ref = objects[i].reflectivity;
                float new_done = done + ref;
                if (new_done > 1) {ref = 0.1f-done;}
                done += new_done;

                r += ref*objects[i].r;
                g += ref*objects[i].g;
                b += ref*objects[i].b;
                if (done >= 1) {
                    break;
                }

                angle += PI;
                move();

                ray_speed /= 50;
                angle += PI;

                dx = objects[i].x-x;
                dy = objects[i].y-y;
                dist = hypot(dx, dy);
                while (dist > objects[i].radius) {
                    move();
                    dx = objects[i].x-x;
                    dy = objects[i].y-y;
                    dist = hypot(dx, dy);
                }
                ray_speed *= 50;

                float normal = atan2(dy, dx);
                angle = angle + 2*(normal-angle) + PI;

                move();
                break;
            }
        }
    }
    void quadrilateralCollision(std::vector<Quadrilateral> objects, int size) {
        return;
        for (int i = 0; i < size; i++) {
            if (x < objects[i].x + objects[i].width && y < objects[i].y + objects[i].height && x > objects[i].x && y > objects[i].y) {
                float ref = objects[i].reflectivity;
                float new_done = done + ref;
                if (new_done > 1) {
                    ref = 0.1f-done;
                }
                done += new_done;
                r += ref*objects[i].r;
                g += ref*objects[i].g;
                b += ref*objects[i].b;
                move();
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

void drawBlock(Quadrilateral quad) {
    glBegin(GL_QUADS);
    glVertex2f(quad.x, quad.y);
    glVertex2f(quad.x+quad.width, quad.y);
    glVertex2f(quad.x+quad.width, quad.y+quad.height);
    glVertex2f(quad.x, quad.y+quad.height);
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
    std::vector<Quadrilateral> quads;
    std::vector<gravitationalBody> gravs;
    World(int cx, int cy, float angle, std::vector<Circle> circs, std::vector<Quadrilateral> quadrilaterals) : viewport(camera(cx, cy, angle)), circles(circs), quads(quadrilaterals) {}
    void initializeRays(int size, Ray rays[]) {
        float angle = viewport.angle + scope/2;
        for (int i = 0; i < size; i++) {
            rays[i] = Ray(viewport.x, viewport.y, angle);
            angle -= angle_difference;
        }
    }
    void runRays(Ray rays[], int size) {
        for (int i = 0; i < size; i++) {
            rays[i].runRay(circles.size(), circles, quads.size(), quads, gravs.size(), gravs);
        }
    }
    int animateRays(Ray rays[], int size) {
        int tally = 0;
        for (int i = 0; i < size; i++) {
            rays[i].step(circles.size(), circles, quads.size(), quads, gravs.size(), gravs);
            if (rays[i].done >= 1) {
                tally++;
            }
        }
        return tally;
    }
    void renderWorld() {
        for (int i = 0; i < circles.size(); i++) {
            glColor4f(circles[i].r, circles[i].g, circles[i].b, 1.0f);
            drawCircle(circles[i].x, circles[i].y, circles[i].radius, 50);
        }
        for (int i = 0; i < quads.size(); i++) {
            glColor4f(quads[i].r, quads[i].g, quads[i].b, 1.0f);
            drawBlock(quads[i]);
        }
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        for (int i = 0; i < gravs.size(); i++) {
            drawCircle(gravs[i].x, gravs[i].y, gravs[i].radius, 50);
        }
    }

    void edit(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;

        for (int i = 0; i < circles.size(); i++) {
            double dist = hypot(circles[i].x-xpos, circles[i].y-ypos);
            if (dist < circles[i].radius) {
                std::cout << "Colour: (" << circles[i].r << ", " << circles[i].g << ", " << circles[i].b << ", " << circles[i].reflectivity << ")" << std::endl;
                std::cout << "Radius: " << circles[i].radius << std::endl;
                std::cout << "Position: (" << circles[i].x << ", " << circles[i].y << ")" << std::endl << std::endl;
                int action;
                std::cin >> action;
                if (action == 0) {
                    std::cout << "New red: ";
                    std::cin >> circles[i].r;
                    std::cout << "New green: ";
                    std::cin >> circles[i].g;
                    std::cout << "New blue: ";
                    std::cin >> circles[i].b;
                    std::cout << "New reflictivity: ";
                    std::cin >> circles[i].reflectivity;
                }
                if (action == 1) {
                    std::cout << "New Radius: ";
                    std::cin >> circles[i].radius;
                }
            }
        }

        for (int i = 0; i < gravs.size(); i++) {
            double dist = hypot(gravs[i].x-xpos, gravs[i].y-ypos);
            if (dist < gravs[i].radius) {
                std::cout << "Gravity: " << gravs[i].gravity << std::endl;
                std::cout << "Radius: " << gravs[i].radius << std::endl;
                std::cout << "Position: (" << gravs[i].x << ", " << gravs[i].y << ")" << std::endl << std::endl;
                int action;
                std::cin >> action;
                if (action == 0) {
                    std::cout << "New Gravity: ";
                    std::cin >> gravs[i].gravity;
                }
                if (action == 1) {
                    std::cout << "New Radius: ";
                    std::cin >> gravs[i].radius;
                }
            }
        }

        for (int i = 0; i < quads.size(); i++) {
            if (xpos > quads[i].x && xpos < quads[i].x + quads[i].width && ypos > quads[i].y && ypos < quads[i].y + quads[i].height) {
                std::cout << "Colour: (" << quads[i].r << ", " << quads[i].g << ", " << quads[i].b << ", " << quads[i].reflectivity << ")" << std::endl;
                std::cout << "Width: " << quads[i].width << std::endl;
                std::cout << "Height: " << quads[i].height << std::endl;
                std::cout << "Position: (" << quads[i].x << ", " << quads[i].y << ")" << std::endl << std::endl;
                int action;
                std::cin >> action;
                if (action == 0) {
                    std::cout << "New red: ";
                    std::cin >> quads[i].r;
                    std::cout << "New green: ";
                    std::cin >> quads[i].g;
                    std::cout << "New blue: ";
                    std::cin >> quads[i].b;
                    std::cout << "New reflictivity: ";
                    std::cin >> quads[i].reflectivity;
                }
                if (action == 1) {
                    std::cout << "New width: ";
                    std::cin >> quads[i].width;
                }
                if (action == 2) {
                    std::cout << "New height: ";
                    std::cin >> quads[i].height;
                }
            }
        }
    }
    void remove(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;
        for (int i = 0; i < circles.size(); i++) {
            double dist = hypot(circles[i].x-xpos, circles[i].y-ypos);
            if (dist < circles[i].radius) {
                circles.erase(circles.begin() + i);
            }
        }
        for (int i = 0; i < gravs.size(); i++) {
            double dist = hypot(gravs[i].x-xpos, gravs[i].y-ypos);
            if (dist < gravs[i].radius) {
                gravs.erase(gravs.begin() + i);
            }
        }
        for (int i = 0; i < quads.size(); i++) {
            if (xpos > quads[i].x && xpos < quads[i].x + quads[i].width && ypos > quads[i].y && ypos < quads[i].y + quads[i].height) {
                quads.erase(quads.begin() + i);
            }
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
    void addGrav(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        double x;
        double y;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;
        gravs.push_back(gravitationalBody(xpos, ypos, 0, 0));
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwGetCursorPos(window, &x, &y);
            y = height - y;
            gravs[gravs.size()-1].radius = int(hypot(x-xpos, y-ypos));
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
            renderWorld();
            initializeRays(num_rays, rays);
            runRays(rays, num_rays);
            renderRays(rays, num_rays);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        std::cout << "Gravity: ";
        std::cin >> gravs[gravs.size()-1].gravity;
    }
    void addQuad(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        double x;
        double y;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;
        quads.push_back(Quadrilateral(xpos, ypos, 0, 0));
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glfwGetCursorPos(window, &x, &y);
            y = height - y;
            quads[quads.size()-1].width = x - xpos;
            quads[quads.size()-1].height = y - ypos;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
            renderWorld();
            initializeRays(num_rays, rays);
            runRays(rays, num_rays);
            renderRays(rays, num_rays);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    void move(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        double x;
        double y;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;
        for (int i = 0; i < circles.size(); i++) {
            double dist = hypot(circles[i].x-xpos, circles[i].y-ypos);
            if (dist < circles[i].radius) {
                float dx = circles[i].x-xpos;
                float dy = circles[i].y-ypos;
                while (!glfwWindowShouldClose(window)) {
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glfwGetCursorPos(window, &x, &y);
                    y = height - y;
                    circles[i].x = x + dx;
                    circles[i].y = y + dy;
                    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
                    renderWorld();
                    initializeRays(num_rays, rays);
                    runRays(rays, num_rays);
                    renderRays(rays, num_rays);
                    glfwSwapBuffers(window);
                    glfwPollEvents();
                }
            }
        }
        for (int i = 0; i < gravs.size(); i++) {
            double dist = hypot(gravs[i].x-xpos, gravs[i].y-ypos);
            if (dist < gravs[i].radius) {
                float dx = gravs[i].x-xpos;
                float dy = gravs[i].y-ypos;
                while (!glfwWindowShouldClose(window)) {
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glfwGetCursorPos(window, &x, &y);
                    y = height - y;
                    gravs[i].x = x + dx;
                    gravs[i].y = y + dy;
                    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
                    renderWorld();
                    initializeRays(num_rays, rays);
                    runRays(rays, num_rays);
                    renderRays(rays, num_rays);
                    glfwSwapBuffers(window);
                    glfwPollEvents();
                }
            }
        }
        for (int i = 0; i < quads.size(); i++) {
            if (xpos > quads[i].x && xpos < quads[i].x + quads[i].width && ypos > quads[i].y && ypos < quads[i].y + quads[i].height) {
                float dx = circles[i].x-xpos;
                float dy = circles[i].y-ypos;
                while (!glfwWindowShouldClose(window)) {
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glfwGetCursorPos(window, &x, &y);
                    y = height - y;
                    quads[i].x = x+dx;
                    quads[i].y = y+dy;
                    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
                    renderWorld();
                    initializeRays(num_rays, rays);
                    runRays(rays, num_rays);
                    renderRays(rays, num_rays);
                    glfwSwapBuffers(window);
                    glfwPollEvents();
                }
            }
        }
    }
    void moveCamera(GLFWwindow* window, int button, int action, int mods) {
        double xpos;
        double ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            double xpos;
            double ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            ypos = height - ypos;
            viewport.x = xpos;
            viewport.y = ypos;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {break;}
            renderWorld();
            initializeRays(num_rays, rays);
            runRays(rays, num_rays);
            renderRays(rays, num_rays);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            double xpos;
            double ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            ypos = height - ypos;
            viewport.angle = atan2(ypos-viewport.y, xpos-viewport.x);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {break;}
            renderWorld();
            initializeRays(num_rays, rays);
            runRays(rays, num_rays);
            renderRays(rays, num_rays);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        while (!glfwWindowShouldClose(window)) {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            double xpos;
            double ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            ypos = height - ypos;
            scope = 2*(viewport.angle - atan2(ypos-viewport.y, xpos-viewport.x));
            angle_difference = scope/num_rays;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {break;}
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



int mouse_mode = 5;
void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (auto* env = static_cast<World*>(glfwGetWindowUserPointer(window))) {
            glfwSetMouseButtonCallback(window, NULL);
            if (mouse_mode == 0) {
                env->edit(window, button, action, mods);
            }

            if (mouse_mode == 1) {
                env->remove(window, button, action, mods);
            }

            if (mouse_mode == 2) {
                env->addGrav(window, button, action, mods);
            }

            if (mouse_mode == 3) {
                env->addCircle(window, button, action, mods);
            }

            if (mouse_mode == 4) {
                env->move(window, button, action, mods);
            }

            if (mouse_mode == 5) {
                env->moveCamera(window, button, action, mods);
            }
            glfwSetMouseButtonCallback(window, mouse_callback);
        }
    }
}



int tally=0;
bool display_mode = false;

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

    std::vector<Quadrilateral> quads;
    std::vector<Circle> circles = {Circle(width, height/2, height/4)};
    circles[0].reflectivity = 0.5;
    World environment = World(width/4, height/2, 0, circles, quads);
    environment.initializeRays(num_rays, rays);
    glfwSetWindowUserPointer(window, &environment);
    glfwSetMouseButtonCallback(window, mouse_callback);

    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {mouse_mode = 1;}
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {mouse_mode = 2;}
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {mouse_mode = 3;}
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {mouse_mode = 4;}
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {mouse_mode = 5;}
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {mouse_mode = 0;}
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {display_mode = true;}
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {display_mode = false;}

        

        if (display_mode) {
            if (tally >= num_rays) {
                tally = 0;
                environment.initializeRays(num_rays, rays);
            }
            tally = environment.animateRays(rays, num_rays);
            Sleep(5);
        } else {
            environment.initializeRays(num_rays, rays);
            environment.runRays(rays, num_rays);
        }

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