// 以下の ifdef ブロックは DLL から簡単にエクスポートさせるマクロを作成する標準的な方法です。 
// この DLL 内のすべてのファイルはコマンドラインで定義された ATS_EXPORTS シンボル
// でコンパイルされます。このシンボルはこの DLL が使用するどのプロジェクト上でも未定義でなけ
// ればなりません。この方法ではソースファイルにこのファイルを含むすべてのプロジェクトが DLL 
// からインポートされたものとして ATS_API 関数を参照し、そのためこの DLL はこのマク 
// ロで定義されたシンボルをエクスポートされたものとして参照します。
//#pragma data_seg(".shared")
//#pragma data_seg()

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

		// 冷房の制御
		if (Cooler == 0)
		{
			//Cooler = (m_month >= 6 && m_month <= 9);
		}

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
	}


	// 曜日設定地上子で設定
	void SetYobi(int yobi)
	{
		yobi_set = yobi;//10以上が設定年号、1の位が設定曜日（1～7）
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

	//曜日取得関数
	int GetDayOfWeek(int y, int m, int d)
	{
		// 1月と2月は前年の13月、14月として計算する（ツェラーの性質）
		if (m < 3) {
			y--;
			m += 12;
		}

		return(y + y / 4 - y / 100 + y / 400 + (13 * m + 8) / 5 + d + 700) % 7;
	}

};	// CDate
