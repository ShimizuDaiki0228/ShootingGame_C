#include "DxLib.h";
#include "main.h";
#include <stdlib.h>

//�萔�̒�`
const int SCREEN_WIDTH = 1200, SCREEN_HEIGHT = 720;
const int FPS = 60;
const int IMG_ENEMY_MAX = 5; // �G�̉摜�̖����i��ށj
const int BULLET_MAX = 100; // ���@�����˂���e�̍ő吔
const int ENEMY_MAX = 100; //�G�@�̍ő吔
const int STAGE_DISTANCE = FPS * 60; // �X�e�[�W�̒���
enum {ENE_BULLET, ENE_ZAK01, ENE_ZAK02, ENE_ZAK03, ENE_BOSS}; //�G�@�̎��

//�Q�[���Ɏg�p����ϐ���z����`
int _imgGalaxy, _imgFloor, _imgWallL, _imgWallR; // �w�i�摜
int _imgFighter, _imgBullet; //���@�Ǝ��@�̒e�̉摜
int _imgEnemy[IMG_ENEMY_MAX]; //�G�@�̉摜
int _imgExplosion;
int _imgItem; // �A�C�e���̉摜
int _bgm, _gameOverSE, _gameClearSE, _explosionSE, _itemSE, _shotSE;
int _distance = 0; // �X�e�[�W�I�[�܂ł̋���
int _bossIdx = 0;
int _stage = 1;
int _score = 0;
int _highScore = 10000; 

struct OBJECT player; //���@�p�̍\���̕ϐ�
struct OBJECT bullet[BULLET_MAX]; //�e�p�̍\���̂̔z��
struct OBJECT enemy[ENEMY_MAX];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetWindowText("�V���[�e�B���O�Q�[��");
	SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
	ChangeWindowMode(TRUE);
	if (DxLib_Init() == -1) return -1;
	SetBackgroundColor(0, 0, 0);
	SetDrawScreen(DX_SCREEN_BACK);

	initGame();
	initVariable();
	PlaySoundMem(_bgm, DX_PLAYTYPE_LOOP);

	_distance = STAGE_DISTANCE;

	while (1)
	{
		ClearDrawScreen();

		scrollBG(1);
		if (_distance > 0) _distance--;
		DrawFormatString(0, 0, 0xffff00, "Score = %d, HighScore = %d", _score, _highScore);
		
		//�G���@�̏o��
		if (_distance % 60 == 1)
		{
			int x = 100 + rand() % (SCREEN_WIDTH - 200); //�o���ʒux���W�B�[100�ȓ��ɂ͏o�Ă��Ȃ��悤��
			int y = -50;
			int enemyType = 1 + rand() % 2;
			if (enemyType == ENE_ZAK01) setEnemy(x, y, 0, 3, ENE_ZAK01, _imgEnemy[ENE_ZAK01], 1);
			else if (enemyType == ENE_ZAK02)
			{
				int vx = 0;
				if (player.x < x - 50) vx -= 3;
				else if (player.x > x + 50) vx += 3;
				setEnemy(x, y, vx, 5, ENE_ZAK02, _imgEnemy[ENE_ZAK02], 3);
			}
		}

		//�G���@3�̏o��
		if (_distance % 120 == 1)
		{
			int x = 100 + rand() % (SCREEN_WIDTH - 200); // �o���ʒux���W
			setEnemy(x, -100, 0, 40 + rand() % 20, ENE_ZAK03, _imgEnemy[ENE_ZAK03], 5);
		}

		//�{�X�o��
		if (_distance == 1) _bossIdx = setEnemy(SCREEN_WIDTH / 2, -120, 0, 1, ENE_BOSS, _imgEnemy[ENE_BOSS], 200);

		moveEnemy();

		movePlayer();
		moveBullet();
		stageMap();

		ScreenFlip();
		WaitTimer(1000 / FPS);
		if (ProcessMessage() == -1) break;
		if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break;
	}

	DxLib_End();
	return 0;
}

//�������p�̊֐�
void initGame(void)
{
	// �w�i�p�̉摜�̓ǂݍ���
	_imgGalaxy = LoadGraph("image/bg0.png");
	_imgFloor = LoadGraph("image/bg1.png");
	_imgWallL = LoadGraph("image/bg2.png");
	_imgWallR = LoadGraph("image/bg3.png");

	// ���@�Ǝ��@�̒e�̉摜�̓ǂݍ���
	_imgFighter = LoadGraph("image/fighter.png");
	_imgBullet = LoadGraph("image/bullet.png");

	//�G�@�̉摜�̓ǂݍ���
	for (int i = 0; i < IMG_ENEMY_MAX; i++)
	{
		char file[] = "image/enemy*.png";
		file[11] = (char)('0' + i);
		_imgEnemy[i] = LoadGraph(file);
	}

	_imgExplosion = LoadGraph("image/explosion.png");
	_imgItem = LoadGraph("image/item.png");

	_bgm = LoadSoundMem("sound/bgm.mp3");
	_gameOverSE = LoadSoundMem("sound/gameover.mp3");
	_gameClearSE = LoadSoundMem("sound/stageclear.mp3");
	_explosionSE = LoadSoundMem("sound/explosion.mp3");
	_itemSE = LoadSoundMem("sound/item.mp3");
	_shotSE = LoadSoundMem("sound/shot.mp3");
	ChangeVolumeSoundMem(128, _bgm);
	ChangeVolumeSoundMem(128, _gameOverSE);
	ChangeVolumeSoundMem(128, _gameClearSE);
}

/// <summary>
/// �Q�[���J�n���̏����l��������֐�
/// </summary>
/// <param name=""></param>
void initVariable(void)
{
	player.x = SCREEN_WIDTH / 2;
	player.y = SCREEN_HEIGHT / 2;
	player.vx = 5;
	player.vy = 5;
}


void drawImage(int img, int x, int y)
{
	int w, h;
	GetGraphSize(img, &w, &h);
	DrawGraph(x - w / 2, y - h / 2, img, TRUE);
}

/// <summary>
/// ���@�𓮂����֐�
/// </summary>
/// <param name=""></param>
void movePlayer(void)
{
	static int oldSpaceKey; //1�O�̃X�y�[�X�L�[�̏�Ԃ�ێ�����ϐ�
	static int countSpaceKey; //�X�y�[�X�L�[�����������Ă���ԁA�J�E���g�A�b�v����ϐ�
	if (CheckHitKey(KEY_INPUT_UP))
	{
		player.y -= player.vy;
		if (player.y < 30) player.y = 30;
	}
	if (CheckHitKey(KEY_INPUT_DOWN))
	{
		player.y += player.vy;
		if (player.y > SCREEN_HEIGHT - 30) player.y = SCREEN_HEIGHT - 30;
	}
	if (CheckHitKey(KEY_INPUT_LEFT))
	{
		player.x -= player.vx;
		if (player.x < 30) player.x = 30;
	}
	if (CheckHitKey(KEY_INPUT_RIGHT))
	{
		player.x += player.vx;
		if (player.x > SCREEN_WIDTH - 30) player.x = SCREEN_WIDTH - 30;
	}

	if (CheckHitKey(KEY_INPUT_SPACE))
	{
		if (oldSpaceKey == 0) setBullet(); // �X�y�[�X�L�[���������u�Ԕ��˂���B
		else if (countSpaceKey % 10 == 0) setBullet(); // �X�y�[�X�L�[�����������Ă���ԁA���Ԋu�Ŕ��˂���B
		countSpaceKey++;
	}
	else
	{
		countSpaceKey = 0;
	}
	oldSpaceKey = CheckHitKey(KEY_INPUT_SPACE); // �X�y�[�X�L�[�̏�Ԃ�ێ�
	drawImage(_imgFighter, player.x, player.y);
}

/// <summary>
/// �w�i���X�N���[�����ĕ\��������
/// </summary>
/// <param name="spd"></param>
void scrollBG(int spd)
{
	static int galaxyY, floorY, wallY;
	galaxyY = (galaxyY + spd) % SCREEN_HEIGHT;
	DrawGraph(0, galaxyY - SCREEN_HEIGHT, _imgGalaxy, FALSE);
	DrawGraph(0, galaxyY, _imgGalaxy, FALSE);
	floorY = (floorY + spd * 2) % 120;
	for (int i = -1; i < 6; i++) DrawGraph(240, floorY + i * 120, _imgFloor, TRUE);
	wallY = (wallY + spd * 4) % 240;
	DrawGraph(0, wallY - 240, _imgWallL, TRUE);
	DrawGraph(SCREEN_WIDTH - 300, wallY - 240, _imgWallR, TRUE);
}

/// <summary>
/// �e�̃Z�b�g
/// </summary>
/// <param name=""></param>
void setBullet(void)
{
	for (int i = 0; i < BULLET_MAX; i++)
	{
		if (bullet[i].isState == 0)
		{
			bullet[i].x = player.x;
			bullet[i].y = player.y - 20;
			bullet[i].vx = 0;
			bullet[i].vy = -20;
			bullet[i].isState = 1;
			break;
		}
	}
}

/// <summary>
/// �e�̈ړ�
/// </summary>
/// <param name=""></param>
void moveBullet(void)
{
	for (int i = 0; i < BULLET_MAX; i++)
	{
		if (bullet[i].isState == 0) continue;
		bullet[i].x += bullet[i].vx;
		bullet[i].y += bullet[i].vy;
		drawImage(_imgBullet, bullet[i].x, bullet[i].y);
		if (bullet[i].y < -100) bullet[i].isState = 0;
	}
}

/// <summary>
/// �G�@�̃Z�b�g
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="vx"></param>
/// <param name="vy"></param>
/// <param name="pettern"></param>
/// <param name="img"></param>
/// <param name="durability"></param>
/// <returns></returns>
int setEnemy(int x, int y, int vx, int vy, int pattern, int img, int durability)
{
	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (enemy[i].isState == 0)
		{
			enemy[i].x = x;
			enemy[i].y = y;
			enemy[i].vx = vx;
			enemy[i].vy = vy;
			enemy[i].isState = 1;
			enemy[i].pattern = pattern;
			enemy[i].image = img;
			enemy[i].durability = durability * _stage; //�X�e�[�W���i�ނ��Ƃɋ����Ȃ�
			GetGraphSize(img, &enemy[i].width, &enemy[i].height); 
			return i;
		}
	}
	return -1;
}

/// <summary>
/// �G�@�𓮂���
/// </summary>
/// <param name=""></param>
void moveEnemy(void)
{
	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (enemy[i].isState == 0) continue;
		if (enemy[i].pattern == ENE_ZAK03)
		{
			if (enemy[i].vy > 1)
			{
				enemy[i].vy *= 0.9;
			}
			else if (enemy[i].vy > 0)
			{
				setEnemy(enemy[i].x, enemy[i].y, 0, 6, ENE_BULLET, _imgEnemy[ENE_BULLET], 0);
				enemy[i].vx = 8;
				enemy[i].vy = -4;
			}
		}
		if (enemy[i].pattern == ENE_BOSS)
		{
			if (enemy[i].y > SCREEN_HEIGHT - 120) enemy[i].vy = -2;
			if (enemy[i].y < 120)
			{
				if (enemy[i].vy < 0)
				{
					for (int bx = -2; bx <= 2; bx++)
					{
						for (int by = 0; by <= 3; by++)
						{
							if (bx == 0 && by == 0) continue;
							setEnemy(enemy[i].x, enemy[i].y, bx * 2, by * 2, ENE_BULLET, _imgEnemy[ENE_BULLET], 0);
						}
					}
					enemy[i].vy = 2;
				}
			}
		}
		enemy[i].x += enemy[i].vx;
		enemy[i].y += enemy[i].vy;
		drawImage(enemy[i].image, enemy[i].x, enemy[i].y);
		if (enemy[i].x < -200
			|| SCREEN_WIDTH + 200 < enemy[i].x
			|| enemy[i].y < -200
			|| SCREEN_HEIGHT + 200 < enemy[i].y) enemy[i].isState = 0;

		if (enemy[i].durability > 0)
		{
			for (int j = 0; j < BULLET_MAX; j++)
			{
				if (bullet[j].isState == 0) continue;
				int dx = abs((int)(bullet[j].x - enemy[i].x));
				int dy = abs((int)(bullet[j].y - enemy[i].y));
				if (dx < enemy[i].width / 2 && dy < enemy[i].height / 2)
				{
					bullet[i].isState = 0;
					damageEnemy(i, 1);
				}
			}
		}
	}
}

/// <summary>
/// �G�Ƀ_���[�W��^����
/// </summary>
/// <param name="n">n�Ԗڂ̓G</param>
/// <param name="damage">�_���[�W��</param>
void damageEnemy(int n, int damage)
{
	SetDrawBlendMode(DX_BLENDMODE_ADD, 192);
	DrawCircle(enemy[n].x, enemy[n].y, (enemy[n].width + enemy[n].height) / 4, 0xff0000, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	_score += 100;
	if (_score > _highScore) _highScore = _score;
	enemy[n].durability -= damage;
	if (enemy[n].durability <= 0)
	{
		enemy[n].isState = 0;
	}
}

/// <summary>
/// �X�e�[�W�}�b�v
/// </summary>
/// <param name=""></param>
void stageMap(void)
{
	int mx = SCREEN_WIDTH - 30, my = 60; //�}�b�v�\���ʒu
	int width = 20, height = SCREEN_HEIGHT - 120; //�}�b�v�̕��A����
	int pos = (SCREEN_HEIGHT - 140) * _distance / STAGE_DISTANCE; // ���@�̔�s���Ă���ʒu
	SetDrawBlendMode(DX_BLENDMODE_SUB, 128); // ���Z�ɂ��`��̏d�ˍ��킹
	DrawBox(mx, my, mx + width, my + height, 0xffffff, TRUE); //�X�e�[�W�̈ʒu��\������o�[
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawBox(mx - 1, my - 1, mx + width + 1, my + height + 1, 0xffffff, FALSE); //�o�[�̘g��
	DrawBox(mx, my + pos, mx + width, my + pos + 20, 0x0080ff, TRUE); //���@�̈ʒu
}