/*
 * UARTDriver.c
 *
 *  Created on: 22 Şub 2026
 *      Author: mehmet_dora
 */

#include "UARTDriver.h"
#include "stm32f4xx.h"
#include <string.h>

volatile char* tx_buffer;	// Buffer array-pointer
volatile uint16_t tx_index;	// Buffer için sonraki yazılacak index tutucu
volatile uint16_t tx_len;	// Buffer boyutunu tutucu

volatile uint8_t rx_data;
volatile uint8_t rx_flag;


void uart_init(void){


	RCC->APB1ENR |= (1UL << 17); 		// usart2 clock enable, AF7 için


	RCC->AHB1ENR |= (1UL << 0);			// A portunun clock enable
	// USART2 çıkışları A portundan sağlanacağı için bu port çıkışlarının clockunu enable et



	// İstenen GPIOA pinlerinin modunu AF olarak ayarlanması , PA2 ve PA3
	GPIOA->MODER &= ~(3UL << (2*2));
	GPIOA->MODER &= ~(3UL << (2*3));

	GPIOA->MODER |= (2UL << (2*2));
	GPIOA->MODER |= (2UL << (2*3));



	// haberleşme için ilgili pinlerin high speed olarak ayarlanması , PA2 ve PA3
	GPIOA->OSPEEDR |= (3UL << (2*2)) | (3UL << (2*3));





	// İlgili pinlerin alternate fonksiyon olarak ayarlanması

	GPIOA->AFR[0] |= (7UL << (4*2)) | (7UL << (4*3));


	USART2->CR1 &= ~(1UL << 12);	// Data boyutunun 8 bit olarak ayarlanması


	// BAUD RATE Hesaplama
	/*
	 * Bu BBR register'ına yazılacak değer ,APB1 Clock frekansı/(Oversampling x USARTDIV) = BAUD RATE olarak hesaplanır.
	 * Elimizde Clock frekansı ve istesiğimiz Baud rate değeri ve istenen oversampling değerleri ile
	 * Usartdiv değeri bulunur. Bu değer kesirli çıkabilir. Hatayı en aza indirmek için çıkan sonucun
	 * tam kısmı 15-4 bitlerine yazılır. Kalan ondalık kısmı ise 0-3 bitlerine yazılır.
	 *
	 * Buradaki sonucun kesirli kısmı önce oversampling değeri ile çarpılır, ardından çıkan sonuç
	 * en yakın tam sayıya yuvarlandıktan sonra bu tam sayı hex formatında 0-3 bitlik fraction kısmına yazılır.
	 *
	 */
	/*
	 * Clock sinyali  = 42.000.000
	 * istenen Baud rate = 115200
	 * oversampling = 16
	 * Sonuç olarak bulunan USARTDIV = 22.786(22 -> mantissa, 0.786 -> fraction)
	 * Mantissa = 22
	 * Fraction = 12.576'dan -> 13 yani hex D
	 */

	USART2->BRR = (22UL << 4) | (13UL);		// eski değerlerin kalmaması için direkt = kullan




	// Transmitter enable ve receiver enable yapmak

	USART2->CR1 |= (1UL << 2) | (1UL << 3);


	// USART2 RX interrupt enable
	USART2->CR1 |= (1UL << 5);		// Bu bit RXENIE ile RX için interruot a izin verir
	NVIC_EnableIRQ(USART2_IRQn);	// Bu NVIC in bu interrupt için erişilebilmesini sağlar.


	USART2->CR1 |= (1UL << 13);		// Usart2 enable




}



void uart_send_char(uint8_t data){

	USART2->DR = data;
	while( !(USART2->SR & (1UL << 6)) ){};		// Önceki veri gönderilip yeniden gönderilmeye hazıl olunana kadar bekle

	/*
	 * POLLING BASE UART -> while içinde sürekli beklemek polling yöntemidir. İşlemciyi gereksiz yorar.
	 *	Polling yöntemi ile işlemci sürekli 6. bitin kontrolünü yapar, sistem kilitlenir.
	 *	Bu yöntem bu nedenle yoğun işler ve gerçek zamanlı sistemler için kullanılamaz.
	 *
	 *
	 * Bu kodda biz init() kodunda belirlediğimiz gibi 8 bitlik datayı DR register'ına yazıyoruz.
	 * DR register'ı arka planda veriyi paketler ve gönderir. Yani bitleri iletişim hattına süren UART'tır.
	 * Yani veriyi -> Start_bit- data(8bit) - stop_bit şeklinde paketler
	 *
	 * Sonrasındaki while içinde SR register'ının 6. biti Transmission Complete 'dir.
	 * Anlamı ise DR register tamamen boş, yeni veri koyabilirsin demektir.
	 * Sonuç olarak biz bu while döngüsü ile DR register'ına koyduğumuz veri gönderilip DR register
	 * boşalana kadar beklemesini sağlıyoruz.
	 *
	 * UART donanımı max 8-9 bit gönderebildiği için bu fonksiyon göndermek istediğimiz veriyi
	 * 8-9 bitlik parçalar halinde göndermemizi sağlar. Başka bir fonksiyon içinde bu fonk.
	 * kullanılarak tüm veri parça parça gönderilebilir.
	 *
	 */
}

void uart_send_string(char* string){
	while(*string){
		uart_send_char(*string++);
	}

	/*
	 *
	 * Bu fonksiyon ise göndermek istediğimiz tam verinin(string ifade) önceki parça parça gönderen
	 * fonksiyon ile gönderilmesini sağlar. Yani bu fonksiyon ile direkt istediğimiz veriyi göndermiş oluruz
	 *
	 * Göndereceğimiz veri string olduğundan string ifadeler de bir array gibidir. Kendi isimleri aslında
	 * adreslerini gösterir. String ifadelerin sonunda \0 karakteri bulunur ve bunu kullanarak string in
	 * sonuna geldiğimizi biliriz.
	 *
	 * While döngüsü içinde de char pointer türünde aldığımız string ifadeyi tek tek her bir karakterini
	 * pointer ile dönerek her karakteri yukarıdaki fonksiyon ile iletiyoruz. Böylece tüm string iletilmiş
	 * oluyor. String sonundaki \0 a gelindiğinde de döngüden çıkılarak işlem bitiyor.
	 */
}

uint8_t uart_read_char(){

	uint8_t temp;

	while(! (USART2->SR & (1UL << 5))){};		// Receive data register okumak için hazır olana kadar bekle

	temp = USART2->DR;
	return temp;

	/*
	 * POLLING BASE UART -> while içinde sürekli beklemek polling yöntemidir. İşlemciyi gereksiz yorar.
	 *
	 *
	 * Bu fonksiyon ile DR register'ından gelen veriler okunur. while koşulundaki SR register'ının
	 * 5. biti RXNE(RX not empty) dir. Bu flag DR içinde okunmamış veri olduğunu belirtir. Buradaki
	 * while döngüsü ise biz veri okumaya başladığımızda RXNE flag set edilmemişse , yani okunmamış veri
	 * DR içinde yoksa beklemesini sağlar. Eğer veri gelirse yani flag set edildiğinde bu while döngüsünden
	 * çıkılır.
	 *
	 * Sonrasında DR register içindeki veri okunur. Bu okuma işlemi ile RXNE flag i otomatik clear edilir.
	 * Gelen veri artık CPU'dadır. Bundan sonra gelen veri kullanılabilir.
	 */



}








// INTERRUPT USART2



uint8_t uart_read_char_IT(void){
	rx_flag = 0;
	return rx_data;

	/*
	 * Gelen veri interrupt ile rx_data değişkenine kaydedilmişti , bu fonksiyon ise main içerisinde
	 * çağrılarak bu rx_data verisini kullanamyı sağlar. Sonraki veri gelmesi halinde yine main içindeki
	 * döngüde kontrol edilmesi için tekrar rx_flag clear edilir.
	 */
}

void uart_send_string_IT(char* data){


	tx_buffer = data;		// Gelen string türündeki veri buffer'a kaydedildi
	tx_len = strlen(data);	// Bu verinin uzunluk bilgisi alındı
	tx_index = 0;			// Başlangıç değeri verildi


	USART2->DR = tx_buffer[tx_index++]; // ilk byte manuel olarak buradan gönderiliyor
										// bu gönderme tamamlanması sonucunda TXE flag set edilir.
										// Eğer TXEIE bit set edilmiş ise TXE'nin set edilmesi interrupt oluşmasını sağlar.

	USART2->CR1 |= (1UL << 7); 		// TXEIE biti ile TX için interrupt oluşmasına izin verilir.


	/*
	 * Bu fonksiyon sadece gönderilecek olan verileri buffer üzerinde kopyasını alır , uzunluk bilgisi
	 * ve başlama indexini ayarlar. Sonrasında gönderilecek verinin ilk karakteri DR register'ına yazılarak
	 * ilk gönderim başlatılır. Sonrasında CR1 register ile TX interrupt enable edilir. Yani veri
	 * gönderildiği an bu interrupt tetiklenecek.
	 * Sonrasındaki gönderme işlemleri interrupt fonksiyonu üzerinden devam eder.
	 */
}




// UART2 Interrupt Handler
void USART2_IRQHandler(void){


	// interrupt türü RXNE ise ve RX için interrupt enable edilmiş ise
	if((USART2->SR & (1UL << 5)) && (USART2->CR1 & (1UL << 5)) != RESET){
		rx_data = USART2->DR;
		rx_flag = 1;

		/*
		 * Burada veri DR register'ına geldiği an RXNE biti set edilir. Yani RXNE set edilmesi demek
		 * interrupt üretileceği demektir. RXNEIE biti init fonksiyonu içerisinde set edildiği için
		 * interrupt otomatik oluşur. Fakat RX tarafı için bir başlatmaya gerek yoktur. Karşı taraf veri
		 * gönderdiği an arka planda DR a veriler yazılır. RXNE set edilir ve bu interrupt oluşmasını
		 * tetikler. Gelen veri rx_data değişkeni ile CPU nun üzerinde işlem yapabilmesi sağlanır.
		 * Buradaki rx_flag ise gelen verinin geldiği anda oluşan bu interrupt ile bir veri geldiğini
		 * main içindeki ana döngüde kontrol etmek için kullanılır. Yani veri geldiğini döngü içinde
		 * kontrol ederiz , geldiği an rx_data değişkenine kaydedilmiş verimizi ne yapacaksak main
		 * içerisinde kullanabiliriz
		 */
	}


	// interrupt türü TXE ise  ve TX için interrupt enable edilmiş ise
	if((USART2->SR & (1UL << 7)) && (USART2->CR1 & (1UL << 7)) != RESET){

		if(tx_index < tx_len){
			USART2->DR = tx_buffer[tx_index++];
		}else{
			USART2->CR1 &= ~(1UL << 7);			// TXEIE biti clear edilir.
		}

		/*
			İlk if ile gönderilecek string'in son karakterien gelindiği kontrol edilir. Eğer son
			bite gelinmiş ise TXEIE biti clear edilerek artık TX üzerinden interrupt oluşturulması
			kapatılmış olunur.

			Eğer son karaktere gelinmemişse tx_index ile tutulan gönderilmesi gereken karakterin indexi
			tx_buffer içinden bulunur ve ardından DR register'ına bu karakter yazılır. Ardından da
			bu gönderim tamamlanması ile oluşacak interrupt ile sonraki karakterin gönderilmesini sağlamak
			için tx_index değeri bir arttırılır.
		*/

		/*
		 * Buradaki TX için önemli nokta tx_len değerine gelindiğinde yani tüm karakterler gönderildiğinde
		 * TXEIE clear edilerek TX için interrupt özelliğinin kapatılmasıdır. Bu işlemin yapılmasının
		 * nedeni tüm veriler gönderildikten sonra DR yine boş kalacaktır. Bu durumda DR register'ının
		 * boş olduğunu haber veren TXE biti sürekli 1 olacaktır. Yani sürekli interrupt'a girilecektir.
		 * Bu sorun ile herhangi veri gönderilmemiş olsa da CPU bu interrupt döngüsünde takılı kalacaktır.
		 *
		 * Bunu engellemek için tüm karakterlerin gönderilmesi sonunda TX için interrupt özelliği
		 * kapatılmalıdır. Zaten eğer tekrar veri gönderilmek istenirse yukarıdaki ilk karakterin
		 * gönderildiği fonksiyon içinde TX için interrupt enable yapılıyor.
		 */


	}



}



