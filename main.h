#pragma once

struct OBJECT
{
	int x; //x���W
	int y; //y���W
	double vx; //x�������̑���
	double vy; //y�������̑���
	int isState; //���݂��邩
	int pattern; //�G�@�̓����̃p�^�[��
	int image; //�摜
	int width; //�摜�̕�
	int height; //�摜�̍���
	int shield; //�ϋv��
	int timer; //�^�C�}�[
};

//�֐��̃v���g�^�C�v�錾
void initGame(void);
void scrollBG(int spd);
void initVariable(void);
void drawImage(int img, int x, int y);
void movePlayer(void);
void setBullet(void);
void moveBullet(void);
int setEnemy(int x, int y, double vx, double vy, int pattern, int img, int shield);
void moveEnemy(void);
void stageMap(void);
void damageEnemy(int n, int damage);
void drawText(int x, int y, const char* txt, int val, int col, int fontSize);
void drawParameter(void);
void setEffect(int x, int y, int pattern);
void drawEffect(void);
void setItem(void);
void moveItem(void);
void drawTextCenter(int x, int y, const char* txt, int col, int fontSize);
void drawTextBlinking(int x, int y, const char* txt, int col, int fontSize);
void drawTextFade(int x, int y, const char* txt, int col, int fontSize, int alpha);
double EaseOutCubic(double x);
int CalculateAlphaEaseOutCubicMethod(int time, int duration, int alpha);