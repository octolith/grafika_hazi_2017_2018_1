//=============================================================================================
// Framework for the ray tracing homework
// ---------------------------------------------------------------------------------------------
// Name   : Levai Marcell
// Neptun : ZCH7OS
//=============================================================================================

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <vector>

const unsigned int windowWidth = 600, windowHeight = 600;

int majorVersion = 3, minorVersion = 3;

struct vec3 {
	float x, y, z;
	vec3(float x0 = 0, float y0 = 0, float z0 = 0) { x = x0; y = y0; z = z0; }
	vec3 operator*(float a) const { return vec3(x * a, y * a, z * a); }
	vec3 operator/(float d) const { return vec3(x / d, y / d, z / d); }
	vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
	void operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; }
	vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
	vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
	vec3 operator-() const { return vec3(-x, -y, -z); }
	vec3 normalize() const { return (*this) * (1 / (Length() + 0.000001)); }
	float Length() const { return sqrtf(x * x + y * y + z * z); }
	operator float*() { return &x; }
};

float dot(const vec3& v1, const vec3& v2) {
	return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

vec3 cross(const vec3& v1, const vec3& v2) {
	return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

class Material {
	vec3 kd, ks;
	float  shininess;
public:
	vec3 ka;
	Material(vec3 _ka, vec3 _kd, vec3 _ks, float _shininess) : ka(_ka), kd(_kd), ks(_kd) { shininess = _shininess; }
	vec3 shade(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 inRad) {
		vec3 reflRad(0, 0, 0);
		float cosTheta = dot(normal, lightDir);
		if (cosTheta < 0) return reflRad;
		reflRad = inRad * kd * cosTheta;
		vec3 halfway = (viewDir + lightDir).normalize();
		float cosDelta = dot(normal, halfway);
		if (cosDelta < 0) return reflRad;
		return reflRad + inRad * ks * pow(cosDelta, shininess);
	}
};

struct Hit {
	float t;
	vec3 position;
	vec3 normal;
	Material * material;
	Hit() { t = -1; }
};

struct Ray {
	vec3 start, dir;
	Ray(vec3 _start, vec3 _dir) { start = _start; dir = _dir.normalize(); }
};


class Intersectable {
protected:
	Material * material;
public:
	virtual Hit intersect(const Ray& ray) = 0;
};

struct Sphere : public Intersectable {
	vec3 center;
	float radius;

	Sphere(const vec3& _center, float _radius, Material* _material) {
		center = _center;
		radius = _radius;
		material = _material;
	}
	Hit intersect(const Ray& ray) {
		Hit hit;
		vec3 dist = ray.start - center;
		float b = dot(dist, ray.dir) * 2.0;
		float a = dot(ray.dir, ray.dir);
		float c = dot(dist, dist) - radius * radius;
		float discr = b * b - 4.0 * a * c;
		if (discr < 0) return hit;
		float sqrt_discr = sqrtf(discr);
		float t1 = (-b + sqrt_discr) / 2.0 / a;
		float t2 = (-b - sqrt_discr) / 2.0 / a;
		if (t1 <= 0 && t2 <= 0) return hit;
		if (t1 <= 0 && t2 > 0)       hit.t = t2;
		else if (t2 <= 0 && t1 > 0)  hit.t = t1;
		else if (t1 < t2)            hit.t = t1;
		else                         hit.t = t2;
		hit.position = ray.start + ray.dir * hit.t;
		hit.normal = (hit.position - center) / radius;
		hit.material = material;
		return hit;
	}
};

class Camera {
	vec3 eye, lookat, right, up;
public:
	void set(vec3 _eye, vec3 _lookat, vec3 vup, double fov) {
		eye = _eye;
		lookat = _lookat;
		vec3 w = eye - lookat;
		float f = w.Length();
		right = cross(vup, w).normalize() * f * tan(fov / 2);
		up = cross(w, right).normalize() * f * tan(fov / 2);
	}
	Ray getray(int X, int Y) {
		vec3 dir = lookat + right * (2.0 * (X + 0.5) / windowWidth - 1) + up * (2.0 * (Y + 0.5) / windowHeight - 1) - eye;
		return Ray(eye, dir);
	}
};

struct Light {
	vec3 direction;
	vec3 Le;
	Light(vec3 _direction, vec3 _Le) {
		direction = _direction.normalize();
		Le = _Le;
	}
};

class Scene {
	std::vector<Intersectable *> objects;
	std::vector<Light *> lights;
	Camera camera;
	vec3 La;
public:
	void build() {
		vec3 eye = vec3(0, 0, 2);
		vec3 vup = vec3(0, 1, 0);
		vec3 lookat = vec3(0, 0, 0);
		float fov = 45 * M_PI / 180;
		camera.set(eye, lookat, vup, fov);

		La = vec3(0.1f, 0.1f, 0.1f);
		lights.push_back(new Light(vec3(1, 1, 1), vec3(3, 3, 3)));

		vec3 kd(0.3f, 0.2f, 0.1f), ks(10, 10, 10);
		vec3 ka = kd * M_PI;
		objects.push_back(new Sphere(vec3(0, 0, 0), 0.5f, new Material(ka, kd, ks, 50)));
	}

	void render(vec3 image[]) {
#pragma omp parallel for
		for (int Y = 0; Y < windowHeight; Y++) {
			for (int X = 0; X < windowWidth; X++) image[Y * windowWidth + X] = trace(camera.getray(X, Y));
		}
	}

	Hit firstIntersect(Ray ray) {
		Hit bestHit;
		for (Intersectable * object : objects) {
			Hit hit = object->intersect(ray); //  hit.t < 0 if no intersection
			if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t))  bestHit = hit;
		}
		return bestHit;
	}

	vec3 trace(Ray ray, int depth = 0) {
		Hit hit = firstIntersect(ray);
		if (hit.t < 0) return La;
		vec3 outRadiance = hit.material->ka * La;
		for (Light * light : lights) {
			outRadiance += hit.material->shade(hit.normal, -ray.dir, light->direction, light->Le);
		}
		return outRadiance;
	}
};

Scene scene;

void getErrorInfo(unsigned int handle) {
	int logLen, written;
	glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
	if (logLen > 0) {
		char * log = new char[logLen];
		glGetShaderInfoLog(handle, logLen, &written, log);
		printf("Shader log:\n%s", log);
		delete log;
	}
}

// check if shader could be compiled
void checkShader(unsigned int shader, char * message) {
	int OK;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
	if (!OK) {
		printf("%s!\n", message);
		getErrorInfo(shader);
	}
}

// check if shader could be linked
void checkLinking(unsigned int program) {
	int OK;
	glGetProgramiv(program, GL_LINK_STATUS, &OK);
	if (!OK) {
		printf("Failed to link shader program!\n");
		getErrorInfo(program);
	}
}

// vertex shader in GLSL
const char *vertexSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	out vec2 texcoord;

	void main() {
		texcoord = (vertexPosition + vec2(1, 1))/2;							// -1,1 to 0,1
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1); 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char *fragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	in  vec2 texcoord;			// interpolated texture coordinates
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = texture(textureUnit, texcoord); 
	}
)";


// handle of the shader program
unsigned int shaderProgram;

class FullScreenTexturedQuad {
	unsigned int vao, textureId;	// vertex array object id and texture id
public:
	void Create(vec3 image[]) {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo;		// vertex buffer objects
		glGenBuffers(1, &vbo);	// Generate 1 vertex buffer objects

								// vertex coordinates: vbo0 -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it active, it is an array
		float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };	// two triangles forming a quad
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

																	  // Create objects by setting up their vertex data on the GPU
		glGenTextures(1, &textureId);  				// id generation
		glBindTexture(GL_TEXTURE_2D, textureId);    // binding
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, image); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		int location = glGetUniformLocation(shaderProgram, "textureUnit");
		if (location >= 0) {
			glUniform1i(location, 0);		// texture sampling unit is TEXTURE0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureId);	// connect the texture to the sampler
		}
		glDrawArrays(GL_TRIANGLE_FAN, 0, 6);	// draw two triangles forming a quad
	}
};

// The virtual world: single quad
FullScreenTexturedQuad fullScreenTexturedQuad;
vec3 image[windowWidth * windowHeight];	// The image, which stores the ray tracing result

										// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	scene.build();
	scene.render(image);
	fullScreenTexturedQuad.Create(image);

	// Create vertex shader from string
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	checkShader(vertexShader, "Vertex shader error");

	// Create fragment shader from string
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkShader(fragmentShader, "Fragment shader error");

	// Attach shaders to a single program
	shaderProgram = glCreateProgram();
	if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Connect the fragmentColor to the frame buffer memory
	glBindFragDataLocation(shaderProgram, 0, "fragmentColor");	// fragmentColor goes to the frame buffer memory

																// program packaging
	glLinkProgram(shaderProgram);
	checkLinking(shaderProgram);
	glUseProgram(shaderProgram); 	// make this program run
}

void onExit() {
	glDeleteProgram(shaderProgram);
	printf("exit");
}

// Window has become invalid: Redraw
void onDisplay() {
	fullScreenTexturedQuad.Draw();
	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
	}
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}

int main(int argc, char * argv[]) {
	glutInit(&argc, argv);
#if !defined(__APPLE__)
	glutInitContextVersion(majorVersion, minorVersion);
#endif
	glutInitWindowSize(windowWidth, windowHeight);				// Application window is initially of resolution 600x600
	glutInitWindowPosition(100, 100);							// Relative location of the application window
#if defined(__APPLE__)
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);  // 8 bit R,G,B,A + double buffer + depth buffer
#else
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutCreateWindow(argv[0]);

#if !defined(__APPLE__)
	glewExperimental = true;	// magic
	glewInit();
#endif

	printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
	printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
	printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
	printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	onInitialization();

	glutDisplayFunc(onDisplay);                // Register event handlers
	glutMouseFunc(onMouse);
	glutIdleFunc(onIdle);
	glutKeyboardFunc(onKeyboard);
	glutKeyboardUpFunc(onKeyboardUp);
	glutMotionFunc(onMouseMotion);

	glutMainLoop();
	onExit();
	return 1;
}

/*

struct vec4 {
float v[4];

vec4(float x = 0, float y = 0, float z = 0, float w = 1) {
v[0] = x; v[1] = y; v[2] = z; v[3] = w;
}
};

struct mat4 {
public:
float m[4][4];
mat4() {}
mat4(float m00, float m01, float m02, float m03,
float m10, float m11, float m12, float m13,
float m20, float m21, float m22, float m23,
float m30, float m31, float m32, float m33) {
m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
}

mat4 operator*(const mat4& right) {
mat4 result;
for (int i = 0; i < 4; i++) {
for (int j = 0; j < 4; j++) {
result.m[i][j] = 0;
for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
}
}
return result;
}
operator float*() {
return &m[0][0];
}

void SetUniform(unsigned shaderProg, char * name) {
int loc = glGetUniformLocation(shaderProg, name);
glUniformMatrix4fv(loc, 1, GL_TRUE, &m[0][0]);
}
};

mat4 Translate(float tx, float ty, float tz) {
return mat4(1, 0, 0, 0,
0, 1, 0, 0,
0, 0, 1, 0,
tx, ty, tz, 1);
}

//mat4 Rotate(float angle, float wx, float wy, float wz) { … }
//mat4 Scale(float sx, float sy, float sz) { … }

float sign(float f) {
if (f < 0) {
return -1.0f;
}
else {
return 1.0f;
}
}

class Material {
public:
vec3 F0;
vec3 n;		// toresmutato
vec3 kd, ks;
float shine;
bool rough;
bool reflective;
bool refractive;
vec3 ka;

vec3 reflect(vec3 inDir, vec3 normal) {
return inDir - normal * dot(normal, inDir) * 2.0f;
}
vec3 refract(vec3 inDir, vec3 normal) {

// uveg toresmutatoi egyformak, igy vec3 n helyett egy komponenssel eleg szamolni
float ior = n.x; // depends on whether from inside or outside
float cosa = -dot(normal, inDir); // from inside?
if (cosa < 0) {
cosa = -cosa;
normal = -normal;
ior = 1 / n.x;
}
float disc = 1 - (1 - cosa * cosa) / ior / ior;
if (disc < 0) return reflect(inDir, normal); // magic !!!
return inDir / ior + normal * (cosa / ior - sqrt(disc));
}
vec3 Fresnel(vec3 inDir, vec3 normal) { // in/out irrelevant
float cosa = fabs(dot(normal, inDir));

// TODO: use regular Fresnel equation
return F0 + (vec3(1, 1, 1) - F0) * pow(1 - cosa, 5);
}
vec3 shade(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 inRad) {
vec3 reflRad(0, 0, 0);
float cosTheta = dot(normal, lightDir);
if (cosTheta < 0) return reflRad;
reflRad = inRad * kd * cosTheta;;
vec3 halfway = (viewDir + lightDir).normalize();
float cosDelta = dot(normal, halfway);
if (cosDelta < 0) return reflRad;
return reflRad + inRad * ks * pow(cosDelta, shine);
}
};


class GoldMaterial : public Material {
public:
GoldMaterial() {
rough = false;
reflective = true;
refractive = false;
n = vec3(0.17f, 0.35f, 1.5f);
ks = vec3(3.1f, 2.7f, 1.9f);
ka = vec3(3.1f, 2.7f, 1.9f);
}
};

class GlassMaterial : public Material {
public:
GlassMaterial() {
reflective = true;
refractive = true;
n = vec3(1.5f, 1.5f, 1.5f);
ks = vec3(0.0f, 0.0f, 0.0f);
ka = vec3(0.0f, 0.0f, 0.0f);
}
};

class SilverMaterial : public Material {
public:
SilverMaterial() {
rough = false;
reflective = true;
refractive = false;
n = vec3(0.14f, 0.16f, 0.13f);
ks = vec3(4.1f, 2.3f, 3.1f);
}
};


class TorusTriangle : public Intersectable {
public:
	vec3 a;
	vec3 b;
	vec3 c;
	vec3 torusCenter;
	TorusTriangle(Material* material, vec3 torusCenter, vec3 a, vec3 b, vec3 c) {
		this->material = material;
		this->a = a;
		this->b = b;
		this->c = c;
		this->torusCenter = torusCenter;
	}
	Hit intersect(const Ray& ray) {
		// TODO
		Hit h;
		h.normal = (torusCenter - a + torusCenter - b + torusCenter - c) / 3;;
		return h;
	}
};


class Torus {
public:
	Material* material;
	vec3 center;
	float r;
	float R;
	Torus(Material* material, vec3 center, float r, float R) {
		this->material = material;
		this->center = center;
		this->r = r;
		this->R = R;
	}
};



class Scene {
public:
Camera camera;
vec3 La;
Intersectable* objects[150];
int intersectables;
Light* lights[3];

void build() {
intersectables = 0;

camera.eye = vec3(0, 0, 0);			// értelmes értékeket kellene ideírni
camera.fov = 0.5f;					// fél radián
La = vec3(0.1f, 0.1f, 0.1f);		// ambiens fény kicsi

GoldMaterial gold;
Torus torus(&gold, vec3(0,0,0), 20.0f, 150.0f);

vec3 torus1points[18];
for (int v = 0; v < 6; v++) {
for (int u = 0; u < 3; u++) {
torus1points[v * 3 + u] = vec3(
(torus.R + torus.r * cosf((float)u/6 * 2.0f * (float)(M_PI))) * cosf((float)v/3 * 2.0f * (float)(M_PI)),
(torus.R + torus.r * cosf((float)u/6 * 2.0f * (float)(M_PI))) * sinf((float)v/3 * 2.0f * (float)(M_PI)),
torus.r * sinf((float)u/6 * 2.0f * (float)(M_PI))
);
}
}

//torus1points transzformalasa

vec3 torus2points[18];
for (int v = 0; v < 6; v++) {
for (int u = 0; u < 3; u++) {
torus2points[v * 3 + u] = vec3(
(torus.R + torus.r * cosf((float)u / 6 * 2.0f * (float)(M_PI))) * cosf((float)v / 3 * 2.0f * (float)(M_PI)),
(torus.R + torus.r * cosf((float)u / 6 * 2.0f * (float)(M_PI))) * sinf((float)v / 3 * 2.0f * (float)(M_PI)),
torus.r * sinf((float)u / 6 * 2.0f * (float)(M_PI))
);
}
}

//torus2points transzformalasa

vec3 torus3points[18];
for (int v = 0; v < 6; v++) {
for (int u = 0; u < 3; u++) {
torus3points[v * 3 + u] = vec3(
(torus.R + torus.r * cosf((float)u / 6 * 2.0f * (float)(M_PI))) * cosf((float)v / 3 * 2.0f * (float)(M_PI)),
(torus.R + torus.r * cosf((float)u / 6 * 2.0f * (float)(M_PI))) * sinf((float)v / 3 * 2.0f * (float)(M_PI)),
torus.r * sinf((float)u / 6 * 2.0f * (float)(M_PI))
);
}
}

//torus3points transzformalasa

// minden torusz haromszogeinek letrehozasa

for (int i = 0; i < 18; i++) {
// haromszogek gyartasa
// objects[i] = Triangle(torus.material, torus.center, points...
}

for (int i = 0; i < 18; i++) {
// haromszogek gyartasa
// objects[18 + i] = Triangle(torus.material, torus.center, points...
}

for (int i = 0; i < 18; i++) {
// haromszogek gyartasa
// objects[36 + i] = Triangle(torus.material, torus.center, points...
}




}

void render() {

}

Hit firstIntersect(Ray ray) {
Hit bestHit;
for (int i = 0; i < INTERSECTABLES; i++) {	// for each intersectable object
Hit hit = objects[i]->intersect(ray); //  hit.t < 0 if no intersection
if (hit.t > 0 && (bestHit.t < 0 || hit.t < bestHit.t)) {
bestHit = hit;
}
}
return bestHit;
}

vec3 trace(Ray ray, int depth) {
vec3 outRadiance;
if (depth > MAXDEPTH) {
outRadiance = La;
}
else {
outRadiance = vec3(0, 0, 0);
Hit hit = firstIntersect(ray);
if (hit.t < 0) {
return La;
}
if (hit.material->rough) {
outRadiance = hit.material->ka * La;
for (int i = 0; i < LIGHTS; i++) {	// for each light

// na hogy itt például ezek mi a tökömök
Ray shadowRay(hit.position + hit.normal * 0.000001f * sign(hit.normal * V), L);
Hit shadowHit = firstIntersect(shadowRay);
if (shadowHit.t < 0 || shadowHit.t > fabs(hit.position - yl))
outRadiance = outRadiance + hit.material->shade(hit.normal, V, lights[i]->Lout, Lel);
}
}
if (hit.material->reflective) {
vec3 reflectionDir = hit.material->reflect(ray.dir, hit.normal);
Ray reflectedRay(hit.position + hit.normal * 0.000001f * sign(hit.normal * V), reflectionDir);
outRadiance = outRadiance + trace(reflectedRay, depth + 1) * hit.material->Fresnel(V, hit.normal);
}
if (hit.material->refractive) {
vec3 refractionDir = hit.material->refract(ray.dir, hit.normal);
Ray refractedRay(hit.position - hit.normal * 0.000001f * sign(hit.normal * V), refractionDir);
outRadiance = outRadiance + trace(refractedRay, depth + 1) * (vec3(1, 1, 1) - hit.material->Fresnel(V, hit.normal));
}
}
return outRadiance;
}
};
*/