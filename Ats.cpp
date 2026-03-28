// Ats.cpp : DLL アプリケーション用のエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "atsplugin.h"
#include <time.h>
#include "Ats.h"
#include "ini.h"

//キーボード管理
class CKeyHandler {
private:
	int m_vKey;          // 仮想キーコード (0x43など)
	bool m_isPressed;    // 今押されているか
	bool m_wasPressed;   // 前回チェック時に押されていたか

public:
	CKeyHandler(int vKey) : m_vKey(vKey), m_isPressed(false), m_wasPressed(false) {}

	// キーが「新しく押された瞬間」だけ true を返す
	bool IsNewlyPressed() {
		m_isPressed = (GetAsyncKeyState(m_vKey) & 0x8000) != 0;
		bool result = (m_isPressed && !m_wasPressed);
		m_wasPressed = m_isPressed;
		return result;
	}

	// 設定変更用
	void SetKey(int vKey) { m_vKey = vKey; }
};

CDate g_date; // 日付
CAir g_air; //空調
CKeyHandler g_keyNext(0x00); // 右回し用
CKeyHandler g_keyPrev(0x00); // 左回し用

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}


ATS_API void WINAPI Load() 
{
	// [Control] セクションからキーコード読み込み
	g_keyNext.SetKey(LoadInt(L"Control", L"AC_Key_Next", 0xBA));
	g_keyPrev.SetKey(LoadInt(L"Control", L"AC_Key_Prev", 0xBB));

}

ATS_API int WINAPI GetPluginVersion()
{
	return ATS_VERSION;
}

ATS_API void WINAPI SetVehicleSpec(ATS_VEHICLESPEC vehicleSpec)
{
	g_svcBrake = vehicleSpec.BrakeNotches;
	g_emgBrake = g_svcBrake + 1;
}

ATS_API void WINAPI Initialize(int brake)
{
	g_date.initialize();
	g_air.isInitialized = false;
	g_speed = 0;
}

ATS_API ATS_HANDLES WINAPI Elapse(ATS_VEHICLESTATE vehicleState, int *panel, int *sound)
{
	g_deltaT = vehicleState.Time - g_time;
	g_time = vehicleState.Time;
	g_speed = vehicleState.Speed;

	if (!g_air.isInitialized && g_time > 0) {
		g_air.initialize(g_time);
		g_air.isInitialized = true;
	}
	if (g_air.isInitialized)
	{
		if (g_keyNext.IsNewlyPressed()) {
			// 右回し: 0(OFF) -> 1(ON) -> 2(AUTO) -> 0...
			g_air.acSwitch = min(g_air.acSwitch + 1, 2);
		}

		if (g_keyPrev.IsNewlyPressed()) {
			// 左回し: 0 -> 2 -> 1 -> 0...
			g_air.acSwitch = max(g_air.acSwitch -1, 0);
		}

		g_air.Update(g_time, g_air.acSwitch);
	}
	int roomInt = (int)(g_air.GetRoomTemp() * 10 + 0.5f); // 四捨五入
	int outerInt = (int)(g_air.GetOuterTemp() * 10 + 0.5f);

	g_date.update();

	// ハンドル出力
	g_output.Brake = g_brakeNotch;
	g_output.Reverser = g_reverser;
	g_output.Power = g_powerNotch;
	g_output.ConstantSpeed = ATS_CONSTANTSPEED_CONTINUE;

	// パネル出力
	// 日付
	panel[218] = g_date.year_disp % 100;
	panel[219] = g_date.month_disp;
	panel[220] = g_date.date_disp;
	panel[221] = g_date.yobi_disp;

	// 外気温 (228: 10の位, 229: 1の位, 230: 0.1の位)
	panel[228] = (outerInt / 100) % 10;
	panel[229] = (outerInt / 10) % 10;
	panel[230] = outerInt % 10;

	// 内気温 (225: 10の位, 226: 1の位, 227: 0.1の位)
	panel[225] = (roomInt / 100) % 10;
	panel[226] = (roomInt / 10) % 10;
	panel[227] = roomInt % 10;

	//スイッチ類
	panel[222] = g_air.acSwitch; //エアコンCgS
	panel[223] = g_air.GetACMode(); //動作モード

	// サウンド出力
	sound[226] = g_air.GetACMode() == 0 || g_air.GetACMode() == 3 ? ATS_SOUND_STOP : ATS_SOUND_PLAYLOOPING;

    return g_output;
}

ATS_API void WINAPI SetPower(int notch)
{
	g_powerNotch = notch;
}

ATS_API void WINAPI SetBrake(int notch)
{
	g_brakeNotch = notch;
}

ATS_API void WINAPI SetReverser(int pos)
{
	g_reverser = pos;
}

ATS_API void WINAPI KeyDown(int atsKeyCode)
{
}

ATS_API void WINAPI KeyUp(int hornType)
{
}

ATS_API void WINAPI HornBlow(int atsHornBlowIndex)
{
}

ATS_API void WINAPI DoorOpen()
{
	g_pilotlamp = false;
}

ATS_API void WINAPI DoorClose()
{
	g_pilotlamp = true;
}

ATS_API void WINAPI SetSignal(int signal)
{
}

ATS_API void WINAPI SetBeaconData(ATS_BEACONDATA beaconData)
{
	switch (beaconData.Type)
	{
		case BEACON_DATE: //曜日設定
			g_date.SetYobi(beaconData.Optional);
			break;
		case BEACON_AIR: //空調設定
			g_air.SetTargetOuterTemp((float)beaconData.Optional / 10);
			break;
	}
}

ATS_API void WINAPI Dispose() {}