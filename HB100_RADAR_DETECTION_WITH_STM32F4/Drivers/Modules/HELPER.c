/*
 * HELPER.c
 *
 *  Created on: 1 Mar 2026
 *      Author: mehmet_dora
 */


/*
5- Convert integer to string
*/

void int2char(int num, char str[])
{
	char lstr[30];			// ASCII kodlarında 0 değerinin karşılığı
	int cnt = 0;
	int div = 10;
	int j = 0;


	// sayı 10 dan büyükse sayıyı basakmalarına ayırma
	while( num >= div)
	{
		lstr[cnt] = num % div + 0x30;	// hex 30 ile ekleyerek rakamı elde etme
		num /= 10;						// son basamağı kaydettikten sonra sayıyı bir basamak sağa kaydır
		cnt++;							// basamak sayısını arttır
	}


	lstr[cnt] = num + 0x30;
	for(j= cnt ; j >=0;j--)
	{
		str[cnt-j] = lstr[j];
	}

}




void clear_array(char arr[] ,int size){
	for(int i = 0; i < size; i++){
		arr[i] = 0;
	}

}


