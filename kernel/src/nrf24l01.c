/*#include <drivers/nrf24l01.h>
#include <kernel/nrf24l01.h>


handle_t nrf24l01_open(id_t id){
	nrf24l01_init();
}

void nrf24l01_send(const char *addr, uint8_t *data, uint8_t size){
	nrf24l01_settxaddr(addr);
}

void nrf24l01_recv(const char *addr, uint8_t *data, uint8_t size){
	uint8_t pipe = 0;
	uint8_t buffer[NRF24L01_PAYLOAD];
	if(nrf24l01_readready(&pipe)){
		nrf24l01_read(buffer);
		memcpy(data, buffer,
			(NRF24L01_PAYLOAD > size)?NRF24L01_PAYLOAD:size); 
	}
}

static void foo_(void){
	uint8_t txrxrole = 1; // 1 transmitter 0 receiver
	uint8_t i = 0;

	//nrf24l01 variables
	uint8_t bufferout[NRF24L01_PAYLOAD];
	uint8_t bufferin[NRF24L01_PAYLOAD];

	//setup port
	BUTTONROLE_DDR &= ~(1<<BUTTONROLE); //input
	BUTTONROLE_PORT &= ~(1<<LEDOUT); //off
	BUTTONSEND_DDR &= ~(1<<BUTTONSEND); //input
	BUTTONSEND_PORT &= ~(1<<LEDOUT); //off
	LEDOUT_DDR |= (1<<LEDOUT); //output
	LEDOUT_PORT &= ~(1<<LEDOUT); //off

	//init uart
	uart_init(UART_BAUD_SELECT(38400, F_CPU));

	//init interrupt
	sei();

	uart_printf("INIT NRF..\n");

	//init nrf24l01
	nrf24l01_init();

	if(txrxrole == ROLETX)
		uart_puts("starting as tx...\r\n");
	else if(txrxrole == ROLERX)
		uart_puts("starting as rx...\r\n");

	//setup buffer
	for(i=0; i<sizeof(bufferout); i++)
		bufferout[i] = i+'a';
	for(i=0; i<sizeof(bufferin); i++)
		bufferin[i] = 0;

	//sending buffer addresses
	uint8_t sendpipe = 0, recvpipe = 0;
	uint8_t addrtx0[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP0;
	uint8_t addrtx1[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP1;
	uint8_t addrtx2[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP2;
	uint8_t addrtx3[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP3;
	uint8_t addrtx4[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP4;
	uint8_t addrtx5[NRF24L01_ADDRSIZE] = NRF24L01_ADDRP5;

	nrf24l01_printinfo(uart_puts, uart_putc);
	
	//main loop
	for(;;) {
		if(txrxrole == ROLETX) {
				uart_printf("sending data, on pipe %d...", sendpipe);

				if(sendpipe == 0) {
					//set tx address for pipe 0
					nrf24l01_settxaddr(addrtx0);
				} else if(sendpipe == 1) {
					//set tx address for pipe 1
					nrf24l01_settxaddr(addrtx1);
				} else if(sendpipe == 2) {
					//set tx address for pipe 2
					nrf24l01_settxaddr(addrtx2);
				} else if(sendpipe == 3) {
					//set tx address for pipe 3
					nrf24l01_settxaddr(addrtx3);
				} else if(sendpipe == 4) {
					//set tx address for pipe 4
					nrf24l01_settxaddr(addrtx4);
				} else if(sendpipe == 5) {
					//set tx address for pipe 5
					nrf24l01_settxaddr(addrtx5);
				}

				//write buffer
				uint8_t writeret = nrf24l01_write(bufferout);

				if(writeret == 1)
					uart_puts("ok\r\n");
				else if(writeret == 2)
					uart_puts("fail.. max retries reached!\r\n"); 
				else
					uart_puts("failed\r\n");

				sendpipe++;
				sendpipe%=6;
				
			nrf24l01_printinfo(uart_puts, uart_putc);
	
			_delay_ms(1000);
		} else if(txrxrole == ROLERX) {

			//rx
			uint8_t pipe = 0;
			if(nrf24l01_readready(&pipe)) { //if data is ready
				uart_printf("getting data, on pipe %d...", pipe);

				//read buffer
				nrf24l01_read(bufferin);

				uint8_t samecheck = 1;
				uart_printf("  data: ");
				for(i=0; i<sizeof(bufferin); i++) {
					if(bufferin[i] != bufferout[i])
						samecheck = 0;
					uart_putc(bufferin[i]); 
				}
				uart_putc('\n');
				
				if(samecheck)
					uart_puts("  check ok\r\n");
				else
					uart_puts("  check fails\r\n");
				for(i=0; i<sizeof(bufferin); i++)
					bufferin[i] = 0;
			}
			
			recvpipe++;
			recvpipe%=6;
			_delay_ms(10);

		}
	}
}
*/
