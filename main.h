#pragma once

struct OBJECT
{
	int x; //x座標
	int y; //y座標
	int vx; //x軸方向の速さ
	int vy; //y軸方向の速さ
	int isState; //存在するか
	int pattern; //敵機の動きのパターン
	int image; //画像
	int width; //画像の幅
	int height; //画像の高さ
	int durability; //耐久力
	int timer; //タイマー
};

//関数のプロトタイプ宣言
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
