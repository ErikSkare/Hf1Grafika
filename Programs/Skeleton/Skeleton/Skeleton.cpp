//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Skáre Erik
// Neptun : Z7ZF6D
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

const char * const vertexSource = R"(
	#version 330
	precision highp float;

	uniform vec2 translation;
	uniform float scale;
	layout(location = 0) in vec2 vp;

	void main() {
		gl_Position = vec4(vp.x * scale + translation.x, vp.y * scale + translation.y, 0, 1);
	}
)";

const char * const fragmentSource = R"(
	#version 330
	precision highp float;
	
	uniform vec3 color;
	out vec4 outColor;

	void main() {
		outColor = vec4(color, 1);
	}
)";

float dotHyperbolic(const vec3& v1, const vec3& v2) {
	return v1.x * v2.x + v1.y * v2.y - v1.z * v2.z;
}

float lengthHyperbolic(const vec3& v) {
	return sqrtf(dotHyperbolic(v, v));
}

vec3 normalizeHyperbolic(const vec3& v) {
	return v / lengthHyperbolic(v);
}

vec3 getClosePoint(const vec3& point) {
	return vec3(point.x, point.y, sqrtf(point.x * point.x + point.y * point.y + 1));
}

vec3 getCloseVector(const vec3& point, const vec3& vector) {
	vec3 result = vector + dotHyperbolic(point, vector) * point;
	return normalizeHyperbolic(result) * lengthHyperbolic(vector);
}

vec3 perpendicular(const vec3& point, const vec3& vector) {
	vec3 c = cross(point, vector);
	return getCloseVector(point, normalizeHyperbolic({c.x, c.y, -c.z}) * lengthHyperbolic(vector));
}

vec3 lineLocationAfterTime(const vec3& point, const vec3& velocity, float time) {
	return getClosePoint(point * coshf(time) + velocity * sinhf(time));
}

vec3 lineVelocityAfterTime(const vec3& point, const vec3& velocity, float time) {
	return getCloseVector(lineLocationAfterTime(point, velocity, time), point * sinhf(time) + velocity * coshf(time));
}

float distanceToPoint(const vec3& from, const vec3& to) {
	return acoshf(dotHyperbolic(-from, to));
}

vec3 directionToPoint(const vec3& from, const vec3& to) {
	float distance = distanceToPoint(from, to);
	return getCloseVector(from, (to - from * coshf(distance)) / sinhf(distance));
}

vec3 pointByDistanceAndDirection(const vec3& point, const vec3& direction, float distance) {
	vec3 normalizedDirection = normalizeHyperbolic(direction);
	return lineLocationAfterTime(point, normalizedDirection, distance);
}

vec3 rotate(const vec3& point, const vec3& vector, float angle) {
	vec3 normalized = normalizeHyperbolic(vector);
	return lengthHyperbolic(vector) * getCloseVector(point, normalized * cosf(angle) + perpendicular(point, normalized) * sinf(angle));
}

vec2 project(const vec3& point) {
	return vec2(point.x / (1 + point.z), point.y / (1 + point.z));
}

vec2 projectedCenterOfCircle(const vec3& point, float radius) {
	vec3 direction = getCloseVector(point, { 1, 0, 0 });
	vec2 center = { 0, 0 };

	int iteration = 100;
	for (int i = 0; i < iteration; ++i) {
		center = center + project(pointByDistanceAndDirection(point, rotate(point, direction, 2 * M_PI * i / iteration), radius));
	}
	center = center / iteration;

	return center;
}

float projectedRadiusOfCircle(const vec3& point, float radius) {
	vec3 direction = getCloseVector(point, { 1, 0, 0 });
	vec2 center = projectedCenterOfCircle(point, radius);
	return length(project(pointByDistanceAndDirection(point, direction, radius)) - center);
}

GPUProgram gpuProgram;

class Element {
protected:
	unsigned int vao = NULL;
	unsigned int vertexCount = 0;
	const GLenum mode;

public:
	Element(GLenum mode): mode(mode) {}

	virtual void Initialize() = 0;

	virtual void Draw(const vec3& color, const vec2& translation = {0, 0}, float scale = 1) {
		if (vao == NULL)
			throw "Inicializálás szükséges a Draw elõtt!";
		gpuProgram.setUniform(color, "color");
		gpuProgram.setUniform(translation, "translation");
		gpuProgram.setUniform(scale, "scale");
		glBindVertexArray(vao);
		glDrawArrays(mode, 0, vertexCount);
	}

	virtual ~Element() {}
};

class Circle : public Element {
public:
	Circle(const int vertexCount): Element(GL_TRIANGLE_FAN) {
		this->vertexCount = vertexCount;
	}

	void Initialize() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		unsigned int vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		std::vector<vec2> points(vertexCount);
		
		for (int i = 0; i < vertexCount; ++i) {
			points[i].x = cosf(2 * M_PI * i / vertexCount);
			points[i].y = sinf(2 * M_PI * i / vertexCount);
		}

		glBufferData(GL_ARRAY_BUFFER,
			2 * vertexCount * sizeof(float),
			&points[0],
			GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,
			2, GL_FLOAT, GL_FALSE,
			0, NULL);
	}
};

class Path : public Element {
	unsigned int vbo = NULL;
	std::vector<vec2> points;

public:
	Path(): Element(GL_LINE_STRIP) {}

	void Initialize() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,
			2, GL_FLOAT, GL_FALSE,
			0, NULL);
	}

	void AddPoint(vec2 point) {
		points.push_back(point);
		vertexCount++;
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 2 * vertexCount * sizeof(float), &points[0], GL_DYNAMIC_DRAW);
	}
};

Circle circle(100);

class Hami {
	vec3 color;
	vec3 position;
	vec3 velocity;
	Path path;
	float mouthOffset = 0;
	float radius = 0.25;

public:
	Hami(const vec3& color, const vec3& position, const vec3& velocity): color(color), position(position), velocity(velocity) {}

	void Initialize() {
		path.Initialize();
	}
	
	void Move(float dt) {
		vec3 savedPosition = position;
		position = lineLocationAfterTime(savedPosition, velocity, dt);
		velocity = lineVelocityAfterTime(savedPosition, velocity, dt);
		path.AddPoint(project(position));
	}
	
	void Rotate(float dt, float angularSpeed) {
		velocity = rotate(position, velocity, angularSpeed * dt);
	}

	void Draw(const vec3& watching) {
		vec3 mouthPosition = pointByDistanceAndDirection(position, velocity, radius + mouthOffset);
		vec3 eyeOnePosition = pointByDistanceAndDirection(position, rotate(position, velocity, M_PI / 5), radius);
		vec3 eyeTwoPosition = pointByDistanceAndDirection(position, rotate(position, velocity, -M_PI / 5), radius);
		vec3 pupilOnePosition = pointByDistanceAndDirection(eyeOnePosition, directionToPoint(position, watching), radius / 5);
		vec3 pupilTwoPosition = pointByDistanceAndDirection(eyeTwoPosition, directionToPoint(position, watching), radius / 5);

		circle.Draw(color, projectedCenterOfCircle(position, radius), projectedRadiusOfCircle(position, radius));
		circle.Draw({ 0, 0, 0 }, projectedCenterOfCircle(mouthPosition, radius / 4), projectedRadiusOfCircle(mouthPosition, radius / 4));
		circle.Draw({ 1, 1, 1 }, projectedCenterOfCircle(eyeOnePosition, radius / 4), projectedRadiusOfCircle(eyeOnePosition, radius / 4));
		circle.Draw({ 1, 1, 1 }, projectedCenterOfCircle(eyeTwoPosition, radius / 4), projectedRadiusOfCircle(eyeTwoPosition, radius / 4));
		circle.Draw({ 0, 0, 1 }, projectedCenterOfCircle(pupilOnePosition, radius / 8), projectedRadiusOfCircle(pupilOnePosition, radius / 8));
		circle.Draw({ 0, 0, 1 }, projectedCenterOfCircle(pupilTwoPosition, radius / 8), projectedRadiusOfCircle(pupilTwoPosition, radius / 8));
	}

	void DrawPath() {
		path.Draw({ 1, 1, 1 });
	}

	void Hamm(long time) {
		mouthOffset = (sinf(time / 1000.0f * 2) + 1) * radius / 8;
	}

	vec3 GetPosition() const {
		return position;
	}
};

Hami red(
	vec3(1, 0, 0), 
	vec3(-1, 1, sqrtf(3)), 
	normalizeHyperbolic(getCloseVector({ -1, 1, sqrtf(3) }, { 1, 0, 0 })));
Hami green(
	vec3(0, 1, 0), 
	vec3(1, -1, sqrtf(3)), 
	normalizeHyperbolic(getCloseVector({1, -1, sqrtf(3)}, {1, 0, 0})));
bool pressed[256] = { false };
long lastTimeIdle = 0;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	red.Initialize();
	green.Initialize();
	circle.Initialize();

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

void onDisplay() {
	glClearColor(0.5, 0.5, 0.5, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	circle.Draw({0, 0, 0});
	red.DrawPath();
	green.DrawPath();
	green.Draw(red.GetPosition());
	red.Draw(green.GetPosition());

	glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
	pressed[key] = true;
	glutPostRedisplay();
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
	pressed[key] = false;
	glutPostRedisplay();
}

void onMouseMotion(int pX, int pY) {}

void onMouse(int button, int state, int pX, int pY) {}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
	float dt = (time - lastTimeIdle) / 1000.0f;

	if (pressed['e']) red.Move(dt);
	if (pressed['s']) red.Rotate(dt, -2 * M_PI / 5);
	if (pressed['f']) red.Rotate(dt, 2 * M_PI / 5);

	green.Move(dt);
	green.Rotate(dt, 2 * M_PI / 3);

	red.Hamm(time);
	green.Hamm(time);

	lastTimeIdle = time;
	glutPostRedisplay();
}
