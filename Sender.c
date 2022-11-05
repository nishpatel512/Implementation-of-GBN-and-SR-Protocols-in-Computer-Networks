#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <stdint.h>

#define MAX_DATASIZE 1024
#define CHECKSUM_ERROR 1
#define TIMEOUT_ERROR 2
#define PACKET_LOST_ERROR 3

unsigned int seq_num = 0;
unsigned int recvr_seq_num = 0;
unsigned int initial_seq_no = 0;
unsigned int window_size_cntr = 0;
unsigned int packets_sent = 0;
unsigned int window_size = 0;
unsigned int BASE = 0;
unsigned int error_packet_seq_no = 0;
unsigned int temp = 0;
unsigned int ERROR_VAL = 0;
unsigned int Window_Size_Array[1024] = {0};

struct segment_packet {
	int seq_no;
	int checksum;
	char data[MAX_DATASIZE];
};

uint16_t calc_checksum(struct segment_packet recvd_packet) {

	uint16_t checksum = 0;
	uint16_t looper = 0;

	checksum = recvd_packet.seq_no;

	for(looper=0;looper<MAX_DATASIZE;looper++) {
		checksum = checksum + recvd_packet.data[looper];
	}

	return checksum ^ 0xFF;
}

uint8_t verify_checksum(struct segment_packet recvd_packet ) {

	uint16_t checksum = 0;
	checksum = calc_checksum(recvd_packet);

	return (checksum == recvd_packet.checksum);
}


int main(int argc, char *argv[]) {

	int seq_no_range = 0;
	struct sigaction sig_action;
	int sock_fd;
	struct sockaddr_in recvr_sock_addr;
	struct sockaddr_in sender_sock_addr;
	struct segment_packet data_packet;
	struct segment_packet ack_packet;
	unsigned int looper = 0;

	/*================================================Terminal Arguments Translation=======================================================*/
	if(argc < 4) {
		printf("\nusage: %s inputfile portNum num_of_packets\n",argv[0]);
		return 0;
	}

	FILE *ip_fp;
	char input_filename[50] = "./";
	char protocol[3] = {0};
	char *m,*N,*timeout,*seg_size;
	int num_m, num_N, num_timeout, num_seg_size, portno = 0, no_of_packets = 0;
	char *str;

	portno = atoi(argv[2]);
	no_of_packets = atoi(argv[3]);
	strcat(input_filename, argv[1]);
	ip_fp = fopen(input_filename, "r");
	fseek(ip_fp,0, SEEK_END);
	long fsize = ftell(ip_fp);
	fseek(ip_fp, 0, SEEK_SET);
	char * input_filedata = malloc(fsize +1);
	fread(input_filedata, fsize,1 , ip_fp);
	strcpy(protocol, input_filedata);
	strtok_r(protocol, "\n", &m);
	strtok_r(m, "\n", &timeout);
	strtok_r(m, " ", &N);
	num_m = atoi(m);
	num_N = atoi(N);
	strtok_r(timeout, "\n", &seg_size);
	num_timeout = atoi(timeout);
	num_timeout = num_timeout/1000000;
	num_seg_size = atoi(seg_size);
	seq_no_range = pow(2.0,num_m);
	window_size = num_N;
	//printf("file_data : \n%s",input_filedata);    
	printf("portno = %d\n",portno);
	printf("timeout : %d\n",num_timeout);
	printf("no_of_packets = %d\n",no_of_packets);
	printf("seg_size : %d\n",num_seg_size);
	if(num_seg_size > MAX_DATASIZE) {
		printf("segment size too large.\nPlease give size below 1024 bytes\n");
		return 0;
	}
	printf("protocol : %s\n",protocol);
	printf("m : %d\n",num_m);
	printf("N : %d\n",num_N);
	printf("sequence no range = %d\n",seq_no_range);
	printf("window size = %d\n",window_size);
	/*==================================================================================================================================*/

	/*=======================================================socket related section=====================================================*/
	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		printf ("socket() failed");
	}
	printf ("\nsocket created\n");

	memset(&recvr_sock_addr,0,sizeof(recvr_sock_addr));
	recvr_sock_addr.sin_family = AF_INET;
	recvr_sock_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	recvr_sock_addr.sin_port = htons(portno);

	struct timeval read_timeout;
	read_timeout.tv_sec = 4;
	read_timeout.tv_usec = 0;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	/*===================================================GBN===========================================================================*/
	if(strcmp(protocol,"SR") == 0) {
		int ret_recv=0;
		int no_error = 1;
		int recvr_size =sizeof(recvr_sock_addr);
		strcpy(data_packet.data,"Hey this is the data from sender");

		while(packets_sent < no_of_packets) {
			if(no_error == 1) {
				for(looper=0;looper<window_size;looper++) {
					Window_Size_Array[looper] = 0;
				}
			}
			sleep(2);
			for(window_size_cntr=0;window_size_cntr< window_size;window_size_cntr++) {	
				if(Window_Size_Array[window_size_cntr] == 0) {
					data_packet.seq_no = seq_num;	
					data_packet.checksum = calc_checksum(data_packet);
					if(sendto(sock_fd,&data_packet,
								sizeof(data_packet),0,(struct sockaddr *)&recvr_sock_addr,
								sizeof(recvr_sock_addr)) < 0)	{
						printf("\nsendto() failed");
					}
					seq_num++;
					if(seq_num == seq_no_range-1) {
						seq_num = 0;
					}
				}
			}

			printf("\nSending %d; Timer Started\n",BASE);
			for(window_size_cntr=0;window_size_cntr<window_size;window_size_cntr++) {
				if(Window_Size_Array[window_size_cntr] == 0) {
					no_error = 1;
					if((recvfrom(sock_fd,&ack_packet,sizeof(ack_packet),0,(struct sockaddr *)&recvr_sock_addr,&recvr_size)) < 0) {
						printf("ACK not received recvfrom failed\n");
						ERROR_VAL = TIMEOUT_ERROR;
						no_error = 0;
					}
					if(recvr_seq_num > ack_packet.seq_no) {
						no_error = 0;
						ERROR_VAL = PACKET_LOST_ERROR;
						printf("packet lost error\n");
					}
					if(no_error) {
						Window_Size_Array[window_size_cntr] = 1;
						printf("Received ACK : %d	Expected ACK : %d\n",ack_packet.seq_no,recvr_seq_num);
						memset(ack_packet.data,0,sizeof(ack_packet.data));
						recvr_seq_num++;
						if(recvr_seq_num == seq_no_range-1) {
							recvr_seq_num = 0;
						}
					} else {
						break;
					}
				}
			}
			if(no_error == 0) {
				seq_num = initial_seq_no;
				recvr_seq_num = initial_seq_no;
				if(ERROR_VAL == TIMEOUT_ERROR) {
					printf("Timer Expired; Resending the failed packets\n");
				} else if(ERROR_VAL == PACKET_LOST_ERROR) {
					printf("Packet Lost; Resending the failed packets\n");
				}
			} else {
				BASE = BASE + window_size;
				initial_seq_no = seq_num;
				packets_sent = packets_sent + window_size;
				temp = no_of_packets - packets_sent;
				if(temp > num_N) {
					window_size = num_N;
				} else {
					window_size = temp;
				}
				printf("Communication successfull sliding window %d\n",packets_sent);
			}
		}
	}
	if(strcmp(protocol,"GBN") == 0) {
		int ret_recv=0;
		int no_error = 1;
		int recvr_size =sizeof(recvr_sock_addr);
		strcpy(data_packet.data,"Hey this is the data from sender");

		while(packets_sent < no_of_packets) {
			sleep(2);
			for(window_size_cntr=0;window_size_cntr< window_size;window_size_cntr++) {
				data_packet.seq_no = seq_num;
				data_packet.checksum = calc_checksum(data_packet);
				if(sendto(sock_fd,&data_packet,sizeof(data_packet),0,(struct sockaddr *)&recvr_sock_addr,sizeof(recvr_sock_addr)) < 0)                                {
					printf("\nsendto() failed");
				}
				seq_num++;
				if(seq_num == seq_no_range-1) {
					seq_num = 0;
				}
			}

			printf("\nSending %d; Timer Started\n",BASE);
			for(window_size_cntr=0;window_size_cntr<window_size;window_size_cntr++) {
				no_error = 1;
				if((recvfrom(sock_fd,&ack_packet,sizeof(ack_packet),0,(struct sockaddr *)&recvr_sock_addr,&recvr_size)) < 0) {
					printf("ACK not received recvfrom failed\n");
					ERROR_VAL = TIMEOUT_ERROR;
					no_error = 0;
				}
				if(recvr_seq_num > ack_packet.seq_no) {
					no_error = 0;
					ERROR_VAL = PACKET_LOST_ERROR;
					printf("packet lost error\n");
				}
				if(no_error) {
					printf("Received ACK : %d       Expected ACK : %d\n",ack_packet.seq_no,recvr_seq_num);
					memset(ack_packet.data,0,sizeof(ack_packet.data));
					recvr_seq_num++;
					if(recvr_seq_num == seq_no_range-1) {
						recvr_seq_num = 0;
					}
				} else {
					break;
				}
			}
			if(no_error == 0) {
				seq_num = initial_seq_no;
				recvr_seq_num = initial_seq_no;
				if(ERROR_VAL == TIMEOUT_ERROR) {
					printf("Timer Expired; Resending the complete Window\n");
				} else if(ERROR_VAL == PACKET_LOST_ERROR) {
					printf("Packet Lost; Resending the complete window\n");
				}
			} else {
				BASE = BASE + window_size;
				initial_seq_no = seq_num;
				packets_sent = packets_sent + window_size;
				temp = no_of_packets - packets_sent;
				if(temp > num_N) {
					window_size = num_N;
				} else {
					window_size = temp;
				}
				printf("Communication successfull sliding window %d\n",packets_sent);
			}
		}

}

close(sock_fd);

return 0;
}
