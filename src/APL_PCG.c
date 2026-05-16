#ifndef	APL_PCG_C
#define	APL_PCG_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iocslib.h>
#include <doslib.h>

#include <usr_macro.h>
#include "BIOS_PCG.h"
#include "IF_PCG.h"
#include "IF_FileManager.h"
#include "IF_Math.h"
#include "APL_PCG.h"

/* define定義 */

/* グローバル変数 */

/* 関数のプロトタイプ宣言 */
void PCG_INIT_CHAR(void);

/* 関数 */
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_INIT_CHAR(void)
{
	uint32_t	i, j;
	uint32_t	uPCG_list = 0;
	uint32_t	uPCG_num;
	uint16_t	uPCG_SP_offset = 0;
	uint16_t	uPCG_SP_next = 0x0;	/* 0=BG */
	uint8_t		ubOK;
	uint8_t		ubPlaneUp = FALSE;
	uint8_t		ubPri = 0x00;
	uint8_t		ubPal = 0x00;
	uint16_t	uPlane = 0xFFFF;

	uPCG_SP_offset = PCG_16x16_AREA;

	puts("SP PCG_INIT_CHAR");
	/* スプライト管理用バッファのクリア */
	for(uPCG_num = 0; uPCG_num < PCG_NUM_MAX; uPCG_num++)
	{
		memset(&g_stPCG_DATA[uPCG_num], 0, sizeof(ST_PCG) );
	}

	/* 自車スプライト */
	for(uPCG_num = 0; uPCG_num < PCG_NUM_MAX; uPCG_num++)
	{
		ST_PCG stPCG;
		
		/* たちまち初期化は不要 */
		stPCG.x			= 0;	/* x座標 */
		stPCG.y			= 0;	/* y座標 */
		stPCG.dx		= 0;	/* 移動量x */
		stPCG.dy		= 0;	/* 移動量y */
		stPCG.Anime		= 0;	/* 現在のアニメ */
		stPCG.Anime_old	= 0;	/* 前回のアニメ */
		
		ubPlaneUp = FALSE;

		switch(uPCG_num)
		{
#if 0
			case	BG_DATA:
			{
				ubOK = FALSE;
				break;
			}
#endif
			case	SP_PAT001	:	/* ブロック */
			case	SP_PAT002	:	/* ブロック */
			case	SP_PAT003	:	/* ブロック */
			case	SP_PAT004	:	/* ブロック */
			{
				uPCG_list = 0;
				ubPlaneUp = TRUE;
				ubOK = TRUE;
				break;
			}
			default:
			{
				ubOK = FALSE;
				break;
			}
		}
		
		if(ubOK == TRUE)
		{
			stPCG.Pat_w			= g_stST_PCG_LIST[uPCG_list].Pat_w;
			stPCG.Pat_h			= g_stST_PCG_LIST[uPCG_list].Pat_h;
			stPCG.Pat_AnimeMax	= g_stST_PCG_LIST[uPCG_list].Pat_AnimeMax;
			stPCG.Pat_DataMax	= stPCG.Pat_w * stPCG.Pat_h * stPCG.Pat_AnimeMax;
			ubPal 				= g_stST_PCG_LIST[uPCG_list].Pal;				/* パレット番号 */
#if  CNF_XSP
			PCG_Load_Data(NULL, 0x00, stPCG, uPCG_num, 3);	/* 3:メモリ確保のみ */
			ubPri = 0x20;														/* プライオリティ */
			uPlane = uPCG_SP_offset + uPCG_SP_next;	/* スプライトNo. */
#else
			PCG_Load_Data(NULL, 0x00, stPCG, uPCG_num, 3);	/* 3:メモリ確保のみ */
			g_stPCG_DATA[uPCG_num].pPatData = (uint16_t *)&g_pcg_dat[SP_16_SIZE * (uPCG_SP_offset + uPCG_SP_next)];	/* 只のコピー */
			ubPri = 0xFF;
			uPlane = 0xFFFF;
#endif
			if(ubPlaneUp == TRUE)
			{
				uPCG_SP_next += stPCG.Pat_DataMax;
			}

			/* スプライト定義設定 */
			for(j=0; j < stPCG.Pat_DataMax; j++)
			{
#if  CNF_XSP
				*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + j)	= SetXSPinfo(0, 0, ubPal, ubPri);	/* パターンコードテーブル */
#else
	#if XEiJ_Ex_SP
				*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + j)	= SetBGcodeEx(0, 0, 0, ubPal, 0xFF);	/* パターンコードテーブル */
	#else
				*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + j)	= SetBGcode(0, 0, ubPal, j);	/* パターンコードテーブル */
//				*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + j)	= SetBGcode2(0, 0, ubPal);	/* パターンコードテーブル */
	#endif
#endif
			}
			g_stPCG_DATA[uPCG_num].update	= FALSE;
			g_stPCG_DATA[uPCG_num].validty	= TRUE;
			g_stPCG_DATA[uPCG_num].Plane	= uPlane;		/* プレーンNo.(XSPはスプライト番号) */
		}
		else
		{
			g_stPCG_DATA[uPCG_num].Pat_w		= 0;
			g_stPCG_DATA[uPCG_num].Pat_h		= 0;
			g_stPCG_DATA[uPCG_num].update	= FALSE;
			g_stPCG_DATA[uPCG_num].validty	= FALSE;
		}


#ifdef DEBUG
//		printf("%d, %d, %d, %d, %d, %d\n", uPCG_num, stPCG.Pat_w, stPCG.Pat_h, stPCG.Pat_AnimeMax, stPCG.Pat_DataMax, ubPal);
#endif
#if 1
		if((uPCG_num % 10) == 0)
		{
			printf("\n");
		}
		printf("..........");
		printf("\033[10D");	/* $1B ESC[10D 左に10文字分移動 */
		for(i=0; i<(uPCG_num % 10)+1; i++)
		{
			if(ubOK == TRUE)	printf("*");
			else 				printf("x");
		}
		printf("\n");
		printf("\033[1A");	/* $1B ESC[1A 1行上に移動 */
#endif

#ifdef DEBUG
//		printf("g_stPCG_DATA[%d]=0x%p\n", uPCG_num, &g_stPCG_DATA[uPCG_num]);
//		KeyHitESC();	/* デバッグ用 */
#endif
	}

	if((uPCG_SP_offset + uPCG_SP_next) >= PCG_MAX)
	{
		printf("\nERROR:PCG定義OVER(%d >= %d)\n", (uPCG_SP_offset + uPCG_SP_next), PCG_MAX);
	}
	else
	{
		printf("\n(%d)...Ok!\n", (uPCG_SP_offset + uPCG_SP_next));
	}
}

#endif	/* APL_PCG_C */

