#include <Novice.h>
#include <cmath>
#include <process.h>
#include <mmsystem.h>
#include <string>

#pragma comment(lib,"wsock32.lib")
#pragma comment(lib, "winmm.lib")

DWORD WINAPI Threadfunc(void*);
HWND hwMain;

const char kWindowTitle[] = "サーバ";
const int kWindowWidth = 1280;
const int kWindowHeight = 720;

struct Pos {
	float x;
	float y;
};

struct Circle {
	Pos pos;
	float radius;
};

class Player {
public:
	void Initialize(Pos pos, float radius, int color, float speed) {
		player_.pos = pos;
		player_.radius = radius;
		color_ = color;
		speed_ = speed;
	}
	void Update() {
		if (Novice::CheckHitKey(DIK_UP)) {
			player_.pos.y -= speed_;
		}
		if (Novice::CheckHitKey(DIK_DOWN)) {
			player_.pos.y += speed_;
		}
		if (Novice::CheckHitKey(DIK_LEFT)) {
			player_.pos.x -= speed_;
		}
		if (Novice::CheckHitKey(DIK_RIGHT)) {
			player_.pos.x += speed_;
		}
	}
	void Draw() {
		Novice::DrawEllipse((int)player_.pos.x, (int)player_.pos.y, (int)player_.radius, (int)player_.radius, 0.0f, color_, kFillModeSolid);
	}
	/// <summary>
	/// 円の情報ゲッター
	/// </summary>
	/// <param name="player"></param>
	/// <param name="fixed"></param>
	Circle GetPlayer() {
		return player_;
	}
	void SetPos(Circle currentPos) {
		player_ = currentPos;
	}
	void SetColor(int color) {
		color_ = color;
	}
private:
	Circle player_;
	unsigned int color_;
	float speed_;
};

bool ChkCollision(const Circle player, const Circle fixed) {
	float dx = fixed.pos.x - player.pos.x;
	float dy = fixed.pos.y - player.pos.y;
	float distance = std::sqrtf(powf(dx, 2) + (powf(dy, 2)));
	return distance <= (player.radius + fixed.radius);
}

Player* player = new Player();
Player* fixed = new Player();
//Circle構造体をよい
Circle fixed_;
//Player用のCirlce構造体
Circle player_;

// キー入力結果を受け取る箱
char keys[256] = { 0 };
char preKeys[256] = { 0 };

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WSADATA wdData;
	static HANDLE hThread;
	static DWORD dwID;

	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1280, 720);
	hwMain = GetDesktopWindow();
	// winsock初期化
	WSAStartup(MAKEWORD(2, 0), &wdData);

	//スレッド制作/更新
	hThread = (HANDLE)CreateThread(NULL, 0, &Threadfunc, (LPVOID)&fixed_, 0, &dwID);

	fixed->Initialize({ kWindowWidth / 2, kWindowHeight / 2 }, 50.0f, 0xFF0000FF, 2.0f);
	player->Initialize({ 200,100 }, 25.0f, 0xFFFFFFFF, 5.0f);

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		fixed->Update();
		//クライアントのグローバル変数にplayerの位置を代入
		fixed_ = fixed->GetPlayer();

		if (ChkCollision(player_, fixed_)) {
			player->SetColor(0x0000FFFF);
		}
		else {
			player->SetColor(0xFFFFFFFF);
		}

		player->SetPos(player_);
		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		fixed->Draw();
		player->Draw();

		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();

	//winsock終了
	WSACleanup();

	return 0;
}

/* 通信スレッド関数 */
DWORD WINAPI Threadfunc(void*) {

	SOCKET sWait, sConnect;
	u_short wPort = 8000;
	SOCKADDR_IN saConnect, saLocal;
	int iLen, iRecv;

	// リスンソケットをオープン
	sWait = socket(AF_INET, SOCK_STREAM, 0);
	ZeroMemory(&saLocal, sizeof(saLocal));

	// 8000番に接続待機用ソケット作成
	memset(&saLocal, 0, sizeof(SOCKADDR_IN));
	saLocal.sin_family = AF_INET;
	saLocal.sin_addr.s_addr = INADDR_ANY;
	saLocal.sin_port = htons(wPort);

	//チェック
	if (sWait == INVALID_SOCKET) {
		SetWindowText(hwMain, L"listenソケットオープンエラー");
		return 1;
	}
	//bindをする
	if (bind(sWait, (sockaddr*)&saLocal, sizeof(saLocal)) == SOCKET_ERROR) {

		closesocket(sWait);
		SetWindowText(hwMain, L"bind接続待機ソケット失敗");
		return 1;
	}
	//listen
	if (listen(sWait, 2) == SOCKET_ERROR) {

		closesocket(sWait);
		SetWindowText(hwMain, L"listen接続待機ソケット失敗");

		return 1;
	}

	SetWindowText(hwMain, L"接続待機ソケット成功");

	iLen = sizeof(saConnect);
	// 接続受け入れて送受信用ソケット作成
	sConnect = accept(sWait, (sockaddr*)&saConnect, &iLen);
	if (sConnect == INVALID_SOCKET) {
		SetWindowText(hwMain, L"acceptエラー");
		closesocket(sConnect);
		return 1;
	}

	SetWindowText(hwMain, L"が接続してきました");
	SetWindowText(hwMain, L"accept関数成功です");

	// 接続待ち用ソケット解放
	closesocket(sWait);

	if (sConnect == INVALID_SOCKET) {

		shutdown(sConnect, 2);
		closesocket(sConnect);
		shutdown(sWait, 2);
		closesocket(sWait);

		SetWindowText(hwMain, L"ソケット接続失敗");

		return 1;
	}

	SetWindowText(hwMain, L"ソケット接続成功");

	iRecv = 0;

	while (1)
	{
		int nRcv;

		// クライアント側キャラの位置情報を受け取り
		nRcv = recv(sConnect, (char*)&player_, sizeof(player_), 0);

		// サーバ側キャラの位置情報を送信
		send(sConnect, (const char*)&fixed_, sizeof(fixed_), 0);

		if (nRcv == SOCKET_ERROR)break;
	}

	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}