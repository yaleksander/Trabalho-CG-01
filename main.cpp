// incluimos <windows.h> caso estejamos usando esse sistema operacional
#ifdef WIN32
#include <windows.h>
#endif

// biblioteca do glut e algumas básicas de c++
#include <GL/glui.h>
#include <iostream>
#include <cstdlib>
#include <cmath>

// número máximo de pontos
#define MAXNUM 100
// precisão do cursor para selecionar pontos
#define LONGE 0.05
// densidade (thickness) da curva BSpline
#define THICKN 200

// só pra não precisar escrever std:: antes de qualquer operação básica de entrada ou saída
using namespace std;

// dimensões da janela
int windowX = 600;
int windowY = 600;
// controle para arrastar pontos
int moving = -1;
// quantidade de pontos
int quant = 0;
// controle para mostrar/esconder pontos e linhas de construção
bool verPontos = true;
bool verLinhas = true;
// vetor com as coordenadas dos pontos
float pontos[100][3];
// vetor com as dimensões do ortho
float orthoDim[4];

// função auxiliar pra transformar um int em uma string
string toString(int n);
// função auxiliar pra calcular a distância euclidiana entre dois pontos
float dist(float a, float b, float c, float d);
// função auxiliar pra adicionar um ponto na tela
void createPoint(float x, float y);
// função auxiliar pra apagar um ponto da tela
void erasePoint(int p);
// função auxiliar para selecionar um ponto quando o mouse é clicado
void selectPoint(int x, int y, bool left);
// função auxiliar de BSpline que prepara o desenho
void desenhaCurva(void);
// função para desenhar a curva BSpline
void BSpline(float vet[4][2]);
// função para desenhar na tela
void display(void);
// função de callback pra quando a tela for redimensionada
void reshape(int w, int h);
// função de callback do teclado
void keyboard(unsigned char key, int x, int y);
// função de callback do mouse
void mouse(int button, int state, int x, int y);
// função de callback de arrastar o mouse (precisa estar segurando o botão)
void motion(int x, int y);

int main(int argc, char** argv)
{
	// inicializando o programa
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(windowX, windowY);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Curvas Interativas");

	// instruções para o usuário
	cout << endl;
	cout << "  Use o botão esquerdo do mouse para criar ou mover um ponto na tela" << endl;
	cout << "  Use o botão direito do mouse para apagar um ponto da tela" << endl;
	cout << "  Use a tecla \'l\' para mostrar/esconder as linhas de construção" << endl;
	cout << "  Use a tecla \'p\' para mostrar/esconder os pontos" << endl;
	cout << "  Use a tecla \"ESC\" para encerrar o programa" << endl;
	cout << endl;

	// associando funções de callback
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutReshapeFunc(reshape);

	// iniciando o ciclo principal do programa
	glutMainLoop();
	return 0;
}

string toString(int n)
{
	// 0 não entraria no while, então retornamos essa exceção logo de uma vez
	if (n == 0)
		return "0";

	string s = "";

	// adicionamos o último dígito do número (%10) ao final da string em cada iteração e continuamos enquanto n for diferente de 0
	while (n)
	{
		// '0' é para passar o número pro padrão ANSI
		s = s + (char) ('0' + n % 10);

		// dividindo um int por 10, simplesmente descartamos seu último dígito. Ao dividir por 10 um int menor que 10, temos 0
		n /= 10;
	}
	return s;
}

float dist(float a, float b, float c, float d)
{
	return sqrt((c - a) * (c - a) + (d - b) * (d - b));
}

void createPoint(float x, float y)
{
	if (quant < MAXNUM)
	{
		pontos[quant][0] = x;
		pontos[quant][1] = y;
		pontos[quant][2] = 0;

		// assim que o ponto é criado, já podemos associar a variável de movimento 'moving' a ele
		moving = quant++;
	}
}

void erasePoint(int p)
{
	while (++p < quant)
	{
		pontos[p-1][0] = pontos[p][0];
		pontos[p-1][1] = pontos[p][1];
		pontos[p-1][2] = pontos[p][2];
	}
	quant--;
}

void selectPoint(int x, int y, bool left)
{
	// conversão explicada anteriormente
	float a = -(((float) x * (orthoDim[1] - orthoDim[0]) / (float) windowX) / orthoDim[0] + 1);
	float b = -(((float) y * (orthoDim[3] - orthoDim[2]) / (float) windowY) / orthoDim[2] + 1);

	// 'menor' é inicializado como a maior distância possível de forma que qualquer ponto dentro do ortho vai ter que tomar o seu lugar
	float menor = dist(orthoDim[0], orthoDim[2], orthoDim[1], orthoDim[3]);

	// 'p' indica o ponto selecionado. -1 quer dizer que nenhum ponto foi selecionado no final
	int p = -1;

	for (int i = 0; i < quant; i++)
	{
		float d = dist(pontos[i][0], pontos[i][1], a, b);

		// escolhe o ponto mais próximo do cursor do mouse caso ele não esteja muito longe
		// a constante 'LONGE' determina a distância máxima do cursor pro clique funcionar
		if (d < menor && d < LONGE)
		{
			menor = d;
			p = i;
		}
	}

	// se a variável booleana 'left' for falsa, o botão direito foi clicado, o que significa que o ponto deve ser apagado
	if (p >= 0 && !left)
		erasePoint(p);

	// se 'left' for verdadeira, o botão esquerdo foi clicado, o que significa que o usuário pode mover um ponto ou criar um novo
	if (p >= 0 && left)
		moving = p;

	// caso 'p' seja -1, nenhum ponto foi selecionado pois todos estão muito longe. É criado um ponto novo então
	if (p < 0 && left)
		createPoint(a, b);

	// após criar, apagar ou mover um ponto, a tela é atualizada
	glutPostRedisplay();
}

void desenhaCurva(void)
{
	// são necessários pelo menos 4 pontos para desenhar a curva
	if(quant > 3)
	{
		float vet[4][2];

		// é feita uma iteração de 4 em 4 (0, 1, 2, 3) -> (1, 2, 3, 4) -> (2, 3, 4, 5) -> ...
		for(int i = 0; i < quant - 3; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				vet[j][0] = pontos[i+j][0];
				vet[j][1] = pontos[i+j][1];
			}
			// desenhamos o trecho da curva correspondente a esta iteração
			BSpline(vet);
		}
	}
}

void BSpline(float vet[4][2])
{
	float pontoAux[2];
	float intervalo;
	float mtrIntrv[4];

	// matriz de transformação usada nos cálculos a seguir
	float mtrTrnsf[4][4] = {{-1,  3, -3, 1},
				{ 3, -6,  3, 0},
				{-3,  0,  3, 0},
				{ 1,  4,  1, 0}};

	// aqui a linha começa a ser desenhada efetivamente
	glBegin(GL_LINE_STRIP);
		for(int i = 0; i <= THICKN; i++)
		{
			// Cria os intervalos variando de 0 a 1
			intervalo = (float) i / THICKN;

			// Monta a Matriz com os intervalos
			mtrIntrv[0] = intervalo * intervalo * intervalo;
			mtrIntrv[1] = intervalo * intervalo;
			mtrIntrv[2] = intervalo;
			mtrIntrv[3] = 1;

			// Equação para calculo da posição do ponto para um intervalo t
			// S(t) = T * 1/6 * M * P
			for(int j = 0; j < 4; j++)
				for(int k = 0; k < 4; k++)
				{
					pontoAux[0] += mtrIntrv[k] * (1.0 / 6) * mtrTrnsf[k][j] * vet[j][0];
					pontoAux[1] += mtrIntrv[k] * (1.0 / 6) * mtrTrnsf[k][j] * vet[j][1];
				}

			// Cria ponto do intervalo t
			glVertex2f(pontoAux[0], pontoAux[1]);

			// Zera os pontos para o proximo calculo
			for(int j = 0; j < 2; j++)
				pontoAux[j] = 0;
		}
	glEnd();
}

void display(void)
{
	// cor de fundo
	glClearColor(0.8, 0.8, 0.8, 0.0);

	// limpa a tela e o buffer de profundidade
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (verPontos)
	{
		// tamanho de cada ponto
		glPointSize(10);

		// cor vermelha
		glColor3f(1.0, 0.0, 0.0);

		// são desenhados os pontos
		glBegin(GL_POINTS);
			for (int i = 0; i < quant; i++)
				glVertex3fv(pontos[i]);
		glEnd();

		// cor azul com verde (parece azul claro)
		glColor3f(0.0, 1.0, 1.0);

		// são escritos os nomes dos pontos (P0, P1, P2, ..., PN)
		for (int i = 0; i < quant; i++)
		{
			// transformamos o contador numa string (com a letra 'P' na frente) para ser escrito na tela
			string s = 'P' + toString(i);

			// coordenadas do ponto
			glRasterPos2f(pontos[i][0], pontos[i][1]);

			// desenha-se a string com o nome do ponto (cada char é uma iteração)
			for (int j = 0; j < s.size(); j++)
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, s[j]);
		}
	}
	if (verLinhas)
	{
		// cor verde
		glColor3f(0.0, 1.0, 0.0);
		for (int i = 0; i < quant - 1; i++)
		{
			// pegamos um ponto no vetor, o próximo ponto, criamos uma reta e atualizamos o contador
			glBegin(GL_LINES);
				glVertex3f(pontos[i][0], pontos[i][1], pontos[i][2]);
				glVertex3f(pontos[i+1][0], pontos[i+1][1], pontos[i+1][2]);
			glEnd();
		}
	}

	// cor azul
	glColor3f(0.0, 0.0, 1.0);

	// espessura da linha azul
	glLineWidth(2.0);

	// por algum motivo misterioso, quando a função para definir os pontos de controle não está aqui, às vezes é criada uma linha horizontal
	// inicialmente nós imaginamos que era um problema com o buffer, mas após uma série de tentativas e erros, decidimos usar essa função. Funcionou
	glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, quant, &pontos[0][0]);

	// função para desenhar a nossa curva BSpline
	desenhaCurva();

	// no final o buffer é limpo
	glutSwapBuffers();
}

void reshape(int w, int h)
{
	// atualizamos as dimensões da tela
	windowX = w;
	windowY = h;

	// atualizamos o viewport e o ortho como visto no código 03_quad_color_reshape.cpp
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// o ortho fica proporcional como visto no código 03_quad_color_reshape.cpp
	// a função 'reshape' é chamada logo no início do programa, portanto ortho é inicializado aqui
	if (w <= h)
	{
		orthoDim[0] = -1.0;
		orthoDim[1] =  1.0;
		orthoDim[2] = -1 * (GLfloat) h / (GLfloat) w;
		orthoDim[3] =  1 * (GLfloat) h / (GLfloat) w;
	}
	else
	{
		orthoDim[0] = -1 * (GLfloat) w / (GLfloat) h;
		orthoDim[1] =  1 * (GLfloat) w / (GLfloat) h;
		orthoDim[2] = -1.0;
		orthoDim[3] =  1.0;
	}
	glOrtho(orthoDim[0], orthoDim[1], orthoDim[2], orthoDim[3], orthoDim[4], orthoDim[5]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'l':
			// pressionar 'p' mostra/esconde os pontos na tela
			verLinhas = !verLinhas;
			break;
		case 'p':
			// pressionar 'l' mostra/esconde as linhas de construção na tela
			verPontos = !verPontos;
			break;
		case 27:
			// pressionar "ESC" encerra o programa
			exit(0);
			break;
		default:
			// se um botão diferente for pressionado, a função retorna sem chamar glutPostRedisplay()
			return;
	}
	// só acontece quando o usuário pressiona 'l' ou 'p'
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
	// a coordenada y é invertida por padrão
	y = windowY - y;

	// GLUT_DOWN é pra impedir que a função seja chamada duas vezes
	// (uma vez quando o usuário aperta um botão e outra quando ele solta)
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		selectPoint(x, y, true);
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		selectPoint(x, y, false);
}

void motion(int x, int y)
{
	// 'moving' tem valor -1 quando nenhum ponto está sendo arrastado e o id do ponto quando algum está
	if (moving == -1)
		return;

	// a coordenada y é invertida por padrão
	y = windowY - y;

	// transformando as coordenadas da tela em coordenadas do ortho:
	// Proporção = 'T'atual * ('O'final - 'O'inicial) / 'T'max
	// O'atual' = (Proporção + 'O'inicial) / 'O'inicial = Proporção / 'O'inicial + 1
	// a variável é multiplicada por -1 porque 'O'inicial é sempre negativo
	float a = -(((float) x * (orthoDim[1] - orthoDim[0]) / (float) windowX) / orthoDim[0] + 1);
	float b = -(((float) y * (orthoDim[3] - orthoDim[2]) / (float) windowY) / orthoDim[2] + 1);

	// se o ponto está dentro da tela
	if (a > orthoDim[0] && a < orthoDim[1] && b > orthoDim[2] && b < orthoDim[3])
	{
		// atualiza a posição do ponto e manda a tela fazer o desenho de novo
		pontos[moving][0] = a;
		pontos[moving][1] = b;
		glutPostRedisplay();
	}
}
