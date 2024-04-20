#pragma once

struct OBJECT
{
	int x; //x���W
	int y; //y���W
	int vx; //x�������̑���
	int vy; //y�������̑���
	int isState; //���݂��邩
	int pattern; //�G�@�̓����̃p�^�[��
	int image; //�摜
	int width; //�摜�̕�
	int height; //�摜�̍���
	int durability; //�ϋv��
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
int setEnemy(int x, int y, int vx, int vy, int pattern, int img, int durability);
void moveEnemy(void);
void stageMap(void);
void damageEnemy(int n, int damage);
