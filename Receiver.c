#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 


#define MAX_DATASIZE 1024
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

	/*================================================Terminal Arguments Translation=======================================================*/
	if(argc < 3) {
		printf("\nusage: %s inputfile portNum\n",argv[0]);
		return 1;
	}

	FILE *ip_fp;
	char input_filename[50] = "./";
	char protocol[3] = {0};
	char *m,*N,*timeout,*seg_size;
	int num_m, num_N, num_timeout, num_seg_size;
	int portno = 0;
	int iteration = 0;

	portno = atoi(argv[2]);
	printf("portno = %d\n",portno);

	strcat(input_filename, argv[1]);
	ip_fp = fopen(input_filename, "r");
	fseek(ip_fp,0, SEEK_END);
	long fsize = ftell(ip_fp);
	fseek(ip_fp, 0, SEEK_SET);
	char * input_filedata = malloc(fsize +1);
	fread(input_filedata, fsize,1 , ip_fp);
	//printf("file_data : \n%s",input_filedata);	
	strcpy(protocol, input_filedata);	
	strtok_r(protocol, "\n", &m);	
	printf("protocol : %s\n",protocol);	
	strtok_r(m, "\n", &timeout);	
	strtok_r(m, " ", &N);
	num_m = atoi(m);
	num_N = atoi(N);	
	printf("m : %d\n",num_m);	
	printf("N : %d\n",num_N);	
	strtok_r(timeout, "\n", &seg_size);
	num_timeout = atoi(timeout);
	num_seg_size = atoi(seg_size);	
	printf("timeout : %d\n",num_timeout);
	printf("seg_size : %d\n",num_seg_size);
	/*==================================================================================================================================*/


	/*=============================================socket related calls=================================================================*/
	struct sockaddr_in recvr_socket, sender_socket;
	int socket_fd, i, slen = sizeof(sender_socket) , recv_len;

	if ((socket_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("socket");
		return 1;
	}

	memset((char *) &recvr_socket, 0, sizeof(recvr_socket));
	recvr_socket.sin_family = AF_INET;
	recvr_socket.sin_port = htons(portno);
	recvr_socket.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(socket_fd , (struct sockaddr*)&recvr_socket, sizeof(recvr_socket) ) == -1) {
		printf("bind");
		return 1;
	}
	/*==================================================================================================================================*/

	struct segment_packet data_packet, ack_packet;
	data_packet.seq_no = 0;
	data_packet.checksum = 0;
	memset(data_packet.data,0,MAX_DATASIZE);
	int iterator =0;
	strcpy(ack_packet.data,"ACK from receiver");

	printf("Waiting for data...\n");
	while(1) {
		fflush(stdout);
		iterator++;

		if ((recv_len = recvfrom(socket_fd,&data_packet,sizeof(data_packet), 0, (struct sockaddr *) &sender_socket, &slen)) == -1) {
			printf("recvfrom()");
		}
		printf("Received Segment %d\n",data_packet.seq_no);
		if(iterator != 9) {
		ack_packet.seq_no = data_packet.seq_no;
		ack_packet.checksum = calc_checksum(ack_packet);
		printf("ACK Sent : %d",ack_packet.seq_no);
		if (sendto(socket_fd, &ack_packet,sizeof(ack_packet), 0, (struct sockaddr*) &sender_socket, slen) == -1) {
			printf("sendto()");
		}
		}
	}

	close(socket_fd);

	return 0;
}
