#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <ctime>

const double PI = 3.14159265358979323846;
long frames = 0;
const int width = 1850; // Better if a multiple of resolution
const int height = 1000;
bool random = true;
float ray_distance = 200;
float ray_speed = 15; // lifetime = ray speed / ray degredation
float scope = PI;
const int num_rays = 100;
const int resolution = width/num_rays;
float angle_difference = scope/num_rays;

struct Circle {
    float x;
    float y;
    float vx = float(rand()%100)/50 - 1;
    float vy = float(rand()%100)/50 - 1;
    float radius = rand()%40 + 20;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    Circle(int sx, int sy) : x(sx), y(sy) {}
};

struct point {
    float x;
    float y;
    point(float sx, float sy) : x(sx), y(sy) {}
};

struct Move {
    float acceleration;
    float turning;
};

bool ray_wrapping = true;

class Ray {
    public:
    float x;
    float y;
    float angle;
    bool done = false;
    bool collided = false;
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
            if (distance_traveled > ray_distance) {
                done = true;
            }

        }
    }

    private:
    void move() {
        x += cosf(angle)*ray_speed;
        y += sinf(angle)*ray_speed;
        inBounds();
    }
    void checkCollisions(int num_circles, std::vector<Circle> circles) {
        radialCollision(circles, num_circles);
    }
    void radialCollision(std::vector<Circle> objects, int size) {
        for (int i = 0; i < size; i++) {
            float dx = objects[i].x-x;
            float dy = objects[i].y-y;
            float dist = hypot(dx, dy);
            if (dist < objects[i].radius) {
                done = true;
                collided = true;
                break;
            }
        }
    }
    void inBounds() {
        if (ray_wrapping) {
            if (x < 0) {
                x += width;
            }
            if (x > width) {
                x -= width;
            }
            if (y < 0) {
                y += height;
            }
            if (y > height) {
                y -= height;
            }
        } else {
            if (x < 0) {
                done = true;
                collided = true;
            }
            if (x > width) {
                done = true;
                collided = true;
            }
            if (y < 0) {
                done = true;
                collided = true;
            }
            if (y > height) {
                done = true;
                collided = true;
            }
        }
    }
};

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

class Agent {
    public:
    int player_size = 10;
    float x = width/3;
    float y = height/2;
    float velocity = 1;
    float angle = 0;
    bool dead = false;
    Agent() {}
    Agent(float sx, float sy, float sa) : x(sx), y(sy), angle(sa) {}
    void step(Move action, std::vector<Circle> objects) {
        processAction(action);
        move();
        radialCollision(objects);
    }
    void move() {
        x += cosf(angle)*velocity;
        y += sinf(angle)*velocity;
        inBounds();
    }
    void processAction(Move action) {
        action.turning = std::clamp(action.turning, float(-PI/10), float(PI/10));
        velocity += action.acceleration;
        angle += action.turning;
        if (velocity < 0) {
            velocity *= -1;
            angle += PI;
        }
        velocity = std::clamp(velocity, 0.0f, 2.0f);
    }
    void radialCollision(std::vector<Circle> objects) {
        for (int i = 0; i < objects.size(); i++) {
            float dx = objects[i].x-x;
            float dy = objects[i].y-y;
            float dist = hypot(dx, dy);
            if (dist < objects[i].radius+player_size) {
                dead = true;
                break;
            }
        }
    }
    void inBounds() {
        if (x < 0) {x += width;}
        if (x > width) {x -= width;}
        if (y < 0) {y += height;}
        if (y > height) {y -= height;}
    }
};

void renderRays(Ray rays[], int size) {
    glColor4f(0.6f, 0.6f, 0.6f, 0.5f);
    glBegin(GL_LINES);

    for (int j = 0; j < size; j++) {
        for (int i = 1; i < rays[j].path.size()-1; i++) {
            if (rays[j].path[i].x - rays[j].path[i+1].x > ray_speed || rays[j].path[i+1].x - rays[j].path[i].x > ray_speed || rays[j].path[i].y - rays[j].path[i+1].y > ray_speed || rays[j].path[i+1].y - rays[j].path[i].y > ray_speed) {continue;}
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
        if (path[i].x - path[i+1].x > ray_speed || path[i+1].x - path[i].x > ray_speed || path[i].y - path[i+1].y > ray_speed || path[i+1].y - path[i].y > ray_speed) {continue;}
        glVertex2f(path[i].x, path[i].y);
        glVertex2f(path[i+1].x, path[i+1].y);
    }

    glEnd();
}

class World {
    public:
    Agent character;
    std::vector<Circle> circles;
    int num_objects;
    World(int num_objs) : num_objects(num_objs) {reset();}

    void step(int num_rays, Ray rays[], Move action) {
        initializeRays(num_rays, rays);
        runRays(rays, num_rays);
        if (character.dead) {
            reset();
            step(num_rays, rays, action);
            return;
        }
        for (int i = 0; i < circles.size(); i++) {
            circles[i].x += circles[i].vx;
            circles[i].y += circles[i].vy;
            if (circles[i].x-circles[i].radius > width) {circles[i].x = -circles[i].radius;}
            if (circles[i].y-circles[i].radius > height) {circles[i].y = -circles[i].radius;}
            if (circles[i].x+circles[i].radius < 0) {circles[i].x = width+circles[i].radius;}
            if (circles[i].y+circles[i].radius < 0) {circles[i].y = height+circles[i].radius;}
        }
        character.step(action, circles);
    }
    void renderWorld() {
        for (int i = 0; i < circles.size(); i++) {
            glColor4f(circles[i].r, circles[i].g, circles[i].b, 1.0f);
            drawCircle(circles[i].x, circles[i].y, circles[i].radius, 50);
        }
        glColor4f(0.4f, 0.6f, 0.6f, 1.0f);
        drawCircle(character.x, character.y, character.player_size, 50);
    }
    private:
    void initializeRays(int size, Ray rays[]) {
        float angle =character.angle + scope/2;
        for (int i = 0; i < size; i++) {
            rays[i] = Ray(character.x,character.y, angle);
            angle -= angle_difference;
        }
    }
    void runRays(Ray rays[], int size) {
        character.move();
        for (int i = 0; i < size; i++) {
            rays[i].runRay(circles.size(), circles);
        }
    }
    void reset() {
        circles.clear();
        float angle = (rand()%int(PI*2*1000))/1000 - PI;
        float x = rand()%width;
        float y = rand()%height;
        character = Agent(x, y, angle);

        for (int i = 0; i < num_objects; i++) {
            addCircle();
        }
    }
    void addCircle() {
        float x = rand()%width;
        float y = rand()%height;
        float dist = hypot(character.x-x, character.y-y);
        while (dist < character.player_size+ray_distance) {
            x = rand()%width;
            y = rand()%height;
            dist = hypot(character.x-x, character.y-y);
        }
        circles.push_back(Circle(x, y));
    }
};

void drawVision(Ray rays[], int size) {
    glBegin(GL_POINTS);

    for (int j = 0; j < size; j++) {
        glColor3f(rays[j].collided, rays[j].collided, rays[j].collided);
        glVertex2f(resolution/2+resolution*j, resolution/2);
    }

    glEnd();
}

GLFWwindow* initiate_window() {
    GLFWwindow* window = glfwCreateWindow(width, height, "2D raytracer", NULL, NULL);
    return window;
}

Move getMoveAvoidance(Ray rays[], int size) {
    float right = 0;
    float left = 0;
    float middle = 0;
    float third_size = size/3;
    for (int i = 0; i < size; i++) {
        if (i < third_size) {
            left += rays[i].collided;
        } else if (i > third_size*2) {
            right += rays[i].collided;
        } else {
            middle += rays[i].collided;
        }
    }
    left /= floor(third_size);
    middle /= floor(third_size);
    right /= floor(third_size);
    Move action;
    action.turning = std::clamp(right-left, -0.05f, 0.05f);
    if (middle == 0) {
        action.acceleration = 0.1f;
    } else {
        action.acceleration = -middle;
    }
    return action;
}

Move getMoveLR(Ray rays[], int size) {
    float right = 0;
    float left = 0;
    float half_size = size/2;
    for (int i = 0; i < size; i++) {
        if (i < half_size) {
            left += rays[i].collided;
        } else {
            right += rays[i].collided;
        }
    }
    left /= floor(half_size);
    right /= floor(half_size);
    Move action;
    action.turning = std::clamp(right-left, -0.05f, 0.05f);
    action.acceleration = 0.05;
    if (abs(left-right) < 0.05 && left > 0.025 && right > 0.025) {
        action.acceleration = -15*abs(left-right);
    }
    return action;
}

Move getMoveTurning(Ray rays[], int size, float angle) {
    std::vector<std::vector<int>> groups;
    int longest = 0;
    int length = 0;
    int longest_index = 0;
    std::vector<int> group;
    for (int i = 0; i < size; i++) {
        if (!rays[i].collided) {
            group.push_back(i);
            length++;
        } else {
            if (length > longest) {longest = length; longest_index = groups.size();}
            length = 0;
            groups.push_back(group);
            group.clear();
        }
    }
    Move action;
    int index = ceil(longest/2);
    if (groups.size() != 0) {
        if (groups[longest_index].size() != 0) {
            action.acceleration = 1;
            action.turning = (angle - rays[groups[longest_index][index]].angle)/20;
        }
    } else {
        action = getMoveAvoidance(rays, size);
    }
    return action;
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
    World environment = World(50);
    environment.circles.push_back(Circle(width/2, height/2));
    glfwSetWindowUserPointer(window, &environment);
    glfwSetMouseButtonCallback(window, NULL);

    environment.step(num_rays, rays, getMoveAvoidance(rays, num_rays));

    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        double xpos;
        double ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        ypos = height - ypos;

        //environment.character.angle = atan2(ypos-environment.character.y, xpos-environment.character.x);
        //environment.character.velocity = 0;
        //if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {environment.character.velocity = 0.3;}
        //if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {environment.character.velocity = -0.3;}

        Move action;
        action.acceleration = 0;
        action.turning = 0;

        environment.step(num_rays, rays, getMoveLR(rays, num_rays));

        action = getMoveLR(rays, num_rays);

        std::cout << "Acceleration: " << action.acceleration << std::endl << "Turning: " << action.turning << std::endl;

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

// Finish Physics system
// Make input system
// Make Algorithm