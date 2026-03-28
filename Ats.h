// 以下の ifdef ブロックは DLL から簡単にエクスポートさせるマクロを作成する標準的な方法です。 
// この DLL 内のすべてのファイルはコマンドラインで定義された ATS_EXPORTS シンボル
// でコンパイルされます。このシンボルはこの DLL が使用するどのプロジェクト上でも未定義でなけ
// ればなりません。この方法ではソースファイルにこのファイルを含むすべてのプロジェクトが DLL 
// からインポートされたものとして ATS_API 関数を参照し、そのためこの DLL はこのマク 
// ロで定義されたシンボルをエクスポートされたものとして参照します。
//#pragma data_seg(".shared")
//#pragma data_seg()
#pragma once
#include <cstdlib>
#include <cmath>   // sinf 用にこれも必要かもしれません

#define BEACON_DATE 607 // 日時設定

int g_emgBrake; // 非常ノッチ
int g_svcBrake; // 常用最大ノッチ
int g_brakeNotch; // ブレーキノッチ
int g_powerNotch; // 力行ノッチ
int g_reverser; // レバーサ
bool g_pilotlamp; // パイロットランプ
int g_time; // 現在時刻
float g_speed; // 速度計の速度[km/h]
int g_deltaT; // フレーム時間[ms/frame]

ATS_HANDLES g_output; // 出力

class CDate
{
public:
	// 時間変数・構造体
	time_t t;
	struct tm* status;

	// 出力
	int Cooler; // 冷房
	int CoolerSound; // 冷房音
	int year_disp;// 年（1～12）
	int month_disp;// 月(1～12)
	int date_disp;// 日（1～31）
	int yobi_disp;//曜日（日～土）

	// 初期化する
	void initialize(void)
	{
		yobi_set = 0; //地上子曜日設定
		year_disp = 0;
		month_disp = 0;
		date_disp = 0;
		yobi_disp = 0;

		Cooler = 0; // 冷房
		CoolerSound = ATS_SOUND_STOP; // 冷房音

		update();
	}

	// 日付を更新する
	void update(void)
	{
		time(&t);
		status = localtime(&t);
		month_disp = status->tm_mon + 1;
		date_disp = status->tm_mday;
		year_disp = status->tm_year + 1900;
		yobi_disp = status->tm_wday + 1;

		// 日時補正を行う
		if (yobi_set > 0) {
			setdate(yobi_set);
		}
		/* M系廃止に伴いコメントアウト
		else {
			year_disp = m_year;
			month_disp = m_month;
			date_disp = m_date;
			yobi_disp = m_yobi;
		}
		*/

		// 冷房の制御
		if (Cooler == 0)
		{
			//Cooler = (m_month >= 6 && m_month <= 9);
		}
	}


	// 曜日設定地上子で設定
	void SetYobi(int yobi)
	{
		yobi_set = yobi;//10以上が設定年号、1の位が設定曜日（1～7）
	}

	// 現在時刻（h）を返す
	float GetCurrentTimeInHours()
	{
		int second = g_time / 1000;
		return second / 3600;
	}

private:
	int yobi_set; //曜日設定

	// 日時補正
	void setdate(int yobi) {
		// Readmeでは 日=1～土=7 なので、1を引いて 0(日)～6(土) に変換
		// これを「targetWday（目標の曜日）」として定義します
		int targetWday = (yobi % 10) - 1;

		if (yobi >= 10) {//曜日が10以上→年指定
			year_disp = yobi / 10; //年は設定した年数
		}

		/* M系廃止に伴いコメントアウト
		else {//曜日が9以下→年は今の年
			year_disp = m_year;
		}
		month_disp = m_month; //月は一切変動しない
		*/

		//	現状の曜日
		int currentWday = GetDayOfWeek(year_disp, month_disp, date_disp);

		//	ターゲットとのずれ
		int diff = targetWday - currentWday;

		struct tm t = { 0 };
		t.tm_year = year_disp - 1900;
		t.tm_mon = month_disp - 1;
		t.tm_mday = date_disp + diff;

		mktime(&t);
		
		// 最終的な値を disp 系に書き戻す
		year_disp = t.tm_year + 1900;
		month_disp = t.tm_mon + 1;
		date_disp = t.tm_mday;
		yobi_disp = t.tm_wday + 1; // 表示だけ 1-7 に戻す
	}

	//　曜日取得関数
	int GetDayOfWeek(int y, int m, int d)
	{
		// 1月と2月は前年の13月、14月として計算する（ツェラーの性質）
		if (m < 3) {
			y--;
			m += 12;
		}

		return(y + y / 4 - y / 100 + y / 400 + (13 * m + 8) / 5 + d + 700) % 7;
	}

	/*
	// 外気温を決定
	float CalculateCurrentTemp() {
		// 1. 月ごとのベース気温（近年の東京などの都市部をイメージ）
		float monthlyBase[] = { 0, 6, 7, 12, 18, 23, 26, 30, 31, 27, 20, 14, 8 };
		float base = monthlyBase[month_disp] + day_offset; // day_offsetは初期化時に決定

		// 2. 時刻補正（14時をピークとしたサインカーブ）
		// GetCurrentTimeInHours() は「14.5（14時30分）」のような小数値を返す想定
		float hour = GetCurrentTimeInHours();
		float timeVariation = 5.0f * sinf((hour - 8.0f) * 3.14159f / 12.0f);

		// 3. 走行中の「ゆらぎ」 (極端な変化を避けるため、微小なランダム値を蓄積)
		// 毎フレームではなく、一定時間ごとに小さな値を加減算する
		static float wobble = 0.0f;
		wobble += ((rand() % 100 - 50) / 1000.0f); // -0.05 ~ +0.05の極小変化
		if (wobble > 1.0f)  wobble = 1.0f;  // 最大1度の振れ幅に制限
		if (wobble < -1.0f) wobble = -1.0f;

		return base + timeVariation + wobble;
	}*/



};	// CDate

extern CDate g_date; // 日付

class CAir
{
public:
	void initialize(int atsTime)
	{
		// 1. まず「その日の運勢」を決める（これがないと気温計算がズレる）
		day_offset = (float)(rand() % 61 - 30) / 10.0f;
		wobble = 0.0f;
		last_time = atsTime;

		// 2. 運勢が決まった後に、初期の外気温を算出する
		float initialOuterTemp = CalculateNextOuterTemp();
		this->outer_temp = initialOuterTemp; // 初期値を保持
		targetOuterTemp = outer_temp;

		// 3. 室内温度の初期設定
		// 「運用中」か「出庫直後」かをランダムで決めるロジックを入れるとリアルです
		if (rand() % 100 < 50) {
			// すでに空調が効いている状態
			if (initialOuterTemp > 25.0f) room_temp = 24.5f;
			else if (initialOuterTemp < 10.0f) room_temp = 19.0f;
			else room_temp = initialOuterTemp;
		}
		else {
			// 外気と同じ（放置されていた車両）
			room_temp = initialOuterTemp;
		}

		current_mode = 0; // 最初は「切」からスタート
	}

	// --- 外部から毎フレーム呼ぶためのメイン関数 ---
	void Update(int atsTime, int acSwitch) {

		// 1. 経過時間(dt)の計算
		if (last_time == 0) { last_time = atsTime; return; }
		float dt = (atsTime - last_time) / 1000.0f;
		if (dt <= 0) return;
		last_time = atsTime;

		// --- 外気温の計算 ---

	// A. まず「時刻と季節」からベースとなる外気温を算出
		float baseOuter = CalculateNextOuterTemp();

		// B. 地上子から指定された「理想の気温(target)」と「ベース(base)」の差を計算
		// 例：ベースが15度で、地上子が8度を指定したら、目指すべきオフセットは -7度
		float targetOffset = targetOuterTemp - baseOuter;

		// C. 現在のオフセットを目標オフセットにじわじわ近づける
		float step = 0.1f * dt; // 1秒間に0.1度変化
		if (offsetOuterTemp < targetOffset) {
			offsetOuterTemp = min(offsetOuterTemp + step, targetOffset);
		}
		else if (offsetOuterTemp > targetOffset) {
			offsetOuterTemp = max(offsetOuterTemp - step, targetOffset);
		}

		// 3. 基本の外気温 = ベース + 補正
		float nominalOuter = baseOuter + offsetOuterTemp;

		// 4. ★ 最後に「ゆらぎ」だけを別途加算する
		// (rand()等を使った計算をここで行う。nominalOuterをベースにする)
		this->outer_temp = ApplyWobble(nominalOuter);

		// 3. 車内温度の更新
		this->room_temp = CalculateNextRoomTemp(this->room_temp, this->outer_temp, this->current_mode, dt);

		// 4. 空調モードの判定 (ヒステリシス実装)
		this->current_mode = DetermineNextMode(acSwitch);
	}

	void SetTargetOuterTemp(float temp) { targetOuterTemp = temp; }

	// --- 外部（Ats.cpp）で音やパネルを制御するための Getter ---
	int GetACMode() const { return current_mode; }
	float GetRoomTemp() const { return room_temp; }
	float GetOuterTemp() const { return outer_temp; }

private:
	float outer_temp;
	float room_temp;
	float targetOuterTemp;  // 地上子から指定された目標の外気温
	float offsetOuterTemp = 0.0f;
	int current_mode;
	float wobble; // 揺らぎだけは内部で蓄積保持が必要
	float day_offset;  // その日の運勢
	int   last_time;   // 前回の実行時刻

	// 外気温の更新：前の値を引数で受け取る
	float CalculateNextOuterTemp() {

		// 1. 月ごとのベース気温（近年の東京などの都市部をイメージ）
		float monthlyBase[] = { 0, 6, 12, 18, 20, 23, 26, 30, 31, 27, 20, 13, 7 };
		float base = monthlyBase[g_date.month_disp] + day_offset; // day_offsetは初期化時に決定

		// 2. 時刻補正（14時をピークとしたサインカーブ）
		// GetCurrentTimeInHours() は「14.5（14時30分）」のような小数値を返す想定
		float hour = g_date.GetCurrentTimeInHours();
		float timeVariation = 5.0f * sinf((hour - 8.0f) * 3.14159f / 12.0f);

		return base + timeVariation;
	}

	// 揺らぎの加算
	float ApplyWobble(float temp)
	{
		// 揺らぎ（wobble）の更新
		this->wobble += ((rand() % 101 - 50) / 10000.0f);
		if (this->wobble > 1.0f)  this->wobble = 1.0f;
		if (this->wobble < -1.0f) this->wobble = -1.0f;

		// 前の値に依存させる場合（例：急変防止のフィルタリング）はここで currentOuter を使う
		// 今回は「ベース + 揺らぎ」が絶対値として出るので、そのまま返してもOK
		return temp + this->wobble;
	}

	// 車内温度の更新：前の値を受け取り、計算結果を返す
	float CalculateNextRoomTemp(float currentRoom, float outer, int mode, float dt) {
		float nextTemp = currentRoom;

		// 1. 自然熱交換（外気に近づく）
		nextTemp += (outer - currentRoom) * 0.01f * dt;

		// 2. 乗客・機器の発熱
		nextTemp += 0.005f * dt;

		// 3. 空調の効果
		if (mode == 2)      nextTemp -= 0.06f * dt; // 冷房
		else if (mode == 3) nextTemp += 0.06f * dt; // 暖房

		return nextTemp; // 新しい温度を返す
	}

	// --- 空調モード判定ロジック ---
	int DetermineNextMode(int acSwitch) {
		if (acSwitch == 0) return 0; // 【切】
		if (acSwitch == 1) {        // 【入】
			return (outer_temp > 22.0f) ? 2 : 1; // 暑ければ冷房、そうでなければ送風
		}

		// 【自動】(acSwitch == 2) のヒステリシス判定
		int next = current_mode;
		// 冷房の起動・停止
		if (room_temp > 26.5f) next = 2;
		else if (room_temp < 25.0f && current_mode == 2) next = 1;

		// 暖房の起動・停止 (音なし)
		if (room_temp < 17.0f) next = 3;
		else if (room_temp > 19.0f && current_mode == 3) next = 0;

		// 送風判定
		if (room_temp > 22.0f && next == 0) next = 1;

		return next;
	}
};