#ifndef	BIOS_PCG_C
#define	BIOS_PCG_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iocslib.h>
#include <doslib.h>

#include <usr_macro.h>
#include "BIOS_PCG.h"
#include "IF_FileManager.h"
#include "IF_Math.h"
#include "IF_Memory.h"

/* define定義 */

/* グローバル変数 */
/* ＰＣＧデータ */
int8_t			g_sp_list[PCG_LIST_MAX][256]	=	{0};
uint32_t		g_sp_list_max	=	0u;
static size_t	g_sp_dat_size[PCG_LIST_MAX]	=	{0};

int8_t			g_pal_list[PAL_LIST_MAX][256]	=	{0};
uint32_t		g_pal_list_max	=	0u;
static size_t	g_pal_dat_size[PAL_LIST_MAX]	=	{0};

int8_t 			g_pcg_alt[PCG_MAX + 1];			/* XSP 用 PCG 配置管理テーブル	スプライト PCG パターン最大使用数 + 1 バイトのサイズが必要。*/
int8_t 			g_pcg_dat[PCG_MAX * 128];		/* ＰＣＧデータファイル読み込みバッファ */
uint16_t 		g_pal_dat[PAL_LIST_MAX][16];	/* パレットデータファイル読み込みバッファ */

ST_PCG	g_stPCG_DATA[PCG_MAX] = {0};
ST_PCG_LIST g_stST_PCG_LIST[PCG_LIST_MAX] = {0};

#if XEiJ_Ex_SP
volatile uint16_t	*XEiJ_ExSP_BNKCTRL = (uint16_t *)0xEB0812u;
volatile uint16_t	*XEiJ_ExSP_BNKSEL  = (uint16_t *)0xEB0814u;
#endif

/* 関数のプロトタイプ宣言 */
void PCG_VIEW(uint8_t);
void PCG_Rotation(uint16_t *, uint16_t *, uint8_t, uint8_t, int16_t, int16_t, uint8_t *, uint8_t, uint16_t, uint16_t);
int16_t PCG_Main(uint16_t);
ST_PCG *PCG_Get_Info(uint16_t);
int16_t PCG_Set_Info(ST_PCG, uint16_t);
void PCG_Set_PAL_Data(int16_t, int16_t, int16_t);
int16_t PCG_Load_Data(int8_t *, uint16_t , ST_PCG , uint16_t , uint8_t );
int16_t PCG_Load_PAL_Data(int8_t *, uint16_t, uint16_t);
int16_t PCG_PAL_Change(uint16_t , uint16_t , uint16_t);
int32_t SP_DEFCG_Ex(int32_t, int16_t, int16_t, int8_t, int32_t);
int32_t SP_REGST_Ex(int32_t, int32_t, int32_t, int32_t, int32_t);
void PCG_PCG_to_vCG(uint16_t *, uint16_t *, int16_t, int16_t);
void PCG_vCG_to_PCG(uint16_t *, uint16_t *, int16_t, int16_t);
void PCG_RotationEx(uint16_t *, uint16_t *, int16_t, int16_t, uint16_t, uint16_t);
/* 関数 */
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_VIEW(uint8_t bSW)
{
	if((bSW & Bit_0) != 0u)
	{
		_iocs_sp_on();				/* スプライト表示をＯＮ */

		if((bSW & Bit_1) != 0u)
		{
			_iocs_bgctrlst(0, 0, 1);	/* ＢＧ０表示ＯＮ */
		}

		if((bSW & Bit_2) != 0u)
		{
			_iocs_bgctrlst(1, 1, 1);	/* ＢＧ１表示ＯＮ */
		}
	}
	else
	{
		if((bSW & Bit_1) != 0u)
		{
			_iocs_bgctrlst(0, 0, 0);	/* ＢＧ０表示ＯＦＦ */
		}

		if((bSW & Bit_2) != 0u)
		{
			_iocs_bgctrlst(1, 1, 0);	/* ＢＧ１表示ＯＦＦ */
		}

		_iocs_sp_off();				/* スプライト表示をＯＦＦ */
	}
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_Rotation(uint16_t *pDst, uint16_t *pSrc, uint8_t bX_num, uint8_t bY_num, int16_t pos_x, int16_t pos_y, uint8_t *sp_num, uint8_t palNum, uint16_t ratio, uint16_t rad)
{
	int16_t	x, y, vx, vy;
	int16_t	width, height;
	uint64_t	uPCG;
	uint64_t	uPCG_ARY[8];
	uint64_t	*pDstWork, *pSrcWork;
	uint8_t	*pBuf, *pDstBuf = NULL, *pSrcBuf = NULL;
	uint64_t	uADDR;
	uint64_t	uPointer_ADR, uPointer_ADR_X, uPointer_ADR_Y, uPointer_ADR_subX, uPointer_ADR_subY;
	uint16_t	uMem_size;
	uint8_t	bShift;
	uint64_t	code = 0;
	uint8_t	V=0, H=0;
	uint8_t	spNum;	
//	int8_t	bEx_num=0;
	
	/* src size */
	width =  16 * (int16_t)bX_num;
	height = 16 * (int16_t)bY_num;
	uMem_size = width * height * sizeof(uint8_t);
	
	if((ratio > 0) && (ratio <= 16))
	{
		bShift = ratio;	/* def:2048(=8) */
	}
	else
	{
		bShift = 8;
	}
//	Message_Num(&bShift,   0, 9, 2, MONI_Type_UC, "%2d");
//	Message_Num(&bEx_num, 11, 9, 2, MONI_Type_SC, "%2d");
	
	/* 作業エリア確保 */
	pSrcBuf = (uint8_t*)MyMalloc( (int32_t)uMem_size );
	if( pSrcBuf == NULL )
	{
		return;
	}
	pBuf = pSrcBuf;

	/* PCG -> CG */
	uPointer_ADR = (uint64_t)pSrc;
	uPointer_ADR_Y = 0ul;
	
	for(y = 0; y < bY_num; y++)	/* 一行分 */
	{
		uPointer_ADR_subY = 0ul;
		
		for(vy = 0; vy < 16; vy++)	/* 16bit分(8dot+8dot) */
		{
			uPointer_ADR_X = 0ul;
			
			for(x = 0; x < bX_num; x++)	/* 一列分 */
			{
				uPointer_ADR_subX = 0ul;
				
				for(vx = 0; vx < 2; vx++)	/* 16bit分 */
				{
					uADDR = uPointer_ADR + uPointer_ADR_Y + uPointer_ADR_subY + uPointer_ADR_X + uPointer_ADR_subX;	/* 原点 */
					pSrcWork = (uint64_t *)uADDR;	/* 0と2/1と3 */

					uPCG = (uint64_t)*pSrcWork;
					
					*pBuf = (uint8_t)((uPCG >> 28ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 24ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 20ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 16ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 12ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >>  8ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >>  4ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG        ) & 0x0Fu);
					pBuf++;
					
					uPointer_ADR_subX += 0x40ul;
				}
				uPointer_ADR_X += 0x80ul;
			}
			uPointer_ADR_subY += 0x04ul;
		}
		uPointer_ADR_Y += 0x800ul;
	}

	/* 画像処理 */
#if 0
	/* 更に領域を拡張する機能を実装しようとしたが断念 */
	width =  16 * Mmax(((int16_t)bX_num + bEx_num), 1);
	height = 16 * Mmax(((int16_t)bY_num + bEx_num), 1);
	uMem_size = width * height * sizeof(uint8_t);
#endif
	
	pDstBuf = (uint8_t*)MyMalloc( (int32_t)uMem_size );
	if( pDstBuf == NULL )
	{
		MyMfree(pSrcBuf);	/* メモリ解放 */
		return;
	}
	pBuf = pDstBuf;
	memset(pBuf, 0, (size_t)uMem_size);
	
	if((rad == 0) && (bShift == 8))
	{
		memcpy( pDstBuf, pSrcBuf, (size_t)uMem_size );	/* 只のコピー */
	}
	else
	{
		int16_t	dx, dy;
		int16_t	width_h, height_h;
		int16_t	cos, sin;
		uint8_t	*pDstGR, *pSrcGR;
		
		cos = APL_Cos(rad);
		sin = APL_Sin(rad);
		width_h = width >> 1;
		height_h = height >> 1;

		pSrcGR = pSrcBuf;
		pDstGR = pBuf;
		
		for(y = 0; y < height; y++)
		{
			dy = y - height_h;
			
			for(x = 0; x < width; x++)
			{
				dx = x - width_h;
				
				vx = (((dx * cos) - (dy * sin)) >> bShift) + width_h;
				vy = (((dx * sin) + (dy * cos)) >> bShift) + height_h;
				
				if( (vx >= 0) && (vy >= 0) && (vx < width) && (vy < height) )
				{
					pSrcGR = pSrcBuf + ((vy * width) + vx);
					*pDstGR = *pSrcGR;
				}
				pDstGR++;
			}
		}
	}
	MyMfree(pSrcBuf);	/* メモリ解放 */

	/* CG -> PCG */
	pBuf = pDstBuf;

	uPointer_ADR = (uint64_t)pDst;
	uPointer_ADR_Y = 0ul;

	for(y = 0; y < bY_num; y++)	/* 一行分 */
	{
		uPointer_ADR_subY = 0ul;
		
		for(vy = 0; vy < 16; vy++)	/* 16bit分(8dot+8dot) */
		{
			uPointer_ADR_X = 0ul;
			
			for(x = 0; x < bX_num; x++)	/* 一列分 */
			{
				uPointer_ADR_subX = 0ul;
				
				for(vx = 0; vx < 2; vx++)	/* 16bit分 */
				{
					uADDR = uPointer_ADR + uPointer_ADR_Y + uPointer_ADR_subY + uPointer_ADR_X + uPointer_ADR_subX;	/* 原点 */
				
					uPCG_ARY[7] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[6] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[5] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[4] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[3] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[2] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[1] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[0] = (uint64_t)*pBuf;
					pBuf++;
					
					uPCG = 	((uPCG_ARY[7] << 28ul) & 0xF0000000ul) |
							((uPCG_ARY[6] << 24ul) & 0x0F000000ul) |
							((uPCG_ARY[5] << 20ul) & 0x00F00000ul) |
							((uPCG_ARY[4] << 16ul) & 0x000F0000ul) |
							((uPCG_ARY[3] << 12ul) & 0x0000F000ul) |
							((uPCG_ARY[2] <<  8ul) & 0x00000F00ul) |
							((uPCG_ARY[1] <<  4ul) & 0x000000F0ul) |
							((uPCG_ARY[0]        ) & 0x0000000Ful);

					pDstWork = (uint64_t *) uADDR;
					*pDstWork = uPCG;

					uPointer_ADR_subX += 0x40ul;
				}
				uPointer_ADR_X += 0x80ul;
			}
			uPointer_ADR_subY += 0x04ul;
		}
		uPointer_ADR_Y += 0x800ul;
	}
	
	MyMfree(pDstBuf);	/* メモリ解放 */

	/* PCG -> SP */
	uPointer_ADR = (uint64_t)pDst;
	uPointer_ADR_Y = 0ul;
	spNum = *sp_num;
	
	for(y = 0; y < bY_num; y++)	/* 一行分 */
	{
		uPointer_ADR_X = 0ul;

		for(x = 0; x < bX_num; x++)	/* 一列分 */
		{
			uint8_t	patNum;
			patNum = (uint8_t)( (uint64_t) ( (uPointer_ADR - 0xEB8000ul) + uPointer_ADR_Y + uPointer_ADR_X ) / 0x80ul); /*0x43*/;
			code = (uint64_t)( 0xCFFFU & ( ((V & 0x01U)<<15U) | ((H & 0x01U)<<14U) | ((palNum & 0xFU)<<8U) | (patNum & 0xFFU) ) );
			SP_REGST( spNum++, -1, pos_x + (x * 16), pos_y + (y * 16), code, 3);
			*sp_num = spNum;
			uPointer_ADR_X += 0x80ul;
		}
		uPointer_ADR_Y += 0x800ul;
	}
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
// vCGは、1byteの縦(16の倍数)×横(16の倍数)の仮想CG(値はパレット番号) pDstは、予めメモリを確保しておく
void PCG_PCG_to_vCG(uint16_t *pDst, uint16_t *pSrc, int16_t width, int16_t height)
{
	int16_t		x, y, vx, vy;
	uint64_t	uPCG;
	uint64_t	*pSrcWork;
	uint8_t		*pBuf;
	uint64_t	uADDR;
	uint64_t	uPointer_ADR, uPointer_ADR_X, uPointer_ADR_Y, uPointer_ADR_subX, uPointer_ADR_subY;
	uint8_t 	bX_num, bY_num;

	if(pDst == NULL )
	{
		return;
	}
	pBuf = (uint8_t *)pDst;

	bX_num = width / 16;
	bY_num = height / 16;
	
	/* PCG -> CG */
	uPointer_ADR = (uint64_t)pSrc;
	uPointer_ADR_Y = 0ul;

	for(y = 0; y < bY_num; y++)	/* 一行分 */
	{
		uPointer_ADR_subY = 0ul;
		
		for(vy = 0; vy < 16; vy++)	/* 16bit分(8dot+8dot) */
		{
			uPointer_ADR_X = 0ul;
			
			for(x = 0; x < bX_num; x++)	/* 一列分 */
			{
				uPointer_ADR_subX = 0ul;
				
				for(vx = 0; vx < 2; vx++)	/* 16bit分 */
				{
					uADDR = uPointer_ADR + uPointer_ADR_Y + uPointer_ADR_subY + uPointer_ADR_X + uPointer_ADR_subX;	/* 原点 */
					pSrcWork = (uint64_t *)uADDR;	/* 0と2/1と3 */

					uPCG = (uint64_t)*pSrcWork;
					
					*pBuf = (uint8_t)((uPCG >> 28ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 24ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 20ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 16ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >> 12ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >>  8ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG >>  4ul) & 0x0Fu);
					pBuf++;
					*pBuf = (uint8_t)((uPCG        ) & 0x0Fu);
					pBuf++;
					
					uPointer_ADR_subX += 0x40ul;
				}
				uPointer_ADR_X += 0x80ul;
			}
			uPointer_ADR_subY += 0x04ul;
		}
		uPointer_ADR_Y += 0x800ul;
	}
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
// vCGは、1byteの縦(16の倍数)×横(16の倍数)の仮想CG(値はパレット番号) pDstは、書き換えたいPCGデータのポインタ
void PCG_vCG_to_PCG(uint16_t *pDst, uint16_t *pSrc, int16_t width, int16_t height)
{
	int16_t	x, y, vx, vy;
	uint64_t	uPCG;
	uint64_t	uPCG_ARY[8];
	uint64_t	*pDstWork;
	uint8_t		*pBuf;
	uint64_t	uADDR;
	uint64_t	uPointer_ADR, uPointer_ADR_X, uPointer_ADR_Y, uPointer_ADR_subX, uPointer_ADR_subY;
	uint8_t 	bX_num, bY_num;
	
	if(pSrc == NULL )
	{
		return;
	}
	pBuf = (uint8_t *)pSrc;

	bX_num = width / 16;
	bY_num = height / 16;
	
	/* CG -> PCG */
	uPointer_ADR = (uint64_t)pDst;
	uPointer_ADR_Y = 0ul;

	for(y = 0; y < bY_num; y++)	/* 一行分 */
	{
		uPointer_ADR_subY = 0ul;
		
		for(vy = 0; vy < 16; vy++)	/* 16bit分(8dot+8dot) */
		{
			uPointer_ADR_X = 0ul;
			
			for(x = 0; x < bX_num; x++)	/* 一列分 */
			{
				uPointer_ADR_subX = 0ul;
				
				for(vx = 0; vx < 2; vx++)	/* 16bit分 */
				{
					uADDR = uPointer_ADR + uPointer_ADR_Y + uPointer_ADR_subY + uPointer_ADR_X + uPointer_ADR_subX;	/* 原点 */
				
					uPCG_ARY[7] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[6] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[5] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[4] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[3] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[2] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[1] = (uint64_t)*pBuf;
					pBuf++;
					uPCG_ARY[0] = (uint64_t)*pBuf;
					pBuf++;
					
					uPCG = 	((uPCG_ARY[7] << 28ul) & 0xF0000000ul) |
							((uPCG_ARY[6] << 24ul) & 0x0F000000ul) |
							((uPCG_ARY[5] << 20ul) & 0x00F00000ul) |
							((uPCG_ARY[4] << 16ul) & 0x000F0000ul) |
							((uPCG_ARY[3] << 12ul) & 0x0000F000ul) |
							((uPCG_ARY[2] <<  8ul) & 0x00000F00ul) |
							((uPCG_ARY[1] <<  4ul) & 0x000000F0ul) |
							((uPCG_ARY[0]        ) & 0x0000000Ful);

					pDstWork = (uint64_t *) uADDR;
					*pDstWork = uPCG;

					uPointer_ADR_subX += 0x40ul;
				}
				uPointer_ADR_X += 0x80ul;
			}
			uPointer_ADR_subY += 0x04ul;
		}
		uPointer_ADR_Y += 0x800ul;
	}
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_RotationEx(uint16_t *pDst, uint16_t *pSrc, int16_t width, int16_t height, uint16_t ratio, uint16_t rad)
{
	size_t	uMem_size;
	
	/* src size */
	uMem_size = width * height * sizeof(uint8_t);

	if((rad == 0) && (ratio == 15))	/* 変更なし */
	{
		memcpy( (uint8_t *)pDst, (uint8_t *)pSrc, uMem_size );	/* 只のコピー */
	}
	else	/* 回転拡大縮小 */
	{
		int16_t	x, y, vx, vy;
		int16_t	dx, dy;
		int16_t	width_h, height_h;
		int16_t	width_new, height_new;
		int16_t	width_new_h, height_new_h;
		int16_t	cos, sin;
		uint8_t	*pDstGR, *pSrcGR;
		uint8_t	*pDstWork, *pSrcWork;
		uint8_t	*pDstBuf, *pSrcBuf;

#if 0
		uint8_t bX_num, bY_num;
		int16_t x_min = 32767, y_min = 32767, x_max = 0, y_max = 0, wide;

		wide = Mmax(width, height);
#endif

		pSrcBuf = (uint8_t *)MyMalloc( uMem_size );
		pSrcWork = pSrcBuf;

		/* PCG to vCG */
		PCG_PCG_to_vCG((uint16_t *)pSrcBuf, pSrc, width, height);
		
		/* 回転拡大縮小 */
		cos = APL_Cos(rad);
		sin = APL_Sin(rad);
		width_h = width >> 1;
		height_h = height >> 1;
#if 0
		for(y = 0; y < height; y++)
		{
			dy = (y - height_h);
			
			for(x = 0; x < width; x++)
			{
				dx = (x - width_h);
				
				vx = Mdiv256((dx * cos) - (dy * sin)) + width_h;
				vy = Mdiv256((dx * sin) + (dy * cos)) + height_h;

				x_min = Mmin(x_min, vx);
				y_min = Mmin(y_min, vy);
				x_max = Mmax(x_max, vx);
				y_max = Mmax(y_max, vy);
			}
		}

		width_new  = (((x_max - x_min) + 15) / 16) * 16;
		height_new = (((y_max - y_min) + 15) / 16) * 16;
		width_new_h = width_new >> 1;
		height_new_h = height_new >> 1;
		printf("w/h(%3d, %3d)->(%3d, %3d) min(%3d, %3d) max(%3d, %3d) ratio/rad(%3d, %3d)\n", width, height, width_new, height_new, x_min, y_min, x_max, y_max, ratio, rad);
//		KeyHitESC();	/* デバッグ用 */
//		uMem_size = width_new * height_new * sizeof(uint8_t);

		bX_num = width_new / 16;
		bY_num = height_new / 16;

#else
		width_new  = width;
		height_new = height;
		width_new_h = width_h;
		height_new_h = height_h;
#endif

		pDstBuf = (uint8_t *)MyMalloc( uMem_size );
		pDstWork = pDstBuf;

		/* Change アフィン変換 */
		for(y = 0; y < height_new; y++)
		{
			switch(ratio)
			{
			case  0:	{dy = (y -height_h);	break;}
			case  1:	{dy = Mmul_1p10(y -height_h);	break;}
			case  2:	{dy = Mmul_1p20(y -height_h);	break;}
			case  3:	{dy = Mmul_1p25(y -height_h);	break;}
			case  4:	{dy = Mmul_1p30(y -height_h);	break;}
			case  5:	{dy = Mmul_1p40(y -height_h);	break;}
			case  6:	{dy = Mmul_1p50(y -height_h);	break;}
			case  7:	{dy = Mmul_1p60(y -height_h);	break;}
			case  8:	{dy = Mmul_1p70(y -height_h);	break;}
			case  9:	{dy = Mmul_1p75(y -height_h);	break;}
			case 11:	{dy = Mmul_1p90(y -height_h);	break;}
			case 10:	{dy = Mmul_1p80(y -height_h);	break;}
			default:	{dy = Mmul_1p00(y -height_h);	break;}
			}


			for(x = 0; x < width_new; x++)
			{
				switch(ratio)
				{
				case  0:	{dx = (x - width_h);	break;}
				case  1:	{dx = Mmul_1p10(x - width_h);	break;}
				case  2:	{dx = Mmul_1p20(x - width_h);	break;}
				case  3:	{dx = Mmul_1p25(x - width_h);	break;}
				case  4:	{dx = Mmul_1p30(x - width_h);	break;}
				case  5:	{dx = Mmul_1p40(x - width_h);	break;}
				case  6:	{dx = Mmul_1p50(x - width_h);	break;}
				case  7:	{dx = Mmul_1p60(x - width_h);	break;}
				case  8:	{dx = Mmul_1p70(x - width_h);	break;}
				case  9:	{dx = Mmul_1p75(x - width_h);	break;}
				case 10:	{dx = Mmul_1p80(x - width_h);	break;}
				case 11:	{dx = Mmul_1p90(x - width_h);	break;}
				default:	{dx = Mmul_1p00(x - width_h);	break;}
				}
				
				vx = Mdiv256((dx * cos) - (dy * sin)) + width_new_h;
				vy = Mdiv256((dx * sin) + (dy * cos)) + height_new_h;

				pDstGR = pDstWork + ((y * width_new) + x);
				
				if( (vx >= 0) && (vy >= 0) && (vx < width) && (vy < height) )
				{
					pSrcGR = pSrcWork + ((vy * width) + vx);
					*pDstGR = *pSrcGR;
				}
				else
				{
					*pDstGR = 0;
				}
			}
		}
		
		/* vCG to PCG */
		PCG_vCG_to_PCG(pDst, (uint16_t *)pDstWork, width, height);

		MyMfree(pSrcBuf);	/* メモリ解放 */
		MyMfree(pDstBuf);	/* メモリ解放 */
	}
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t PCG_Main(uint16_t pcg_num_max)
{
	int16_t	ret = 0;
	uint16_t	Plane_num;
#if  CNF_XSP
#else
	#if XEiJ_Ex_SP
	int16_t i;
	uint16_t	Bank_num = 0;
	#endif
#endif
	int16_t		uPCG_num;
	int16_t		uWidth = 0, uHeight = 0;
	int16_t		x, y, mx, my, ofx, ofy, pat_size, Anime_pat_size, pat_data_addr, DataMax;
	uint8_t		Anime_num, Anime_num_old;
	int16_t		count, pat_count, total_pat_count;
	int16_t		bank_bit;

#if  CNF_XSP
#else
	int32_t		sync = 0;
	int16_t		uPAL_num = 0;
	int16_t		view_count = 0;
	static uint16_t	s_uPCG_Num = PCG_16x16_AREA;
	static uint16_t	s_uPCG_pat_count_Max = 0;

#endif

#if  CNF_XSP
#else
	#if XEiJ_Ex_SP
	#else
		sync = 0x80000000;
	#endif
#endif

	mx = 0;
	my = 0;
	pat_count = 0;
	total_pat_count = 0;

	for(uPCG_num = 0; uPCG_num < pcg_num_max; uPCG_num++)
	{
		if( g_stPCG_DATA[uPCG_num].Pat_h == 0 )
		{
			continue;
		}
		
		if( g_stPCG_DATA[uPCG_num].Pat_w == 0 )
		{
			continue;
		}

		DataMax = g_stPCG_DATA[uPCG_num].Pat_DataMax;

		if(g_stPCG_DATA[uPCG_num].validty == FALSE)
		{
				total_pat_count += DataMax;
				continue;
		}

		/* プレーン番号 */
		Plane_num = g_stPCG_DATA[uPCG_num].Plane;

		/* パターンサイズ */
		pat_size = g_stPCG_DATA[uPCG_num].Pat_w * g_stPCG_DATA[uPCG_num].Pat_h;
		
		/* アニメーション番号 */
		Anime_num_old = g_stPCG_DATA[uPCG_num].Anime_old;	/* 前回のアニメの番号 */
		Anime_num = g_stPCG_DATA[uPCG_num].Anime;			/* 現在のアニメの番号 */
		
		Anime_pat_size = Anime_num * pat_size;	/* 現在のアニメの位置 */
		
		/* スプライト設定 */
		count = 0;
		uHeight = 0;

		for( y = 0; y < g_stPCG_DATA[uPCG_num].Pat_h; y++ )
		{
			uWidth = 0;
			for( x = 0; x < g_stPCG_DATA[uPCG_num].Pat_w; x++ )
			{
#if  CNF_XSP
#else
				int16_t	uPCG_prw;
				bank_bit = 0;
#endif
				pat_data_addr = (count + (Anime_pat_size));
#if  CNF_XSP
				/* XSPは複数登録できるので不要 */
#else
				/* PCGを定義、再定義する */
				if(	g_stPCG_DATA[uPCG_num].Plane == 0xFFFF ) /* 初回 */
				{
					if(count == 0)
					{
						Plane_num = s_uPCG_Num + total_pat_count;

						uPAL_num = (uPCG_num % 2);
						PCG_Set_PAL_Data(uPAL_num, 1, 1);	/* パレットデータ設定 */
					}
	#if XEiJ_Ex_SP
					uPAL_num = g_stST_PCG_LIST[uPCG_num].Pal % PAL_MAX;
					if(uPAL_num == 0)uPAL_num = 1;
					PCG_Set_PAL_Data(uPAL_num, g_stST_PCG_LIST[uPCG_num].Pal, 0);	/* パレットデータ設定 */
					/* バンク切替 */
					Bank_num = (total_pat_count / 256) & 0x0Fu;
					/* V H バンク パレット パターンを設定 */
					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) = SetBGcodeEx(0, 0, Bank_num, uPAL_num, (total_pat_count % 256));
					/* PCGを定義してパターンデータ書き換え */
					SP_DEFCG_Ex(total_pat_count % PCG_CODE_MAX, 0, 0, SP_16x16, (int32_t)(g_stPCG_DATA[uPCG_num].pPatData + Mmul64(count + ((Anime_pat_size) % DataMax))) );
#ifdef DEBUG
//					if(count == 0)
//					{
//						printf("PCG_Main(%4d)=(0x%04X)B(%2d)P(%2d)PL(%2d)N(%4d)\n", \
//								uPCG_num, *(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr), \
//								Bank_num, uPAL_num, g_stST_PCG_LIST[uPCG_num].Pal, (total_pat_count % 256) );
//						printf("\033[1A");	/* $1B ESC[1A 1行上に移動 */
//						KeyHitESC();	/* デバッグ用 */
//					}
#endif
	#else
					/* PCGを定義してパターンデータ書き換え */
					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) &= ~0xF00u;
					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) |= SetBGcode2(0,0, uPAL_num + 1);
//					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) = SetBGcode( 0, 0, 1, (total_pat_count % 256));
					/* メモリからPCGエリアへコピー */
					SP_DEFCG_FULL(
					//_iocs_sp_defcg( 
									total_pat_count % PCG_CODE_MAX,
									SP_16x16,
									(int32_t)(g_stPCG_DATA[uPCG_num].pPatData + Mmul64(count + ((Anime_pat_size) % DataMax))) );	/* 64 = 128byte */

	#endif
					if(pat_count >= PCG_CODE_MAX)
					{
						printf("Error:PCG_Main(%d)=パターンオーバー(%d)\n", uPCG_num, pat_count);
					}
				}
				else if(Anime_num_old != Anime_num)	/* PCGを定義はそのままでアニメーション変更 */
				{
					/* PCGを定義してパターンデータ書き換え */
					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) = Mbset(*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr), 0xFF, count);
					/* メモリからPCGエリアへコピー */
//					_iocs_sp_defcg( (Plane_num + pat_count) & (PCG_CODE_MAX-1),
//									SP_16x16,
//									g_stPCG_DATA[uPCG_num].pPatData + Mmul64(count + ((Anime_pat_size) % DataMax)) );	/* 64 = 128byte */
				}
#endif
				/* 表示 */
				if(	g_stPCG_DATA[uPCG_num].update == TRUE )	/* 更新アリ */
				{
#if	CNF_PCG_AUTO_ACCELERATION
					if(count == 0)
					{
						/* 移動量 */
						mx = g_stPCG_DATA[uPCG_num].x += g_stPCG_DATA[uPCG_num].dx;
						my = g_stPCG_DATA[uPCG_num].y += g_stPCG_DATA[uPCG_num].dy;
					}
#else
						mx = g_stPCG_DATA[uPCG_num].x;
						my = g_stPCG_DATA[uPCG_num].y;
#endif

#if  CNF_XSP
#else
	#if XEiJ_Ex_SP
	#else
					/* パターンデータ書き換え */
					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) &= ~0xFFU;
					*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr) |= (Plane_num + Anime_num + count) & 0xFFu;
					if((Plane_num + Anime_num + count) > 0xFFu){
						bank_bit = Bit_2;
					}

	#endif
					/* プライオリティ変更 */
					uPCG_prw = 3;
#endif
					ofx = SP_X_OFFSET;
					ofy = SP_Y_OFFSET;
				}
				else
				{
					/* 画面外へ移動 */
					mx = 0;
					my = 0;
					ofx = 0;
					ofy = 0;
#if  CNF_XSP
#else
					/* プライオリティ変更 */
					uPCG_prw = 0;
#endif
				}
							
				/* スプライトレジスタの設定 */
#if  CNF_XSP
				ret = xsp_set(	(mx >> PCG_LSB) + uWidth  + SP_X_OFFSET,
								(my >> PCG_LSB) + uHeight + SP_Y_OFFSET,
								(Plane_num + pat_data_addr) & 0x7FFFu,
								*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr));
#else
	#if XEiJ_Ex_SP
				SP_REGST_Ex(	view_count + sync,
								(mx >> PCG_LSB) + uWidth  + SP_X_OFFSET,
								(my >> PCG_LSB) + uHeight + SP_Y_OFFSET,
								*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr),
								uPCG_prw);
	#else
				SP_REGST_FULL(
//				_iocs_sp_regst(
								view_count,
								sync,	/* mode:最上位ビット 0=垂直同期検出 1=垂直同期検出しない */
								(mx >> PCG_LSB) + uWidth  + ofx,
								(my >> PCG_LSB) + uHeight + ofy,
								*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + pat_data_addr),
								uPCG_prw | bank_bit);
	#endif
#endif
				uWidth += SP_W;

				count++;
				pat_count++;
#if  CNF_XSP
#else
				view_count++;
				total_pat_count++;
				sync = 0x80000000;

				s_uPCG_pat_count_Max = Mmax(s_uPCG_pat_count_Max, view_count);
				if(view_count >= PCG_VIEW_MAX)view_count = 0;
#endif
				if(pat_count >= PCG_MAX)break;
			}
			uHeight += SP_H;
		}
		
#if  CNF_XSP
#else
		g_stPCG_DATA[uPCG_num].Plane = Plane_num;	/* 先頭のプレーン番号を格納 */
#endif

#if	CNF_PCG_AUTO_UPDATE_OFF		
		g_stPCG_DATA[uPCG_num].update = FALSE;	/* 更新済みにする */
#endif

#ifdef DEBUG
//		printf("PCG(%d)=(%d,%d)(0x%4x)\n", uPCG_num, g_stPCG_DATA[uPCG_num].x + uWidth, g_stPCG_DATA[uPCG_num].y + uHeight, *(g_stPCG_DATA[uPCG_num].pPatCodeTbl) );
//		KeyHitESC();	/* デバッグ用 */
#endif
	}
#if  CNF_XSP
#else
	#if XEiJ_Ex_SP
	for( i = view_count; i < s_uPCG_pat_count_Max; i++ )
	{
		SP_REGST_Ex( i + sync,	0,	0,	*(g_stPCG_DATA[0].pPatCodeTbl),	0);
	}
	#endif
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
ST_PCG *PCG_Get_Info(uint16_t uNum)
{
	ST_PCG *ret;

	ret = (ST_PCG *)&g_stPCG_DATA[uNum];	/* 現情報を取得 */
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t PCG_Set_Info(ST_PCG stPCG, uint16_t uNum)
{
	int16_t	ret = 0;

	g_stPCG_DATA[uNum] = stPCG;	/* 更新 */
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_SP_PAL_DataLoad(void)
{
#if 0
	FILE *fp;
	int8_t sBuf[128];
#endif
	int32_t	i,j;
	size_t	FileSize;

	/*-----------------[ ＰＣＧデータ読み込み ]-----------------*/
	for(i = 0, j = 0; i < g_sp_list_max; i++)
	{
		if(GetFileLength(g_sp_list[i], &FileSize) == -1)
		{
#ifdef DEBUG	/* デバッグコーナー */
			printf("SP File %2d = %s GetFileLength error\n", i, g_sp_list[i]);
#endif
		}
		else
		{
			if(FileSize != 0)
			{
				/* メモリに登録 */
				g_sp_dat_size[i] = File_Load_Split(g_sp_list[i], &g_pcg_dat[j * SP_16_SIZE], PCG_MAX * SP_16_SIZE, sizeof(int8_t), (g_stST_PCG_LIST[0].Pat_w * g_stST_PCG_LIST[0].Pat_h) / 2, i);
//				g_sp_dat_size[i] = File_Load(g_sp_list[i], &g_pcg_dat[j * SP_16_SIZE], sizeof(int8_t), FileSize);
				printf("PCG_SP_PAL_DataLoad SP %2d = %s = size(%ld[byte]=%d)\n", i, g_sp_list[i], g_sp_dat_size[i], FileSize);
				j += (g_sp_dat_size[i]/SP_16_SIZE);
			}
		}
	}

	/*--------[ スプライトパレットデータ読み込みと定義 ]--------*/
#if 1
	for(i = 0; i < g_pal_list_max; i++)
	{
		if(GetFileLength(g_pal_list[i], &FileSize) == -1)
		{

		}
		else
		{
			if(FileSize != 0)
			{
				/* メモリに登録 */
				if(FileSize <= 512)
				{
					g_pal_dat_size[i] = (int32_t)File_Load(g_pal_list[i], &g_pal_dat[i][0], sizeof(int8_t), FileSize);
				}
				else
				{
					g_sp_dat_size[i] = File_Load_Split(g_pal_list[i], &g_pal_dat[i][0], PAL_LIST_MAX * PAL_MAX, sizeof(int8_t), 2 * PAL_MAX, i);
					printf("PCG_SP_PAL_DataLoad PAL %2d = %s = size(%d[byte]=%d)\n", i, g_pal_list[i], g_pal_dat_size[i], FileSize);
				}
			}
		}
	}
#endif
	
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_PAL_DataLoad(void)
{
#if 0
	FILE *fp;
	int8_t sBuf[128];
#endif
	int32_t	i;
	size_t	FileSize;

	/*--------[ スプライトパレットデータ読み込みと定義 ]--------*/
#if 1
	for(i = 0; i < g_pal_list_max; i++)
	{
		if(GetFileLength(g_pal_list[i], &FileSize) == -1)
		{

		}
		else
		{
			if(FileSize != 0)
			{
				/* メモリに登録 */
				g_pal_dat_size[i] = (int32_t)File_Load(g_pal_list[i], &g_pal_dat[i*PAL_MAX], sizeof(int8_t), FileSize);
				printf("PCG_PAL_DataLoad File %2d = %s = size(%d[byte]=%d)\n", i, g_pal_list[i], g_pal_dat_size[i], FileSize);
			}
		}
	}
#endif
	
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
void PCG_Set_PAL_Data(int16_t nPal, int16_t nList, int16_t uOfst)
{
	int16_t i;
	int16_t blk;

	if(nList >= g_pal_list_max)
	{
		return;
	}

	blk = (nPal % PAL_MAX) + uOfst;

	for( i = 0; i < PAL_MAX; i++ )
	{
		_iocs_spalet( 0x80000000 + i, blk, g_pal_dat[nList][i] );
	}
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*===========================================================================================*/
int16_t PCG_Load_Data(int8_t *sFileName, uint16_t uPCG_data_ofst,
						ST_PCG stPCG, uint16_t uPCG_num, uint8_t uPCG_Type)
{
	int16_t	ret = 0;
	uint32_t	j;
	uint32_t	uBufSize;

	/* コピー */
	g_stPCG_DATA[uPCG_num] = stPCG;
	
	/* メモリ確保 */
	uBufSize = stPCG.Pat_DataMax;
	
	/* スプライト定義設定 */
	g_stPCG_DATA[uPCG_num].pPatCodeTbl	= (uint16_t*)MyMalloc((sizeof(uint16_t) * uBufSize) + sizeof(uint8_t));	/* 4byte × サイズ分 + 1byte */
#if 0	
	for(j=0; j < uBufSize; j++)
	{
#if  CNF_XSP
		*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + j)	= SetXSPinfo(0, 0, 0x00, 0x00);	/* パターンコードテーブル */
#else
		*(g_stPCG_DATA[uPCG_num].pPatCodeTbl + j)	= SetBGcode(0, 0, 0x00, 0xFF);	/* パターンコードテーブル */
#endif
	}
#endif
	
	if(uPCG_Type == 3)	/* XSPモードはパターン不要 */
	{
		g_stPCG_DATA[uPCG_num].pPatData		= NULL;
		return ret;	
	}
	/* スプライト画像データ */
	uBufSize = stPCG.Pat_DataMax;
	
	g_stPCG_DATA[uPCG_num].pPatData		= (uint16_t*)MyMalloc((SP_16_SIZE * uBufSize) + sizeof(uint8_t) );		/* 128byte × アニメーション数 + 1byte */
	
	memset(g_stPCG_DATA[uPCG_num].pPatData, 0, (SP_16_SIZE * uBufSize) + sizeof(uint8_t));	/* 0クリア */
	
	if(uPCG_Type == 4)	/* XSPモードはパターン不要 */
	{
		return ret;	
	}
	/* スプライトデータ読み込み */
	{
		uint32_t uSP_MoveSize;
		FILE *fp;

		fp = fopen( sFileName, "rb" ) ;	/* ファイルオープン */
		if(fp == NULL)
		{
			return -1;
		}
		
		switch(uPCG_Type)
		{
			case 0:
			{
				fseek(fp, (uPCG_data_ofst * SP_16_SIZE), SEEK_SET);	/* 目的データの先頭位置 */
				
				fread( g_stPCG_DATA[uPCG_num].pPatData		/* パターンの格納先 */
					,  SP_16_SIZE							/* 1PCG = メモリサイズ 16x16 */
					,  uBufSize								/* 何パターン使うのか？ */
					,  fp	);								/* ファイルポインタ */
				break;
			}
			case 1:
			{
				uSP_MoveSize = Mdiv2(SP_16_SIZE);	/* 8dot */
				
				for(j=0; j < stPCG.Pat_DataMax; j++)
				{
					uint16_t *pPt = g_stPCG_DATA[uPCG_num].pPatData;
					
					fseek( fp, (uPCG_data_ofst * SP_16_SIZE) + (j * uSP_MoveSize), SEEK_SET);	/* 目的データの先頭位置 */
					
					fread( pPt + (uSP_MoveSize * j)
							,  uSP_MoveSize		/* 1PCG = メモリサイズ */
							,  1				/* 何パターン使うのか？ */
							,  fp
							);
				}
				break;
			}
			case 2:
			{
				uSP_MoveSize = Mdiv4(SP_16_SIZE);	/* 4dot */

				for(j=0; j < stPCG.Pat_DataMax; j++)
				{
					uint16_t *pPt = g_stPCG_DATA[uPCG_num].pPatData;
					
					fseek( fp, (uPCG_data_ofst * SP_16_SIZE) + (j * uSP_MoveSize), SEEK_SET);	/* 目的データの先頭位置 */
					
					fread( pPt + (uSP_MoveSize * j)
							,  uSP_MoveSize		/* 1PCG = メモリサイズ */
							,  1				/* 何パターン使うのか？ */
							,  fp
							);
				}
				break;
			}
			default:
			{
				uSP_MoveSize = SP_16_SIZE;
				break;
			}
		}
		fclose( fp ) ;		/* ファイルクローズ */
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
int16_t PCG_Load_PAL_Data(int8_t *sFileName, uint16_t uPAL_data_ofst, uint16_t uPAL_Set_block)
{
	int16_t	ret = 0;
	
	FILE *fp;
	uint32_t i;
	uint16_t	pal_dat[ 128 ]; /* パレットデータファイル読み込みバッファ */

	if(uPAL_data_ofst == 0)return -1;	/* 0番はテキストと共用なので */
	
	/*--------[ スプライトパレットデータ読み込みと定義 ]--------*/

	fp = fopen( sFileName , "rb" ) ;
	if(fp == NULL)
	{
		ret = -1;
	}
	else
	{
		fseek( fp, 2 * 16 * (uPAL_data_ofst - 1), SEEK_SET);	/* 目的データの先頭位置 */
		
		fread( pal_dat
			,  2		/* 1palet = 2byte */
			,  16		/* 16palet * 16block */
			,  fp
			) ;
		fclose( fp ) ;

		/* スプライトパレットに転送 */
		for( i = 0 ; i < 16 ; i++ )
		{
			_iocs_spalet( (0x80000000 | (i & 0x0F)), (uPAL_Set_block & 0x0F) + 1, pal_dat[i] ) ;
		}
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
int16_t PCG_PAL_Change(uint16_t uPAL_Set_mode, uint16_t uPAL_Set_block, uint16_t uCol)
{
	int16_t	ret = 0;

#if 1
	volatile uint16_t *PALETTE_ADR = (uint16_t *)0xE82200;
	uint16_t *pPal;
	
	pPal = (uint16_t *)PALETTE_ADR + (Mmul16(uPAL_Set_block) + uPAL_Set_mode);
	*pPal = uCol;
#ifdef DEBUG
//	printf("PCG_PAL_Change[0x%p]=(%d,%d)0x%d\n", pPal, uPAL_Set_mode, uPAL_Set_block, uCol);
//	KeyHitESC();	/* デバッグ用 */
#endif
#else
	/* スプライトパレットに転送 */
	_iocs_spalet( (0x80000000 | (uPAL_Set_mode & 0x0F)), (uPAL_Set_block & 0x0F) + 1, uCol) ;
#endif
	
	return ret;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/*
;IOCS $C0 _SP_INIT
;	機能	スプライト・バックグラウンドを初期化します。
;		_SP_INITで拡張機能を有効にしたプロセスが終了するとき拡張機能を無効にします。
;	入力	レジスタ	値
;		d1.l		'SPRD'$53505244	拡張機能を使用する
;		d2.l		0～7		バンク制御
;				-1		常駐確認(初期化は行わない)
;	出力	レジスタ	値
;		d0.l		-3		拡張機能を使用できない
;				-1		スプライト画面を使用できない
;				0		正常終了
;		常駐確認のとき
;		d0.l		$53xxxxxx	バージョン
*/
/*===========================================================================================*/
int32_t SP_INIT_Ex(void)
{
	struct	_regs	stInReg = {0}, stOutReg = {0};
	uint32_t	retReg;
	
	stInReg.d0 = 0xC0;				/* SP_INITによるIOCSコール */
	stInReg.d1 = 0x53505244;		/* 拡張機能有効 'SPRD'$53505244 */
	stInReg.d2 = 7;					/* 0～7		バンク制御 */

	retReg = _iocs_trap15(&stInReg, &stOutReg);	/* Trap 15 */

	if(retReg < 0)
	{
		retReg = _iocs_sp_init();	/* スプライトの初期化 */
	}

	return retReg;
}
/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/* 
;IOCS $C4 _SP_DEFCG
;	機能	パターンを設定します。
;		パターンを反転する機能が追加されています。
;	入力	レジスタ	値
;		d1.w		0～4095		パターン番号
;		d2.w[15]	0～1		上下反転
;		d2.w[14]	0～1		左右反転
;		d2.b		0～1		サイズ。0=8x8,1=16x16
;		a1.l		偶数		バッファのアドレス
;	出力	レジスタ	値
;		d0.l		-1		スプライト画面を使用できない
;				0		正常終了
*/
/*===========================================================================================*/
int32_t SP_DEFCG_Ex(int32_t sp_num, int16_t sp_v, int16_t sp_h, int8_t sp_size, int32_t sp_buff_adr)
{
	struct	_regs	stInReg = {0}, stOutReg = {0};
	uint32_t	retReg;
	
	stInReg.d0 = 0xC4;				/* _SP_DEFCGによるIOCSコール */
	stInReg.d1 = sp_num;			/* パターン番号 0～4095 */
	stInReg.d2 = (sp_v<<15|sp_h<<14) + sp_size;
									/* 上下反転 0～1		 */
									/* 左右反転 0～1		 */
									/* サイズ   0=8x8,1=16x16 */
	stInReg.a1 = sp_buff_adr;		/* バッファのアドレス */

	retReg = _iocs_trap15(&stInReg, &stOutReg);	/* Trap 15 */

	return retReg;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/* 
;IOCS $C6 _SP_REGST
;	機能	スプライトスクロールレジスタを設定します。
;		豆腐ビットをクリアします。
;	入力	レジスタ	値
;		d1.l[31]	1		VDISPの立ち下がりを待たない
;		d1.w		0～1015		スプライト番号(連番)
;		d2.l[31]	1		X座標を設定しない
;		d2.w		0～1023		X座標
;		d3.l[31]	1		Y座標を設定しない
;		d3.w		0～1023		Y座標
;		d4.l[31]	1		キャラクタを設定しない
;		d4.w		0～65535	キャラクタ
;		d5.l[31]	1		プライオリティを設定しない
;		d5.w		0～65535	プライオリティ
;	出力	レジスタ	値
;		d0.l		-1		スプライト画面を使用できない
;				0		正常終了
*/
/*===========================================================================================*/
int32_t SP_REGST_Ex(int32_t sp_num, int32_t sp_x, int32_t sp_y, int32_t sp_char, int32_t sp_pri)
{
	struct	_regs	stInReg = {0}, stOutReg = {0};
	uint32_t	retReg;
	
	stInReg.d0 = 0xC6;				/* _SP_REGSTによるIOCSコール */
	stInReg.d1 = sp_num;			/* スプライト番号(連番) 0～1015 */
	stInReg.d2 = sp_x & 0x3FFu;		/* X座標 0～1023 */
	stInReg.d3 = sp_y & 0x3FFu;		/* Y座標 0～1023 */
	stInReg.d4 = sp_char & 0xFFFFu;	/* キャラクタ 0～65535 */
	stInReg.d5 = sp_pri & 0xFFFFu;	/* プライオリティ 0～65535 */

	retReg = _iocs_trap15(&stInReg, &stOutReg);	/* Trap 15 */

	return retReg;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/* 
番号	$C4
名前	SP_DEFCG
機能	パターンを設定します。
入力	d1.w		0～511		パターン番号
		d2.b		0～1		サイズ。0=8x8,1=16x16
		a1.l		偶数		バッファのアドレス
出力	d0.l		-1		スプライト画面を使用できない
					0		正常終了
*/
/*===========================================================================================*/
int32_t SP_DEFCG_FULL(int32_t sp_num, int8_t sp_size, int32_t sp_buff_adr)
{
	struct	_regs	stInReg = {0}, stOutReg = {0};
	uint32_t	retReg;
	
	stInReg.d0 = 0xC4;				/* _SP_DEFCGによるIOCSコール */
	stInReg.d1 = sp_num;			/* パターン番号 0～511 */
	stInReg.d2 = sp_size;			/* サイズ   0=8x8,1=16x16 */
	stInReg.a1 = sp_buff_adr;		/* バッファのアドレス */

	retReg = _iocs_trap15(&stInReg, &stOutReg);	/* Trap 15 */

	return retReg;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/* 
番号	$C6
名前	SP_REGST
機能	スプライトスクロールレジスタを設定します。
入力	d1.l	bit31	0～1		VDISPの立ち下がり。0=待つ,1=待たない
		d1.b			0～127		スプライト番号
		d2.l	bit31	0～1		X座標の設定。0=する,1=しない
		d2.w			0～1023		X座標
		d3.l	bit31	0～1		Y座標の設定。0=する,1=しない
		d3.w			0～1023		Y座標
		d4.l	bit31	0～1		キャラクタの設定。0=する,1=しない
		d4.w			0～65535	キャラクタ
		d5.l	bit31	0～1		プライオリティの設定。0=する,1=しない
		d5.w			0～7		プライオリティ
出力	d0.l		-1		スプライト画面を使用できない
					0		正常終了
*/
/*===========================================================================================*/
int32_t SP_REGST_FULL(int32_t sp_num, int32_t sync, int32_t sp_x, int32_t sp_y, int32_t sp_char, int32_t sp_pri)
{
	struct	_regs	stInReg = {0}, stOutReg = {0};
	uint32_t	retReg;
	
	stInReg.d0 = 0xC6;							/* _SP_REGSTによるIOCSコール */
	stInReg.d1 = sync + (sp_num & 0x7F);		/* スプライト番号(連番) 0～127 */
	stInReg.d2 = sp_x & 0x3FFu;					/* X座標 0～1023 */
	stInReg.d3 = sp_y & 0x3FFu;					/* Y座標 0～1023 */
	stInReg.d4 = sp_char & 0xFFFFu;				/* キャラクタ 0～65535 */
	stInReg.d5 = sp_pri & 0x7u;					/* プライオリティ 0～7 */

	retReg = _iocs_trap15(&stInReg, &stOutReg);	/* Trap 15 */

	return retReg;
}

/*===========================================================================================*/
/* 関数名	：	*/
/* 引数		：	*/
/* 戻り値	：	*/
/*-------------------------------------------------------------------------------------------*/
/* 機能		：	*/
/* 
番号	$CF
名前	SPALET
機能	スプライトパレットを設定または取得します。
	パレットブロック0は指定できません。
入力	d1.l	bit31	0～1	VDISPの立ち下がり。0=待つ,1=待たない
		d1.b	0～255			パレットブロック<<4|パレットコード
		d2.b	0～15			パレットブロック(優先)
		d3.l	bit31	0～15	モード。0=設定,1=取得
		d3.w	0～65535		カラーコード
出力	d0.l	-2				パレットブロック0が指定された
			0～65535	設定前のカラーコード
*/
/*===========================================================================================*/


#endif	/* BIOS_PCG_C */

