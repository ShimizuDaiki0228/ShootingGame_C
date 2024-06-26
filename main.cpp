#include "DxLib.h";
#include "main.h";
#include <stdlib.h>
#include <math.h>
#include <iostream>

//定数の定義
const int SCREEN_WIDTH = 1200, SCREEN_HEIGHT = 720;
const int FPS = 60;
const int IMG_ENEMY_MAX = 5; // 敵の画像の枚数（種類）
const int BULLET_MAX = 100; // 自機が発射する弾の最大数
const int ENEMY_MAX = 100; //敵機の最大数
const int STAGE_DISTANCE = FPS * 10; // ステージの長さ
const int PLAYER_SHIELD_MAX = 8; // 自機のシールドの最大値
const int EFFECT_MAX = 100; //エフェクトの最大数
const int ITEM_TYPE = 3; //アイテムの種類
const int WEAPON_LV_MAX = 10; // 武器レベルの最大値
const int PLAYER_SPEED_MAX = 20; // 自機の速さの最大値
const int FIRST_PLAYER_SPEED = -35; // ゲーム開始時に画面外から出現してくる自機の速さ
const int START_WARNING_TIME = 500; //ボスが登場する前の警告を表示し始める時間
enum { ENE_BULLET, ENE_ZAK01, ENE_ZAK02, ENE_ZAK03, ENE_BOSS }; //敵機の種類
enum { EFF_EXPLODE, EFF_RECOVER }; // エフェクトの種類
enum { TITLE, PLAY, GAMEOVER, CLEAR }; //シーンの種類
//ポーズ画面で選択できるテキスト一覧
const char* PAUSE_OPTIONS[] = {
		"Press SPACE to return to Game",
		"Press SPACE to return to Title",
		"Press SPACE to restart to Game"
};
const int PAUSE_OPTIONS_SIZE = sizeof(PAUSE_OPTIONS) / sizeof(PAUSE_OPTIONS[0]);
//タイトル画面で選択できるテキスト一覧
const char* TITLE_OPTIONS[] = {
	"Press SPACE to Start.",
	"Press SPACE to Tutorial."
};
const int TITLE_OPTIONS_SIZE = sizeof(TITLE_OPTIONS) / sizeof(TITLE_OPTIONS[0]);

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
int _weaponLV = 1; // 自機の武器のレベル（同時に発射される弾数）
int _scene = TITLE;
int _timer = 0; // 時間の進行を管理
bool _isPlayerReady; //自機を動かす準備ができているかどうか。はじめは画面下に隠れており画面内にでてくるとtrueになる
int _currentPlayerVYTemp; //ステージ更新ごとにvyを変更する必要があるため、別の変数で保存しておきたい
int _gameoverDelayTimeMag = 1; // ゲームオーバー時の爆発時にスローにするための倍率
int _shakeDuration; //画面の揺れを起こす残り時間
int _shakeMagnitude; // 画面の揺れの強さ
int _shakeOffsetX;
int _shakeOffsetY;

struct OBJECT player; //自機用の構造体変数
struct OBJECT bullet[BULLET_MAX]; //弾用の構造体の配列
struct OBJECT enemy[ENEMY_MAX];
struct OBJECT effect[EFFECT_MAX]; // エフェクト用の構造体の配列
struct OBJECT item;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetWindowText("シューティングゲーム");
	SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
	ChangeWindowMode(TRUE);
	if (DxLib_Init() == -1) return -1;
	SetBackgroundColor(0, 0, 0);
	SetDrawScreen(DX_SCREEN_BACK);

	initGame();

	_distance = STAGE_DISTANCE;
	int titleTextAlpha = 0;
	int titleBlackLayerAlpha = 255; //　クリエイターの名前を表示する黒画面の透明度
	int titleTextHeight = SCREEN_HEIGHT / 2; //タイトルテキストを上から表示する際の初期位置
	bool isCreatorScreenFinished = false;
	int warningScreenAlpha = 0; // ボスが出現する前に表示する警告画面の透明度

	bool isGamePause = false; // ゲームがポーズ状態かどうか
	int selectTextNumber = 0; //ポーズ画面のどのテキストを選択しているか
	int oldKeyUpInTextSelect = 0; //ポーズ画面で長押しで選択されないように最後に上ボタンが押されたタイミングを監視する
	int oldKeyDownInTextSelect = 0; //ポーズ画面で長押しで選択されないように最後に下ボタンが押されたタイミングを監視する

	while (1)
	{
		ClearDrawScreen();


		int spd = 1;
		static int oldSpaceKey; //画面遷移で連続して次の画面に行かないように
		
		if ((_scene == PLAY && _distance == 0 ) || isGamePause) spd = 0; // ボス戦はスクロール停止

		scrollBG(spd); //背景のスクロール

		if (!isGamePause)
		{
			moveEnemy(); //敵機の制御
			moveBullet(); //弾の制御
			moveItem(); //アイテムの制御
			drawEffect(); //エフェクトの描画
			stageMap(); // ステージマップ
			drawParameter(); //自機のシールドなどのパラメーターを表示

			// スコア、ハイスコア、ステージ数の表示
			drawText(10, 10, "SCORE %07d", _score, 0xffffff, 30);
			drawText(SCREEN_WIDTH - 300, 10, "HIGHSCORE %07d", _highScore, 0xffffff, 30);
			drawText(SCREEN_WIDTH - 145, SCREEN_HEIGHT - 40, "STAGE %02d", _stage, 0xffffff, 30);
		}
		

		
		_timer++;
		switch (_scene)
		{
		case TITLE:
			if (titleBlackLayerAlpha > 0)
			{
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, titleBlackLayerAlpha);
				DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(0, 0, 0), TRUE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			}
			
			if (50 <= _timer && _timer < 200)
			{
				if (titleTextAlpha < 255) titleTextAlpha += 5;
				drawTextFade(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.3, "CREATED BY", 0xffffff, 50, titleTextAlpha);
				drawTextFade(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.7, "DAIKI SHIMIZU", 0xffffff, 50, titleTextAlpha);
			}
			else if (200 <= _timer && !isCreatorScreenFinished)
			{
				titleTextAlpha -= 5;
				if (titleTextAlpha < 0)
				{
					titleTextAlpha = 0;
					isCreatorScreenFinished = true;
					PlaySoundMem(_bgm, DX_PLAYTYPE_LOOP);
				}
				drawTextFade(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.3, "CREATED BY", 0xffffff, 50, titleTextAlpha);
				drawTextFade(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.7, "DAIKI SHIMIZU", 0xffffff, 50, titleTextAlpha);
			}

			if (isCreatorScreenFinished)
			{
				if(titleBlackLayerAlpha > 0) titleBlackLayerAlpha--;
				if (titleTextHeight > 0)
				{
					titleTextHeight -= 2;
					if (CheckHitKey(KEY_INPUT_SPACE) && oldSpaceKey == 0)
					{
						titleBlackLayerAlpha = 0;
						titleTextHeight = 0;
						oldSpaceKey = 1;
					}
				}
				
				drawTextCenter(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.3 - titleTextHeight, "Shooting Game", 0xffffff, 80);

				if (titleTextHeight <= 0)
				{
					selectText(TITLE_OPTIONS, TITLE_OPTIONS_SIZE, 0.7, 0.1, &selectTextNumber, &oldKeyUpInTextSelect, &oldKeyDownInTextSelect);

					if (CheckHitKey(KEY_INPUT_SPACE) && oldSpaceKey == 0)
					{
						resetSelectTextParameter(&selectTextNumber, &oldKeyUpInTextSelect, &oldKeyDownInTextSelect, &oldSpaceKey);
						titleTextHeight = SCREEN_HEIGHT / 2;
						initVariable();
						_scene = PLAY;
					}
				}
			}
			
			break;

		case PLAY:

			if (CheckHitKey(KEY_INPUT_LSHIFT)) isGamePause = true;
			if (isGamePause)
			{
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
				DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x000000, TRUE);
				selectText(PAUSE_OPTIONS, PAUSE_OPTIONS_SIZE, 0.25, 0.25, &selectTextNumber, &oldKeyUpInTextSelect, &oldKeyDownInTextSelect);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

				if (CheckHitKey(KEY_INPUT_SPACE))
				{
					if (selectTextNumber == 1)
					{
						//敵機及び自機のリセットのため、ゲーム開始時と一部処理が重なっているが仕方なし
						initVariable();
						_scene = TITLE;
					}
					else if(selectTextNumber == 2) initVariable();

					resetSelectTextParameter(&selectTextNumber, &oldKeyUpInTextSelect, &oldKeyDownInTextSelect, &oldSpaceKey);
					isGamePause = false;
				}
				
			}
			else
			{
				if (!_isPlayerReady)
				{
					player.y += player.vy;
					player.vy *= 0.8;
					drawImage(_imgFighter, player.x, player.y);
					if (player.y <= SCREEN_HEIGHT - 200)
					{
						_isPlayerReady = true;
						player.vy = _currentPlayerVYTemp;
					}
				}
				else
				{
					movePlayer(); //自機の制御
					if (_distance > 0) _distance--;

					//雑魚機の出現
					//ボスが出現する前に警告画面を表示したいので、一定距離以下では出現しないように
					if (START_WARNING_TIME < _distance && _distance % 20 == 0)
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
					if (START_WARNING_TIME < _distance && _distance < 900 && _distance % 30 == 0)
					{
						int x = 100 + rand() % (SCREEN_WIDTH - 200); // 出現位置x座標
						int y = -100;
						int vy = 40 + rand() % 20;
						setEnemy(x, y, 0, vy, ENE_ZAK03, _imgEnemy[ENE_ZAK03], 5);
					}

					//ボス登場前に警告画面を表示する
					if (_distance <= START_WARNING_TIME - 200 && 1 < _distance)
					{
						if (_distance == START_WARNING_TIME - 200) _timer = 0;

						warningScreenAlpha = calculateAlphaEaseOutCubicMethod(_timer, 60, 170);
						SetDrawBlendMode(DX_BLENDMODE_ALPHA, warningScreenAlpha);
						drawTextCenter(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, "WARNING!!!", 0xffff00, 120);
						DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xff0000, TRUE);
						SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
					
					}

					//ボス出現
					if (_distance == 1) _bossIdx = setEnemy(SCREEN_WIDTH / 2, -120, 0, 1, ENE_BOSS, _imgEnemy[ENE_BOSS], 200);

					//アイテムの出現
					if (_distance % 800 == 1) setItem();

					//自機の体力がなくなった場合
					if (player.shield == 0)
					{
						StopSoundMem(_bgm); //BGM停止
						_scene = GAMEOVER;
						_timer = 0; //自機の爆発演出時間を制御するため
						break;
					}
				}
			}

			
			break;

		case GAMEOVER:
			if (_timer == 1)
			{
				WaitTimer(1000); // 時間を一時的に止める
				_gameoverDelayTimeMag = 3;
			}
			else if(_timer <= FPS)
			{
				if (_timer < FPS)
				{
					if (_timer % 7 == 0) setEffect(player.x + rand() % 81 - 40, player.y + rand() % 81 - 40, EFF_EXPLODE);
				}
				else if (_timer == FPS)
				{
					PlaySoundMem(_gameOverSE, DX_PLAYTYPE_BACK);
					_gameoverDelayTimeMag = 1;
				}
			}
			else
			{
				drawTextCenter(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.3, "GAME OVER", 0xff0000, 80);
			}
			

			if (_timer > FPS * 5)
			{
				drawTextBlinking(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.7, "Press SPACE to title.", 0xffffff, 30);
				if (CheckHitKey(KEY_INPUT_SPACE))
				{
					_scene = TITLE;
					StopSoundMem(_gameOverSE);
					PlaySoundMem(_bgm, DX_PLAYTYPE_LOOP);
				}
			}
			break;

		case CLEAR: 
			movePlayer(); // 自機の制御
			if (_timer < FPS * 3)
			{
				if (_timer % 7 == 0)
					setEffect(enemy[_bossIdx].x + rand() % 201 - 100, enemy[_bossIdx].y + rand() % 201 - 100, EFF_EXPLODE);
			}
			else if (_timer == FPS * 3)
			{
				PlaySoundMem(_gameClearSE, DX_PLAYTYPE_BACK);
			}
			else
			{
				drawTextCenter(SCREEN_WIDTH * 0.5, SCREEN_HEIGHT * 0.3, "STAGE CLEAR!", 0xffffff, 80);
			}
			if (_timer > FPS * 5)
			{
				drawTextBlinking(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.7, "Press SPACE to NextStage.", 0xffffff, 30);
				if (CheckHitKey(KEY_INPUT_SPACE))
				{
					_isPlayerReady = false;
					player.x = SCREEN_WIDTH / 2;
					player.y = SCREEN_HEIGHT;
					player.vy = FIRST_PLAYER_SPEED;
					_currentPlayerVYTemp = player.vy;
					_stage++;
					_distance = STAGE_DISTANCE;
					_scene = PLAY;
					StopSoundMem(_gameClearSE, DX_PLAYTYPE_BACK);
					PlaySoundMem(_bgm, DX_PLAYTYPE_LOOP);
				}
			}

			break;
		}


		


		oldSpaceKey = CheckHitKey(KEY_INPUT_SPACE);

		updateScreenShake(); // スクリーンシェイクの更新
		ScreenFlip();
		WaitTimer(1000 / FPS * _gameoverDelayTimeMag);
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
	player.y = SCREEN_HEIGHT;
	player.vx = 5;
	player.vy = FIRST_PLAYER_SPEED; //ゲームスタート時に自機が画面外から出てくるように
	player.shield = PLAYER_SHIELD_MAX;
	GetGraphSize(_imgFighter, &player.width, &player.height);
	for (int i = 0; i < ENEMY_MAX; i++) enemy[i].isState = 0; // 全ての敵機を存在しない状態に
	_score = 0;
	_stage = 1;
	_noDamageFrameCount = 0;
	_weaponLV = 1;
	_distance = STAGE_DISTANCE;
	_isPlayerReady = false;
	_currentPlayerVYTemp = 5;
}

/// <summary>
/// ゲーム中にポーズを行う
/// </summary>
/// <param name=""></param>
/// <returns></returns>
void pauseGameScreenProcess(void)
{

}

// スクリーンシェイクを開始する関数
void startScreenShake(int duration, int magnitude) {
	_shakeDuration = duration;
	_shakeMagnitude = magnitude;
}

/// <summary>
/// ダメージを受けた時などの画面の揺れを制御を行う
/// </summary>
/// <param name=""></param>
void updateScreenShake() {
	if (_shakeDuration > 0) {
		// 揺れがアクティブな間は持続時間を減らす
		_shakeDuration--;

		// 揺れのオフセットを計算（ランダムに-から+の範囲で）
		_shakeOffsetX = (rand() % (_shakeMagnitude * 2 + 1)) - _shakeMagnitude;
		_shakeOffsetY = (rand() % (_shakeMagnitude * 2 + 1)) - _shakeMagnitude;

		// 画面の描画位置をオフセットする
		SetDrawScreen(DX_SCREEN_BACK);
		SetDrawArea(0, 0, SCREEN_WIDTH + _shakeOffsetX, SCREEN_HEIGHT + _shakeOffsetY);
	}
	else {
		// 揺れがない場合は描画範囲をリセット
		SetDrawArea(-10, -10, SCREEN_WIDTH + 10, SCREEN_HEIGHT + 10);
	}
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
	// 被ダメ時は2フレームに一回、通常時は常に表示
	// ダメージを受けた際の画面の揺れも考慮した描画を実装している
	if(_noDamageFrameCount % 4 < 2) drawImage(_imgFighter, player.x + _shakeOffsetX / 2, player.y);
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
	for (int n = 0; n < _weaponLV; n++)
	{
		int x = player.x - (_weaponLV * 5) + n * 10;
		int y = player.y - 20;
		for (int i = 0; i < BULLET_MAX; i++)
		{
			if (bullet[i].isState == 0)
			{
				bullet[i].x = x;
				bullet[i].y = y;
				bullet[i].vx = 0;
				bullet[i].vy = -40;
				bullet[i].isState = 1;
				break;
			}
		}
	}
	PlaySoundMem(_shotSE, DX_PLAYTYPE_BACK); // 効果音
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
int setEnemy(int x, int y, double vx, double vy, int pattern, int img, int durability)
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
				startScreenShake(20, 20); // 20フレームの間、強度20で画面を揺らす
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
		setEffect(enemy[n].x, enemy[n].y, EFF_EXPLODE); //爆発演出
		if (enemy[n].pattern == ENE_BOSS)
		{
			StopSoundMem(_bgm);
			_scene = CLEAR;
			_timer = 0;
		}
	}
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
	drawText(x, y - 50, "WEAPON LV %02d", _weaponLV, 0xffffff, 20);
	drawText(x, y - 75, "SPEED %02d", player.vx, 0xffffff, 20);

}

/// <summary>
/// エフェクトのセット
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="pattern"></param>
void setEffect(int x, int y, int pattern)
{
	static int effectNum;
	effect[effectNum].x = x;
	effect[effectNum].y = y;
	effect[effectNum].isState = 1;
	effect[effectNum].pattern = pattern;
	effect[effectNum].timer = 0;
	effectNum = (effectNum + 1) % EFFECT_MAX;
	if (pattern == EFF_EXPLODE) PlaySoundMem(_explosionSE, DX_PLAYTYPE_BACK); //効果音
}

/// <summary>
/// エフェクトを描画する
/// </summary>
/// <param name=""></param>
void drawEffect(void)
{
	int ix;
	for (int i = 0; i < EFFECT_MAX; i++)
	{
		if (effect[i].isState == 0) continue;
		switch (effect[i].pattern)
		{
		case EFF_EXPLODE:
			ix = effect[i].timer * 128; //画像の切り出し位置
			DrawRectGraph(effect[i].x - 64, effect[i].y - 64, ix, 0, 128, 128, _imgExplosion, TRUE, FALSE);
			effect[i].timer++;
			if (effect[i].timer == 7) effect[i].isState = 0;
			break;

		case EFF_RECOVER:
			if (effect[i].timer < 30)
				SetDrawBlendMode(DX_BLENDMODE_ADD, effect[i].timer * 8);
			else
				SetDrawBlendMode(DX_BLENDMODE_ADD, (60 - effect[i].timer) * 8);

			for(int i = 3; i < 8; i++) 
				DrawCircle(player.x, player.y, (player.width + player.height) / i, 0x2040c0, TRUE);

			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			effect[i].timer++;
			if (effect[i].timer == 60) effect[i].isState = 0;
			break;
		}
	}
}

/// <summary>
/// アイテムをセットする
/// </summary>
/// <param name=""></param>
void setItem(void)
{
	item.x = (SCREEN_WIDTH / 4) * (1 + rand() % 3);
	item.y = -16;
	item.vx = 15;
	item.vy = 1;
	item.isState = 1;
	item.timer = 0;
}

/// <summary>
/// アイテムの処理
/// </summary>
/// <param name=""></param>
void moveItem(void)
{
	if (item.isState == 0) return;
	item.x += item.vx;
	item.y += item.vy;
	if (item.timer % 60 < 30) item.vx -= 1;
	else item.vx += 1;
	if (item.y > SCREEN_HEIGHT + 16) item.isState = 0;
	item.pattern = (item.timer / 120) % ITEM_TYPE; //時間経過でアイテムのタイプを変更する
	item.timer++;
	DrawRectGraph(item.x - 20, item.y - 16, item.pattern * 40, 0, 40, 32, _imgItem, TRUE, FALSE);
	// if(scene == GAMEOVER) return;
	int dis = pow(item.x - player.x, 2) + pow(item.y - player.y, 2);
	if (dis < pow(60, 2))
	{
		item.isState = 0;
		if (item.pattern == 0)
		{
			if (player.vx < PLAYER_SPEED_MAX)
			{
				player.vx += 3;
				player.vy += 3;
			}
		}
		else if (item.pattern == 1)
		{
			if (player.shield < PLAYER_SHIELD_MAX) player.shield++;
			setEffect(player.x, player.y, EFF_RECOVER);
		}
		else if (item.pattern == 2)
		{
			if (_weaponLV < WEAPON_LV_MAX) _weaponLV++;
		}
		PlaySoundMem(_itemSE, DX_PLAYTYPE_BACK);
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
/// テキストを中央に表示させる
/// </summary>
void drawTextCenter(int x, int y, const char* txt, int col, int fontSize)
{
	//drawText関数と被ってしまうが、事前にテキストの大きさを適切に取得しておく必要があるためフォントサイズを変更する
	SetFontSize(fontSize);
	int strWidth = GetDrawStringWidth(txt, strlen(txt));
	x -= strWidth / 2;
	y -= fontSize / 2;
	drawText(x, y, txt, ' ', col, fontSize);
}

/// <summary>
/// テキストをフェードさせる
/// 最初のタイトルの時のテキストに適用する
/// </summary>
void drawTextFade(int x, int y, const char* txt, int col, int fontSize, int alpha)
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	drawTextCenter(x, y, txt, col, fontSize);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// Ease Out Cubic関数
double easeOutCubic(double x) {
	double p = 1 - x;
	return 1 - pow(p, 3);
}

// アルファ値を計算する関数
// 
int calculateAlphaEaseOutCubicMethod(int time, int duration, int alpha) {
	// 正規化された時間 (0.0 から 1.0)
	// duration の半分で折り返す
	double normalizedTime = static_cast<double>(time % duration) / duration;
	double triangleWave;

	// 三角波を生成（0から1へ上昇し、再び0へ下降）
	if (normalizedTime < 0.5) {
		triangleWave = 2.0 * normalizedTime;  // 0.0から1.0へ
	}
	else {
		triangleWave = 2.0 * (1.0 - normalizedTime);  // 1.0から0.0へ
	}

	// Ease Out Cubicを適用して滑らかな変化を作る
	double eased = easeOutCubic(triangleWave);

	// アルファ値 (0 から 255)
	return static_cast<int>(alpha * eased);
}

/// <summary>
/// テキストを点滅して表示する
/// </summary>
void drawTextBlinking(int x, int y, const char* txt, int col, int fontSize)
{
	// アルファ値を計算 (80フレームごとに繰り返し)
	int _alpha = calculateAlphaEaseOutCubicMethod(_timer, 80, 255);
	// 描画モードをアルファブレンドにして透明度を時間に合わせて変更させる
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, _alpha);
	drawTextCenter(x, y, txt, col, fontSize);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

/// <summary>
/// いくつかの選択肢からどれを選択するか
/// </summary>
/// <param name="txt"></param>
/// <param name="height"></param>
/// <param name="heightInterval"></param>
void selectText(const char** txt, int txtSize, double height, double heightInterval, int* selectTextNumber, int* oldKeyUp, int* oldKeyDown)
{
	for (int i = 0; i < txtSize; i++) {
		int y = SCREEN_HEIGHT * (height + heightInterval * i);
		if (i == *selectTextNumber) {
			drawTextBlinking(SCREEN_WIDTH / 2, y, txt[i], 0xff0000, 50);
		}
		else {
			drawTextCenter(SCREEN_WIDTH / 2, y, txt[i], 0xffffff, 50);
		}
	}


	if (CheckHitKey(KEY_INPUT_UP) && *oldKeyUp == 0)
		*selectTextNumber = (*selectTextNumber - 1 + txtSize) % txtSize;
	if (CheckHitKey(KEY_INPUT_DOWN) && *oldKeyDown == 0)
		*selectTextNumber = (*selectTextNumber + 1) % txtSize;

	*oldKeyUp = CheckHitKey(KEY_INPUT_UP);
	*oldKeyDown = CheckHitKey(KEY_INPUT_DOWN);
}

void resetSelectTextParameter(int* selectTextNumber, int* oldKeyUp, int* oldKeyDown, int* oldSpaceKey)
{
	*selectTextNumber = 0;
	*oldKeyUp = 0;
	*oldKeyDown = 0;
	*oldSpaceKey = 1;
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