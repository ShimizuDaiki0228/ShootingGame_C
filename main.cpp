#include "DxLib.h";
#include "main.h";

//定数の定義
const int SCREEN_WIDTH = 1200, SCREEN_HEIGHT = 720;
const int FPS = 60;
const int IMG_ENEMY_MAX = 5; // 敵の画像の枚数（種類）
const int BULLET_MAX = 100; // 自機が発射する弾の最大数

//ゲームに使用する変数や配列を定義
int _imgGalaxy, _imgFloor, _imgWallL, _imgWallR; // 背景画像
int _imgFighter, _imgBullet; //自機と自機の弾の画像
int _imgEnemy[IMG_ENEMY_MAX]; //敵機の画像
int _imgExplosion;
int _imgItem; // アイテムの画像
int _bgm, _gameOverSE, _gameClearSE, _explosionSE, _itemSE, _shotSE;

struct OBJECT player; //自機用の構造体変数
struct OBJECT bullet[BULLET_MAX]; //弾用の構造体の配列

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetWindowText("シューティングゲーム");
	SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
	ChangeWindowMode(TRUE);
	if (DxLib_Init() == -1) return -1;
	SetBackgroundColor(0, 0, 0);
	SetDrawScreen(DX_SCREEN_BACK);

	initGame();
	initVariable();
	PlaySoundMem(_bgm, DX_PLAYTYPE_LOOP);

	while (1)
	{
		ClearDrawScreen();

		scrollBG(1);
		movePlayer();
		moveBullet();

		ScreenFlip();
		WaitTimer(1000 / FPS);
		if (ProcessMessage() == -1) break;
		if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break;
	}

	DxLib_End();
	return 0;
}

//初期化用の関数
void initGame(void)
{
	// 背景用の画像の読み込み
	_imgGalaxy = LoadGraph("image/bg0.png");
	_imgFloor = LoadGraph("image/bg1.png");
	_imgWallL = LoadGraph("image/bg2.png");
	_imgWallR = LoadGraph("image/bg3.png");

	// 自機と自機の弾の画像の読み込み
	_imgFighter = LoadGraph("image/fighter.png");
	_imgBullet = LoadGraph("image/bullet.png");

	//敵機の画像の読み込み
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
/// ゲーム開始時の初期値を代入する関数
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
/// 自機を動かす関数
/// </summary>
/// <param name=""></param>
void movePlayer(void)
{
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
	if (CheckHitKey(KEY_INPUT_SPACE)) setBullet();
	drawImage(_imgFighter, player.x, player.y);
}

/// <summary>
/// 背景をスクロールして表示させる
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
/// 弾のセット
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
/// 弾の移動
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