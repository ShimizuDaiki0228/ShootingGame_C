#include "DxLib.h";
#include "main.h";
#include <stdlib.h>

//定数の定義
const int SCREEN_WIDTH = 1200, SCREEN_HEIGHT = 720;
const int FPS = 60;
const int IMG_ENEMY_MAX = 5; // 敵の画像の枚数（種類）
const int BULLET_MAX = 100; // 自機が発射する弾の最大数
const int ENEMY_MAX = 100; //敵機の最大数
const int STAGE_DISTANCE = FPS * 60; // ステージの長さ
const int PLAYER_SHIELD_MAX = 8; // 自機のシールドの最大値
enum {ENE_BULLET, ENE_ZAK01, ENE_ZAK02, ENE_ZAK03, ENE_BOSS}; //敵機の種類

//ゲームに使用する変数や配列を定義
int _imgGalaxy, _imgFloor, _imgWallL, _imgWallR; // 背景画像
int _imgFighter, _imgBullet; //自機と自機の弾の画像
int _imgEnemy[IMG_ENEMY_MAX]; //敵機の画像
int _imgExplosion;
int _imgItem; // アイテムの画像
int _bgm, _gameOverSE, _gameClearSE, _explosionSE, _itemSE, _shotSE;
int _distance = 0; // ステージ終端までの距離
int _bossIdx = 0;
int _stage = 1;
int _score = 0;
int _highScore = 10000; 
int _noDamageFrameCount = 0; // 無敵状態時の時間を保持


struct OBJECT player; //自機用の構造体変数
struct OBJECT bullet[BULLET_MAX]; //弾用の構造体の配列
struct OBJECT enemy[ENEMY_MAX];

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

	_distance = STAGE_DISTANCE;

	while (1)
	{
		ClearDrawScreen();

		scrollBG(1);
		if (_distance > 0) _distance--;
		DrawFormatString(0, 0, 0xffff00, "Score = %d, HighScore = %d", _score, _highScore);
		
		//雑魚機の出現
		if (_distance % 60 == 1)
		{
			int x = 100 + rand() % (SCREEN_WIDTH - 200); //出現位置x座標。端100以内には出てこないように
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

		//雑魚機3の出現
		if (_distance % 120 == 1)
		{
			int x = 100 + rand() % (SCREEN_WIDTH - 200); // 出現位置x座標
			setEnemy(x, -100, 0, 40 + rand() % 20, ENE_ZAK03, _imgEnemy[ENE_ZAK03], 5);
		}

		//ボス出現
		if (_distance == 1) _bossIdx = setEnemy(SCREEN_WIDTH / 2, -120, 0, 1, ENE_BOSS, _imgEnemy[ENE_BOSS], 200);

		//自機のシールドなどのパラメーターを表示
		drawParameter();

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
	player.shield = PLAYER_SHIELD_MAX;
	GetGraphSize(_imgFighter, &player.width, &player.height);
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
	static int oldSpaceKey; //1つ前のスペースキーの状態を保持する変数
	static int countSpaceKey; //スペースキーを押し続けている間、カウントアップする変数
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
		if (oldSpaceKey == 0) setBullet(); // スペースキーを押した瞬間発射する。
		else if (countSpaceKey % 10 == 0) setBullet(); // スペースキーを押し続けている間、一定間隔で発射する。
		countSpaceKey++;
	}
	else
	{
		countSpaceKey = 0;
	}
	oldSpaceKey = CheckHitKey(KEY_INPUT_SPACE); // スペースキーの状態を保持
	if (_noDamageFrameCount > 0) _noDamageFrameCount--;
	//　被ダメ時は2フレームに一回、通常時は常に表示
	if(_noDamageFrameCount % 4 < 2) drawImage(_imgFighter, player.x, player.y);
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

/// <summary>
/// 敵機のセット
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
			enemy[i].shield = durability * _stage; //ステージが進むごとに強くなる
			GetGraphSize(img, &enemy[i].width, &enemy[i].height); 
			return i;
		}
	}
	return -1;
}

/// <summary>
/// 敵機を動かす
/// </summary>
/// <param name=""></param>
void moveEnemy(void)
{
	for (int i = 0; i < ENEMY_MAX; i++)
	{
		if (enemy[i].isState == 0) continue;
		
		//敵機3の特殊行動パターン
		//高速から低速になり、弾で攻撃すると逃げるように移動する
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

		//ボスの特殊行動パターン
		//画面の下側まで来ると上に移動する
		//画面の上に来ると弾で攻撃し、再度下に移動してくる
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

		//敵機に共通の行動パターン
		enemy[i].x += enemy[i].vx;
		enemy[i].y += enemy[i].vy;
		drawImage(enemy[i].image, enemy[i].x, enemy[i].y);

		//画面外に移動するとデータを破棄する
		if (enemy[i].x < -200
			|| SCREEN_WIDTH + 200 < enemy[i].x
			|| enemy[i].y < -200
			|| SCREEN_HEIGHT + 200 < enemy[i].y) enemy[i].isState = 0;

		//敵機のシールド
		if (enemy[i].shield > 0)
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

		// 自機が通常時の場合に触れた時の処理
		if (_noDamageFrameCount == 0)
		{
			int dx = abs(enemy[i].x - player.x);
			int dy = abs(enemy[i].y - player.y);
			if (dx < enemy[i].width / 2 + player.width / 2 && dy < enemy[i].height / 2 + player.height / 2)
			{
				if (player.shield > 0) player.shield--;
				_noDamageFrameCount = FPS;
			}
		}
	}
}

/// <summary>
/// 敵にダメージを与える
/// </summary>
/// <param name="n">n番目の敵</param>
/// <param name="damage">ダメージ量</param>
void damageEnemy(int n, int damage)
{
	SetDrawBlendMode(DX_BLENDMODE_ADD, 192);
	DrawCircle(enemy[n].x, enemy[n].y, (enemy[n].width + enemy[n].height) / 4, 0xff0000, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	_score += 100;
	if (_score > _highScore) _highScore = _score;
	enemy[n].shield -= damage;
	if (enemy[n].shield <= 0)
	{
		enemy[n].isState = 0;
	}
}

/// <summary>
/// 影を付けた文字列と値を表示する関数
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="txt"></param>
/// <param name="val"></param>
/// <param name="col"></param>
/// <param name="fontSize"></param>
void drawText(int x, int y, const char* txt, int val, int col, int fontSize)
{
	SetFontSize(fontSize);
	DrawFormatString(x + 1, y + 1, 0x000000, txt, val);
	DrawFormatString(x, y, col, txt, val);
}

/// <summary>
/// 自機に関するパラメーターを表示
/// </summary>
/// <param name=""></param>
void drawParameter(void)
{
	int x = 10; int y = SCREEN_HEIGHT - 30;
	DrawBox(x, y, x + PLAYER_SHIELD_MAX * 30, y + 20, 0x000000, TRUE);
	for (int i = 0; i < player.shield; i++)
	{
		//自機のパラメーターの色を計算して表示する
		int r = 128 * (PLAYER_SHIELD_MAX - i) / PLAYER_SHIELD_MAX;
		int g = 255 * i / PLAYER_SHIELD_MAX;
		int b = 160 + 96 * i / PLAYER_SHIELD_MAX;
		DrawBox(x + 2 + i * 30, y + 2, x + 28 + i * 30, y + 18, GetColor(r, g, b), TRUE);
	}
	drawText(x, y - 25, "SHILED LV %02d", player.shield, 0xffffff, 20);

}

/// <summary>
/// ステージマップ
/// </summary>
/// <param name=""></param>
void stageMap(void)
{
	int mx = SCREEN_WIDTH - 30, my = 60; //マップ表示位置
	int width = 20, height = SCREEN_HEIGHT - 120; //マップの幅、高さ
	int pos = (SCREEN_HEIGHT - 140) * _distance / STAGE_DISTANCE; // 自機の飛行している位置
	SetDrawBlendMode(DX_BLENDMODE_SUB, 128); // 減算による描画の重ね合わせ
	DrawBox(mx, my, mx + width, my + height, 0xffffff, TRUE); //ステージの位置を表示するバー
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawBox(mx - 1, my - 1, mx + width + 1, my + height + 1, 0xffffff, FALSE); //バーの枠線
	DrawBox(mx, my + pos, mx + width, my + pos + 20, 0x0080ff, TRUE); //自機の位置
}