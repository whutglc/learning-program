#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdlib.h>
#include "glc_Compress.h"
#include"glc_Huffman.h"

using namespace std;

#define OK 1
#define ERROR 0

const int SIZE = 256;
/*
[函数名称]InitHead
[函数功能]扫描原文件，初始化压缩文件头
[函数参数]const char *pFilename:输入原文件名
[函数参数]Head &head:输出文件头
[返回值]int :ERROR 异常：OK 正常
*/
int InitHead(const char * pFilname, HEAD & sHead)
{
	strcpy(sHead.type, "HUF");		//文件类型
	sHead.length = 0;				//源文件长度
	for (int i = 0; i < SIZE; i++)
	{
		sHead.weight[i] = 0;		//权值
	}

	//以二进制流形式打开文件
	FILE *in = fopen(pFilname, "rb");


	//扫描文件，获得权重
	int ch;
	while ((ch = fgetc(in)) != EOF)
	{
		sHead.weight[ch]++;
		sHead.length++;
	}

	//关闭文件
	fclose(in);
	in = NULL;
	return OK;
}

/*
[函数名称]Compress
[函数功能]压缩文件
[函数参数]const char*pFilename:输入原文件名
[返 回 值]int:ERROR 异常 ；OK 正常
*/
int Compress(const char * pFilename)
{
	/**************************************************/
	//打开并扫描文件
	cout << "正在读取文件……" << endl;
	int weight[256] = { 0 };  //打开文件，获取权重
	FILE* in = fopen(pFilename, "rb");
	int tempch;
	while ((tempch = getc(in)) != EOF)
	{
		weight[tempch]++;
	}
	//测试扫描结果：显示256种字节出现的次数
	//if (TestWeight(weight))
	//	cout << "测试完毕！！！" << endl << endl;

	cout << "文件读取完毕！\n" << endl << endl;
	fclose(in);

	/**************************************************/
	//将编码生成Huffman树
	int i;
	int n = 256;						//Huffman树共有n个叶子节点
	int m = 2 * n - 1;					//那么就有2n+1个节点

	HuffmanTree pHT = new HTNode[m + 1];	//定义Huffman树
	CreateHuffmanTree(pHT, weight, n);

	//	cout << "测试生成的Huffman树" << endl;
	//	if (TestHufTree(pHT))
	//		cout << "测试完毕！" << endl;

	//生成Huffman编码
	char** pHC = new char*[n + 1]; //编码
	for (int i = 1; i <= n; i++)
		pHT[i].weight = weight[i - 1];

	//	cout << "测试生成的Huffman树" << endl;
	//测试生成的Huffman树
	//cout << "生成Huffman树测试过程：\n";
	//if (TestHufTree(pHT))
	//	cout << "测试完毕！" << endl;


	HuffmanCoding(pHC, pHT);

	//cout << "测试：显示字节的Huffman编码" << endl;
	//cout << "Byte\tHuffmanCode" << endl;
	//TestHufCode(511, pHT, pHC);

	/**************************************************/

	//计算编码缓冲区大小
	int nSize = 0;
	for (int i = 0; i < 256; i++)
		nSize += weight[i] * strlen(pHC[i + 1]);
	nSize = (nSize % 8) ? nSize / 8 + 1 : nSize / 8;


	//对编码文件进行压缩
	char *pBuffer = NULL;
	pBuffer = new char[nSize];
	memset(pBuffer, 0, (nSize)* sizeof(char));
	Encode(pFilename, pHC, pBuffer, nSize);
	if (!pBuffer) {
		return ERROR;
	}

	HEAD sHead;
	InitHead(pFilename, sHead);
	cout << "目标文件大小：" << sHead.length << "字节" << endl;
	int afterlen = WriteFile(pFilename, sHead, pBuffer, nSize);
	cout << "压缩文件大小：" << afterlen << "字节  \n其中头文件sHead大小：" << sizeof(sHead) << "字节" << endl;
	cout << "压缩比率：" << (double)afterlen * 100 / sHead.length << "%" << endl;

	delete pHT; delete[] pHC; delete pBuffer;

	return OK;
}

/*
[函数名称]Encode
[函数功能]扫描源文件，根据Huffman编码表对其进行压缩，压缩结果暂存到缓冲区中
[函数参数]const char *pFilename 源文件名
[函数参数]const HCode pHC Huffman编码表
[函数参数]BUFFER pBuffer: 编码缓冲区
[函数参数]const int nSize; 缓冲区大小
[返回值]int：ERROR 异常 OK 正常
*/
int Encode(const char * pFilname, const HuffmanCode pHC, char * pBuffer, const int nSize)
{
	//打开文件
	FILE *in = fopen(pFilname, "rb");

	//开辟缓冲区ni
	pBuffer = (char *)malloc(nSize * sizeof(char));
	if (!pBuffer)
	{
		cout << "开辟缓冲区失败" << endl;
	}

	char cd[SIZE] = { 0 };		//工作区
	int pos = 0;				//缓冲区指针
	int ch;
	//扫描文件
	while ((ch = fgetc(in)) != EOF)
	{
		strcat(cd, pHC[ch + 1]);

		//压缩编码
		while (strlen(cd) >= 8)
		{
			pBuffer[pos++] = Str2byte(cd);
			for (int i = 0; i<SIZE - 8; i++)
			{
				cd[i] = cd[i + 8];
			}
		}
	}
	if (strlen(cd) > 0)
	{
		pBuffer[pos++] = Str2byte(cd);
	}
	fclose(in);		//关闭文件

	return OK;
}

//生成压缩文件
int WriteFile(const char * pFilename, const HEAD sHead, const char * pBuffer, const int nSize)
{
	//生成文件名
	char filename[256] = { 0 };
	strcpy(filename, pFilename);
	strcat(filename, ".huf");

	//以二进制流形式打开文件
	FILE * out = fopen(filename, "wb");

	//写文件
	fwrite(&sHead, sizeof(HEAD), 1, out);

	//写压缩后的编码
	fwrite(pBuffer, sizeof(char), nSize, out);
	//关闭文件，释放文件指针
	fclose(out);
	out = NULL;

	cout << "生成压缩文件：" << filename << endl;
	int len = sizeof(HEAD) + strlen(pFilename) + 1 + nSize;
	return len;
}

//将字符串转换成字节
char Str2byte(const char * pBinStr)
{
	char b = 0x00;
	for (int i = 0; i < 8; i++)
	{
		b = b << 1;		//左移一位
		if (pBinStr[i] == '1')
		{
			b = b | 0x01;
		}
	}
	return b;
}


int TestWeight(int weight[]) {

	cout << "显示256种字节出现的次数：" << endl;
	cout << "Byte\t" << "Weight\t" << endl;

	for (int i = 0; i < 256; i++) {
		printf("0x%02X\t%d\n", i, weight[i]);
		//cout << i << "\t" << weight[i] << endl;
	}
	return OK;
}