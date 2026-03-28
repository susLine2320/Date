// 以下の ifdef ブロックは DLL から簡単にエクスポートさせるマクロを作成する標準的な方法です。 
// この DLL 内のすべてのファイルはコマンドラインで定義された ATS_EXPORTS シンボル
// でコンパイルされます。このシンボルはこの DLL が使用するどのプロジェクト上でも未定義でなけ
// ればなりません。この方法ではソースファイルにこのファイルを含むすべてのプロジェクトが DLL 
// からインポートされたものとして ATS_API 関数を参照し、そのためこの DLL はこのマク 
// ロで定義されたシンボルをエクスポートされたものとして参照します。
//#pragma data_seg(".shared")
//#pragma data_seg()

#define ATS_BEACON_S 0 // Sロング
#define ATS_BEACON_SN 1 // SN直下
#define ATS_BEACON_SNRED 2 // SN誤出発防止
#define ATS_BEACON_P 3 // P停止信号
#define ATS_BEACON_EMG 4 // P即停(非常)
#define ATS_BEACON_SVC 5 // P即停(常用)
#define ATS_BEACON_SPDLIM 6 // P分岐器速度制限
#define ATS_BEACON_SPDMAX 7 // P最高速度
#define ATS_BEACON_SPP 8 // 停車駅通過防止装置

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
	int year_disp;
	int month_disp;
	int date_disp;
	int yobi_disp;
	int yobi_set; //曜日設定
	int pastyobi;

	// 初期化する
	void initialize(void)
	{
		m_month = 0; // 月
		m_date = 0;
		m_year = 0;
		m_yobi = 0;
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
		m_month = status->tm_mon + 1;
		m_date = status->tm_mday;
		m_year = status->tm_year + 1900;
		m_yobi = status->tm_wday + 1;

		// 冷房の制御
		if (Cooler == 0)
		{
			Cooler = (m_month >= 6 && m_month <= 9);
		}

		// 日時補正を行う
		if (yobi_set > 0) {
			setdate(yobi_set);
		}
		else {
			year_disp = m_year;
			month_disp = m_month;
			date_disp = m_date;
			yobi_disp = m_yobi;
		}
	}

	void SetYobi(int yobi)//曜日設定地上子で設定
	{
		yobi_set = yobi;//10以上が設定年号、1の位が設定曜日（1～7）
	}

	// 日時補正
	void setdate(int yobi) {
		// Readmeでは 日=1～土=7 なので、1を引いて 0(日)～6(土) に変換
		// これを「targetWday（目標の曜日）」として定義します
		int targetWday = (yobi % 10) - 1;

		if (yobi >= 10) {//曜日が10以上→年指定
			year_disp = yobi / 10; //年は設定した年数
		}
		else {//曜日が9以下→年は今の年
			year_disp = m_year;
		}
		month_disp = m_month; //月は一切変動しない

		//	現状の曜日
		int currentWday = GetDayOfWeek(year_disp, month_disp, m_date);

		//	ターゲットとのずれ
		int diff = targetWday - currentWday;

		struct tm t = { 0 };
		t.tm_year = year_disp - 1900;
		t.tm_mon = month_disp - 1;
		t.tm_mday = m_date + diff;

		mktime(&t);
		/*
		if (pastyobi != yobi % 10)//設定する曜日と違っているなら
		{
			date_disp = m_date + (yobi % 10 - pastyobi - 7);//過去の曜日=2/設定曜日=3なら1日進める
			if (date_disp < 1)//1切ったら7日繰り上げ
			{
				date_disp = date_disp + 7;
				if (date_disp < 1)//1切ったら7日繰り上げ
				{
					date_disp = date_disp + 7;
				}
			}
		}
		else {
			date_disp = m_date;
		}
		*/
		
		year_disp = t.tm_year + 1900;
		month_disp = t.tm_mon + 1;
		date_disp = t.tm_mday;
		yobi_disp = t.tm_wday + 1; // 表示だけ 1-7 に戻す
	}
	/*
		void set(void) {
			//year_disp = m_year % 100;

		}

		//過去の今日の曜日を取得
	int zeller(int year, int month, int day)
		{
			// month が 1 または 2 である場合は微調整をします。
			if (month == 1 || month == 2)
			{
				// １月は前年の１３月、２月は前年の１４月とします。
				year--;
				month += 12;
			}
			// 地球の公転周期の有理数近似 365 + 1/4 - 1/100 + 1/400 とあわせて、また、小数点演算を整数かすることで 30 日と 31 日の誤差を吸収するらしいです。
			// 年を上位 (yH) と下位 (yL) とに分離します。

			int yH = int(year / 100);
			int yL = year - (yH * 100);

			// Zeller の公式を用いて week を計算します。
			int week = (yH >> 2) - 2 * yH + (yL >> 2) + yL + int((month + 1) * 2.6) + day;
			pastyobi =  (week % 7) + 1; //過去の今日の曜日
		}
	*/

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

private:
	int m_month; // 月(1～12)
	int m_date; // 日（1～31）
	int m_year; // 年（1～12）
	int m_yobi; //曜日（日～土）
};	// CDate
