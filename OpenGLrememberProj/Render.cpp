#include "Render.h"
#include <windows.h>
#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"
#include "MyOGL.h"
#include "Camera.h"
#include "Light.h"
#include "Primitives.h"
#include "MyShaders.h"
#include "ObjLoader.h"
#include "GUItextRectangle.h"
#include "Texture.h"

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;
bool MagicMode = true;
bool StopLight = true;

//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()

ObjFile *model;

Texture texture1;

Shader s[10];  //массивчик для десяти шейдеров

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}
}  camera;   //создаем объект камеры

//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(0, 0, 32);
	}
	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
	}
	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}
} light;  //создаем источник света

//старые координаты мыши
int mouseX = 0, mouseY = 0;

float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom;
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}

	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{


	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)	{	}

void keyUpEvent(OpenGL *ogl, int key)	{	}

ObjFile stone, ICE, Platform;

Texture stoneTex, CircleStone, Normal;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{
	//настройка текстур
	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	//массив трехбайтных элементов  (R G B)

	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 

	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/
	{

		s[0].VshaderFileName = "shaders\\v.vert";
		s[0].FshaderFileName = "shaders\\textureShader0.frag"; 
		s[0].LoadShaderFromFile(); 
		s[0].Compile();
	
		s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
		s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[1].Compile(); //компилируем

		s[2].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[2].FshaderFileName = "shaders\\IceShader.frag"; //имя файла фрагментного шейдера
		s[2].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[2].Compile(); //компилируем

		s[3].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[3].FshaderFileName = "shaders\\textureShader1.frag"; //имя файла фрагментного шейдера
		s[3].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[3].Compile(); //компилируем

		s[4].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[4].FshaderFileName = "shaders\\textureShader2.frag"; //имя файла фрагментного шейдера
		s[4].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[4].Compile(); //компилируем

		s[5].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[5].FshaderFileName = "shaders\\textureShader3.frag"; //имя файла фрагментного шейдера
		s[5].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[5].Compile(); //компилируем

		s[6].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[6].FshaderFileName = "shaders\\textureShader4.frag"; //имя файла фрагментного шейдера
		s[6].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[6].Compile(); //компилируем

		s[7].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
		s[7].FshaderFileName = "shaders\\textureShader5.frag"; //имя файла фрагментного шейдера
		s[7].LoadShaderFromFile(); //загружаем шейдеры из файла
		s[7].Compile(); //компилируем
	}

	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m

	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\stoneTex.obj_m", &stone);
	stoneTex.loadTextureFromFile("textures//tex.bmp");
	stoneTex.bindTexture();

	loadModel("models\\ICE.obj_m", &ICE);

	glActiveTexture(GL_TEXTURE1);
	loadModel("models\\Platform.obj_m", &Platform);
	CircleStone.loadTextureFromFile("textures//StonePlatform1.bmp");
	CircleStone.bindTexture();

	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("E - вкл/выкл Ритуал\nQ - вкл/выкл Движение света\nW/S - поднять/опустить сферы\nA/D - Ускорить замедлить\nF - обратный ход сфер\nR - обратный ход света",0,0,0);
}

Vector3 CircleC(double G, float r, float h)
{
	Vector3 res(0, 0, 0);
	glBegin(GL_LINE_STRIP);	

	for (double i = 0; i <= G; i += 0.01)
	{
		double x = r * cos(i * 3.141593);
		double y = r * sin(i  * 3.141593);
		Vector3 res1(x, y, h);
		res = res1;
	}
	glEnd();
	return res;
}

Vector3 CircleC90(double G, float r, float h)
{
	Vector3 res(0, 0, 0);
	glBegin(GL_LINE_STRIP);

	for (double i = 0; i <= G; i += 0.01)
	{
		double z = h + r/2 * cos(i * 3.141593);
		double x = r * sin(i * 3.141593);
		Vector3 res1(x, 0, z);
		res = res1;
	}
	glEnd();
	return res;
}

Vector3 CircleC_90(double G, float r, float h)
{
	Vector3 res(0, 0, 0);
	glBegin(GL_LINE_STRIP);

	for (double i = 0; i <= G; i += 0.01)
	{
		double z = h + r / 2 * cos((i+0.5) * 3.141593);
		double y = r * sin((i + 0.5) * 3.141593);
		Vector3 res1(0, y, z);
		res = res1;
	}
	glEnd();
	return res;
}

Vector3 CircleC45(double G, float r, float h)
{
	Vector3 res(0, 0, 0);
	
	glBegin(GL_LINE_STRIP);

	for (double i = 0; i <= G; i += 0.01)
	{
		double z = h + r / 2 * cos((i + 1) * 3.141593);
		double y = r * sin((i + 1) * 3.141593);
		double x = r * sin((i + 1) * 3.141593);
		Vector3 res1(x/ 1.43, y/1.43, z);
		res = res1;
	}
	glEnd();
	
	return res;
}

Vector3 CircleC_45(double G, float r, float h)
{
	Vector3 res(0, 0, 0);

	glBegin(GL_LINE_STRIP);

	for (double i = 0; i <= G; i += 0.01)
	{
		double z = h + r / 2 * cos((i + 1.5) * 3.141593);
		double y = r * sin((i + 1.5) * 3.141593);
		double x = r * sin((i + 1.5) * 3.141593);
		Vector3 res1(-x / 1.43, y / 1.43, z);
		res = res1;
	}
	glEnd();

	return res;
}

float anim_h = 0.01,     h1 = 0.1, t1 = 0.01;
double anim_t = anim_h,  h = h1, t = t1;

void Render(OpenGL *ogl)
{   
	tick_o = tick_n;
	tick_n = GetTickCount();
	Time += (tick_n - tick_o) / 1000.0;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//===================================
	//Прогать тут  

	glActiveTexture(GL_TEXTURE1);
	s[0].UseShader();
	{
		//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
		int location0 = glGetUniformLocationARB(s[0].program, "light_pos");
		//Шаг 2 - передаем ей значение
		glUniform3fARB(location0, light.pos.X(), light.pos.Y(), light.pos.Z());

		location0 = glGetUniformLocationARB(s[0].program, "Ia");
		glUniform3fARB(location0, 0.2, 0.2, 0.2);

		location0 = glGetUniformLocationARB(s[0].program, "Id");
		glUniform3fARB(location0, 1.0, 1.0, 1.0);

		location0 = glGetUniformLocationARB(s[0].program, "Is");
		glUniform3fARB(location0, .7, .7, .7);

		location0 = glGetUniformLocationARB(s[0].program, "ma");
		glUniform3fARB(location0, 0.2, 0.2, 0.1);

		location0 = glGetUniformLocationARB(s[0].program, "md");
		glUniform3fARB(location0, 0.4, 0.65, 0.5);

		location0 = glGetUniformLocationARB(s[0].program, "ms");
		glUniform4fARB(location0, 0.9, 0.8, 0.3, 25.6);

		location0 = glGetUniformLocationARB(s[0].program, "camera");
		glUniform3fARB(location0, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

		location0 = glGetUniformLocationARB(s[0].program, "tex");
		glUniform1iARB(location0, 1);
	}
	PUSH;
	glScaled(1.1, 1.1, 1.1);
	CircleStone.bindTexture();
	Platform.DrawObj();
	POP;

	glActiveTexture(GL_TEXTURE0);
	//glActiveTexture(GL_TEXTURE2);
	s[1].UseShader();
	{

		//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
		int location = glGetUniformLocationARB(s[1].program, "light_pos");
		//Шаг 2 - передаем ей значение
		glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

		location = glGetUniformLocationARB(s[1].program, "Ia");
		glUniform3fARB(location, 0.2, 0.2, 0.2);

		location = glGetUniformLocationARB(s[1].program, "Id");
		glUniform3fARB(location, 1.0, 1.0, 1.0);

		location = glGetUniformLocationARB(s[1].program, "Is");
		glUniform3fARB(location, .7, .7, .7);

		location = glGetUniformLocationARB(s[1].program, "ma");
		glUniform3fARB(location, 0.2, 0.2, 0.1);

		location = glGetUniformLocationARB(s[1].program, "md");
		glUniform3fARB(location, 0.4, 0.65, 0.5);

		location = glGetUniformLocationARB(s[1].program, "ms");
		glUniform4fARB(location, 0.9, 0.8, 0.3, 25.6);

		location = glGetUniformLocationARB(s[1].program, "camera");
		glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

		location = glGetUniformLocationARB(s[1].program, "tex");
		glUniform1iARB(location, 0);

		location = glGetUniformLocationARB(s[1].program, "Normal");
		glUniform1iARB(location, 2);
	}

	PUSH;
	glScaled(1, 1, 1.3);
	glTranslated(0, 0, -1);
	glRotated(24.5, 0, 0, 1);
	stoneTex.bindTexture();
	stone.DrawObj();
	POP;

	PUSH;
	glScaled(3, 3, 3);
	glTranslated(0, 0, -0.3);
	stoneTex.bindTexture();
	stone.DrawObj();
	POP;

	PUSH;
	glScaled(.35, .35, 1);
	glTranslated(0, 0, 0.5);
	glRotated(24.5, 0, 0, 1);
	glNormal3d(0,0,1);
	CircleStone.bindTexture();
	Platform.DrawObj();
	POP;
	
	s[2].UseShader();
	{
		//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
		int location1 = glGetUniformLocationARB(s[2].program, "light_pos");
		//Шаг 2 - передаем ей значение
		glUniform3fARB(location1, light.pos.X(), light.pos.Y(), light.pos.Z());

		location1 = glGetUniformLocationARB(s[2].program, "Ia");
		glUniform3fARB(location1, 0.2, 0.2, 0.2);

		location1 = glGetUniformLocationARB(s[2].program, "Id");
		glUniform3fARB(location1, 1.0, 1.0, 1.0);

		location1 = glGetUniformLocationARB(s[2].program, "Is");
		glUniform3fARB(location1, .7, .7, .7);

		location1 = glGetUniformLocationARB(s[2].program, "ma");
		glUniform3fARB(location1, 0.2, 0.2, 0.1);

		location1 = glGetUniformLocationARB(s[2].program, "md");
		glUniform3fARB(location1, 0.4, 0.65, 0.5);

		location1 = glGetUniformLocationARB(s[2].program, "ms");
		glUniform4fARB(location1, 0.9, 0.8, 0.3, 25.6);

		location1 = glGetUniformLocationARB(s[2].program, "camera");
		glUniform3fARB(location1, camera.pos.X(), camera.pos.Y(), camera.pos.Z());
	}

	PUSH;
	glScaled(3, 3, 3);
	glRotated(90, 1, 0, 0);
	ICE.DrawObj();
	POP;
	
	glDisable(GL_LIGHTING);

	float r = 33.0;

	if (OpenGL::isKeyPressed('E')) { MagicMode = !MagicMode; }
	if (OpenGL::isKeyPressed('Q')) { StopLight = !StopLight; }
	if (OpenGL::isKeyPressed('F')) { anim_h = anim_h * -1; }
	if (OpenGL::isKeyPressed('R')) { t1 = t1 * -1; }

	if (MagicMode)
	{
		PUSH;
		Vector3 pos5 = { 0, 0, h + 15 };
		s[3].UseShader();
		Sphere D5;
		D5.pos = pos5;
		D5.scale = D5.scale * 2.5;
		D5.Show();
		POP;
		

		PUSH;
		Vector3 pos1 = CircleC90(anim_t, r / 3, (h + 15) / 2);
		glTranslated(pos1.X(), pos1.Y(), pos1.Z());
		s[4].UseShader();

		Sphere D1;
		D1.pos = pos1;
		D1.scale = D1.scale * 0.8;
		D1.Show();
		POP;

		PUSH;
		Vector3 pos2 = CircleC_90(anim_t, r / 3, (h + 15) / 2);
		glTranslated(pos2.X(), pos2.Y(), pos2.Z());

		s[5].UseShader();

		Sphere D2;
		D2.pos = pos2;
		D2.scale = D2.scale * 0.8;
		D2.Show();
		POP;

		PUSH;
		Vector3 pos3 = CircleC45(anim_t, r / 3, (h + 15) / 2);
		glTranslated(pos3.X(), pos3.Y(), pos3.Z());

		s[6].UseShader();

		Sphere D3;
		D3.pos = pos3;
		D3.scale = D3.scale * 0.8;
		D3.Show();
		POP;

		PUSH;
		Vector3 pos4 = CircleC_45(anim_t, r / 3, (h + 15) / 2);
		glTranslated(pos4.X(), pos4.Y(), pos4.Z());

		s[7].UseShader();

		Sphere D4;
		D4.pos = pos4;
		D4.scale = D4.scale * 0.8;
		D4.Show();
		POP;

		s[1].UseShader();
		Shader::DontUseShaders();
		PUSH;
		Vector3 pos = CircleC(t / 2, r / 2, 15.0 / 2);
		light.pos = CircleC(t / 2, r, 15);
		glTranslated(pos.X(), pos.Y(), pos.Z());
		glColor3f(.7, .7, .7);

		Sphere D;
		D.pos = pos;
		D.scale = D.scale * 1.5;
		D.Show();
		
		POP;
		
	}

	if (StopLight)
		t += t1;
	anim_t += anim_h;

	if (OpenGL::isKeyPressed('W')) { h += h1; }
	if (OpenGL::isKeyPressed('S')) { h -= h1; }
	if (OpenGL::isKeyPressed('D')) { anim_h = anim_h * 1.1; }
	if (OpenGL::isKeyPressed('A')) { anim_h = anim_h * 0.9; }

}   //конец тела функции

bool gui_init = false;

void RenderGUI(OpenGL* ogl)
{
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);

	glActiveTexture(GL_TEXTURE0);
	rec.Draw();

	Shader::DontUseShaders();
}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}