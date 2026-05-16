#ifndef	MAIN_C
#define	MAIN_C

#include <iocslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <doslib.h>
#include <io.h>
#include <math.h>
#include <time.h>
#include <interrupt.h>

#include <usr_macro.h>
#include <apicglib.h>

#include "main.h"

#include "BIOS_CRTC.h"
#include "BIOS_DMAC.h"
#include "BIOS_MFP.h"
#include "BIOS_PCG.h"
#include "BIOS_MPU.h"
#include "IF_Draw.h"
#include "IF_FileManager.h"
#include "IF_Graphic.h"
#include "IF_Input.h"
#include "IF_MACS.h"
#include "IF_Math.h"
#include "IF_Memory.h"
#include "IF_Mouse.h"
#include "IF_Music.h"
#include "IF_PCG.h"
#include "IF_Text.h"
#include "IF_Task.h"
#include "APL_Menu.h"
#include "APL_PCG.h"
#include "APL_Score.h"

//#define 	W_BUFFER	/* ダブルバッファリングモード */
//#define	FPS_MONI	/* FPS計測 */
//#define	CPU_MONI	/* CPU計測 */
#define	MEM_MONI	/* MEM計測 */

/* グローバル変数 */
uint32_t	g_unTime_cal = 0u;
uint32_t	g_unTime_cal_PH = 0u;
uint32_t	g_unTime_Pass = 0xFFFFFFFFul;
int32_t		g_nCrtmod = 0;
int32_t		g_nMaxUseMemSize;
int16_t		g_CpuTime = 0;
uint8_t		g_mode;
uint8_t		g_mode_rev;
uint8_t		g_Vwait = 0;
uint8_t		g_bFlip = FALSE;
uint8_t		g_bFlip_old = FALSE;
uint16_t	g_unFrameCount;
int16_t		g_AnimeCountMax = 6572;	/*  */
int16_t		g_AnimeSkipCount = 0;
int16_t		g_AnimeSkipCountMax = 2;
uint32_t	g_MusicLength = 0;	/* ms */
uint16_t	g_SpSize;	/*  */
#ifdef FPS_MONI
uint8_t		g_bFPS_PH = 0u;
#endif
uint8_t		g_bExit = TRUE;
int16_t		g_GameSpeed;

uint8_t		g_ubDemoPlay = FALSE;
uint8_t		g_ubPhantomX = FALSE;

/* グローバル構造体 */
#ifdef DEBUG	/* デバッグコーナー */
uint16_t	g_uDebugNum = 0;
//uint16_t	g_uDebugNum = (Bit_7|Bit_4|Bit_0);
#endif

int16_t		g_nSP_x_ofset[30] = {	0, 24, 24, 0, 0, 0, 0, 0, 0, 0,
									0, 24, 24, 0, 0, 0, 0, 0, 0, 0,
									0, 24, 24, 0, 0, 0, 0, 0, 0, 0 };
								/*  0  1  2  3  4  5  6  7  8  9 */
int16_t		g_nSP_x_move[30] = {   -8, 8, 8,16, 8, 8, 8,32, 8, 8,
									0, 0, 0,80,12,16,16,16, 0, 8,
									0, 0, 0, 8, 8, 0,80, 8,16, 8 };

uint8_t		*g_pMasterImage = NULL;

/* 関数のプロトタイプ宣言 */
int16_t main(int16_t, int8_t**);
static void App_Init(void);
static void App_exit(void);
int16_t	BG_main(uint32_t);
void App_TimerProc( void );
int16_t App_RasterProc( uint16_t * );
void App_VsyncProc( void );
void App_HsyncProc( void );
int16_t	App_FlipProc( void );
int16_t	SetFlip(uint8_t);
int16_t	GetGameMode(uint8_t *);
int16_t	SetGameMode(uint8_t);

void (*usr_abort)(void);	/* ユーザのアボート処理関数 */

/* 関数 */
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t main_Task(void)
{
	int16_t	ret = 0;
	uint16_t	uFreeRunCount=0u;
#ifdef DEBUG	/* デバッグコーナー */
//	uint32_t	unTime_cal = 0u;
//	uint32_t	unTime_cal_PH = 0u;
#endif
	int16_t	pal_mode = FALSE;

	int16_t	loop;
	int16_t	pushCount = 0;
	int16_t	AnimeCount = 0;
	int16_t	AnimeCount_old = 0;

	uint8_t	bMode;
	
	ST_TASK		stTask = {0}; 
	ST_CRT		stCRT;
	
//	int16_t	mv_x = 0;
	int16_t	rad = 0;
	int16_t	ratio = 0;

	static int8_t s_b_Updata;

	/* 変数の初期化 */
#ifdef W_BUFFER
	SetGameMode(1);
#else
	SetGameMode(0);
#endif	
	loop = 1;
	g_unFrameCount = 0;
	
	/* 乱数 */
	srandom(TIMEGET());	/* 乱数の初期化 */
	
	do	/* メインループ処理 */
	{
		uint32_t time_st;
#if 0
		ST_CRT	stCRT;
#endif
#ifdef DEBUG	/* デバッグコーナー */
		uint32_t time_now;
		GetNowTime(&time_now);
#endif

#ifdef DEBUG
//		Draw_Box(	stCRT.hide_offset_x + X_POS_MIN - 1,
//					stCRT.hide_offset_y + Y_POS_BD + 1,
//					stCRT.hide_offset_x + X_POS_MAX - 1,
//					stCRT.hide_offset_y + g_TB_GameLevel[g_nGameLevel] + 1,
//					G_PURPLE, 0xFFFF);
#endif
		PCG_START_SYNC(g_Vwait);	/* 同期開始 */

		/* 時刻設定 */
		GetNowTime(&time_st);	/* メイン処理の開始時刻を取得 */
		SetStartTime(time_st);	/* メイン処理の開始時刻を記憶 */
		
		/* タスク処理 */
		TaskManage();				/* タスクを管理する */
		GetTaskInfo(&stTask);		/* タスクの情報を得る */

		/* モード引き渡し */
		GetGameMode(&bMode);
		GetCRT(&stCRT, bMode);	/* 画面座標取得 */

#if CNF_VDISP
		/* V-Syncで入力 */
#else
		if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
		{
		}
#endif

#ifdef DEBUG	/* デバッグコーナー */
//		DirectInputKeyNum(&g_uDebugNum, 3);	/* キーボードから数字を入力 */
#endif

		if((GetInput() & KEY_b_ESC ) != 0u)	/* ＥＳＣキー */
		{
			/* 終了 */
			pushCount = Minc(pushCount, 1);
			if(pushCount >= 5)
			{
				/* 終了処理 */
				loop = 0;
				SetTaskInfo(SCENE_EXIT);	/* 終了シーンへ設定 */
			}
		}
		else if((GetInput() & KEY_b_HELP ) != 0u)	/* HELPキー */
		{
			if( (stTask.bScene != SCENE_GAME_OVER_S) && (stTask.bScene != SCENE_GAME_OVER) )
			{
				/* リセット */
				pushCount = Minc(pushCount, 1);
				if(pushCount >= 6)
				{

				}
				else if(pushCount >= 5)
				{
					SetTaskInfo(SCENE_INIT);	/* 終了シーンへ設定 */
				}
			}
		}		
		else if((GetInput() & KEY_b_Q ) != 0u)	/* Qキー */
		{
			if( (stTask.bScene != SCENE_GAME_OVER_S) && (stTask.bScene != SCENE_GAME_OVER) )
			{
				/* リセット */
				pushCount = Minc(pushCount, 1);
				if(pushCount >= 6)
				{

				}
				else if(pushCount >= 5)
				{
					SetTaskInfo(SCENE_EXIT);	/* 終了シーンへ設定 */
				}
			}
		}		
		else
		{
			pushCount = 0;
		}

		switch(stTask.bScene)
		{
			case SCENE_INIT:	/* 初期化シーン */
			{
				T_Clear();		/* テキストクリア */
				SetTaskInfo(SCENE_TITLE_S);	/* シーン(開始処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_TITLE_S:
			{
				int16_t	i;

				Set_CRT_Contrast(0);	/* 真っ暗にする */
				G_PAGE_SET(0b0001);	/* GR0 */

				for(i = SP_PAT001; i < PCG_NUM_MAX; i++)
				{
#if 1
					ST_PCG	*p_stPCG = NULL;
					p_stPCG = PCG_Get_Info(i);	/* SP */

					if(p_stPCG != NULL)
					{
						p_stPCG->validty = FALSE;
						p_stPCG->update	=FALSE;
					}
#endif
				}

				Set_CRT_Contrast(-1);	/* もとに戻す */
				SetTaskInfo(SCENE_TITLE);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_TITLE:
			{
				SetTaskInfo(SCENE_TITLE_E);	/* シーン設定 */
				break;
			}
			case SCENE_TITLE_E:
			{
				SetTaskInfo(SCENE_START_S);	/* シーン(処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_START_S:
			{
				SetTaskInfo(SCENE_START);	/* シーン設定 */
				break;
			}
			case SCENE_START:
			{
				size_t ulFileSize;
				size_t	pal_size;
				uint32_t uCal;
				uint32_t numerator;  		// 分子
				uint32_t denominator;    	// 分母（6572 * 3）
				uint32_t base;
				uint32_t rem;
				ST_PCG	*p_stPCG = NULL;

				// 512 = 480(6) + 32
				G_PAGE_SET(0b0010);	/* GR1 */
				if(G_Load(2, 0, 0, 1) < 0)	/* 256色 ページ1*/
				{
					T_Clear();		/* テキストクリア */
				}
				G_Palette_HALF();	/* パレットを設定値から半分にします */

				p_stPCG = PCG_Get_Info(0);	/* SP */
				g_SpSize = (p_stPCG->Pat_w * SP_W) * (p_stPCG->Pat_h * SP_H) / 2;

				GetFileLength(g_sp_list[0], &ulFileSize);
				g_AnimeCountMax = ulFileSize / g_SpSize;
				
				GetFileLength(g_pal_list[1], &pal_size);
				if(pal_size <= 512)
				{
					pal_mode = FALSE;
				}
				else
				{
					pal_mode = TRUE;
				}

				GetFileLength(adpcm_list[0], &ulFileSize);
				uCal = ulFileSize / 78;	/* 15.6kHz限定 */
//				printf("Size(%d)time(%d[s])(%d)\n", g_AnimeCountMax, uCal/100, uCal);	/* 動画秒数 */

				numerator = uCal * 200;	/* タイマー分解能：50?s = 0.00005秒 */
				uCal = numerator / g_AnimeCountMax;
//				printf("count(%d[times]%d)\n", numerator, uCal);	/* 動画1枚あたりのカウント */

				uCal = (uCal / 256) + 1;
				denominator = g_AnimeCountMax * uCal;
//				printf("denominator %d, %d[times]\n", denominator, uCal);	/* 動画1枚あたりのカウント */

				base = numerator / denominator;   // 整数部
				rem  = numerator % denominator;   // 余り
	
//				printf("TimerSet(%d)(%d)(%d))\n", base, rem, denominator);	/* TimerSet */

				TimerD_EXIT();
				TimerD_INIT(base, rem, denominator, uCal);

				AnimeCount = 0;
				AnimeCount_old = 0;
				g_Vwait = 1;

				T_Clear();		/* テキストクリア */

				SetTaskInfo(SCENE_START_E);	/* シーン設定 */
				break;
			}
			case SCENE_START_E:
			{
				int32_t i;
				ST_PCG	*p_stPCG = NULL;

				g_AnimeSkipCount = 0;
				
				for(i = SP_PAT001; i < PCG_NUM_MAX; i++)
				{
					p_stPCG = PCG_Get_Info(i);	/* SP */

					if(p_stPCG != NULL)
					{
						p_stPCG->x = TO_FIXED_POINT((WIDTH - (p_stPCG->Pat_w * SP_W)) / 2 );
						p_stPCG->y = TO_FIXED_POINT((HEIGHT - (p_stPCG->Pat_h * SP_H)) / 2);
						p_stPCG->dx = 0;
						p_stPCG->dy = 0;
						p_stPCG->Anime = 0;
						p_stPCG->angle = 0;
						p_stPCG->validty = FALSE;
						p_stPCG->update	= FALSE;

						PCG_PRI_CHG(p_stPCG, PCG_PRI_HIGH);	/* プライオリティ */
					}
				}

				p_stPCG = PCG_Get_Info(AnimeCount_old);	/* SP */
				if(p_stPCG != NULL)
				{
					p_stPCG->Plane = 0xFFFF;
					p_stPCG->validty = FALSE;
					p_stPCG->update	= FALSE;
				}

				p_stPCG = PCG_Get_Info(AnimeCount);	/* SP */
				if(p_stPCG != NULL)
				{
					p_stPCG->Plane = 0xFFFF;
					p_stPCG->validty = TRUE;
					p_stPCG->update	= TRUE;
				}

				if(pal_mode == TRUE)
				{
					if(File_Load_Split(g_pal_list[1], &g_pal_dat[1][0], PAL_LIST_MAX * PAL_MAX, sizeof(int8_t), 2 * PAL_MAX, 0) != 0)
					{
							PCG_Set_PAL_Data(0, 1, 1);
					}
				}
				PCG_Set_PAL_Data(0, 1, 1);
				PCG_Set_PAL_Data(1, 1, 1);

				if(File_Load_Split(g_sp_list[0], p_stPCG->pPatData, PCG_MAX * 128, sizeof(int8_t), g_SpSize, 0) == 0)
				{
					SetTaskInfo(SCENE_GAME1_E);	/* シーン(処理)へ設定 */
				}

				AnimeCount_old = AnimeCount;
				AnimeCount++;

				SetTaskInfo(SCENE_GAME1_S);	/* シーン(処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_GAME1_S:
			{
				s_b_Updata = FALSE;

				ADPCM_Play(SE_OK);	/* 決定音 */
				SetNowTime(0);
				
				SetTaskInfo(SCENE_GAME1);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_GAME1:
			{
				uint32_t uNowPat;
				uint32_t uNowPatCal;

//				KeyHitESC();	/* デバッグ用 */

				GetNowTime(&uNowPat);
				uNowPatCal = uNowPat;
				if(uNowPatCal >= g_AnimeCountMax)
				{
					SetTaskInfo(SCENE_START_E);	/* シーン設定 */
					break;
				}
				else
				{
					ST_PCG	*p_stPCG = NULL;

					p_stPCG = PCG_Get_Info(AnimeCount);	/* SP */
					if(p_stPCG != NULL)
					{
						p_stPCG->Plane = 0xFFFF;
						p_stPCG->validty = TRUE;
						p_stPCG->update	= TRUE;
					}

					if(pal_mode == TRUE)
					{
						if(File_Load_Split(g_pal_list[1], &g_pal_dat[1][0], PAL_LIST_MAX * PAL_MAX, sizeof(int8_t), 2 * PAL_MAX, uNowPatCal) != 0)	/* PAL_LIST_MAX * 16 = 32 */
						{
//							PCG_Set_PAL_Data(0, 1, 1);
						}
					}

					if(File_Load_Split(g_sp_list[0], p_stPCG->pPatData, 65535, sizeof(int8_t), g_SpSize, uNowPatCal) == 0)	/* PCG_MAX * 128 = 65535 */
					{
						SetTaskInfo(SCENE_GAME1_E);	/* シーン(処理)へ設定 */
					}

//					printf("%04d\n", uNowPatCal);
//					printf("\033[1A");	/* $1B ESC[1A 1行上に移動 */

					p_stPCG = PCG_Get_Info(AnimeCount_old);	/* SP */
					if(p_stPCG != NULL)
					{
						p_stPCG->Plane = 0xFFFF;
						p_stPCG->validty = FALSE;
						p_stPCG->update	= FALSE;
					}

					AnimeCount_old = AnimeCount;

					AnimeCount++;
					if(AnimeCount >= PCG_NUM_MAX)
					{
						AnimeCount = 0;
					}

					SetTaskInfo(SCENE_GAME2);	/* シーン(処理)へ設定 */
				}
				break;
			}
			case SCENE_GAME1_E:
			{
				SetTaskInfo(SCENE_GAME_OVER_S);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_GAME2_S:
			{
				static int8_t s_b_UP;
				static int8_t s_b_DOWN;
				static int8_t s_b_RIGHT;
				static int8_t s_b_LEFT;
				static int8_t s_b_A;
				static int8_t s_b_B;

				int8_t ms_x;
				int8_t ms_y;
				int8_t ms_left;
				int8_t ms_right;
				int32_t ms_pos_x;
				int32_t ms_pos_y;

				/* マウス操作 */
				Mouse_GetDataPos(&ms_x, &ms_y, &ms_left, &ms_right);

				Mouse_GetPos(&ms_pos_x, &ms_pos_y);

				if(ms_left != 0)
				{
					SetInput(KEY_b_Z);
				}
				if(ms_right != 0)
				{
					SetInput(KEY_b_X);
				}

				/* ジョイスティック操作 */

				/* メニュー番号の実行 */
				if(	((GetInput_P1() & JOY_A ) != 0u)	||		/* A */
					((GetInput() & KEY_b_Z) != 0u)		||		/* A(z) */
					((GetInput() & KEY_b_SP ) != 0u)		)	/* スペースキー */
				{
					if(s_b_A == FALSE)
					{
						s_b_A = TRUE;
					
						rad+=10;
						if(rad > 360)rad = 0;
					}
					else
					{
					}
				}
				else
				{
					if(s_b_A == TRUE)
					{
//						SetTaskInfo(SCENE_GAME1_E);	/* シーン(処理)へ設定 */
					}
					s_b_A = FALSE;
				}

				/* キャンセル */
				if(	((GetInput_P1() & JOY_B ) != 0u)	||		/* B */
					((GetInput() & KEY_b_X) != 0u)	)			/* B(z) */
				{
					if(s_b_B == FALSE)
					{
						s_b_B = TRUE;
						rad-=10;
						if(rad < 0)rad = 350;
					}
				}
				else
				{
					s_b_B = FALSE;
				}

				/* メニュー番号の実行（各メニューの番号変更＆実行） */
				if(	((GetInput_P1() & JOY_LEFT ) != 0u )	||	/* LEFT */
					((GetInput() & KEY_LEFT) != 0 )		)	/* 左 */
				{
					if(s_b_LEFT == FALSE)
					{
						s_b_LEFT = TRUE;
					}
#if 0
					s_b_LEFT = TRUE;
					if((g_unFrameCount % 2) == 0)
					{
						AnimeCount--;
						if(AnimeCount < 0)
						{
							AnimeCount = PCG_NUM_MAX-1;
						}
					}
#else
#endif
				}
				else if( ((GetInput_P1() & JOY_RIGHT ) != 0u )	||	/* RIGHT */
						((GetInput() & KEY_RIGHT) != 0 )			)	/* 右 */
				{
					if(s_b_RIGHT == FALSE)
					{
						s_b_RIGHT = TRUE;
					}
#if 0
					s_b_RIGHT = TRUE;
					if((g_unFrameCount % 2) == 0)
					{
						AnimeCount++;
						if(AnimeCount >= PCG_NUM_MAX)
						{
							AnimeCount = 0;
						}
					}
#else
#endif
				}
				else
				{
					s_b_LEFT = FALSE;
					s_b_RIGHT = FALSE;
				}

				/* メニュー番号変更 */
				if(	((GetInput_P1() & JOY_UP ) != 0u )	||	/* UP */
					((GetInput() & KEY_UPPER) != 0 )	)	/* 上 */
				{
					if(s_b_UP == FALSE)
					{
						s_b_UP = TRUE;

						ratio++;
						if(ratio >= 12)ratio = 11;
					}
				}
				else if(((GetInput_P1() & JOY_DOWN ) != 0u )	||	/* UP */
						((GetInput() & KEY_LOWER) != 0 )	)	    /* 下 */
				{
					if(s_b_DOWN == FALSE)
					{
						s_b_DOWN = TRUE;

						ratio--;
						if(ratio < 0)ratio = 0;
					}
				}
				else
				{
					s_b_UP = FALSE;
					s_b_DOWN = FALSE;

					if((ms_x != 0) || (ms_y != 0))
					{
					}
				}
				if( (s_b_UP == TRUE)    ||
					(s_b_DOWN == TRUE)  ||
					(s_b_RIGHT == TRUE) ||
					(s_b_LEFT == TRUE)  ||
					(s_b_A == TRUE)     ||
					(s_b_B == TRUE)    	)
				{
					s_b_Updata = TRUE;
				}
				s_b_Updata = TRUE;

				if(s_b_Updata == TRUE)
				{
#if 0
					int32_t i;
					ST_PCG	*p_stPCG = NULL;

					p_stPCG = PCG_Get_Info(SP_PAT001 + AnimeCount_old);	/* SP */
					if(p_stPCG != NULL)
					{
						p_stPCG->Plane = 0xFFFF;
						p_stPCG->validty = FALSE;
						p_stPCG->update	= FALSE;
						if(g_pMasterImage != NULL)
						{
							memcpy(p_stPCG->pPatData, g_pMasterImage, (p_stPCG->Pat_w * SP_W) * (p_stPCG->Pat_h * SP_H));
							MyMfree(g_pMasterImage);	/* メモリ解放 */
							g_pMasterImage = NULL;
						}
					}

					if(s_b_RIGHT == TRUE)	mv_x += g_nSP_x_move[AnimeCount];
					if(s_b_LEFT == TRUE)	mv_x -= g_nSP_x_move[AnimeCount];

					p_stPCG = PCG_Get_Info(SP_PAT001 + AnimeCount);	/* SP */
					if(p_stPCG != NULL)
					{
						p_stPCG->x = TO_FIXED_POINT( g_nSP_x_ofset[AnimeCount] + mv_x);
						p_stPCG->y = TO_FIXED_POINT( 64 );
						p_stPCG->validty = TRUE;
						p_stPCG->update	= TRUE;

						if(g_pMasterImage == NULL)
						{
							g_pMasterImage = (uint8_t *)MyMalloc( (p_stPCG->Pat_w * SP_W) * (p_stPCG->Pat_h * SP_H));
//							memset(g_pMasterImage, 0, (size_t)(p_stPCG->Pat_w * SP_W) * (p_stPCG->Pat_h * SP_H));
							memcpy(g_pMasterImage, p_stPCG->pPatData, (p_stPCG->Pat_w * SP_W) * (p_stPCG->Pat_h * SP_H));
						}
						PCG_RotationEx(p_stPCG->pPatData, (uint16_t *)g_pMasterImage, (p_stPCG->Pat_w * SP_W), (p_stPCG->Pat_h * SP_H), ratio, rad);
					}
#else
#endif
				}

				if( s_b_Updata == TRUE )
				{
					s_b_Updata = FALSE;
				}
				SetTaskInfo(SCENE_GAME2);	/* シーン(処理)へ設定 */
				break;
			}
			case SCENE_GAME2:
			{
				if( g_AnimeSkipCount >= g_AnimeSkipCount )
				{
					g_AnimeSkipCount = 0;
					SetTaskInfo(SCENE_GAME1);	/* シーン(処理)へ設定 */
					break;
				}
				g_AnimeSkipCount++;
				break;
			}
			case SCENE_GAME2_E:
			{
				SetTaskInfo(SCENE_GAME_OVER_S);	/* シーン(処理)へ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_GAME_OVER_S:	
			{
				SetTaskInfo(SCENE_GAME_OVER);	/* シーン設定 */
				break;
			}
			case SCENE_GAME_OVER:
			{
				SetTaskInfo(SCENE_GAME_OVER_E);	/* シーン設定 */
				break;
			}
			case SCENE_GAME_OVER_E:
			{
				SetTaskInfo(SCENE_EXIT);	/* 終了シーンへ設定 */
				break;
			}
			/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
			case SCENE_EXIT:
			default:	/* 異常シーン */
			{
				ADPCM_Stop();
				loop = 0;	/* ループ終了 */
				break;
			}
		}
		SetFlip(TRUE);	/* フリップ許可 */

		PCG_END_SYNC(g_Vwait);	/* スプライトの出力 */

		uFreeRunCount++;	/* 16bit フリーランカウンタ更新 */
		g_unFrameCount++;	/* 16bit フリーランカウンタ更新 */

#if 0	/* デバッグコーナー */
		/* 処理時間計測 */
		GetNowTime(&time_now);
		unTime_cal = time_now - time_st;	/* LSB:1 UNIT:ms */
		g_unTime_cal = unTime_cal;
		if( stTask.bScene == SCENE_GAME1 )
		{
			unTime_cal_PH = Mmax(unTime_cal, unTime_cal_PH);
			g_unTime_cal_PH = unTime_cal_PH;
		}
#endif

#ifdef MEM_MONI	/* デバッグコーナー */
		GetFreeMem();	/* 空きメモリサイズ確認 */
#endif

	}
	while( loop );

	g_bExit = FALSE;

	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
static void App_Init(void)
{
	uint32_t nNum;

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init 開始");
#endif
#ifdef MEM_MONI	/* デバッグコーナー */
	g_nMaxUseMemSize = GetFreeMem();
	printf("FreeMem(%d[kb])\n", g_nMaxUseMemSize);	/* 空きメモリサイズ確認 */
	puts("App_Init メモリ");
#endif

	Set_SupervisorMode();	/* スーパーバイザーモード */
	/* MFP */
	MFP_INIT();
	if(Get_CPU_Time() == 0xFFFFu){	/* V-Sync割り込み等の初期化処理 */
		/* MFP */
		MFP_EXIT();				/* MFP関連の解除 */
		Set_UserMode();			/* ユーザーモード */
		puts("Timer-Dが必要です。CONFIG.SYSにてPROCESS行を使っている場合、*で無効化してください。");
		exit(-1);	/* 終了 */
	}

	Set_UserMode();			/* ユーザーモード */
#ifdef DEBUG	/* デバッグコーナー */
	printf("App_Init MFP(%d)\n", Get_CPU_Time());
#endif
	if(SetNowTime(0) == FALSE)
	{
		/*  */
	}
	nNum = Get_CPU_Time();	/* 300 = 10MHz基準 */
	if(nNum  < 400)
	{
		g_GameSpeed = 0;
		printf("Normal Speed(%d)\n", g_GameSpeed);
	}
	else if(nNum < 640)
	{
		g_GameSpeed = 2;
		printf("XVI Speed(%d)\n", g_GameSpeed);
	}
	else if(nNum < 800)
	{
		g_GameSpeed = 4;
		printf("RedZone Speed(%d)\n", g_GameSpeed);
	}
	else if(nNum < 2000)
	{
		g_GameSpeed = 7;
		printf("030 Speed(%d)\n", g_GameSpeed);
	}
	else
	{
		g_GameSpeed = 10;
		printf("Another Speed(%d)\n", g_GameSpeed);
	}
	puts("App_Init Input");
	Input_Init();			/* 入力初期化 */

	puts("App_Init FileList");
	Init_FileList_Load();	/* リストファイルの読み込み */

	puts("App_Init Music");
	/* 音楽 */
//	Init_Music();	/* 初期化(スーパーバイザーモードより前)	*/
//	Music_Play(BGM_INIT);	/* Init */
//	Music_Play(BGM_STOP);	/* Stop */

	Init_Sound();	/* 効果音ファイルの読み込み */
	ADPCM_Stop();

	/* スーパーバイザーモード開始 */
	Set_SupervisorMode();

	/* スプライトデータをメモリに読み込み */
	PCG_SP_PAL_DataLoad();	/* スプライト＆ＢＧ＆パレット読み込み */
//	PCG_PAL_DataLoad();
	PCG_INIT_CHAR();		/* スプライト＆ＢＧ定義セット */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init PCG(LOAD)");
#endif

#if CNF_MACS
	/* 動画 */
	MOV_INIT();		/* 初期化処理 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init Movie(MACS)");
#endif
#endif

	/* Task */
	TaskManage_Init();
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init Task");
#endif
	
	/* マウス初期化 */
	Mouse_Init();	/* マウス初期化 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init マウス");
#endif
	
	/* 画面表示 */
//	g_nCrtmod = CRTC_INIT(0);	/* mode=0 512x512 col:16/16 31kHz */
//	g_nCrtmod = CRTC_INIT(1);	/* mode=1 512x512 col:16/16 15kHz */
//	g_nCrtmod = CRTC_INIT(2);	/* mode=2 256x256 col:16/16 31kHz */
//	g_nCrtmod = CRTC_INIT(3);	/* mode=3 256x256 col:16/16 15kHz */
//	g_nCrtmod = CRTC_INIT(4);	/* mode=4 512x512 col:16/16 31kHz */
//	g_nCrtmod = CRTC_INIT(5);	/* mode=5 512x512 col:16/16 15kHz */
//	g_nCrtmod = CRTC_INIT(6);	/* mode=6 256x256 col:16/16 31kHz */
//	g_nCrtmod = CRTC_INIT(7);	/* mode=7 256x256 col:16/16 15kHz */
//	g_nCrtmod = CRTC_INIT(8);	/* mode=8 512x512 col:16/256 31kHz */
//	g_nCrtmod = CRTC_INIT(0x100+8);	/* mode=8 512x512 col:16/256 31kHz */
//	g_nCrtmod = CRTC_INIT(9);	/* mode=9 512x512 col:16/256 15kHz */
	g_nCrtmod = CRTC_INIT(10);	/* mode=10 256x256 col:16/256 31kHz */
//	g_nCrtmod = CRTC_INIT(11);	/* mode=11 256x256 col:16/256 15kHz */
//	g_nCrtmod = CRTC_INIT(12);	/* mode=12 512x512 col:16/65536 31kHz */
//	g_nCrtmod = CRTC_INIT(13);	/* mode=13 512x512 col:16/65536 15kHz */
//	g_nCrtmod = CRTC_INIT(14);	/* mode=14 256x256 col:16/65536 31kHz */
//	g_nCrtmod = CRTC_INIT(15);	/* mode=15 256x256 col:16/65536 15kHz */
//	g_nCrtmod = CRTC_INIT(16);	/* mode=16 764x512 col:16 31kHz */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init CRTC(画面)");
//	KeyHitESC();	/* デバッグ用 */
#endif

	/* グラフィック */
	G_INIT();			/* 画面の初期設定 */
	G_CLR();			/* クリッピングエリア全開＆消す */
	G_HOME(0);			/* ホーム位置設定 */
	G_VIDEO_INIT();		/* ビデオコントローラーの初期化 */
	G_PaletteSetZero();
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init グラフィック");
#endif

	/* スプライト／ＢＧ */
	PCG_INIT(PCG_NUM_MAX);	/* スプライト／ＢＧの初期化 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init スプライト／ＢＧデータ設定");
#endif
	/* DMA */
	DMAC_INIT();

	/* テキスト */
	T_INIT();	/* テキストＶＲＡＭ初期化 */
	T_PALET();	/* テキストパレット初期化 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init T_INIT");
#endif
	g_Vwait = 1;

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_Init 終了");
#endif
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
static void App_exit(void)
{
	int16_t	ret = 0;

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit 開始");
#endif
	
	if(g_bExit == TRUE)
	{
		puts("エラーをキャッチ！ ESC to skip");
		KeyHitESC();	/* デバッグ用 */
	}
	g_bExit = TRUE;

	ADPCM_Stop();

	/* グラフィック */
	G_CLR();			/* クリッピングエリア全開＆消す */

	/* スプライト＆ＢＧ */
	PCG_OFF();		/* スプライト＆ＢＧ非表示 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit スプライト");
#endif

	/* MFP */
	ret = MFP_EXIT();				/* MFP関連の解除 */
#ifdef DEBUG	/* デバッグコーナー */
	printf("App_exit MFP(%d)\n", ret);
#endif

	/* 画面 */
	CRTC_EXIT(0x100 + g_nCrtmod);	/* モードをもとに戻す */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit 画面");
#endif

	Mouse_Exit();	/* マウス非表示 */

	/* テキスト */
	T_EXIT();				/* テキスト終了処理 */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit テキスト");
#endif

	MyMfree(0);				/* 全てのメモリを解放 */
#ifdef DEBUG	/* デバッグコーナー */
	printf("MaxUseMem(%d[kb])\n", g_nMaxUseMemSize - GetMaxFreeMem());
	puts("App_exit メモリ");
#endif

	_dos_kflushio(0xFF);	/* キーバッファをクリア */
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit キーバッファクリア");
#endif

	/*スーパーバイザーモード終了*/
	Set_UserMode();
#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit スーパーバイザーモード終了");
#endif

#ifdef DEBUG	/* デバッグコーナー */
	puts("App_exit 終了");
#endif
}


/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	App_FlipProc(void)
{
	int16_t	ret = 0;
	
#ifdef FPS_MONI	/* デバッグコーナー */
	uint32_t time_now;
	static uint8_t	bFPS = 0u;
	static uint32_t	unTime_FPS = 0u;
#endif

	if(g_bFlip == FALSE)	/* 描画中なのでフリップしない */
	{
		return ret;
	}
	else
	{
#ifdef W_BUFFER
		ST_CRT		stCRT;
		GetCRT(&stCRT, g_mode);	/* 画面座標取得 */
#endif
		SetFlip(FALSE);			/* フリップ禁止 */
					
#ifdef W_BUFFER
		/* モードチェンジ */
		if(g_mode == 1u)		/* 上側判定 */
		{
			SetGameMode(2);
		}
		else if(g_mode == 2u)	/* 下側判定 */
		{
			SetGameMode(1);
		}
		else					/* その他 */
		{
			SetGameMode(0);
		}

		/* 非表示画面を表示画面へ切り替え */
		G_HOME(g_mode);

		/* 部分クリア */
		G_CLR_AREA(	stCRT.hide_offset_x, WIDTH,
					stCRT.hide_offset_y, HEIGHT, 0);
#endif
		
#ifdef FPS_MONI	/* デバッグコーナー */
		bFPS++;
#endif
		ret = 1;
	}

#ifdef FPS_MONI	/* デバッグコーナー */
	GetNowTime(&time_now);
	if( (time_now - unTime_FPS) >= 1000ul )
	{
		g_bFPS_PH = bFPS;
		unTime_FPS = time_now;
		bFPS = 0;
	}
#endif

	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	SetFlip(uint8_t bFlag)
{
	int16_t	ret = 0;

	Set_DI();	/* 割り込み禁止 */
	g_bFlip_old = g_bFlip;	/* フリップ前回値更新 */
	g_bFlip = bFlag;
	Set_EI();	/* 割り込み許可 */

	return ret;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void App_TimerProc( void )
{
	ST_TASK stTask;

	TaskManage();
	GetTaskInfo(&stTask);	/* タスク取得 */

	/* ↓↓↓ この間に処理を書く ↓↓↓ */
	if(stTask.b96ms == TRUE)	/* 96ms周期 */
	{
#if 1
		if(GetJoyAnalogMode() == TRUE)	/* アナログ入力 */
		{
			if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
			{
				g_ubDemoPlay = FALSE;	/* デモ解除 */

				SetTaskInfo(SCENE_INIT);	/* シーン(初期化処理)へ設定 */
			}
		}
		else	/* デジタル入力 */
		{
			if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
			{

			}
		}
#endif

#if 0
		switch(stTask.bScene)
		{
			case SCENE_GAME1:
			case SCENE_GAME2:
			{
#ifdef FPS_MONI	/* デバッグコーナー */
//				int8_t	sBuf[8];
#endif
#ifdef FPS_MONI	/* デバッグコーナー */
				memset(sBuf, 0, sizeof(sBuf));
				sprintf(sBuf, "%3d", g_bFPS_PH);
				Draw_Message_To_Graphic(sBuf, 24, 24, F_MOJI, F_MOJI_BAK);	/* デバッグ */
#endif
				break;
			}
			default:
				break;
		}
#endif
	}
	/* ↑↑↑ この間に処理を書く ↑↑↑ */

	/* タスク処理 */
	UpdateTaskInfo();		/* タスクの情報を更新 */

}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t App_RasterProc( uint16_t *pRaster_cnt )
{
	int16_t	ret = 0;
#if CNF_RASTER
	RasterProc(pRaster_cnt);	/* ラスター割り込み処理 */
#endif /* CNF_RASTER */
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void App_VsyncProc( void )
{
#if CNF_VDISP
	ST_TASK stTask;

//	puts("App_VsyncProc");
	Timer_D_Less_NowTime();

	GetTaskInfo(&stTask);	/* タスク取得 */
	/* ↓↓↓ この間に処理を書く ↓↓↓ */

	switch(stTask.bScene)
	{
		case SCENE_TITLE:
		case SCENE_GAME_OVER:
		{
#if 0
			int8_t x;
			int8_t y;
			int8_t left;
			int8_t right;
			if(GetJoyAnalogMode() == TRUE)	/* アナログ入力 */
			{
			}
			else	/* デジタル入力 */
			{
				if(Input_Main(g_ubDemoPlay) != 0u) 	/* 入力処理 */
				{
					g_ubDemoPlay = FALSE;	/* デモ解除 */

					SetTaskInfo(SCENE_INIT);	/* シーン(初期化処理)へ設定 */
				}
			}

			Mouse_GetDataPos(&x, &y, &left, &right);

			if(x == 0)
			{

			}
			else if(x > 0)
			{
				SetInput(KEY_RIGHT);
			}
			else{
				SetInput(KEY_LEFT);
			}

			if(y == 0)
			{

			}
			else if(y > 0)
			{
				SetInput(KEY_LOWER);
			}
			else{
				SetInput(KEY_UPPER);
			}

			if(left != 0)
			{
				SetInput(KEY_b_Z);
			}

			if(right != 0)
			{
				SetInput(KEY_b_X);
			}
			break;
#endif
		}
		default:	/* 異常シーン */
		{
			break;
		}
	}

	/* ↑↑↑ この間に処理を書く ↑↑↑ */
	
	App_FlipProc();	/* 画面切り替え */

#endif /* CNF_VDISP */
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	GetGameMode(uint8_t *bMode)
{
	int16_t	ret = 0;
	
	*bMode = g_mode;
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t	SetGameMode(uint8_t bMode)
{
	int16_t	ret = 0;
	
	g_mode = bMode;
	if(bMode == 1)
	{
		g_mode_rev = 2;
	}
	else
	{
		g_mode_rev = 1;
	}
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void HelpMessage(void)
{
	T_Clear();		/* テキストクリア */

	printf("=<Help Message>=================================================\n");
	printf("  sp512.X \n");
	printf("   ex. >sp512.x\n");
	printf("        CYNTHIA +32kByteメモリ増設確認ツール\n");
	printf("================================================================\n");
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t main(int16_t argc, int8_t** argv)
{
	int16_t	ret = 0;

	/* COPYキー無効化 */
	init_trap12();
	/* 例外ハンドリング処理 */
	usr_abort = App_exit;	/* 例外発生で終了処理を実施する */
	init_trap14();			/* デバッグ用致命的エラーハンドリング */
#if 0	/* アドレスエラー発生 */
	{
		char buf[10];
		int *A = (int *)&buf[1];
		int B = *A;
		return B;
	}
#endif

	App_Init();		/* 初期化 */

	printf("CYNTHIA +32kByteメモリ増設確認ツール「sp512p.x」Ver%d.%d.%d (c)2026 カタ.\n", MAJOR_VER, MINOR_VER, PATCH_VER);

	if(argc > 1)	/* オプションチェック */
	{
		int16_t i;
		
		for(i = 1; i < argc; i++)
		{
			int8_t	bOption = FALSE;
			int8_t	bFlag;
			
			bOption	= ((argv[i][0] == '-') || (argv[i][0] == '/')) ? TRUE : FALSE;

			if(bOption == TRUE)
			{
				/* ヘルプ */
				bFlag	= ((argv[i][1] == '?') || (argv[i][1] == 'h') || (argv[i][1] == 'H')) ? TRUE : FALSE;
				if(bFlag == TRUE)
				{
					HelpMessage();	/* ヘルプ */
					ret = -1;
					break;
				}

				/* スキップカウント */
				bFlag	= ((argv[i][1] == 's') || (argv[i][1] == 'S')) ? TRUE : FALSE;
				if(bFlag == TRUE)
				{
					g_AnimeSkipCount = atoi(argv[i+1]);
					continue;
				}
				
				/* ヘルプ */
				if(bFlag == FALSE)
				{
					HelpMessage();	/* ヘルプ */
					ret = -1;
					break;
				}
			}
		}
	}
	else
	{
		HelpMessage();	/* ヘルプ */
		ret = -1;
	}
	
	main_Task();	/* メイン処理 */

	App_exit();		/* 終了処理 */
	
	return ret;
}

#endif	/* MAIN_C */
